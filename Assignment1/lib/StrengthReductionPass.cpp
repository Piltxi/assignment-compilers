#include <llvm/ADT/APInt.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/Casting.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <llvm/Transforms/Utils/LocalOpts.hpp>

using namespace llvm;

namespace firstAssignment {

/// Determines if a given constant operand can be optimized by identifying
/// whether it is a power of 2 or close to it. If so, it calculates the optimal
/// shift amount and indicates whether subtraction is required for optimization.
///
/// @param Operand The LLVM Value operand to analyze.
/// @param ConstantValue A pointer to store the constant integer value if
/// applicable.
/// @param RequiresSubtraction Flag indicating if subtraction is needed for
/// optimization.
/// @param ShiftAmount The calculated shift amount required for the
/// optimization.
/// @return True if the operand is optimizable; otherwise, false.
bool isOptimizableConstant(const Value *Operand,
                           const ConstantInt **ConstantValue,
                           bool &RequiresSubtraction, unsigned &ShiftAmount) {

  if (const auto *ConstInt = dyn_cast<ConstantInt>(Operand)) {
    APInt Val = ConstInt->getValue();
    unsigned NearestPower = Val.nearestLogBase2();
    APInt NearestPowerValue = APInt(Val.getBitWidth(), 1) << NearestPower;

    // Check if the value or its closest neighbors are powers of 2
    if (NearestPowerValue == Val) {
      *ConstantValue = ConstInt;
      RequiresSubtraction = false;
      ShiftAmount = NearestPower;
      return true;
    } else if ((NearestPowerValue - 1) == Val ||
               (NearestPowerValue + 1) == Val) {
      *ConstantValue = ConstInt;
      RequiresSubtraction = NearestPowerValue.ugt(Val);
      ShiftAmount = NearestPower;
      return true;
    }
  }

  return false;
}

/// Applies optimization techniques such as strength reduction on multiplication
/// and division instructions. It also considers near powers of 2 to further
/// optimize the operations by utilizing shifts and additions or subtractions.
///
/// @param InstructionRef Reference to the instruction to potentially optimize.
/// @return True if the instruction was optimized; otherwise, false.
bool applyStrengthReduction(Instruction &InstructionRef) {
  auto OperationCode = InstructionRef.getOpcode();
  // Check if operation is eligible for strength reduction
  if (OperationCode != Instruction::Mul && OperationCode != Instruction::SDiv &&
      OperationCode != Instruction::UDiv)
    return false;

  Value *Operand1 = InstructionRef.getOperand(0);
  Value *Operand2 = InstructionRef.getOperand(1);
  const ConstantInt *OptimizableConst = nullptr;
  bool ShouldSubtract = false;
  unsigned ShiftValue = 0;

  // Identify if second operand can be optimized
  if ((OperationCode == Instruction::Mul &&
       !isOptimizableConstant(Operand2, &OptimizableConst, ShouldSubtract,
                              ShiftValue)) ||
      (OperationCode != Instruction::Mul &&
       !isOptimizableConstant(Operand2, &OptimizableConst, ShouldSubtract,
                              ShiftValue)))
    return false;

  IRBuilder<> Builder(&InstructionRef);
  Value *ShiftedValue = nullptr;
  Value *OptimizedInstruction = nullptr;

  // Apply optimization based on operation type
  if (OperationCode == Instruction::Mul) {
    ShiftedValue = Builder.CreateShl(
        Operand1, ConstantInt::get(OptimizableConst->getType(), ShiftValue));
    OptimizedInstruction = ShouldSubtract
                               ? Builder.CreateSub(ShiftedValue, Operand1)
                               : Builder.CreateAdd(ShiftedValue, Operand1);
  } else if (OperationCode == Instruction::SDiv ||
             OperationCode == Instruction::UDiv) {
    if (ShiftValue > 0) { // Valid shift value check
      ShiftedValue =
          (OperationCode == Instruction::SDiv)
              ? Builder.CreateAShr(
                    Operand1,
                    ConstantInt::get(OptimizableConst->getType(), ShiftValue))
              : Builder.CreateLShr(
                    Operand1,
                    ConstantInt::get(OptimizableConst->getType(), ShiftValue));
      OptimizedInstruction = ShiftedValue;
    }
  }

  // Replace original instruction with optimized one if applicable
  if (OptimizedInstruction) {
    InstructionRef.replaceAllUsesWith(OptimizedInstruction);
    InstructionRef.eraseFromParent();
    return true;
  }

  return false;
}

/// Iterates over all instructions within a given basic block and applies
/// optimizations including strength reduction and algebraic simplifications.
///
/// @param BasicBlockRef Reference to the basic block to optimize.
/// @return True if any instruction within the block was optimized; otherwise,
/// false.
bool StrengthReductionPass::runOnBasicBlock(BasicBlock &BasicBlockRef) {
  bool HasChanged = false;
  for (auto &Inst : llvm::make_early_inc_range(BasicBlockRef)) {
    HasChanged |= applyStrengthReduction(Inst);
  }
  return HasChanged;
}

} // namespace firstAssignment
