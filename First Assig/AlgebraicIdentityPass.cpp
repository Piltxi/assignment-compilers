#include <llvm/IR/Constants.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <llvm/Transforms/Utils/LocalOpts.hpp>

using namespace llvm;

namespace firstAssignment{

/// @brief Check if an instruction uses a neutral constant for its operation and can be simplified.
/// Neutral constants are 0 for addition and 1 for multiplication.
/// @param Inst Reference to the instruction to check.
/// @param NeutralConst Pointer to store the neutral constant, if found.
/// @return True if the instruction can be simplified.
static bool canSimplifyUsingNeutralConstant(Instruction &Inst, ConstantInt *&NeutralConst) {
  auto OpCode = Inst.getOpcode();
  Value *Op1 = Inst.getOperand(0);
  Value *Op2 = Inst.getOperand(1);

  // A lambda function to check if the operand is a neutral constant for the operation.
  auto isNeutralConstant = [&](Value *Operand) -> bool {
    if (auto *ConstInt = dyn_cast<ConstantInt>(Operand)) {
      if ((OpCode == Instruction::Add && ConstInt->isZero()) ||
          (OpCode == Instruction::Mul && ConstInt->isOne())) {
        NeutralConst = ConstInt;
        return true;
      }
    }
    return false;
  };

  if (isNeutralConstant(Op1))
    std::swap(Op1, Op2); // Ensure Op2 is the potential neutral constant
  if (!isNeutralConstant(Op2))
    return false; // Early return if Op2 is not a neutral constant

  // Replace the instruction with the non-neutral operand (Op1)
  Inst.replaceAllUsesWith(Op1);
  return true;
}

/// Apply algebraic identity simplifications on all suitable instructions in a basic block.
/// @param BB Reference to the basic block to transform.
/// @return True if any instructions were simplified.
bool AlgebraicIdentityPass::runOnBasicBlock(BasicBlock &BB) {
  bool HasChanged = false;
  for (Instruction &Inst : llvm::make_early_inc_range(BB)) {
    ConstantInt *NeutralConst = nullptr;
    if ((Inst.getOpcode() == Instruction::Add || Inst.getOpcode() == Instruction::Mul) &&
        canSimplifyUsingNeutralConstant(Inst, NeutralConst)) {
      HasChanged = true;
      Inst.eraseFromParent(); // Safe to remove after simplification
    }
  }
  return HasChanged;
}

} // namespace firstAssignment