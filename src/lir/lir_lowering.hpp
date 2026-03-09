#ifndef SRC_LIR_LIR_LOWERING_HPP
#define SRC_LIR_LIR_LOWERING_HPP

#include "../alloc/arena.hpp"
#include "../codegen/target.hpp"
#include "../hir/hir.hpp"
#include "lir.hpp"
#include "lir_builder.hpp"

struct LIRLowering {
  arena::Arena arena;
  lir::Module &module;
  LIRBuilder builder;
  TargetInfo target_info;

  std::unordered_map<hir::Value *, lir::Register> vreg_map;
  std::unordered_map<hir::BasicBlock *, lir::BasicBlock *> block_map;

  lir::Function *current_function = nullptr;
  lir::BasicBlock *insert_block = nullptr;

  explicit LIRLowering(lir::Module &module, TargetInfo target_info)
      : arena(arena::Arena{}), builder(LIRBuilder{arena}), module(module),
        target_info(target_info) {}

  void lower_module(hir::Module &hir_module);
  void lower_function(hir::Function *hir_function);
  void lower_block(hir::BasicBlock *hir_bb);
  void lower_instruction(hir::Instruction *hir_instr);
  [[nodiscard]] lir::Operand lower_operand(hir::Value *value);
  [[nodiscard]] lir::BasicBlock *lower_block_from_operand(hir::Value *value);
  [[nodiscard]] lir::CmpPredicate convert_predicate(hir::ICmpPredicate pred);
  [[nodiscard]] inline lir::BasicBlock *get_mbb(hir::BasicBlock *bb) {
    return block_map[bb];
  }
  void lower_binop(hir::Instruction *hir_instr, lir::Opcode op);

  [[nodiscard]] lir::Operand ensure_reg(lir::Operand op,
                                        lir::Register::RegClass clazz);
  [[nodiscard]] lir::Register::RegClass class_from_type(hir::type::Type *type);

  void eliminate_phis(hir::Function *hir_function);
  void sequentialize_copies(
      lir::Function *func,
      std::vector<std::pair<lir::Register, lir::Operand>> &copies,
      std::vector<lir::Instruction *> &result);
};

#endif // !SRC_LIR_LIR_LOWERING_HPP
