#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <utility>
#include <vector>

#include "llvm/Transforms/Utils/LocalOpts.hpp"

using namespace llvm;

namespace firstAssignment {

/// @brief Extract the operands of an instruction as instructions.
/// @param inst Reference to the instruction to extract the operands from.
/// @return A pair of pointers to the operands as instructions.
static std::pair<Instruction *, Instruction *>
extractOperandsAsInstructions(Instruction &inst) {
  return {dyn_cast<Instruction>(inst.getOperand(0)),
          dyn_cast<Instruction>(inst.getOperand(1))};
}

/// @brief Simplify an addition instruction. If the instruction is an addition
/// of a subtraction, the subtraction is simplified.
/// @param inst Reference to the instruction to simplify.
/// @return The simplified value, or nullptr if no simplification is possible.
static Value *simplifyAddition(Instruction &inst) {
  auto [op1, op2] = extractOperandsAsInstructions(inst);

  if (op1 && op1->getOpcode() == Instruction::Sub &&
      op1->getOperand(1) == inst.getOperand(1)) {
    return op1->getOperand(0);
  }

  if (op2 && op2->getOpcode() == Instruction::Sub &&
      op2->getOperand(1) == inst.getOperand(0)) {
    return op2->getOperand(0);
  }

  return nullptr;
}

/// @brief Simplify a subtraction instruction.
/// If the instruction is a subtraction of an addition, the addition is simplified.
/// @param inst Reference to the instruction to simplify.
/// @return The simplified value, or nullptr if no simplification is possible.
static Value *simplifySubtraction(Instruction &inst) {
  auto [op1, op2] = extractOperandsAsInstructions(inst);

  if (op1 && op1->getOpcode() == Instruction::Add) {
    if (op1->getOperand(0) == inst.getOperand(1))
      return op1->getOperand(1);
    if (op1->getOperand(1) == inst.getOperand(1))
      return op1->getOperand(0);
  }

  if (op2 && op2->getOpcode() == Instruction::Add) {
    IRBuilder<> builder(&inst);
    Value *nonMatchOp = op2->getOperand(0) == inst.getOperand(0)
                            ? op2->getOperand(1)
                            : op2->getOperand(0);
    return builder.CreateSub(ConstantInt::get(op2->getType(), 0), nonMatchOp);
  }

  return nullptr;
}

/// @brief Simplify a multiplication instruction.
/// If the instruction is a multiplication of a division, the division is simplified.
/// @param inst Reference to the instruction to simplify.
/// @return The simplified value, or nullptr if no simplification is possible.
static Value *simplifyMultiplication(Instruction &inst) {
  auto [op1, op2] = extractOperandsAsInstructions(inst);

  if (op1 &&
      (op1->getOpcode() == Instruction::SDiv ||
       op1->getOpcode() == Instruction::UDiv) &&
      op1->getOperand(1) == inst.getOperand(1)) {
    return op1->getOperand(0);
  }

  if (op2 &&
      (op2->getOpcode() == Instruction::SDiv ||
       op2->getOpcode() == Instruction::UDiv) &&
      op2->getOperand(1) == inst.getOperand(0)) {
    return op2->getOperand(0);
  }

  return nullptr;
}

/// @brief Simplify a division instruction.
/// If the instruction is a division of a multiplication, the multiplication is simplified.
/// @param inst Reference to the instruction to simplify.
/// @return The simplified value, or nullptr if no simplification is possible.
static Value *simplifyDivision(Instruction &inst) {
  auto [op1, op2] = extractOperandsAsInstructions(inst);

  if (op1 && op1->getOpcode() == Instruction::Mul) {
    Value *otherOp = op1->getOperand(0) == inst.getOperand(1)
                         ? op1->getOperand(1)
                         : op1->getOperand(0);
    return otherOp;
  }

  if (op2 && op2->getOpcode() == Instruction::Mul) {
    IRBuilder<> builder(&inst);
    Value *inverseOp = op2->getOperand(0) == inst.getOperand(0)
                           ? op2->getOperand(1)
                           : op2->getOperand(0);
    return builder.CreateSDiv(ConstantInt::get(op2->getType(), 1), inverseOp);
  }

  return nullptr;
}

/// @brief Apply the optimization pass on a basic block.
/// @param bb Reference to the basic block to optimize.
/// @return True if the basic block was transformed.
bool MIOptimizationPass::runOnBasicBlock(BasicBlock &bb) {
  bool modified = false;
  std::vector<std::pair<Instruction *, Value *>> replacements;

  for (Instruction &inst : bb) {
    Value *newValue = nullptr;
    switch (inst.getOpcode()) {
    case Instruction::Add:
      newValue = simplifyAddition(inst);
      break;
    case Instruction::Sub:
      newValue = simplifySubtraction(inst);
      break;
    case Instruction::Mul:
      newValue = simplifyMultiplication(inst);
      break;
    case Instruction::SDiv:
    case Instruction::UDiv:
      newValue = simplifyDivision(inst);
      break;
    }

    if (newValue)
      replacements.emplace_back(&inst, newValue);
  }

  for (auto &[inst, newVal] : replacements) {
    ReplaceInstWithValue(inst->getIterator(), newVal);
    modified = true;
  }

  return modified;
}

} // namespace firstAssignment
