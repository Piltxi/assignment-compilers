#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>

#include <llvm/Transforms/Utils/LocalOpts.hpp>

using namespace llvm;
namespace firstAssignment {

/// @brief Apply the optimization pass on a basic block.
/// @param function Reference to the basic block to optimize.
/// @return True if the basic block was transformed.
bool LocalOpts::runOnFunction(Function &function) {
  bool transformed = false;
  for (auto &bb : function) {
    if (runOnBasicBlock(bb)) {
      transformed = true;
    }
  }
  return transformed;
}

/// @brief Apply the optimization pass on a module.
/// @param module Reference to the module to optimize.
/// @param manager Reference to the analysis manager.
/// @return A set of preserved analyses.
PreservedAnalyses LocalOpts::run(Module &module,
                                 ModuleAnalysisManager &manager) {
  for (auto &function : module) {
    if (runOnFunction(function)) {
      return PreservedAnalyses::none();
    }
  }
  return PreservedAnalyses::none();
}

} // namespace firstAssignment
