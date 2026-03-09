#include "lir_builder.hpp"
#include "lir.hpp"
#include <algorithm>

void LIRBuilder::emit_raw(lir::Instruction *instr) const { insert(instr); }

lir::BasicBlock *LIRBuilder::create_block(std::string block_label) {
  auto *mbb = arena.create<lir::BasicBlock>();
  mbb->label = std::move(block_label);
  mbb->id = next_block_id++;
  mbb->parent = function;
  function->blocks.push_back(mbb);
  return mbb;
}

void LIRBuilder::emit_jump(lir::BasicBlock *target) {
  auto *instr = arena.create<lir::Instruction>();
  instr->opcode = lir::Opcode::Jump;
  instr->num_defs = 0;
  instr->operands.push_back(lir::Operand::from_block(target));
  insert(instr);
  insert_block->add_successor(target);
}

void LIRBuilder::emit_cond_jump(lir::CmpPredicate pred,
                                lir::BasicBlock *true_target,
                                lir::BasicBlock *false_target) {
  auto *instr = arena.create<lir::Instruction>();
  instr->opcode = lir::Opcode::CondJump;
  instr->predicate = pred;
  instr->num_defs = 0;
  instr->reads_flags = true;
  instr->operands.push_back(lir::Operand::from_block(true_target));
  instr->operands.push_back(lir::Operand::from_block(false_target));
  insert(instr);
  insert_block->add_successor(true_target);
  insert_block->add_successor(false_target);
}

void LIRBuilder::emit_ret(std::optional<lir::Operand> value) {
  auto *instr = arena.create<lir::Instruction>();
  instr->opcode = lir::Opcode::Ret;
  instr->num_defs = 0;
  if (value)
    instr->operands.push_back(*value);
  insert(instr);
}

lir::Register LIRBuilder::emit_mov(lir::Operand src) {
  auto dst = new_vreg();
  auto *instr = arena.create<lir::Instruction>();
  instr->opcode = lir::Opcode::Mov;
  instr->num_defs = 1;
  instr->operands.push_back(lir::Operand::from_reg(dst));
  instr->operands.push_back(src);
  insert(instr);
  return dst;
}

lir::Register LIRBuilder::emit_copy(lir::Operand src) {
  auto dst = new_vreg();
  auto *instr = arena.create<lir::Instruction>();
  instr->opcode = lir::Opcode::Copy;
  instr->num_defs = 1;
  instr->operands.push_back(lir::Operand::from_reg(dst));
  instr->operands.push_back(src);
  insert(instr);
  return dst;
}

lir::Register LIRBuilder::emit_binop(lir::Opcode op, lir::Operand lhs,
                                     lir::Operand rhs) {
  auto dst = new_vreg();
  if (lhs.is_reg() && lhs.get_reg().get_class() == lir::Register::GPR64 ||
      rhs.is_reg() && rhs.get_reg().get_class() == lir::Register::GPR64) {
    dst.set_class(lir::Register::GPR64);
    rhs.get_reg_mut().set_class(lir::Register::GPR64);
    lhs.get_reg_mut().set_class(lir::Register::GPR64);
  }
  auto *instr = arena.create<lir::Instruction>();
  instr->opcode = op;
  instr->num_defs = 1;
  instr->operands.push_back(lir::Operand::from_reg(dst));
  instr->operands.push_back(lhs);
  instr->operands.push_back(rhs);
  insert(instr);
  return dst;
}

lir::Register LIRBuilder::emit_unop(lir::Opcode op, lir::Operand src) {
  auto dst = new_vreg();
  auto *instr = arena.create<lir::Instruction>();
  instr->opcode = op;
  instr->num_defs = 1;
  instr->operands.push_back(lir::Operand::from_reg(dst));
  instr->operands.push_back(src);
  insert(instr);
  return dst;
}

lir::Register LIRBuilder::emit_cmp(lir::CmpPredicate pred, lir::Operand lhs,
                                   lir::Operand rhs) {
  auto *instr = arena.create<lir::Instruction>();
  instr->opcode = lir::Opcode::Cmp;
  instr->predicate = pred;
  instr->num_defs = 0;
  instr->defs_flags = true;
  instr->operands.push_back(lhs);
  instr->operands.push_back(rhs);
  insert(instr);
  return lir::Register::vreg(0); // no register output
}

lir::Register LIRBuilder::emit_cset(lir::CmpPredicate pred) {
  auto dst = new_vreg();
  auto *instr = arena.create<lir::Instruction>();
  instr->opcode = lir::Opcode::CSet;
  instr->predicate = pred;
  instr->num_defs = 1;
  instr->reads_flags = true;
  instr->operands.push_back(lir::Operand::from_reg(dst));
  insert(instr);
  return dst;
}

lir::Register LIRBuilder::emit_load(lir::Operand addr) {
  auto dst = new_vreg();
  auto *instr = arena.create<lir::Instruction>();
  instr->opcode = lir::Opcode::Load;
  instr->num_defs = 1;
  instr->operands.push_back(lir::Operand::from_reg(dst));
  instr->operands.push_back(addr);
  insert(instr);
  return dst;
}

void LIRBuilder::emit_store(lir::Operand addr, lir::Operand value) {
  auto *instr = arena.create<lir::Instruction>();
  instr->opcode = lir::Opcode::Store;
  instr->num_defs = 0;
  instr->operands.push_back(value);
  instr->operands.push_back(addr);
  insert(instr);
}

lir::Register LIRBuilder::emit_call(std::string_view callee_name,
                                    std::vector<lir::Operand> args,
                                    lir::Register::RegClass ret_clazz,
                                    bool has_return) {
  auto *instr = arena.create<lir::Instruction>();
  instr->opcode = lir::Opcode::Call;
  instr->callee = std::string(callee_name);
  function->has_calls = true;

  lir::Register dst = lir::Register::preg(0);
  dst.set_class(ret_clazz);
  if (has_return) {
    instr->num_defs = 1;
    instr->operands.push_back(lir::Operand::from_reg(dst));
    for (int i = 0; i < 17; i++) {
      instr->add_implicit_def(lir::Register::preg(i));
    }
  }

  for (int i = 0; i < args.size(); i++) {
    auto caller_saved_register = lir::Operand::from_reg(lir::Register::preg(i));
    caller_saved_register.get_reg_mut().set_class(
        args[i].get_reg().get_class());
    auto *copy_instr = arena.create<lir::Instruction>();
    copy_instr->opcode = lir::Opcode::Copy;
    copy_instr->operands.push_back(caller_saved_register);
    copy_instr->operands.push_back(args[i]);
    copy_instr->num_defs = 1;
    emit_raw(copy_instr);
    instr->operands.push_back(caller_saved_register);
  }
  insert(instr);
  auto new_dst = new_vreg();
  new_dst.set_class(ret_clazz);
  auto new_dst_op = lir::Operand::from_reg(new_dst);
  auto dst_op = lir::Operand::from_reg(dst);
  auto *copy_instr = arena.create<lir::Instruction>();
  copy_instr->opcode = lir::Opcode::Copy;
  copy_instr->operands.push_back(new_dst_op);
  copy_instr->operands.push_back(dst_op);
  copy_instr->num_defs = 1;
  emit_raw(copy_instr);
  return new_dst;
}
