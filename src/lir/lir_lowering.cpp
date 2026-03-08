#include "lir_lowering.hpp"
#include "lir.hpp"
#include <unordered_set>
#include <utility>

void LIRLowering::lower_module(hir::Module &hir_module) {
  for (const auto &function : hir_module.functions) {
    LIRLowering::lower_function(function.get());
  }
}

void LIRLowering::lower_function(hir::Function *hir_function) {
  vreg_map.clear();
  block_map.clear();

  current_function = module.add_function(hir_function->name);
  current_function->is_extern = hir_function->is_extern;
  if (hir_function->is_extern)
    return;
  builder.set_function(current_function);

  for (auto *bb : hir_function->blocks) {
    block_map[bb] = builder.create_block(bb->label);
  }
  // Map function arguments to vregs
  builder.set_insert_point(current_function->entry_block());
  for (int i = 0; i < hir_function->arguments.size(); i++) {
    auto arg = hir_function->arguments[i];
    auto arg_reg = lir::Register::preg(i);
    auto callee_saved_reg = target_info.callee_saved[i];
    auto *copy = arena.create<lir::Instruction>();
    copy->opcode = lir::Opcode::Copy;
    copy->operands.push_back(lir::Operand::from_reg(callee_saved_reg));
    copy->operands.push_back(lir::Operand::from_reg(arg_reg));
    copy->num_defs = 1;
    builder.emit_raw(copy);
    vreg_map[arg] = callee_saved_reg;
    current_function->param_regs.push_back(callee_saved_reg);
  }
  for (auto *bb : hir_function->blocks) {
    builder.set_insert_point(block_map[bb]);
    lower_block(bb);
  }

  eliminate_phis(hir_function);
}

void LIRLowering::lower_block(hir::BasicBlock *hir_bb) {
  for (auto *instr : hir_bb->instructions) {
    LIRLowering::lower_instruction(instr);
  }
}
lir::CmpPredicate LIRLowering::convert_predicate(hir::ICmpPredicate pred) {
  switch (pred) {
  case hir::ICmpPredicate::EQ:
    return lir::CmpPredicate::EQ;
  case hir::ICmpPredicate::NE:
    return lir::CmpPredicate::NE;
  case hir::ICmpPredicate::SLT:
    return lir::CmpPredicate::SLT;
  case hir::ICmpPredicate::SLE:
    return lir::CmpPredicate::SLE;
  case hir::ICmpPredicate::SGT:
    return lir::CmpPredicate::SGT;
  case hir::ICmpPredicate::SGE:
    return lir::CmpPredicate::SGE;
  case hir::ICmpPredicate::ULT:
    return lir::CmpPredicate::ULT;
  case hir::ICmpPredicate::ULE:
    return lir::CmpPredicate::ULE;
  case hir::ICmpPredicate::UGT:
    return lir::CmpPredicate::UGT;
  case hir::ICmpPredicate::UGE:
    return lir::CmpPredicate::UGE;
  }
  std::unreachable();
}

static lir::Opcode convert_binop(hir::Opcode op) {
  switch (op) {
  case hir::Opcode::Add:
    return lir::Opcode::Add;
  case hir::Opcode::Sub:
    return lir::Opcode::Sub;
  case hir::Opcode::Mul:
    return lir::Opcode::Mul;
  case hir::Opcode::SDiv:
    return lir::Opcode::SDiv;
  case hir::Opcode::And:
    return lir::Opcode::And;
  case hir::Opcode::Or:
    return lir::Opcode::Or;
  case hir::Opcode::Xor:
    return lir::Opcode::Xor;
  case hir::Opcode::Shl:
    return lir::Opcode::Shl;
  case hir::Opcode::AShr:
    return lir::Opcode::AShr;
  case hir::Opcode::LShr:
    return lir::Opcode::LShr;
  default:
    std::unreachable();
  }
}

lir::BasicBlock *LIRLowering::lower_block_from_operand(hir::Value *value) {
  if (auto *arg = dynamic_cast<hir::BasicBlock *>(value)) {
    return get_mbb(arg);
  }
  std::unreachable();
}

lir::Operand LIRLowering::ensure_reg(lir::Operand op) {
  if (op.is_reg())
    return op;
  auto tmp = builder.emit_mov(op);
  return lir::Operand::from_reg(tmp);
}

lir::Operand LIRLowering::lower_operand(hir::Value *val) {
  if (auto *ci = dynamic_cast<hir::ConstantInt *>(val))
    return lir::Operand::from_imm(ci->signed_value());
  if (auto *bb = dynamic_cast<hir::BasicBlock *>(val))
    return lir::Operand::from_block(get_mbb(bb));
  if (auto *instr = dynamic_cast<hir::Instruction *>(val))
    return lir::Operand::from_reg(vreg_map[val]);

  auto it = vreg_map[val];
  return lir::Operand::from_reg(it);
}

void LIRLowering::lower_instruction(hir::Instruction *hir_instr) {
  switch (hir_instr->opcode) {
  case hir::Opcode::Add:
    lower_binop(hir_instr, lir::Opcode::Add);
    return;
  case hir::Opcode::Sub:
    lower_binop(hir_instr, lir::Opcode::Sub);
    return;
  case hir::Opcode::Mul:
    lower_binop(hir_instr, lir::Opcode::Mul);
    return;
  case hir::Opcode::SDiv:
    lower_binop(hir_instr, lir::Opcode::SDiv);
    return;
  case hir::Opcode::And:
    lower_binop(hir_instr, lir::Opcode::And);
    return;
  case hir::Opcode::Or:
    lower_binop(hir_instr, lir::Opcode::Or);
    return;
  case hir::Opcode::Xor:
    lower_binop(hir_instr, lir::Opcode::Xor);
    return;
  case hir::Opcode::Shl:
    lower_binop(hir_instr, lir::Opcode::Shl);
    return;
  case hir::Opcode::AShr:
    lower_binop(hir_instr, lir::Opcode::AShr);
    return;
  case hir::Opcode::LShr:
    lower_binop(hir_instr, lir::Opcode::LShr);
    return;
  case hir::Opcode::Neg:
  case hir::Opcode::Not: {
    auto dst = builder.emit_unop(lir::Opcode::Neg,
                                 lower_operand(hir_instr->operand(0)));
    vreg_map[hir_instr] = dst;
    return;
  }
  case hir::Opcode::SRem: {
    auto lhs = lower_operand(hir_instr->operand(0));
    auto rhs = lower_operand(hir_instr->operand(1));
    lhs = ensure_reg(lhs);
    rhs = ensure_reg(rhs);
    auto quot = builder.emit_binop(lir::Opcode::SDiv, lhs, rhs);
    auto prod =
        builder.emit_binop(lir::Opcode::Mul, lir::Operand::from_reg(quot), rhs);
    auto rem =
        builder.emit_binop(lir::Opcode::Sub, lhs, lir::Operand::from_reg(prod));
    vreg_map[hir_instr] = rem;
    return;
  }
  case hir::Opcode::ICmp: {
    auto pred = convert_predicate(hir_instr->predicate.value());
    auto dst = builder.emit_cmp(pred, lower_operand(hir_instr->operand(0)),
                                lower_operand(hir_instr->operand(1)));
    vreg_map[hir_instr] = dst;
    return;
  }
  case hir::Opcode::CondBr: {
    auto *condition = dynamic_cast<hir::Instruction *>(hir_instr->operand(0));
    auto predicate = convert_predicate(condition->predicate.value());
    builder.emit_cond_jump(predicate,
                           lower_block_from_operand(hir_instr->operand(1)),
                           lower_block_from_operand(hir_instr->operand(2)));
    return;
  }
  case hir::Opcode::Br:
    builder.emit_jump(lower_block_from_operand(hir_instr->operand(0)));
    return;
  case hir::Opcode::Ret:
    if (hir_instr->operand_count() > 0) {
      auto ret_register = lir::Operand::from_reg(lir::Register::preg(0));
      auto *instr = arena.create<lir::Instruction>();
      instr->opcode = lir::Opcode::Mov;
      instr->num_defs = 1;
      instr->operands.push_back(ret_register);
      instr->operands.push_back(lower_operand(hir_instr->operand(0)));
      builder.emit_raw(instr);
      builder.emit_ret(ret_register);
    } else
      builder.emit_ret();
    return;
  case hir::Opcode::Call: {
    std::vector<lir::Operand> args;
    for (size_t i = 1; i < hir_instr->operand_count(); i++) {
      args.push_back(lower_operand(hir_instr->operand(i)));
    }
    auto *callee = dynamic_cast<hir::Function *>(hir_instr->operand(0));
    auto dst = builder.emit_call(callee->name, std::move(args));
    vreg_map[hir_instr] = dst;
    return;
  }
  case hir::Opcode::Load: {
    auto dst = builder.emit_load(lower_operand(hir_instr->operand(0)));
    vreg_map[hir_instr] = dst;
    return;
  }
  case hir::Opcode::Store:
    builder.emit_store(lower_operand(hir_instr->operand(0)),
                       lower_operand(hir_instr->operand(1)));
    return;
  case hir::Opcode::GetElementPtr: {
    auto base = lower_operand(hir_instr->operand(0));
    auto index = lower_operand(hir_instr->operand(1));
    auto size = hir_instr->type_arg->size_of();
    auto size_op = lir::Operand::from_imm(size);

    auto index_reg = ensure_reg(index);
    auto size_reg = ensure_reg(size_op);
    auto offset = builder.emit_binop(lir::Opcode::Mul, index_reg, size_reg);

    auto ptr = builder.emit_binop(lir::Opcode::Add, base,
                                  lir::Operand::from_reg(offset));
    vreg_map[hir_instr] = ptr;
    return;
  }
  case hir::Opcode::Phi: {
    auto dst = builder.new_vreg();
    vreg_map[hir_instr] = dst;
    return;
  }
  default:
    std::cout << "Unhandled opcode " + (hir_instr->to_string()) << std::endl;
    assert(false && "unhandled opcode in lowering");
  }
}

void LIRLowering::lower_binop(hir::Instruction *hir_instr, lir::Opcode op) {
  auto lhs = lower_operand(hir_instr->operand(0));
  auto rhs = lower_operand(hir_instr->operand(1));

  // First operand must always be a register
  lhs = ensure_reg(lhs);

  // Second operand: check if target accepts immediate
  if (rhs.is_imm() && !target_info.accepts_imm(op))
    rhs = ensure_reg(rhs);

  vreg_map[hir_instr] = builder.emit_binop(op, lhs, rhs);
}

void LIRLowering::eliminate_phis(hir::Function *hir_function) {
  std::unordered_map<hir::BasicBlock *,
                     std::vector<std::pair<lir::Register, lir::Operand>>>
      copies_per_pred;

  for (auto *bb : hir_function->blocks) {
    for (auto *inst : bb->instructions) {
      if (inst->opcode != hir::Opcode::Phi)
        continue;

      auto *phi = dynamic_cast<hir::PhiNode *>(inst);
      auto dst = vreg_map[phi];

      for (size_t i = 0; i < phi->incoming_blocks.size(); i++) {
        auto *incoming_bb = phi->incoming_blocks[i];
        auto *incoming_val = phi->incoming_from(incoming_bb);
        auto src = lower_operand(incoming_val);

        if (src.is_reg() && src.get_reg() == dst)
          continue;

        copies_per_pred[incoming_bb].emplace_back(dst, src);
      }
    }
  }

  for (auto &[hir_pred, copies] : copies_per_pred) {
    auto *pred_mbb = block_map[hir_pred];

    std::vector<lir::Instruction *> sequentialized;
    sequentialize_copies(current_function, copies, sequentialized);

    // Insert before terminator
    auto &instrs = pred_mbb->instructions;
    auto term_it = instrs.end();
    if (!instrs.empty() && instrs.back()->is_terminator())
      term_it = std::prev(instrs.end());

    instrs.insert(term_it, sequentialized.begin(), sequentialized.end());
  }
}

void LIRLowering::sequentialize_copies(
    lir::Function *func,
    std::vector<std::pair<lir::Register, lir::Operand>> &copies,
    std::vector<lir::Instruction *> &result) {

  std::unordered_set<unsigned> is_dst;
  for (auto &[dst, src] : copies)
    is_dst.insert(dst.id);

  // Repeatedly emit copies whose dst is not a src of remaining copies
  std::vector<bool> emitted(copies.size(), false);
  bool progress = true;

  while (progress) {
    progress = false;
    for (size_t i = 0; i < copies.size(); i++) {
      if (emitted[i])
        continue;

      auto &[dst, src] = copies[i];

      bool blocked = false;
      for (size_t j = 0; j < copies.size(); j++) {
        if (j == i || emitted[j])
          continue;
        if (copies[j].second.is_reg() && copies[j].second.get_reg() == dst) {
          blocked = true;
          break;
        }
      }

      if (!blocked) {
        auto *copy = new lir::Instruction();
        copy->opcode = lir::Opcode::Copy;
        copy->num_defs = 1;
        copy->operands.push_back(lir::Operand::from_reg(dst));
        copy->operands.push_back(src);
        result.push_back(copy);
        emitted[i] = true;
        progress = true;
      }
    }
  }

  // Remaining copies form cycles, break each with one temp
  for (size_t i = 0; i < copies.size(); i++) {
    if (emitted[i])
      continue;

    auto tmp = builder.new_vreg();
    auto &[dst, src] = copies[i];

    auto *save = new lir::Instruction();
    save->opcode = lir::Opcode::Copy;
    save->num_defs = 1;
    save->operands.push_back(lir::Operand::from_reg(tmp));
    save->operands.push_back(lir::Operand::from_reg(dst));
    result.push_back(save);

    auto *copy = new lir::Instruction();
    copy->opcode = lir::Opcode::Copy;
    copy->num_defs = 1;
    copy->operands.push_back(lir::Operand::from_reg(dst));
    copy->operands.push_back(src);
    result.push_back(copy);
    emitted[i] = true;

    // Follow the cycle: find who needs the value we saved
    unsigned saved_id = dst.id;
    for (size_t j = 0; j < copies.size(); j++) {
      if (emitted[j])
        continue;
      if (copies[j].second.is_reg() &&
          copies[j].second.get_reg().id == saved_id) {
        copies[j].second = lir::Operand::from_reg(tmp);
        // Now this copy is unblocked — emit it
        auto *c = new lir::Instruction();
        c->opcode = lir::Opcode::Copy;
        c->num_defs = 1;
        c->operands.push_back(lir::Operand::from_reg(copies[j].first));
        c->operands.push_back(copies[j].second);
        result.push_back(c);
        emitted[j] = true;
        saved_id = copies[j].first.id;
        j = 0;
      }
    }
  }
}
