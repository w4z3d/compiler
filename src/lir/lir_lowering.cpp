#include "lir_lowering.hpp"
#include "lir.hpp"

void LIRLowering::lower_module(hir::Module *hir_module) {
  for (const auto &function : hir_module->functions) {
    LIRLowering::lower_function(function.get());
  }
}

void LIRLowering::lower_function(hir::Function *hir_function) {
  current_function = module->add_function(hir_function->name);
  builder.set_function(current_function);

  for (auto *bb : hir_function->blocks) {
    block_map[bb] = builder.create_block(bb->label);
  }
  // Map function arguments to vregs
  for (auto *arg : hir_function->arguments) {
    auto vreg = LIRBuilder::new_vreg();
    vreg_map[arg] = vreg;
    current_function->param_regs.push_back(vreg);
  }
  for (auto *bb : hir_function->blocks) {
    builder.set_insert_point(block_map[bb]);
    lower_block(bb);
  }
}

void LIRLowering::lower_block(hir::BasicBlock *hir_bb) {
  for (auto *instr : hir_bb->instructions) {
    LIRLowering::lower_instruction(instr);
  }
}
static lir::CmpPredicate convert_predicate(hir::ICmpPredicate pred) {
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
  __builtin_unreachable();
}

lir::BasicBlock *LIRLowering::lower_block_from_operand(hir::Value *value) {
  if (auto *arg = dynamic_cast<hir::BasicBlock *>(value)) {
    return get_mbb(arg);
  }
  __builtin_unreachable();
}

lir::Operand LIRLowering::lower_operand(hir::Value *val) {
  if (auto *ci = dynamic_cast<hir::ConstantInt *>(val))
    return lir::Operand::from_imm(ci->signed_value());
  if (auto *bb = dynamic_cast<hir::BasicBlock *>(val))
    return lir::Operand::from_block(get_mbb(bb));

  auto it = vreg_map.find(val);
  assert(it != vreg_map.end() && "value not found in vreg_map");
  return lir::Operand::from_reg(it->second);
}

void LIRLowering::lower_instruction(hir::Instruction *hir_instr) {
  switch (hir_instr->opcode) {
  case hir::Opcode::Add: {
    auto dst = builder.emit_binop(lir::Opcode::Add,
                                  lower_operand(hir_instr->operand(0)),
                                  lower_operand(hir_instr->operand(1)));
    vreg_map[hir_instr] = dst;
    return;
  }
  case hir::Opcode::Sub: {
    auto dst = builder.emit_binop(lir::Opcode::Sub,
                                  lower_operand(hir_instr->operand(0)),
                                  lower_operand(hir_instr->operand(1)));
    vreg_map[hir_instr] = dst;
    return;
  }
  case hir::Opcode::Mul: {
    auto dst = builder.emit_binop(lir::Opcode::Mul,
                                  lower_operand(hir_instr->operand(0)),
                                  lower_operand(hir_instr->operand(1)));
    vreg_map[hir_instr] = dst;
    return;
  }
  case hir::Opcode::SDiv: {
    auto dst = builder.emit_binop(lir::Opcode::SDiv,
                                  lower_operand(hir_instr->operand(0)),
                                  lower_operand(hir_instr->operand(1)));
    vreg_map[hir_instr] = dst;
    return;
  }
  case hir::Opcode::SRem: {
    auto lhs = lower_operand(hir_instr->operand(0));
    auto rhs = lower_operand(hir_instr->operand(1));
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
    builder.emit_cmp(pred, lower_operand(hir_instr->operand(0)),
                     lower_operand(hir_instr->operand(1)));
    auto dst = builder.emit_cset(pred);
    vreg_map[hir_instr] = dst;
    return;
  }
  case hir::Opcode::CondBr: {
    auto cond = lower_operand(hir_instr->operand(0));
    builder.emit_cmp(lir::CmpPredicate::NE, cond, lir::Operand::from_imm(0));
    builder.emit_cond_jump(lir::CmpPredicate::NE,
                           lower_block_from_operand(hir_instr->operand(1)),
                           lower_block_from_operand(hir_instr->operand(2)));
    return;
  }
  case hir::Opcode::Br:
    builder.emit_jump(lower_block_from_operand(hir_instr->operand(0)));
    return;
  case hir::Opcode::Ret:
    if (hir_instr->operand_count() > 0)
      builder.emit_ret(lower_operand(hir_instr->operand(0)));
    else
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
    auto offset = builder.emit_binop(lir::Opcode::Mul, index,
                                     lir::Operand::from_imm(size));
    auto ptr = builder.emit_binop(lir::Opcode::Add, base,
                                  lir::Operand::from_reg(offset));
    vreg_map[hir_instr] = ptr;
    return;
  }
  case hir::Opcode::Phi:
    return;
  default:
    assert(false && "unhandled opcode in lowering");
  }
}
