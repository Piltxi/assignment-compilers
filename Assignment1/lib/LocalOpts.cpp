#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>

#include <llvm/Transforms/Utils/LocalOpts.hpp>

using namespace llvm;
namespace firstAssignment {

/// Applies optimization passes on each basic block within a function to enhance
/// performance. Iterates over each basic block and optimizes it if possible.
///
/// @param FunctionRef A reference to the function being optimized.
/// @return True if any basic block within the function was optimized.
bool LocalOpts::runOnFunction(Function &function) {
  bool transformed = false;
  for (auto &bb : function) {
    if (runOnBasicBlock(bb)) {
      transformed = true;
    }
  }
  return transformed;
}

/// Optimizes each function within a given LLVM module by applying specific
/// optimization techniques. Iterates over all functions in the module and
/// applies optimization passes.
///
/// @param ModuleRef Reference to the module being optimized.
/// @param AnalysisManagerRef Reference to the module's analysis manager.
/// @return A set of analyses that are preserved after the optimization passes.
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
