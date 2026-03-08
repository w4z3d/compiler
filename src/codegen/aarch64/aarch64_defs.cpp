// codegen/aarch64/aarch64_defs.cpp
#include "aarch64_defs.hpp"

static const char *aarch64_reg_names[] = {
    "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",  "x8",
    "x9",  "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17",
    "x18", "x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26",
    "x27", "x28", "fp",  "lr",  "sp",  "xzr"};

const char *TargetInfo::reg_name(lir::Register reg) const {
  if (reg.is_virtual())
    return nullptr;
  if (reg.id <= 32)
    return aarch64_reg_names[reg.id];
  return "???";
}

bool TargetInfo::accepts_imm(lir::Opcode op) const {

  switch (op) {
  case lir::Opcode::Add:
  case lir::Opcode::Sub:
  case lir::Opcode::Cmp:
  case lir::Opcode::And:
  case lir::Opcode::Or:
  case lir::Opcode::Xor:
  case lir::Opcode::Shl:
  case lir::Opcode::AShr:
  case lir::Opcode::LShr:
    return true;
  default:
    return false;
  }
}
