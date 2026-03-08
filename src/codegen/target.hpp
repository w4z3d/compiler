#ifndef SRC_CODEGEN_TARGET_HPP
#define SRC_CODEGEN_TARGET_HPP

#include "../lir/lir.hpp"

struct TargetInfo {
  const lir::Register *arg_regs;
  size_t num_arg_regs;
  lir::Register ret_reg;
  const lir::Register *caller_saved;
  size_t num_caller_saved;
  const lir::Register *callee_saved;
  size_t num_callee_saved;
  const lir::Register *allocatable;
  size_t num_allocatable;

  [[nodiscard]] const char *reg_name(lir::Register reg) const;
  [[nodiscard]] bool accepts_imm(lir::Opcode op) const;
};

#endif
