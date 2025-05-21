#include "peephole_pass.hpp"
#include <iostream>
#include <iterator>

bool MIRPeepholePass::optimize_redundant_mov_rr(
    mir::MachineBasicBlock *block,
    std::list<mir::MachineInstruction *>::iterator &inst_iter) {

  if (inst_iter == block->get_instructions().end()) {
    return false;
  }
  mir::MachineInstruction *current_instruction = *inst_iter;

  bool removed_current = false;
  switch (current_instruction->get_opcode()) {
  case mir::MachineInstruction::MachineOpcode::MOV_RR: {

    const auto from_op = current_instruction->get_ins().at(0).get_op();
    const auto to_op = current_instruction->get_outs().at(0).get_op();

    if (std::holds_alternative<mir::PhysicalRegister>(from_op) &&
        std::holds_alternative<mir::PhysicalRegister>(to_op)) {
      const auto &from = std::get<mir::PhysicalRegister>(from_op);
      const auto &to = std::get<mir::PhysicalRegister>(to_op);

      if (from.get_name() == to.get_name()) {

        inst_iter = block->instructions.erase(inst_iter);
        return true;
      }
    } else {
      std::cerr << "Warning: MOV_RR instruction with non-PhysicalRegister "
                   "operands found. Register allocation probably failed"
                << std::endl;
    }
    break;
  }
  default:
    break;
  }

  return false;
}

bool optimize_stack_operations(
    mir::MachineBasicBlock *block,
    std::list<mir::MachineInstruction *>::iterator &inst_iter) {
  if (inst_iter == block->get_instructions().end()) {
    return false;
  }

  auto *current_inst = *inst_iter;

  auto next_iter = std::next(inst_iter, 1);
  if (next_iter == block->get_instructions().end()) {
    return false;
  }
  auto *next_inst = *next_iter;

  bool changed = false;
  if (current_inst->get_opcode() ==
      mir::MachineInstruction::MachineOpcode::MOV_RI) {
    if (current_inst->get_ins().empty()) {
      return false;
    }

    if (next_inst->get_opcode() ==
        mir::MachineInstruction::MachineOpcode::STORE_MEM_REG) {

      const auto &mov_ri_out_reg = current_inst->get_outs().front().get_op();
      const auto &store_mem_reg_in_reg = next_inst->get_ins().front().get_op();

      if (std::holds_alternative<mir::PhysicalRegister>(mov_ri_out_reg) &&
          std::holds_alternative<mir::PhysicalRegister>(store_mem_reg_in_reg)) {
        if (std::get<mir::PhysicalRegister>(mov_ri_out_reg).get_name() ==
            std::get<mir::PhysicalRegister>(store_mem_reg_in_reg).get_name()) {

          next_inst->set_opcode(
              mir::MachineInstruction::MachineOpcode::STORE_MEM_IMM);
          next_inst->get_ins_mut().clear();
          next_inst->get_ins_mut().push_back(current_inst->get_ins().front());

          inst_iter = block->get_instructions().erase(inst_iter);
          return true;
        } else {
          // Registers don't match
        }
      }
    }
  } else if (current_inst->get_opcode() ==
             mir::MachineInstruction::MachineOpcode::STORE_MEM_REG) {
    if (next_inst->get_opcode() ==
        mir::MachineInstruction::MachineOpcode::LOAD_REG_MEM) {
      const auto &store_target_slot = current_inst->get_outs().front().get_op();
      const auto &load_source_slot = next_inst->get_ins().front().get_op();
      if (std::holds_alternative<mir::StackSlot>(store_target_slot) &&
          std::holds_alternative<mir::StackSlot>(load_source_slot)) {
        if (std::get<mir::StackSlot>(store_target_slot).offset ==
            std::get<mir::StackSlot>(load_source_slot).offset) {
          return false;
        }
      }
    }
  }

  return false;
}
void MIRPeepholePass::transform_block(mir::MachineBasicBlock *block) {
  bool made_change_this_pass;
  int pass_count = 0;
  do {
    pass_count++;
    made_change_this_pass = false;
    std::cout << "Peephole: Starting/Re-starting pass over block "
              << block->get_id() << " this is the " << pass_count << ". pass"
              << std::endl;
    auto inst_iter = block->instructions.begin();
    while (inst_iter != block->instructions.end()) {
      if (optimize_redundant_mov_rr(block, inst_iter)) {
        made_change_this_pass = true;
        goto restart_block_scan;
      }
      if (optimize_stack_operations(block, inst_iter)) {
        made_change_this_pass = true;
        goto restart_block_scan;
      }

      if (inst_iter != block->instructions.end()) {
        ++inst_iter;
      }
    }
  restart_block_scan:;
  } while (made_change_this_pass);
}
