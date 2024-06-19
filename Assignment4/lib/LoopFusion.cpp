#include "LoopFusion.hpp"

#include <cstddef>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/Casting.h>
#include <llvm/Transforms/Utils/LoopSimplify.h>
#include <vector>

using namespace llvm;

PHINode* LoopFusion::getInductionVariable(Loop* L, ScalarEvolution& SE) const {
    PHINode* canonicalIV = L->getCanonicalInductionVariable();
    if (canonicalIV)
        return canonicalIV;
    
    for (auto& phi : L->getHeader()->phis()) {
        SCEVAddRecExpr const* addrc = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(&phi));
        if (!addrc || !addrc->isAffine())
            continue;
        
        SCEV const* step = addrc->getStepRecurrence(SE);
        if (isa<SCEVConstant>(step))
            return &phi;
    }
    return nullptr;
}

bool LoopFusion::areLoopsAdjacent(const Loop* i, const Loop* j) const {
    const BasicBlock* iExitBlock = i->getExitBlock();
    const BasicBlock* jPreheader = j->getLoopPreheader();

    return iExitBlock != nullptr && jPreheader != nullptr && iExitBlock == jPreheader && jPreheader->size() == 1;
}

void LoopFusion::findAdjacentLoops(const std::vector<Loop*>& loops, std::vector<std::pair<Loop*, Loop*>>& adjLoopPairs) const {
    auto allLoopsSubLoopsRange = make_filter_range(
        map_range(loops, [](const Loop* loop) -> const std::vector<Loop*>& {
            return loop->getSubLoops();
        }),
        [](const std::vector<Loop*>& subLoops) {
            return subLoops.size() > 1;
        });

    for (const auto& subLoops : allLoopsSubLoopsRange) {
        findAdjacentLoops(subLoops, adjLoopPairs);
    }

    for (size_t i = 1; i < loops.size(); i++) {
        Loop* loopi = loops[i - 1];
        Loop* loopj = loops[i];

        if (areLoopsAdjacent(loopi, loopj)) {
            adjLoopPairs.emplace_back(loopi, loopj);
        }
    }
}

bool LoopFusion::haveSameTripCount(ScalarEvolution& SE, Loop const* loopi, Loop const* loopj) const {
    unsigned int tripi = SE.getSmallConstantTripCount(loopi);
    unsigned int tripj = SE.getSmallConstantTripCount(loopj);
    return tripi != 0 && tripj != 0 && tripi == tripj;
}

bool LoopFusion::areControlFlowEquivalent(DominatorTree& DT, PostDominatorTree& PT, const Loop* loopi, const Loop* loopj) const {
    return DT.dominates(loopi->getHeader(), loopj->getHeader()) && PT.dominates(loopj->getHeader(), loopi->getHeader());
}

bool LoopFusion::checkNegativeDistanceDeps(Loop* loopi, Loop* loopj) const {
    return true;
}

bool LoopFusion::mergeLoops(Loop* loopFused, Loop* loopToFuse, ScalarEvolution& SE, LoopInfo& LI) const {
    PHINode* loopToFuseIndV = getInductionVariable(loopToFuse, SE);
    PHINode* loopFusedIndV = getInductionVariable(loopFused, SE);

    if (!loopToFuseIndV || !loopFusedIndV)
        return false;

    loopToFuseIndV->replaceAllUsesWith(loopFusedIndV);
    loopFused->getHeader()->getTerminator()->replaceSuccessorWith(loopFused->getExitBlock(), loopToFuse->getExitBlock());

    BasicBlock* entryToSecondaryBody = nullptr;
    BasicBlock* secondaryHeader = loopToFuse->getHeader();
    BasicBlock* secondaryLatch = loopToFuse->getLoopLatch();
    BasicBlock* primaryLatch = loopFused->getLoopLatch();

    for (BasicBlock* succ : successors(secondaryHeader)) {
        if (loopToFuse->contains(succ)) {
            entryToSecondaryBody = succ;
            break;
        }
    }

    for (BasicBlock* pred : predecessors(primaryLatch)) {
        pred->getTerminator()->replaceSuccessorWith(primaryLatch, entryToSecondaryBody);
    }

    for (BasicBlock* pred : predecessors(secondaryLatch)) {
        pred->getTerminator()->replaceSuccessorWith(secondaryLatch, primaryLatch);
    }

    loopToFuse->getHeader()->getTerminator()->replaceSuccessorWith(entryToSecondaryBody, secondaryLatch);

    for (auto& bb : loopToFuse->blocks()) {
        if (bb != secondaryHeader && bb != secondaryLatch) {
            loopFused->addBasicBlockToLoop(bb, LI);
            loopToFuse->removeBlockFromLoop(bb);
        }
    }

    while (!loopToFuse->isInnermost()) {
        Loop* child = *(loopToFuse->begin());
        loopToFuse->removeChildLoop(child);
        loopFused->addChildLoop(child);
    }

    LI.erase(loopToFuse);

    return true;
}

PreservedAnalyses LoopFusion::run(Function& function, FunctionAnalysisManager& fam) {
    PostDominatorTree& PT = fam.getResult<PostDominatorTreeAnalysis>(function);
    DominatorTree& DT = fam.getResult<DominatorTreeAnalysis>(function);
    ScalarEvolution& SE = fam.getResult<ScalarEvolutionAnalysis>(function);
    LoopInfo& loopInfo = fam.getResult<LoopAnalysis>(function);
    const std::vector<Loop*>& topLevelLoops = loopInfo.getTopLevelLoops();
    const std::vector<Loop*>& topLevelLoopsInPreorder = std::vector(topLevelLoops.rbegin(), topLevelLoops.rend());
    std::vector<std::pair<Loop*, Loop*>> adjacentLoops;

    findAdjacentLoops(topLevelLoopsInPreorder, adjacentLoops);

    const auto& loopsToMerge = make_filter_range(adjacentLoops, [this, &PT, &DT, &SE](std::pair<Loop*, Loop*> pair) {
        if (!haveSameTripCount(SE, pair.first, pair.second)) {
            errs() << "Loops do not have the same trip count\n";
            return false;
        }
        if (!areControlFlowEquivalent(DT, PT, pair.first, pair.second)) {
            errs() << "Loops are not control flow equivalent\n";
            return false;
        }
        if (!checkNegativeDistanceDeps(pair.first, pair.second)) {
            errs() << "Loops have negative distance dependencies\n";
            return false;
        }
        return true;
    });

    std::vector<std::pair<Loop*, Loop*>> loopsToMergeVector(loopsToMerge.begin(), loopsToMerge.end());

    for (Loop* l : topLevelLoops)
        outs() << *l << '\n';

    for (int i = 0; i < loopsToMergeVector.size(); i++) {
        bool merged = mergeLoops(loopsToMergeVector[i].first, loopsToMergeVector[i].second, SE, loopInfo);

        if (merged && i != loopsToMergeVector.size() - 1 && loopsToMergeVector[i].second == loopsToMergeVector[i + 1].first) {
            loopsToMergeVector[i + 1].first = loopsToMergeVector[i].first;
        }
    }

    for (Loop* l : loopInfo.getTopLevelLoops())
        outs() << *l << '\n';

    return PreservedAnalyses::none();
}

extern "C" PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "CustomLoopFusion",
        .PluginVersion = LLVM_VERSION_STRING,
        .RegisterPassBuilderCallbacks =
            [](PassBuilder& PB) {
                PB.registerPipelineParsingCallback(
                    [](StringRef name, FunctionPassManager& passManager, ArrayRef<PassBuilder::PipelineElement>) -> bool {
                        if (name == "custom-loopfusion") {
                            passManager.addPass(LoopFusion());
                            return true;
                        }

                        return false;
                    });
            }};
}