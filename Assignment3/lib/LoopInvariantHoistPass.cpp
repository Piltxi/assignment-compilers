#include <llvm/ADT/STLExtras.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Use.h>
#include <llvm/IR/Value.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include "llvm/Transforms/Utils/LoopInvariantHoistPass.hpp"

using namespace llvm;

void LoopInvariantHoistPass::analyze(
    Loop *loop, LoopAnalysisManager &analysisManager,
    LoopStandardAnalysisResults &analysisResults) {

  searchForLoopInvariantInstructionsAndLoopExitingBlocks();
  searchForHoistableInstructions();

  printAnalysisResult();
}

void LoopInvariantHoistPass::
    searchForLoopInvariantInstructionsAndLoopExitingBlocks() {
  outs() << "Searching for loop invariant instructions and loop exiting "
            "blocks...\n";
  for (auto *bb : this->currentLoop->getBlocks()) {
    outs() << "Analyzing basic block: ";
    bb->front().print(errs());
    outs() << "\n";
    auto bbLoopInvariantInstructions = getLoopInvariantInstructions(bb);
    this->loopInvariantInstructions.insert(bbLoopInvariantInstructions.begin(),
                                           bbLoopInvariantInstructions.end());

    if (isLoopExiting(bb)) {
      this->loopExitingBlocks.insert(bb);
      outs() << "Found loop exiting block: ";
      bb->front().print(errs());
      outs() << "\n";
    }
  }
}

std::set<Instruction *>
LoopInvariantHoistPass::getLoopInvariantInstructions(BasicBlock *bb) {
  std::set<Instruction *> loopInvariantInstrs;
  for (auto &instr : *bb) {
    if (instr.isBinaryOp() && isLoopInvariant(instr)) {
      loopInvariantInstrs.insert(&instr);
      outs() << "Found loop invariant instruction: " << instr << "\n";
    }
  }
  return loopInvariantInstrs;
}

void LoopInvariantHoistPass::searchForHoistableInstructions() {
  outs() << "Searching for hoistable instructions...\n";
  for (auto *instr : this->loopInvariantInstructions) {
    outs() << "Analyzing loop invariant instruction: " << *instr << "\n";
    bool dominates = dominatesAllExits(instr);
    bool deadAfterLoop = isDeadAfterLoop(instr);
    bool hasSE = hasSideEffects(instr);

    outs() << "dominatesAllExits: " << dominates
           << ", isDeadAfterLoop: " << deadAfterLoop
           << ", hasSideEffects: " << hasSE << "\n";

    if (dominates && !hasSE) {
      this->hoistableInstructions.insert(instr);
      this->hoistReasons[instr] = "Dominates all exits";
    } else if (deadAfterLoop && !hasSE) {
      this->hoistableInstructions.insert(instr);
      this->hoistReasons[instr] = "Is dead after loop";
    } else {
      this->nonHoistableInstructions.insert(instr);
      this->nonHoistReasons[instr] =
          "Cannot dominate all exits and not dead after loop";
    }
  }
}

bool LoopInvariantHoistPass::isLoopInvariant(const Instruction &instr) {
  outs() << "Checking if instruction is loop invariant: " << instr << "\n";
  bool result = all_of(instr.operands(), [this](const Use &operand) {
    return isLoopInvariant(operand);
  });
  outs() << "Instruction is "
         << (result ? "loop invariant" : "not loop invariant") << ": " << instr
         << "\n";
  return result;
}

bool LoopInvariantHoistPass::isLoopInvariant(const Use &operand) {
  Value *reachingDefinition = operand.get();
  outs() << "Checking if operand is loop invariant: " << *reachingDefinition
         << "\n";

  if (isa<Constant>(reachingDefinition) || isa<Argument>(reachingDefinition)) {
    // Constants and function arguments are always loop invariant
    outs() << "Operand is loop invariant because it is a constant or a "
              "function argument.\n";
    return true;
  } else if (auto *operandDefinitionInstr =
                 dyn_cast<Instruction>(reachingDefinition)) {
    bool result =
        !this->currentLoop->contains(operandDefinitionInstr) ||
        this->loopInvariantInstructions.find(operandDefinitionInstr) !=
            this->loopInvariantInstructions.end();
    outs() << "Operand is "
           << (result ? "loop invariant" : "not loop invariant") << ": "
           << *reachingDefinition << "\n";
    return result;
  }

  errs() << "WARNING: Unsupported type of reaching definition: "
         << *reachingDefinition << "\n";
  return false;
}

bool LoopInvariantHoistPass::isLoopExiting(const BasicBlock *bb) {
  outs() << "Checking if block is loop exiting: " << bb->getName() << "\n";
  for (auto *succ : successors(bb)) {
    if (!this->currentLoop->contains(succ)) {
      outs() << "Block is loop exiting: " << bb->getName() << "\n";
      return true;
    }
  }
  return false;
}

bool LoopInvariantHoistPass::dominatesAllExits(const Instruction *instr) {
  outs() << "Checking if instruction dominates all exits: " << *instr << "\n";
  for (auto *exitingBlock : this->loopExitingBlocks) {
    if (!this->dominatorTree->dominates(instr->getParent(), exitingBlock)) {
      outs() << "Instruction does not dominate exit block: "
             << exitingBlock->getName() << "\n";
      return false;
    }
  }
  outs() << "Instruction dominates all exits: " << *instr << "\n";
  return true;
}

bool LoopInvariantHoistPass::isDeadAfterLoop(const Instruction *instr) {
  outs() << "Checking if instruction is dead after loop: " << *instr << "\n";
  bool result = all_of(instr->uses(), [this](const Use &use) {
    auto *user = dyn_cast<Instruction>(use.getUser());
    if (user == nullptr) {
      errs() << "WARNING: Found a use that is not an instruction: " << *use
             << "\n";
      return false;
    }
    bool contains = this->currentLoop->contains(user);
    outs() << "User " << *user << " is " << (contains ? "inside" : "outside")
           << " the loop\n";
    return contains;
  });
  outs() << "Instruction is "
         << (result ? "dead after loop" : "not dead after loop") << ": "
         << *instr << "\n";
  return result;
}

bool LoopInvariantHoistPass::hasSideEffects(const Instruction *instr) {
  if (isa<StoreInst>(instr) || isa<CallInst>(instr) || instr->isTerminator()) {
    outs() << "Instruction has side effects: " << *instr << "\n";
    return true;
  }
  return false;
}

void LoopInvariantHoistPass::printAnalysisResult() {
  outs() << "--- ANALYSIS RESULT: ---\n";

  outs() << "Loop invariant instructions:\n";
  for (auto *instr : loopInvariantInstructions) {
    outs() << *instr << '\n';
  }

  outs() << "Hoistable instructions:\n";
  for (auto *instr : hoistableInstructions) {
    outs() << *instr << " | Reason: " << this->hoistReasons[instr] << '\n';
  }

  outs() << "Non-hoistable instructions:\n";
  for (auto *instr : nonHoistableInstructions) {
    outs() << *instr << " | Reason: " << this->nonHoistReasons[instr] << '\n';
  }

  outs() << "-------------------------\n";
}

bool LoopInvariantHoistPass::runOnLoop(
    Loop *loop, LoopAnalysisManager &analysisManager,
    LoopStandardAnalysisResults &analysisResults) {
  this->currentLoop = loop;
  this->dominatorTree = &analysisResults.DT;

  analyze(loop, analysisManager, analysisResults);

  auto *preheader = loop->getLoopPreheader();
  if (!preheader) {
    errs() << "The loop is not in canonical form. Skipping.\n";
    return false;
  }

  IRBuilder<> builder(preheader, preheader->begin());
  bool changed = false;

  for (auto *instr : hoistableInstructions) {
    outs() << "Moving instruction: " << *instr << "\n";
    instr->removeFromParent();
    builder.Insert(instr);
    changed = true;
  }

  return changed;
}

PreservedAnalyses
LoopInvariantHoistPass::run(Loop &loop, LoopAnalysisManager &analysisManager,
                            LoopStandardAnalysisResults &analysisResults,
                            LPMUpdater &updater) {
  if (runOnLoop(&loop, analysisManager, analysisResults)) {
    return PreservedAnalyses::none();
  }
  return PreservedAnalyses::all();
}
