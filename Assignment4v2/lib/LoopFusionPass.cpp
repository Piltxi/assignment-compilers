#include "llvm/Transforms/Utils/LoopFusionPass.hpp"

using namespace llvm;

BasicBlock *LoopFusionPass::getLoopHead(const Loop *L) const {
  return L->isGuarded() ? L->getLoopGuardBranch()->getParent()
                        : L->getLoopPreheader();
}

BasicBlock *LoopFusionPass::getLoopExit(const Loop *L) const {
  if (L->isGuarded()) {
    if (auto *B = L->getLoopGuardBranch()) {
      if (B->isConditional()) {
        for (unsigned i = 0; i < B->getNumSuccessors(); ++i) {
          if (!L->contains(B->getSuccessor(i)))
            return B->getSuccessor(i);
        }
      }
    }
  } else {
    return L->getExitBlock();
  }
  return nullptr;
}

bool LoopFusionPass::areLoopsAdjacent(const Loop *Lprev, const Loop *Lnext) const {
  return getLoopExit(Lprev) == getLoopHead(Lnext);
}

bool LoopFusionPass::areLoopsCFE(const Loop *Lprev, const Loop *Lnext, Function &F,
                                 FunctionAnalysisManager &FAM) const {
  const auto *LprevHead = getLoopHead(Lprev);
  const auto *LnextHead = getLoopHead(Lnext);

  auto &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  auto &PDT = FAM.getResult<PostDominatorTreeAnalysis>(F);

  return DT.dominates(LprevHead, LnextHead) && PDT.dominates(LnextHead, LprevHead);
}

bool LoopFusionPass::areLoopsTCE(const Loop *Lprev, const Loop *Lnext, Function &F,
                                 FunctionAnalysisManager &FAM) const {
  auto &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);
  auto LprevTC = SE.getBackedgeTakenCount(Lprev);
  auto LnextTC = SE.getBackedgeTakenCount(Lnext);

  if (isa<SCEVCouldNotCompute>(LprevTC) || isa<SCEVCouldNotCompute>(LnextTC)) {
    return false;
  }
  return SE.isKnownPredicate(CmpInst::ICMP_EQ, LprevTC, LnextTC);
}

bool LoopFusionPass::areLoopsIndependent(const Loop *Lprev, const Loop *Lnext, Function &F,
                                         FunctionAnalysisManager &FAM) const {
  auto &DI = FAM.getResult<DependenceAnalysis>(F);

  for (auto *BB : Lprev->getBlocks()) {
    for (auto &I : *BB) {
      if (isa<LoadInst>(&I) || isa<StoreInst>(&I)) {
        for (auto *BB2 : Lnext->getBlocks()) {
          for (auto &I2 : *BB2) {
            if (isa<LoadInst>(&I2) || isa<StoreInst>(&I2)) {
              if (DI.depends(&I, &I2, true)) {
                return false;
              }
            }
          }
        }
      }
    }
  }
  return true;
}

PHINode *LoopFusionPass::getIVForNonRotatedLoops(Loop *L, ScalarEvolution &SE) const {
  if (L->isCanonical(SE)) {
    return L->getCanonicalInductionVariable();
  }

  for (auto &PHI : L->getHeader()->phis()) {
    if (auto *PHIasADDREC = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(&PHI))) {
      if (PHIasADDREC->getLoop() == L) {
        for (auto *U : PHI.users()) {
          if (auto *Cmp = dyn_cast<ICmpInst>(U)) {
            if (L->contains(Cmp)) {
              return &PHI;
            }
          }
        }
      }
    }
  }
  return nullptr;
}

Loop *LoopFusionPass::merge(Loop *Lprev, Loop *Lnext, Function &F,
                            FunctionAnalysisManager &FAM) {
  auto &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);
  auto &LI = FAM.getResult<LoopAnalysis>(F);

  auto *PL = Lprev->getLoopLatch();
  auto *PB = PL->getSinglePredecessor();
  auto *PH = Lprev->getHeader();
  auto *PPH = Lprev->getLoopPreheader();
  auto *PE = Lprev->getExitBlock();
  auto *PG = Lprev->getLoopGuardBranch();

  auto *NL = Lnext->getLoopLatch();
  auto *NB = NL->getSinglePredecessor();
  auto *NH = Lnext->getHeader();
  auto *NPH = Lnext->getLoopPreheader();
  auto *NE = Lnext->getExitBlock();

  auto PIV = getIVForNonRotatedLoops(Lprev, SE);
  auto NIV = getIVForNonRotatedLoops(Lnext, SE);

  NIV->replaceAllUsesWith(PIV);
  NIV->eraseFromParent();

  SmallVector<PHINode *, 8> PHIsToMove;
  for (auto &I : *NH) {
    if (auto *PHI = dyn_cast<PHINode>(&I)) {
      PHIsToMove.push_back(PHI);
    }
  }

  auto *InsertPoint = PH->getFirstNonPHI();
  for (auto *PHI : PHIsToMove) {
    PHI->moveBefore(InsertPoint);
    for (unsigned i = 0, e = PHI->getNumIncomingValues(); i != e; ++i) {
      if (PHI->getIncomingBlock(i) == NPH) {
        PHI->setIncomingBlock(i, PPH);
      } else if (PHI->getIncomingBlock(i) == NL) {
        PHI->setIncomingBlock(i, PL);
      }
    }
  }

  PH->getTerminator()->replaceSuccessorWith(PE, NE);
  PB->getTerminator()->replaceSuccessorWith(PL, NB);
  NB->getTerminator()->replaceSuccessorWith(NL, PL);
  NH->getTerminator()->replaceSuccessorWith(NB, NL);
  if (PG) {
    PG->setSuccessor(1, NE);
  }

  Lprev->addBasicBlockToLoop(NB, LI);
  Lnext->removeBlockFromLoop(NB);
  LI.erase(Lnext);
  EliminateUnreachableBlocks(F);

  return Lprev;
}

PreservedAnalyses LoopFusionPass::run(Function &F,
                                      FunctionAnalysisManager &FAM) {
  auto &LI = FAM.getResult<LoopAnalysis>(F);

  Loop *Lprev = nullptr;
  bool hasBeenOptimized = false;
  for (auto iter = LI.rbegin(); iter != LI.rend(); ++iter) {
    Loop *L = *iter;
    if (Lprev) {
      if (areLoopsAdjacent(Lprev, L) && areLoopsTCE(Lprev, L, F, FAM) &&
          areLoopsCFE(Lprev, L, F, FAM) &&
          areLoopsIndependent(Lprev, L, F, FAM)) {
        hasBeenOptimized = true;
        Lprev = merge(Lprev, L, F, FAM);
      } else {
        Lprev = L;
      }
    } else {
      Lprev = L;
    }
  }
  return hasBeenOptimized ? PreservedAnalyses::none()
                          : PreservedAnalyses::all();
}
