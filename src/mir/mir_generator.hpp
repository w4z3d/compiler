#ifndef MIR_MIR_GENERATOR
#define MIR_MIR_GENERATOR

#include "../ir/cfg.hpp"
#include "mir.hpp"
#include <vector>
struct MIRGenerator {
private:
  IntermediateRepresentation &representation;
  mir::MIRProgram &mir_program;
  arena::Arena arena;
  std::unordered_map<std::size_t, std::size_t> temp_to_reg{};

  void perform_dfs_basic_block(BasicBlock *current_block,
                               std::set<BasicBlock *> &visited,
                               std::list<BasicBlock *> &linearized_order);
  mir::MachineFunction generate_function(const CFG &cfg);
  void generate_bb(mir::MachineFunction &function, const BasicBlock *bb);
  void generate_add_instruction(mir::MachineFunction &new_block);

  mir::MachineInstruction *create_mov_rr(const mir::MachineOperand &from,
                                         const mir::MachineOperand &to);

  mir::MachineInstruction *create_label(std::size_t block_id);

  mir::MachineInstruction *create_add_rr(const mir::MachineOperand &rhs,
                                         const mir::MachineOperand &target_reg);
  mir::MachineInstruction *create_add_ri(const mir::MachineOperand &from,
                                         const mir::MachineOperand &to);
  mir::MachineInstruction *create_div_rr(const mir::MachineOperand &from,
                                         const mir::MachineOperand &to);
  mir::MachineInstruction *create_sub_rr(const mir::MachineOperand &from,
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
