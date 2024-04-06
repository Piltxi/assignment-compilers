#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <utility>
#include <vector>

#include "llvm/Transforms/Utils/LocalOpts.hpp"

using namespace llvm;

namespace firstAssignment {

/// Extracts the operands from an instruction and casts them to instructions if
/// possible.
///
/// @param instruction Reference to the LLVM instruction to extract operands
/// from.
/// @return A pair of pointers to the operands as instructions, if they can be
/// casted; otherwise, nullptrs.
static std::pair<Instruction *, Instruction *>
extractOperandsAsInstructions(Instruction &instruction) {
  return {dyn_cast<Instruction>(instruction.getOperand(0)),
          dyn_cast<Instruction>(instruction.getOperand(1))};
}

/// Attempts to simplify an addition instruction by identifying and eliminating
/// redundancy.
///
/// This function specifically looks for addition instructions where one of the
/// operands is the result of a subtraction operation that can be simplified.
///
/// @param instruction Reference to the addition instruction to simplify.
/// @return A pointer to the simplified instruction value, or nullptr if
/// simplification isn't applicable.
static Value *simplifyAddition(Instruction &instruction) {
  auto [operand1, operand2] = extractOperandsAsInstructions(instruction);

  if (operand1 && operand1->getOpcode() == Instruction::Sub &&
      operand1->getOperand(1) == instruction.getOperand(1)) {
    return operand1->getOperand(0);
  }

  if (operand2 && operand2->getOpcode() == Instruction::Sub &&
      operand2->getOperand(1) == instruction.getOperand(0)) {
    return operand2->getOperand(0);
  }

  return nullptr;
}

/// Simplifies a subtraction instruction by removing unnecessary operations.
///
/// Looks for cases where subtraction is effectively undoing a previous
/// addition, allowing for simplification.
///
/// @param instruction Reference to the subtraction instruction to simplify.
/// @return A pointer to the simplified instruction value, or nullptr if
/// simplification isn't possible.
static Value *simplifySubtraction(Instruction &instruction) {
  auto [operand1, operand2] = extractOperandsAsInstructions(instruction);

  if (operand1 && operand1->getOpcode() == Instruction::Add) {
    if (operand1->getOperand(0) == instruction.getOperand(1))
      return operand1->getOperand(1);
    if (operand1->getOperand(1) == instruction.getOperand(1))
      return operand1->getOperand(0);
  }

  if (operand2 && operand2->getOpcode() == Instruction::Add) {
    IRBuilder<> builder(&instruction);
    Value *nonMatchingOperand =
        operand2->getOperand(0) == instruction.getOperand(0)
            ? operand2->getOperand(1)
            : operand2->getOperand(0);
    return builder.CreateSub(ConstantInt::get(operand2->getType(), 0),
                             nonMatchingOperand);
  }

  return nullptr;
}

/// Simplifies a multiplication instruction by identifying and eliminating
/// redundancy.
///
/// Specifically targets cases where multiplication undoes a previous division,
/// allowing for simplification.
///
/// @param instruction Reference to the multiplication instruction to simplify.
/// @return A pointer to the simplified instruction value, or nullptr if
/// simplification isn't applicable.
static Value *simplifyMultiplication(Instruction &instruction) {
  auto [operand1, operand2] = extractOperandsAsInstructions(instruction);

  if (operand1 &&
      (operand1->getOpcode() == Instruction::SDiv ||
       operand1->getOpcode() == Instruction::UDiv) &&
      operand1->getOperand(1) == instruction.getOperand(1)) {
    return operand1->getOperand(0);
  }

  if (operand2 &&
      (operand2->getOpcode() == Instruction::SDiv ||
       operand2->getOpcode() == Instruction::UDiv) &&
      operand2->getOperand(1) == instruction.getOperand(0)) {
    return operand2->getOperand(0);
  }

  return nullptr;
}

/// Attempts to simplify a division instruction by removing unnecessary
/// operations.
///
/// Targets cases where division reverses a previous multiplication, enabling
/// simplification.
///
/// @param instruction Reference to the division instruction to simplify.
/// @return A pointer to the simplified instruction value, or nullptr if no
/// simplification is possible.
static Value *simplifyDivision(Instruction &instruction) {
  auto [operand1, operand2] = extractOperandsAsInstructions(instruction);

  if (operand1 && operand1->getOpcode() == Instruction::Mul) {
    Value *alternateOperand =
        operand1->getOperand(0) == instruction.getOperand(1)
            ? operand1->getOperand(1)
            : operand1->getOperand(0);
    return alternateOperand;
  }

  if (operand2 && operand2->getOpcode() == Instruction::Mul) {
    IRBuilder<> builder(&instruction);
    Value *inverseOperand = operand2->getOperand(0) == instruction.getOperand(0)
                                ? operand2->getOperand(1)
                                : operand2->getOperand(0);
    return builder.CreateSDiv(ConstantInt::get(operand2->getType(), 1),
                              inverseOperand);
  }

  return nullptr;
}

/// Applies optimization passes to a basic block, attempting to simplify
/// instructions within it.
///
/// Iterates through each instruction in the basic block, attempting to apply
/// simplifications based on the instruction's type. If simplifications are
/// made, it records these changes.
///
/// @param basicBlock Reference to the basic block to optimize.
/// @return True if any instruction within the basic block was simplified,
/// otherwise false.
bool MIOptimizationPass::runOnBasicBlock(BasicBlock &basicBlock) {
  bool isModified = false;
  std::vector<std::pair<Instruction *, Value *>> replacements;

  for (Instruction &inst : basicBlock) {
    Value *newVal = nullptr;
    switch (inst.getOpcode()) {
    case Instruction::Add:
      newVal = simplifyAddition(inst);
      break;
    case Instruction::Sub:
      newVal = simplifySubtraction(inst);
      break;
    case Instruction::Mul:
      newVal = simplifyMultiplication(inst);
      break;
    case Instruction::SDiv:
    case Instruction::UDiv:
      newVal = simplifyDivision(inst);
      break;
    }

    if (newVal) {
      replacements.emplace_back(&inst, newVal);
    }
  }

  // Apply all collected simplifications.
  for (auto &[inst, newVal] : replacements) {
    auto instIterator = inst->getIterator();
    ReplaceInstWithValue(instIterator, newVal);
    isModified = true;
  }


  return isModified;
}

} // namespace firstAssignment
