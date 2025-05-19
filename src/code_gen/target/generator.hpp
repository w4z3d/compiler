#ifndef CODE_GEN_TARGET_GENERATOR_H
#define CODE_GEN_TARGET_GENERATOR_H
#include "../../mir/mir.hpp"
class Generator {
  // TODO: Add register_alloc information
public:
  virtual std::string generate_program(mir::MIRProgram program) = 0;
  virtual std::string translate_basic_block(mir::MachineBasicBlock *block) = 0;
  virtual std::string
  translate_instruction(mir::MachineInstruction *instruction) = 0;
  virtual std::string translate_function(mir::MachineFunction function) = 0;
};

#endif // !CODE_GEN_TARGET_GENERATOR_H
