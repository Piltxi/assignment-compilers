#include <llvm/ADT/APInt.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/Casting.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <llvm/Transforms/Utils/LocalOpts.hpp>

using namespace llvm;

namespace firstAssignment {

/// @brief Check if the operand is a constant close to a power of 2, and provide
/// optimization details.
/// @param Op Operand to check.
/// @param OutCI Pointer to store the constant integer value if true.
/// @param ShouldSubtract Indicates if the optimization involves subtraction.
/// @param ShiftAmount The amount to shift for the optimization.
/// @return True if an optimization can be applied.
bool isOptimizableConst(const Value *Op, const ConstantInt **OutCI,
                        bool &ShouldSubtract, unsigned &ShiftAmount) {

  if (const auto *CI = dyn_cast<ConstantInt>(Op)) {
    APInt Value = CI->getValue();

    if (Value.isPowerOf2()) {
      *OutCI = CI;
      ShouldSubtract = false;
      ShiftAmount = Value.logBase2();
      return true;
    }

    if ((Value + 1).isPowerOf2()) {
      *OutCI = CI;
      ShouldSubtract = true;
      ShiftAmount = (Value + 1).logBase2();
      return true;
    }

    if ((Value - 1).isPowerOf2()) {
      *OutCI = CI;
      ShouldSubtract = false;
      ShiftAmount = (Value - 1).logBase2();
      return true;
    }

  }

  return false;
}




/// @brief Applies strength reduction on multiplication and division instructions.
/// This version also handles cases close to powers of 2 by leveraging addition
/// or subtraction after shifting.
/// @param Inst Instruction to possibly transform.
/// @return True if the instruction was transformed.
bool applyStrengthReduction(Instruction &Inst) {
  auto OpCode = Inst.getOpcode();
  if (OpCode != Instruction::Mul && OpCode != Instruction::SDiv &&
      OpCode != Instruction::UDiv)
    return false;

  Value *Op1 = Inst.getOperand(0);
  Value *Op2 = Inst.getOperand(1);
  const ConstantInt *CI = nullptr;
  bool ShouldSubtract = false;
  unsigned ShiftAmount = 0;

  // Attempt to identify a near-power-of-two constant
  if ((OpCode == Instruction::Mul &&
       !isOptimizableConst(Op2, &CI, ShouldSubtract, ShiftAmount)) ||
      (OpCode != Instruction::Mul &&
       !isOptimizableConst(Op2, &CI, ShouldSubtract, ShiftAmount)))
    return false;

  IRBuilder<> Builder(&Inst);
  Value *Shifted = nullptr; // Initialize Shifted to null to avoid warnings
  Value *NewInst = nullptr;

  if (OpCode == Instruction::Mul) {
    Shifted =
        Builder.CreateShl(Op1, ConstantInt::get(CI->getType(), ShiftAmount));
    if (ShouldSubtract) {
      NewInst =
          Builder.CreateSub(Shifted, Op1);
    } else {
      NewInst =
          Builder.CreateAdd(Shifted, Op1);
    }
  } else if (OpCode == Instruction::SDiv || OpCode == Instruction::UDiv) {
    if (ShiftAmount > 0) { // Ensure ShiftAmount is valid for optimization
      // Use arithmetic shift for signed division
      if (OpCode == Instruction::SDiv) {
        Shifted = Builder.CreateAShr(
            Op1, ConstantInt::get(CI->getType(), ShiftAmount));
      }
      // Use logical shift for unsigned division
      else if (OpCode == Instruction::UDiv) {
        Shifted = Builder.CreateLShr(
            Op1, ConstantInt::get(CI->getType(), ShiftAmount));
      }

      NewInst = Shifted; // Directly use the shifted value as the optimized
                         // instruction
    }
  }

  // Ensure NewInst is not null before replacing the instruction
  if (NewInst) {
    Inst.replaceAllUsesWith(NewInst);
    Inst.eraseFromParent();
    return true;
  }

  return false;
}

/// @brief Applies algebraic simplifications to addition and multiplication
/// instructions. Simplifies instructions by removing the operation if one of
/// the operands is the neutral element (0 for addition, 1 for multiplication).
/// @param Inst Instruction to possibly simplify.
/// @return True if the instruction was simplified.
bool applyAlgebraicSimplifications(Instruction &Inst) {
  auto OpCode = Inst.getOpcode();
  if (OpCode != Instruction::Mul && OpCode != Instruction::Add)
    return false;

  Value *Op1 = Inst.getOperand(0);
  Value *Op2 = Inst.getOperand(1);
  const ConstantInt *CI = nullptr;

  auto isNeutralElement = [&CI, OpCode](const Value *Op) -> bool {
    if (const auto *CInt = dyn_cast<ConstantInt>(Op)) {
      if ((OpCode == Instruction::Add && CInt->isZero()) ||
          (OpCode == Instruction::Mul && CInt->isOne())) {
        CI = CInt;
        return true;
      }
    }
    return false;
  };

  if (!isNeutralElement(Op2) && !isNeutralElement(Op1))
    return false;

  Value *NonNeutralOp = (Op1 == CI) ? Op2 : Op1;
  Inst.replaceAllUsesWith(NonNeutralOp);
  Inst.eraseFromParent();

  return true;
}

/// @brief Iterates over all instructions in a basic block and applies
/// strength reduction and algebraic simplifications.
/// @param BB Basic block to transform.
/// @return True if any changes were made to the basic block.
bool StrengthReductionPass::runOnBasicBlock(BasicBlock &BB) {
  bool Changed = false;
  for (auto &Inst : llvm::make_early_inc_range(BB)) {
    Changed |= applyStrengthReduction(Inst);
    Changed |= applyAlgebraicSimplifications(Inst);
  }
  return Changed;
}

} // namespace firstAssignment
