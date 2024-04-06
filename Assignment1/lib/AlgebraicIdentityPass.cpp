#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <llvm/Transforms/Utils/LocalOpts.hpp>

using namespace llvm;

namespace firstAssignment {

/// Checks if an instruction can be simplified by using a neutral constant.
/// Neutral constants are specific values that do not change the result of an
/// operation, such as 0 for addition or 1 for multiplication. If such a
/// constant is found, the function indicates that the instruction can be
/// simplified by replacing it with its other operand.
///
/// @param InstructionRef Reference to the instruction to check.
/// @param OutNeutralConstant Output parameter that stores the neutral constant,
/// if found.
/// @return True if the instruction can be simplified, false otherwise.
static bool
CheckAndSimplifyWithNeutralConstant(Instruction &InstructionRef,
                                    ConstantInt *&OutNeutralConstant) {
  auto OperationCode = InstructionRef.getOpcode();
  Value *Operand1 = InstructionRef.getOperand(0);
  Value *Operand2 = InstructionRef.getOperand(1);

  // Lambda function to determine if an operand is a neutral constant based on
  // the operation.
  auto IsNeutralConstant = [&](Value *Operand) -> bool {
    if (auto *ConstInt = dyn_cast<ConstantInt>(Operand)) {
      if ((OperationCode == Instruction::Add && ConstInt->isZero()) ||
          (OperationCode == Instruction::Mul && ConstInt->isOne())) {
        OutNeutralConstant = ConstInt;
        return true;
      }
    }
    return false;
  };

  // Check operands for neutrality and prepare for simplification if applicable.
  if (IsNeutralConstant(Operand1))
    std::swap(Operand1, Operand2); // Ensure Operand2 is checked as the
                                   // potential neutral constant.
  if (!IsNeutralConstant(Operand2))
    return false; // Exit early if no neutral constant is found.

  // Simplify the instruction by replacing it with the non-neutral operand.
  InstructionRef.replaceAllUsesWith(Operand1);
  return true;
}

/// Simplifies instructions within a basic block by applying algebraic identity
/// simplifications. It iterates over all instructions in the block, identifying
/// and simplifying instructions that can be reduced using neutral constants.
/// The function modifies the basic block in place.
///
/// @param BasicBlockRef Reference to the basic block to be transformed.
/// @return True if any instructions were simplified, indicating a modification
/// to the block.
bool AlgebraicIdentityPass::runOnBasicBlock(BasicBlock &BasicBlockRef) {
  bool HasModificationOccurred = false;
  for (Instruction &InstructionRef :
       llvm::make_early_inc_range(BasicBlockRef)) {
    ConstantInt *NeutralConstant = nullptr;
    if ((InstructionRef.getOpcode() == Instruction::Add ||
         InstructionRef.getOpcode() == Instruction::Mul) &&
        CheckAndSimplifyWithNeutralConstant(InstructionRef, NeutralConstant)) {
      HasModificationOccurred = true;
      InstructionRef
          .eraseFromParent(); // Remove the simplified instruction safely.
    }
  }
  return HasModificationOccurred;
}

} // namespace firstAssignment
