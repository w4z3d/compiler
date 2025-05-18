#ifndef CODE_GEN_TARGET_X86_GENERATOR_H
#define CODE_GEN_TARGET_X86_GENERATOR_H

#include "../generator.hpp"

class X86Generator : public Generator {
private:
  static std::string add_assembly_prolouge();

  std::string
  translate_add_rr_instruction(const mir::MachineInstruction &instruction);

public:
  X86Generator() : Generator() {}
  std::string generate_program(mir::MIRProgram program) override;
  std::string translate_basic_block(mir::MachineBasicBlock block) override;
  std::string translate_function(mir::MachineFunction function) override;
  std::string
  translate_instruction(mir::MachineInstruction instruction) override;
};

#endif // !CODE_GEN_TARGET_X86_GENERATOR_H
