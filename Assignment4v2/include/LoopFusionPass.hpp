#ifndef LLVM_TRANSFORMS_LOOPFUSIONPASS_H
#define LLVM_TRANSFORMS_LOOPFUSIONPASS_H

#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

namespace llvm {
class LoopFusionPass : public PassInfoMixin<LoopFusionPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);

private:
  BasicBlock *getLoopHead(const Loop *L) const;
  BasicBlock *getLoopExit(const Loop *L) const;

  PHINode *getIVForNonRotatedLoops(Loop *L, ScalarEvolution &SE) const;

  bool areLoopsAdjacent(const Loop *Lprev, const Loop *Lnext) const;
  bool areLoopsTCE(const Loop *Lprev, const Loop *Lnext, Function &F,
                   FunctionAnalysisManager &FAM) const;
  bool areLoopsCFE(const Loop *Lprev, const Loop *Lnext, Function &F,
                   FunctionAnalysisManager &FAM) const;
  bool areLoopsIndependent(const Loop *Lprev, const Loop *Lnext, Function &F,
                         FunctionAnalysisManager &FAM) const;
  Loop *merge(Loop *Lprev, Loop *Lnext, Function &F,
              FunctionAnalysisManager &FAM);
};
} // namespace llvm

#endif // LLVM_TRANSFORMS_LOOPFUSIONPASS_H
