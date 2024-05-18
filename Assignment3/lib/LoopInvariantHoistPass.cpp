#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Use.h>
#include <llvm/Support/raw_ostream.h>
#include <unordered_set>
#include <vector>

#include "llvm/Transforms/Utils/LoopInvariantHoistPass.hpp"

using namespace llvm;

void LoopInvariantHoistPass::analyze(Loop *loop, LoopAnalysisManager &AM,
                                     LoopStandardAnalysisResults &AR) {
  identifyLoopInvariantInstructionsAndExitingBlocks();
  determineHoistableInstructions();
  printAnalysisResult();
}

void LoopInvariantHoistPass::
    identifyLoopInvariantInstructionsAndExitingBlocks() {
  for (auto *BB : currentLoop->getBlocks()) {
    for (auto &I : *BB) {
      if (isLoopInvariant(I)) {
        loopInvariantInstructions.insert(&I);
        invariantInstructionsOrder.push_back(&I);
      }
    }

    if (isLoopExiting(BB)) {
      loopExitingBlocks.insert(BB);
    }
  }
}

bool LoopInvariantHoistPass::isOperandInvariant(const Use &U) const {
  if (isa<Constant>(U) || isa<Argument>(U))
    return true;

  if (auto *Inst = dyn_cast<Instruction>(U)) {
    return loopInvariantInstructions.count(Inst) ||
           !currentLoop->contains(Inst);
  }
  return false;
}

bool LoopInvariantHoistPass::isLoopInvariant(const Instruction &I) const {
  if (isa<PHINode>(I))
    return false;

  return llvm::all_of(I.operands(),
                      [this](const Use &U) { return isOperandInvariant(U); });
}

void LoopInvariantHoistPass::determineHoistableInstructions() {
  using ExitEdge = std::pair<BasicBlock *, BasicBlock *>;
  SmallVector<ExitEdge, 4> exitEdges;
  currentLoop->getExitEdges(exitEdges);

  auto isUnusedOutsideLoop = [this](const Instruction *I) {
    for (auto *User : I->users()) {
      if (!currentLoop->contains(dyn_cast<Instruction>(User))) {
        return false;
      }
    }
    return true;
  };

  auto isHoistableInstruction = [&exitEdges, this,
                                 &isUnusedOutsideLoop](const Instruction *I) {
    for (const auto &Edge : exitEdges) {
      if (!dominatorTree->dominates(I, Edge.second) &&
          !isUnusedOutsideLoop(I)) {
        return false;
      }
    }
    return true;
  };

  for (auto *I : invariantInstructionsOrder) {
    if (isHoistableInstruction(I)) {
      hoistableInstructions.insert(I);
      if (isUnusedOutsideLoop(I)) {
        hoistReasons[I] = "Instruction is dead after loop";
      } else {
        hoistReasons[I] = "Instruction dominates all exits";
      }
    } else {
      nonHoistableInstructions.insert(I);
      if (!isUnusedOutsideLoop(I)) {
        nonHoistReasons[I] = "Instruction is used outside loop";
      } else {
        nonHoistReasons[I] = "Instruction does not dominate all exits";
      }
    }
  }
}

bool LoopInvariantHoistPass::isLoopExiting(const BasicBlock *BB) {
  return llvm::any_of(successors(BB), [this](const BasicBlock *Succ) {
    return !currentLoop->contains(Succ);
  });
}

void LoopInvariantHoistPass::printAnalysisResult() {
  outs() << "--- ANALYSIS RESULT: ---\n";

  outs() << "Loop invariant instructions:\n";
  for (auto *I : loopInvariantInstructions) {
    outs() << *I << '\n';
  }

  outs() << "Hoistable instructions:\n";
  for (auto *I : hoistableInstructions) {
    outs() << *I << " | Reason: " << hoistReasons[I] << '\n';
  }

  outs() << "Non-hoistable instructions:\n";
  for (auto *I : nonHoistableInstructions) {
    outs() << *I << " | Reason: " << nonHoistReasons[I] << '\n';
  }

  outs() << "-------------------------\n";
}

bool LoopInvariantHoistPass::runOnLoop(Loop *L, LoopAnalysisManager &AM,
                                       LoopStandardAnalysisResults &AR) {
  currentLoop = L;
  dominatorTree = &AR.DT;

  analyze(L, AM, AR);

  auto *Preheader = L->getLoopPreheader();
  if (!Preheader) {
    errs() << "The loop is not in canonical form. Skipping.\n";
    return false;
  }

  IRBuilder<> Builder(Preheader, Preheader->begin());
  bool Changed = false;

  for (auto *I : hoistableInstructions) {
    I->moveBefore(Preheader->getTerminator());
    Changed = true;
  }

  return Changed;
}

PreservedAnalyses LoopInvariantHoistPass::run(Loop &L, LoopAnalysisManager &AM,
                                              LoopStandardAnalysisResults &AR,
                                              LPMUpdater &U) {
  return runOnLoop(&L, AM, AR) ? PreservedAnalyses::none()
                               : PreservedAnalyses::all();
}
