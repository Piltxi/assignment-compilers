#ifndef LOCAL_OPTS_HPP // Traditional include guard for broader compatibility
#define LOCAL_OPTS_HPP

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Instruction.h>

namespace firstAssignment { // Namespace to encapsulate the optimization passes

/// @brief Base class for local optimizations.
/// This class provides a common interface for applying optimizations at the basic block level.
/// Derived classes should implement the runOnBasicBlock method to apply specific optimizations.
class LocalOpts {
public:
    llvm::PreservedAnalyses run(llvm::Module&, llvm::ModuleAnalysisManager&);
    virtual ~LocalOpts() = default; // Virtual destructor for safe polymorphic deletion

protected:
    virtual bool runOnBasicBlock(llvm::BasicBlock&) = 0; // Pure virtual function for basic block optimizations

private:
    bool runOnFunction(llvm::Function&); // Function level optimization
};

/// @brief Pass for performing algebraic identity optimizations within basic blocks.
class AlgebraicIdentityPass final : public llvm::PassInfoMixin<AlgebraicIdentityPass>, public LocalOpts {
protected:
    bool runOnBasicBlock(llvm::BasicBlock&) override; // Override indicates this method implements a base class virtual function
};

/// @brief Pass for performing strength reduction optimizations within basic blocks.
class StrengthReductionPass final : public llvm::PassInfoMixin<StrengthReductionPass>, public LocalOpts {
protected:
    bool runOnBasicBlock(llvm::BasicBlock&) override;
};

/// @brief Pass for performing miscellaneous optimizations within basic blocks.
class MIOptimizationPass final : public llvm::PassInfoMixin<MIOptimizationPass>, public LocalOpts {
protected:
    bool runOnBasicBlock(llvm::BasicBlock&) override;
};

} // namespace firstAssignement

#endif // LOCAL_OPTS_HPP
