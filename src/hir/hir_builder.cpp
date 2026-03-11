#include "hir_builder.hpp"
#include "hir.hpp"
#include "hir_type.hpp"
#include <string_view>

void HIRBuilder::set_insert_point(hir::BasicBlock *block) {
  insert_block_ = block;
  insert_point_ = block->instructions.end();
}
void HIRBuilder::set_insert_before(hir::Instruction *instr) {
  insert_block_ = instr->parent;
  insert_point_ = std::find(insert_block_->instructions.begin(),
                            insert_block_->instructions.end(), instr);
}

hir::Instruction *HIRBuilder::insert(hir::Instruction *i) {
  assert(insert_block_ && "no insert point set");

  i->parent = insert_block_;
  insert_block_->instructions.insert(insert_point_, i);

  return i;
}

hir::Value *HIRBuilder::build_add(hir::Value *lhs, hir::Value *rhs,
                                  std::string_view name) {
  assert(lhs->type == rhs->type && "operand type mismatch");
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::Add, lhs->type, std::vector<hir::Value *>{lhs, rhs});
  instruction->name = name;
  return insert(instruction);
}

hir::Value *HIRBuilder::build_sub(hir::Value *lhs, hir::Value *rhs,
                                  std::string_view name) {
  assert(lhs->type == rhs->type && "operand type mismatch");
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::Sub, lhs->type, std::vector<hir::Value *>{lhs, rhs});
  instruction->name = name;
  return insert(instruction);
}

hir::Value *HIRBuilder::build_mul(hir::Value *lhs, hir::Value *rhs,
                                  std::string_view name) {
  assert(lhs->type == rhs->type && "operand type mismatch");
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::Mul, lhs->type, std::vector<hir::Value *>{lhs, rhs});
  instruction->name = name;
  return insert(instruction);
}
hir::Value *HIRBuilder::build_sdiv(hir::Value *lhs, hir::Value *rhs,
                                   std::string_view name) {
  assert(lhs->type == rhs->type && "operand type mismatch");
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::SDiv, lhs->type, std::vector<hir::Value *>{lhs, rhs});
  instruction->name = name;
  return insert(instruction);
}
hir::Value *HIRBuilder::build_udiv(hir::Value *lhs, hir::Value *rhs,
                                   std::string_view name) {
  assert(lhs->type == rhs->type && "operand type mismatch");
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::UDiv, lhs->type, std::vector<hir::Value *>{lhs, rhs});
  instruction->name = name;
  return insert(instruction);
}
hir::Value *HIRBuilder::build_srem(hir::Value *lhs, hir::Value *rhs,
                                   std::string_view name) {
  assert(lhs->type == rhs->type && "operand type mismatch");
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::SRem, lhs->type, std::vector<hir::Value *>{lhs, rhs});
  instruction->name = name;
  return insert(instruction);
}
hir::Value *HIRBuilder::build_urem(hir::Value *lhs, hir::Value *rhs,
                                   std::string_view name) {
  assert(lhs->type == rhs->type && "operand type mismatch");
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::URem, lhs->type, std::vector<hir::Value *>{lhs, rhs});
  instruction->name = name;
  return insert(instruction);
}
hir::Value *HIRBuilder::build_neg(hir::Value *lhs, std::string_view name) {
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::Neg, lhs->type, std::vector<hir::Value *>{lhs});
  instruction->name = name;
  return insert(instruction);
}

// Bitwise
hir::Value *HIRBuilder::build_and(hir::Value *lhs, hir::Value *rhs,
                                  std::string_view name) {
  assert(lhs->type == rhs->type && "operand type mismatch");
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::And, lhs->type, std::vector<hir::Value *>{lhs, rhs});
  instruction->name = name;
  return insert(instruction);
}
hir::Value *HIRBuilder::build_or(hir::Value *lhs, hir::Value *rhs,
                                 std::string_view name) {
  assert(lhs->type == rhs->type && "operand type mismatch");
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::Or, lhs->type, std::vector<hir::Value *>{lhs, rhs});
  instruction->name = name;
  return insert(instruction);
}
hir::Value *HIRBuilder::build_xor(hir::Value *lhs, hir::Value *rhs,
                                  std::string_view name) {
  assert(lhs->type == rhs->type && "operand type mismatch");
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::Xor, lhs->type, std::vector<hir::Value *>{lhs, rhs});
  instruction->name = name;
  return insert(instruction);
}
hir::Value *HIRBuilder::build_shl(hir::Value *lhs, hir::Value *rhs,
                                  std::string_view name) {
  assert(lhs->type == rhs->type && "operand type mismatch");
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::Shl, lhs->type, std::vector<hir::Value *>{lhs, rhs});
  instruction->name = name;
  return insert(instruction);
}
hir::Value *HIRBuilder::build_lshr(hir::Value *lhs, hir::Value *rhs,
                                   std::string_view name) {
  assert(lhs->type == rhs->type && "operand type mismatch");
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::LShr, lhs->type, std::vector<hir::Value *>{lhs, rhs});
  instruction->name = name;
  return insert(instruction);
}
hir::Value *HIRBuilder::build_ashr(hir::Value *lhs, hir::Value *rhs,
                                   std::string_view name) {
  assert(lhs->type == rhs->type && "operand type mismatch");
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::AShr, lhs->type, std::vector<hir::Value *>{lhs, rhs});
  instruction->name = name;
  return insert(instruction);
}
hir::Value *HIRBuilder::build_not(hir::Value *lhs, std::string_view name) {
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::Not, lhs->type, std::vector<hir::Value *>{lhs});
  instruction->name = name;
  return insert(instruction);
}

hir::Value *HIRBuilder::build_icmp(hir::ICmpPredicate predicate,
                                   hir::Value *lhs, hir::Value *rhs,
                                   std::string_view name) {
  assert(lhs && "LHS must not be null");
  assert(rhs && "RHS must not be null");
  assert(lhs->type == rhs->type && "operand type mismatch");
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::ICmp, context.i1(), std::vector<hir::Value *>{lhs, rhs});
  instruction->name = name;
  instruction->predicate = std::make_optional(predicate);
  return insert(instruction);
}
hir::Value *HIRBuilder::build_alloca(hir::type::Type *allocated_type,
                                     std::string_view name) {
  auto instruction = arena.create<hir::Instruction>(
      hir::Opcode::Alloca, context.ptr(), std::vector<hir::Value *>{});
  instruction->name = name;
  instruction->type_arg = allocated_type;
  return insert(instruction);
}

hir::Value *HIRBuilder::build_load(hir::type::Type *loaded_type,
                                   hir::Value *ptr, std::string_view name) {
  auto *instruction = arena.create<hir::Instruction>(
      hir::Opcode::Load, loaded_type, std::vector<hir::Value *>{ptr});
  instruction->name = name;
  instruction->type_arg = ptr->type;
  return insert(instruction);
}

void HIRBuilder::build_store(hir::Value *value, hir::Value *ptr) {
  auto *instruction =
      arena.create<hir::Instruction>(hir::Opcode::Store, context.void_t(),
                                     std::vector<hir::Value *>{value, ptr});
  instruction->type_arg = value->type;

  insert(instruction);
}

hir::Value *HIRBuilder::build_gep(hir::type::Type *base_type, hir::Value *ptr,
                                  const std::vector<hir::Value *> &indices,
                                  std::string_view name) {

  std::vector<hir::Value *> uses;
  uses.push_back(ptr);
  for (auto *idx : indices)
    uses.push_back(idx);

  auto *instr = arena.create<hir::Instruction>(hir::Opcode::GetElementPtr,
                                               context.ptr(), uses);
  instr->type_arg = base_type;
  instr->name = name;
  return insert(instr);
}
hir::Value *HIRBuilder::build_ptr_to_int(hir::Value *v,
                                         hir::type::IntegerType *to,
                                         std::string_view name) {
  assert(v->type->is_pointer() && "ptrtoint operand must be a pointer");
  assert(to && "target type cannot be null");

  auto *instr = arena.create<hir::Instruction>(hir::Opcode::PtrToInt, to,
                                               std::vector<hir::Value *>{v});
  instr->name = name;
  return insert(instr);
}
hir::Value *HIRBuilder::build_int_to_ptr(hir::Value *v,
                                         hir::type::IntegerType *to,
                                         std::string_view name) {
  assert(v->type->is_integer() && "int_to_ptr operand must be an integer");
  assert(to && "from type cannot be null");

  auto *instr = arena.create<hir::Instruction>(
      hir::Opcode::PtrToInt, context.ptr(), std::vector<hir::Value *>{v});
  instr->name = name;
  return insert(instr);
}

void HIRBuilder::build_br(hir::BasicBlock *target) {
  assert(insert_block_ && "no insert point set");
  assert(!insert_block_->terminator() && "block already has a terminator");
  auto *instruction = arena.create<hir::Instruction>(
      hir::Opcode::Br, context.void_t(), std::vector<hir::Value *>{target});
  insert(instruction);
}

void HIRBuilder::build_cond_br(hir::Value *cond, hir::BasicBlock *true_block,
                               hir::BasicBlock *false_block) {
  assert(insert_block_ && "no insert point set");
  assert(!insert_block_->terminator() && "block already has a terminator");
  assert(cond->type == context.i1() && "cond_br condition must be i1");
  assert(true_block && false_block && "branch targets cannot be null");

  auto *instr = arena.create<hir::Instruction>(
      hir::Opcode::CondBr, context.void_t(),
      std::vector<hir::Value *>{cond, true_block, false_block});
  insert(instr);
}

void HIRBuilder::build_ret(hir::Value *val) {
  assert(insert_block_ && "no insert point set");
  assert(!insert_block_->terminator() && "block already has a terminator");

  std::vector<hir::Value *> operands;
  if (val)
    operands.push_back(val);

  auto *instr = arena.create<hir::Instruction>(
      hir::Opcode::Ret, context.void_t(), std::move(operands));
  insert(instr);
}

hir::Value *HIRBuilder::build_call(hir::Function *callee,
                                   std::vector<hir::Value *> args,
                                   std::string_view name) {
  assert(callee && "callee cannot be null");

  assert(args.size() == callee->function_type->param_types.size() &&
         "argument count mismatch");

  for (size_t i = 0; i < args.size(); i++) {
    assert(args[i]->type == callee->function_type->param_types[i] &&
           "argument type mismatch");
  }

  std::vector<hir::Value *> operands;
  operands.push_back(callee);
  operands.insert(operands.end(), args.begin(), args.end());

  // Result type is the function's return type
  auto *instr = arena.create<hir::Instruction>(
      hir::Opcode::Call, callee->function_type->return_type,
      std::move(operands));
  instr->type_arg = callee->function_type;
  instr->name = name;
  return insert(instr);
}

hir::PhiNode *HIRBuilder::build_phi(hir::type::Type *type,
                                    std::string_view name) {
  assert(insert_block_ && "no insert point set");
  assert(type && "phi type cannot be null");

  // Phi nodes must be at the top of the block — insert before any non-phi
  // Find the first non-phi instruction and insert before it
  auto insert_before_non_phi = [&]() {
    for (auto it = insert_block_->instructions.begin();
         it != insert_block_->instructions.end(); ++it) {
      if ((*it)->opcode != hir::Opcode::Phi) {
        insert_point_ = it;
        return;
      }
    }
    // All instructions are phis or block is empty
    insert_point_ = insert_block_->instructions.end();
  };

  // Temporarily move insert point to phi position
  auto saved_point = insert_point_;
  insert_before_non_phi();

  auto *phi = arena.create<hir::PhiNode>(type, insert_block_);
  phi->name = name;
  insert(phi);

  // Restore insert point
  insert_point_ = saved_point;

  return phi;
}
