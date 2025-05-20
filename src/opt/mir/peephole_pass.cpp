#include "peephole_pass.hpp"

void MIRPeepholePass::transform_block(mir::MachineBasicBlock *block) {
  auto inst_iter = block->instructions.begin();
  while (inst_iter != block->instructions.end()) {
    mir::MachineInstruction *current_instruction =
        *inst_iter; // Get the instruction pointer

    bool removed_current = false;
    switch (current_instruction->get_opcode()) {
    case mir::MachineInstruction::MachineOpcode::MOV_RR: {
      // Ensure there are enough operands before accessing .at(0)
      if (current_instruction->get_ins().empty() ||
          current_instruction->get_outs().empty()) {
        // Handle error or malformed instruction
        ++inst_iter; // Skip this instruction
        continue;    // Continue to next iteration of the while loop
      }

      const auto from_op = current_instruction->get_ins().at(0).get_op();
      const auto to_op = current_instruction->get_outs().at(0).get_op();

      // Check if the variant holds PhysicalRegister
      if (std::holds_alternative<mir::PhysicalRegister>(from_op) &&
          std::holds_alternative<mir::PhysicalRegister>(to_op)) {
        const auto &from = std::get<mir::PhysicalRegister>(from_op);
        const auto &to = std::get<mir::PhysicalRegister>(to_op);

        if (from.get_name() == to.get_name()) {
          std::cout << "Removing MOV_RR instruction where from ("
                    << from.get_name() << ") == to (" << to.get_name() << ")"
                    << std::endl;

          // Erase the instruction from the list.
          // `erase` returns an iterator to the element that follows the last
          // element removed.
          inst_iter = block->instructions.erase(inst_iter);
          removed_current = true;
        }
      } else {
        // Operands are not PhysicalRegisters, handle as needed or skip.
        std::cerr << "Warning: MOV_RR instruction with non-PhysicalRegister "
                     "operands found."
                  << std::endl;
      }
      break;
    }
    default:
      // No action for other opcodes in this specific transformation
      break;
    }

    if (!removed_current) {
      // Only increment the iterator if no element was removed at the current
      // position. If an element was removed, inst_iter already points to the
      // next element.
      ++inst_iter;
    }
  }
}
