#ifndef SRC_LIR_LIR_LOWERING_HPP
#define SRC_LIR_LIR_LOWERING_HPP

#include "../alloc/arena.hpp"
#include "../hir/hir.hpp"
#include "lir.hpp"
#include "lir_builder.hpp"

struct LIRLowering {
  arena::Arena arena;
  lir::Module &module;
  LIRBuilder builder;

  std::unordered_map<hir::Value *, lir::Register> vreg_map;
  std::unordered_map<hir::BasicBlock *, lir::BasicBlock *> block_map;

  lir::Function *current_function = nullptr;
  lir::BasicBlock *insert_block = nullptr;

  explicit LIRLowering(lir::Module &module)
      : arena(arena::Arena{}), builder(LIRBuilder{arena}), module(module) {}

  void lower_module(hir::Module &hir_module);
  void lower_function(hir::Function *hir_function);
  void lower_block(hir::BasicBlock *hir_bb);
  void lower_instruction(hir::Instruction *hir_instr);
  lir::Operand lower_operand(hir::Value *value);
  lir::BasicBlock *lower_block_from_operand(hir::Value *value);
  lir::CmpPredicate convert_predicate(hir::ICmpPredicate pred);
  inline lir::BasicBlock *get_mbb(hir::BasicBlock *bb) { return block_map[bb]; }
};

#endif // !SRC_LIR_LIR_LOWERING_HPP
