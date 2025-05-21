#ifndef MIR_MIR_GENERATOR
#define MIR_MIR_GENERATOR

#include "../ir/cfg.hpp"
#include "mir.hpp"
struct MIRGenerator {
private:
  IntermediateRepresentation &representation;
  mir::MIRProgram &mir_program;
  arena::Arena arena;
  std::unordered_map<std::size_t, std::size_t> temp_to_reg{};

  mir::MachineFunction generate_function(const CFG &cfg);
  mir::MachineBasicBlock *generate_bb(const BasicBlock *bb);
  void generate_add_instruction(mir::MachineBasicBlock *new_block);

  mir::MachineInstruction *create_mov_rr(const mir::MachineOperand &from,
                                         const mir::MachineOperand &to);

  template <class... Ts> struct overload : Ts... {
    using Ts::operator()...;
  };
  static inline bool is_immediate(const mir::MachineOperand &op) {
    return std::holds_alternative<mir::Immediate>(op.get_op());
  }
  static inline bool is_register(const mir::MachineOperand &op) {
    return std::holds_alternative<mir::VirtualRegister>(op.get_op());
  }

public:
  explicit MIRGenerator(IntermediateRepresentation &representation,
                        mir::MIRProgram &program)
      : representation(representation), mir_program(program),
        arena(arena::Arena{}) {}
  void generate();
};

#endif // !MIR_MIR_GENERATOR
