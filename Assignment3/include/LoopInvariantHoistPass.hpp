#ifndef LLVM_TRANSFORMS_UTILS_LOOPINVARIANTHOISTPASS_HPP
#define LLVM_TRANSFORMS_UTILS_LOOPINVARIANTHOISTPASS_HPP

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include <map>
#include <set>
#include <vector>

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
  std::vector<Instruction *> invariantInstructionsOrder;
  std::set<Instruction *> hoistableInstructions;
  std::set<Instruction *> nonHoistableInstructions;

  std::map<Instruction *, std::string> hoistReasons;
  std::map<Instruction *, std::string> nonHoistReasons;

  std::set<BasicBlock *> loopExitingBlocks;

  void identifyLoopInvariantInstructionsAndExitingBlocks();
  bool isOperandInvariant(const Use &operand) const;
  bool isLoopInvariant(const Instruction &instruction) const;
  void determineHoistableInstructions();
  bool isLoopExiting(const BasicBlock *basicBlock);
  void printAnalysisResult();
};

} // end namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_LOOPINVARIANTHOISTPASS_HPP
