#include "mir_generator.hpp"
#include "mir.hpp"

void MIRGenerator::generate() {
  for (const auto &cfg : representation.get_cfgs()) {
    mir_program.add_function(generate_function(cfg));
  }
}

mir::MachineFunction MIRGenerator::generate_function(const CFG &cfg) {
  return mir::MachineFunction{generate_bb(cfg.get_entry_block()), 0};
}

mir::MachineBasicBlock *MIRGenerator::generate_bb(const BasicBlock *bb) {
  const auto new_block = arena.create<mir::MachineBasicBlock>();
  auto ir_op_to_m_op = overload{
      [this](const Var &var) -> mir::MachineOperand {
        return mir::MachineOperand{mir::VirtualRegister{
            temp_to_reg.contains(var.numeral) ? temp_to_reg[var.numeral]
                                              : var.numeral,
            32}};
      },
      [](int32_t var) -> mir::MachineOperand {
        return mir::MachineOperand{mir::Immediate{var}};
      }};
  for (const auto &ir_instruction : bb->get_instructions()) {
    switch (ir_instruction.get_opcode()) {
    case Opcode::ADD: {
      const auto target_reg = mir::MachineOperand{mir::VirtualRegister{
          ir_instruction.get_result().value().numeral, 32}};
      const auto lhs =
          std::visit(ir_op_to_m_op, ir_instruction.get_operands().at(0).value);
      const auto rhs =
          std::visit(ir_op_to_m_op, ir_instruction.get_operands().at(1).value);
      if (is_register(lhs) && is_immediate(rhs)) {
        const auto move_instr = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RR,
            std::vector<mir::MachineOperand>{lhs},
            std::vector<mir::MachineOperand>{target_reg});
        new_block->add_instruction(move_instr);

        const auto mir_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::ADD_RI,
            std::vector<mir::MachineOperand>{target_reg, rhs},
            std::vector<mir::MachineOperand>{target_reg});
        new_block->add_instruction(mir_inst);
      } else if (is_register(rhs) && is_immediate(lhs)) {
        const auto move_instr = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RR,
            std::vector<mir::MachineOperand>{rhs},
            std::vector<mir::MachineOperand>{target_reg});
        new_block->add_instruction(move_instr);

        const auto mir_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::ADD_RI,
            std::vector<mir::MachineOperand>{target_reg, lhs},
            std::vector<mir::MachineOperand>{target_reg});
        new_block->add_instruction(mir_inst);
      } else if (is_register(rhs) && is_register(lhs)) {
        const auto move_instr = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RR,
            std::vector<mir::MachineOperand>{lhs},
            std::vector<mir::MachineOperand>{target_reg});
        new_block->add_instruction(move_instr);

        const auto mir_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::ADD_RR,
            std::vector<mir::MachineOperand>{target_reg, rhs},
            std::vector<mir::MachineOperand>{target_reg});
        new_block->add_instruction(mir_inst);
      }
    } break;
    case Opcode::SUB:
      break;
    case Opcode::MUL:
      break;
    case Opcode::DIV:
      break;
    case Opcode::MOD:
      break;
    case Opcode::STORE: {
      const auto target_reg = mir::MachineOperand{mir::VirtualRegister{
          ir_instruction.get_result().value().numeral, 32}};
      const auto src =
          std::visit(ir_op_to_m_op, ir_instruction.get_operands().at(0).value);
      const auto move_instr = arena.create<mir::MachineInstruction>(
          is_register(src) ? mir::MachineInstruction::MachineOpcode::MOV_RR
                           : mir::MachineInstruction::MachineOpcode::MOV_RI,
          std::vector<mir::MachineOperand>{src},
          std::vector<mir::MachineOperand>{target_reg});
      new_block->add_instruction(move_instr);
    } break;
    case Opcode::NEG:
      break;
    case Opcode::RET: {
      const auto src =
          std::visit(ir_op_to_m_op, ir_instruction.get_operands().at(0).value);
      const auto mov_inst = arena.create<mir::MachineInstruction>(
          is_register(src) ? mir::MachineInstruction::MachineOpcode::MOV_RR
                           : mir::MachineInstruction::MachineOpcode::MOV_RI,
          std::vector<mir::MachineOperand>{src},
          std::vector<mir::MachineOperand>{
              mir::MachineOperand{mir::PhysicalRegister{"eax", 32}}});
      new_block->add_instruction(mov_inst);

      const auto ret_inst = arena.create<mir::MachineInstruction>(
          mir::MachineInstruction::MachineOpcode::RET);
      new_block->add_instruction(ret_inst);
    } break;
    default:
      break;
    }
  }
  return new_block;
}
