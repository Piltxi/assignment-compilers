#ifndef LLVM_TRANSFORMS_UTILS_LOOPINVARIANTHOISTPASS_HPP
#define LLVM_TRANSFORMS_UTILS_LOOPINVARIANTHOISTPASS_HPP

#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/Scalar/LoopPassManager.h>
#include <llvm/Transforms/Utils/LoopUtils.h>
#include <map>
#include <set>
#include <string>

namespace llvm {

class LoopInvariantHoistPass : public PassInfoMixin<LoopInvariantHoistPass> {
public:
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM,
                        LoopStandardAnalysisResults &AR, LPMUpdater &U);
  bool runOnLoop(Loop *L, LoopAnalysisManager &AM,
                 LoopStandardAnalysisResults &AR);
  void analyze(Loop *L, LoopAnalysisManager &AM,
               LoopStandardAnalysisResults &AR);

private:
  Loop *currentLoop;
  DominatorTree *dominatorTree;

  std::set<Instruction *> loopInvariantInstructions;
  std::set<Instruction *> hoistableInstructions;
  std::set<Instruction *> nonHoistableInstructions;

  std::map<Instruction *, std::string> hoistReasons;
  std::map<Instruction *, std::string> nonHoistReasons;

  std::set<BasicBlock *> loopExitingBlocks;

  void searchForLoopInvariantInstructionsAndLoopExitingBlocks();
  std::set<Instruction *> getLoopInvariantInstructions(BasicBlock *BB);
  void searchForHoistableInstructions();

  bool isLoopInvariant(const Instruction &I);
  bool isLoopInvariant(const Use &U);
  bool isLoopExiting(const BasicBlock *BB);
  bool dominatesAllExits(const Instruction *I);
  bool isDeadAfterLoop(const Instruction *I);
  bool hasSideEffects(const Instruction *instr);

  void printAnalysisResult();
};

} // end namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_LOOPINVARIANTHOISTPASS_HPP