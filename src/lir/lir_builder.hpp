#ifndef SRC_LIR_LIR_BUILDER_HPP
#define SRC_LIR_LIR_BUILDER_HPP

#include "../alloc/arena.hpp"
#include "lir.hpp"

struct LIRBuilder {

  static int next_block_id;
  static int next_vreg_id;

  arena::Arena &arena;

  explicit LIRBuilder(arena::Arena &arena) : arena(arena) {}

  lir::Function *function = nullptr;
  lir::BasicBlock *insert_block = nullptr;

  void set_function(lir::Function *fn) { function = fn; }
  void set_insert_point(lir::BasicBlock *mbb) { insert_block = mbb; }
  inline void insert(lir::Instruction *instruction) const {
    insert_block->emit(instruction);
  }

  lir::BasicBlock *create_block(std::string label);
  [[nodiscard]] inline static lir::Register new_vreg() {
    return lir::Register::vreg(next_vreg_id++);
  }

  // Convenience emitters — these emit into the given block
  void emit_jump(lir::BasicBlock *target);
  void emit_cond_jump(lir::CmpPredicate pred, lir::BasicBlock *true_target,
                      lir::BasicBlock *false_target);
  void emit_ret(std::optional<lir::Operand> value = std::nullopt);
  lir::Register emit_mov(lir::Operand src);
  lir::Register emit_copy(lir::Operand src);
  lir::Register emit_binop(lir::Opcode op, lir::Operand lhs, lir::Operand rhs);
  lir::Register emit_unop(lir::Opcode op, lir::Operand src);
  lir::Register emit_cmp(lir::CmpPredicate pred, lir::Operand lhs,
                         lir::Operand rhs);
  lir::Register emit_cset(lir::CmpPredicate pred);
  lir::Register emit_load(lir::Operand addr);
  void emit_store(lir::Operand addr, lir::Operand value);
  lir::Register emit_call(std::string_view callee,
                          std::vector<lir::Operand> args,
                          bool has_return = true);
};

#endif
