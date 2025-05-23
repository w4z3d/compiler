#ifndef CODE_GEN_TARGET_X86_GENERATOR_H
#define CODE_GEN_TARGET_X86_GENERATOR_H

#include "../generator.hpp"
#include <sstream>

class X86Generator : public Generator {
private:
  static std::string add_assembly_prolouge();

  static std::string
  translate_add_rr_instruction(mir::MachineInstruction *instruction);

  static std::string
  translate_sub_rr_instruction(mir::MachineInstruction *instruction);

  static std::string
  translate_load_reg_mem_instruction(mir::MachineInstruction *instruction);
  static std::string
  translate_store_mem_reg_instruction(mir::MachineInstruction *instruction);
  static std::string
  translate_store_mem_imm_instruction(mir::MachineInstruction *instruction);

  static std::string
  translate_ret_instruction(mir::MachineInstruction *instruction);
  static std::string
  translate_mov_rr_instruction(mir::MachineInstruction *instruction);
  static std::string
  translate_mov_ri_instruction(mir::MachineInstruction *instruction);

  static std::string
  translate_div_rr_instruction(mir::MachineInstruction *instruction);
  static std::string
  translate_mul_rr_instruction(mir::MachineInstruction *instruction);

  static std::string
  translate_neg_r_instruction(mir::MachineInstruction *instruction);

  static std::string
  translate_pseudo_label_instruction(mir::MachineInstruction *instruction);

  static std::string
  translate_jmp_instruction(mir::MachineInstruction *instruction);

  static std::string
  translate_cmp_instruction(mir::MachineInstruction *instruction);

  static std::string
  translate_jl_instruction(mir::MachineInstruction *instruction);

public:
  X86Generator() : Generator() {}

  std::string generate_program(mir::MIRProgram program) override;
  std::string translate_function(mir::MachineFunction function) override;
  std::string
  translate_instruction(mir::MachineInstruction *instruction) override;
};

#endif // !CODE_GEN_TARGET_X86_GENERATOR_H
