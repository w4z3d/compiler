#ifndef CODEGEN_ARCH_ARCH_H
#define CODEGEN_ARCH_ARCH_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace arch {

class Register {
  std::string name;
  std::uint32_t size;

  std::vector<Register> sub_register;

public:
  Register(std::string name, std::uint32_t size,
           std::vector<Register> sub_register)
      : name(std::move(name)), size(size),
        sub_register(std::move(sub_register)) {}
};

class Architecture {
private:
  std::string name;

  // General purpose 32 bit registers
  std::vector<Register> GPR32;

  // General purpose 64 bit registers
  std::vector<Register> GPR64;

public:
  Architecture(std::string name, std::vector<Register> GPR32,
               std::vector<Register> GPR64)
      : name(std::move(name)), GPR32(std::move(GPR32)),
        GPR64(std::move(GPR64)) {}
};

class Instruction {
private:
  std::string mnemonic;

  // Asm output string containing placeholders like $$$imm or $reg
  std::string asm_string;

  std::vector<Register> ins;
  std::vector<Register> outs;

  // e.g. division or memory reads/writes
  bool has_sideeffects;

public:
  Instruction(std::string mnemonic, std::string asm_string,
              std::vector<Register> ins, std::vector<Register> outs,
              bool has_sideeffects = false)
      : mnemonic(std::move(mnemonic)), asm_string(std::move(asm_string)),
        ins(std::move(ins)), outs(std::move(outs)),
        has_sideeffects(has_sideeffects) {}
};
} // namespace arch
#endif // !CODEGEN_ARCH_ARCH_H
