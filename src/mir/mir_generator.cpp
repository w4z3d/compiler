#include "mir_generator.hpp"
#include "mir.hpp"
#include <vector>

void MIRGenerator::generate() {
  for (const auto &cfg : representation.get_cfgs()) {
    mir_program.add_function(generate_function(cfg));
  }
}

void MIRGenerator::perform_dfs_basic_block(
    BasicBlock *block, std::set<BasicBlock *> &visited,
    std::list<BasicBlock *> &linearized_order) {
  if (!block) {
    return;
  }

  if (visited.count(block)) {
    return;
  }

  visited.insert(block);
  linearized_order.push_back(block);

  if (block->get_successor_false()) {
    perform_dfs_basic_block(block->get_successor_false().value(), visited,
                            linearized_order);
  }
  if (block->get_successor_true()) {
    std::cout << "Visiting block " << block->get_id() << std::endl;
    perform_dfs_basic_block(block->get_successor_true().value(), visited,
                            linearized_order);
  }
}

mir::MachineFunction MIRGenerator::generate_function(const CFG &cfg) {
  mir::MachineFunction function{};

  std::list<BasicBlock *> linearized_blocks{};
  std::set<BasicBlock *> visited{};
  perform_dfs_basic_block(cfg.entry_block, visited, linearized_blocks);

  // TODO: Dfs over basic blocks (choosing false branch first).
  for (const auto *block : linearized_blocks) {
    generate_bb(function, block);
  }
  return function;
}

mir::MachineInstruction *
MIRGenerator::create_mov_rr(const mir::MachineOperand &from,
                            const mir::MachineOperand &to) {
  return arena.create<mir::MachineInstruction>(
      mir::MachineInstruction::MachineOpcode::MOV_RR,
      std::vector<mir::MachineOperand>{from},
      std::vector<mir::MachineOperand>{to});
}

mir::MachineInstruction *
MIRGenerator::create_add_rr(const mir::MachineOperand &rhs,
                            const mir::MachineOperand &target_reg) {
  return arena.create<mir::MachineInstruction>(
      mir::MachineInstruction::MachineOpcode::ADD_RR,
      std::vector<mir::MachineOperand>{target_reg, rhs},
      std::vector<mir::MachineOperand>{target_reg});
}
mir::MachineInstruction *MIRGenerator::create_label(std::size_t id) {
  return arena.create<mir::MachineInstruction>(
      mir::MachineInstruction::MachineOpcode::DEF_LABEL,
      std::vector<mir::MachineOperand>{
          mir::MachineOperand{mir::Immediate{static_cast<int32_t>(id)}}});
}

void MIRGenerator::generate_bb(mir::MachineFunction &function,
                               const BasicBlock *bb) {
  function.add_instruction(create_label(bb->get_id()));
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

        function.add_instruction(create_mov_rr(lhs, target_reg));
        const auto mir_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::ADD_RI,
            std::vector<mir::MachineOperand>{target_reg, rhs},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(mir_inst);
      } else if (is_register(rhs) && is_immediate(lhs)) {
        function.add_instruction(create_mov_rr(rhs, target_reg));

        const auto mir_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::ADD_RI,
            std::vector<mir::MachineOperand>{target_reg, lhs},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(mir_inst);
      } else if (is_register(rhs) && is_register(lhs)) {
        function.add_instruction(create_mov_rr(lhs, target_reg));
      }
      function.add_instruction(create_add_rr(rhs, target_reg));
    } break;
    case Opcode::SUB: {
      const auto target_reg = mir::MachineOperand{mir::VirtualRegister{
          ir_instruction.get_result().value().numeral, 32}};
      const auto lhs =
          std::visit(ir_op_to_m_op, ir_instruction.get_operands().at(0).value);
      const auto rhs =
          std::visit(ir_op_to_m_op, ir_instruction.get_operands().at(1).value);
      if (is_register(lhs) && is_immediate(rhs)) {
        if (std::get<mir::VirtualRegister>(lhs.get_op()).get_name() !=
            std::get<mir::VirtualRegister>(rhs.get_op()).get_name()) {
          const auto move_instr = arena.create<mir::MachineInstruction>(
              mir::MachineInstruction::MachineOpcode::MOV_RR,
              std::vector<mir::MachineOperand>{lhs},
              std::vector<mir::MachineOperand>{target_reg});
          function.add_instruction(move_instr);
        }
        const auto mir_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::SUB_RI,
            std::vector<mir::MachineOperand>{target_reg, rhs},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(mir_inst);
      } else if (is_register(rhs) && is_immediate(lhs)) {
        const auto move_instr = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RR,
            std::vector<mir::MachineOperand>{rhs},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(move_instr);
        const auto mir_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::SUB_RI,
            std::vector<mir::MachineOperand>{target_reg, lhs},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(mir_inst);
      } else if (is_register(rhs) && is_register(lhs)) {
        const auto move_instr = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RR,
            std::vector<mir::MachineOperand>{lhs},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(move_instr);

        const auto mir_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::SUB_RR,
            std::vector<mir::MachineOperand>{target_reg, rhs},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(mir_inst);
      }
    }

    break;
    case Opcode::MUL: {
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
        function.add_instruction(move_instr);

        const auto mir_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MUL_RI,
            std::vector<mir::MachineOperand>{target_reg, rhs},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(mir_inst);
      } else if (is_register(rhs) && is_immediate(lhs)) {
        const auto move_instr = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RR,
            std::vector<mir::MachineOperand>{rhs},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(move_instr);

        const auto mir_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MUL_RI,
            std::vector<mir::MachineOperand>{target_reg, lhs},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(mir_inst);
      } else if (is_register(rhs) && is_register(lhs)) {
        const auto move_instr = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RR,
            std::vector<mir::MachineOperand>{lhs},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(move_instr);

        const auto mir_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MUL_RR,
            std::vector<mir::MachineOperand>{target_reg, rhs},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(mir_inst);
      }
    } break;
    case Opcode::DIV: {
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
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"eax", 32}}});
        function.add_instruction(move_instr);
        const auto mov_target_rhs = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RI,
            std::vector<mir::MachineOperand>{rhs},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(mov_target_rhs);
        const auto div_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::DIV_RR,
            std::vector<mir::MachineOperand>{target_reg},
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"eax", 32}}},
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"edx", 32}}});
        function.add_instruction(div_inst);
        const auto mov_into_target = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RR,
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"eax", 32}}},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(mov_into_target);
      } else if (is_register(lhs) && is_register(rhs)) {
        const auto move_instr = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RR,
            std::vector<mir::MachineOperand>{lhs},
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"eax", 32}}});
        function.add_instruction(move_instr);

        const auto div_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::DIV_RR,
            std::vector<mir::MachineOperand>{rhs},
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"eax", 32}}},
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"edx", 32}}});
        function.add_instruction(div_inst);
        const auto mov_into_target = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RR,
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"eax", 32}}},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(mov_into_target);
      }
    } break;
    case Opcode::MOD: {
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
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"eax", 32}}});
        function.add_instruction(move_instr);
        const auto mov_target_rhs = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RI,
            std::vector<mir::MachineOperand>{rhs},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(mov_target_rhs);
        const auto div_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOD_RR,
            std::vector<mir::MachineOperand>{target_reg},
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"edx", 32}}},
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"eax", 32}},
                mir::MachineOperand{mir::PhysicalRegister{"edx", 32}}});
        function.add_instruction(div_inst);
        const auto mov_into_target = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RR,
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"edx", 32}}},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(mov_into_target);
      } else if (is_register(lhs) && is_register(rhs)) {
        const auto move_instr = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RR,
            std::vector<mir::MachineOperand>{lhs},
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"eax", 32}}});
        function.add_instruction(move_instr);

        const auto div_inst = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOD_RR,
            std::vector<mir::MachineOperand>{rhs},
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"edx", 32}}},
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"eax", 32}},
                mir::MachineOperand{mir::PhysicalRegister{"edx", 32}}});
        function.add_instruction(div_inst);
        const auto mov_into_target = arena.create<mir::MachineInstruction>(
            mir::MachineInstruction::MachineOpcode::MOV_RR,
            std::vector<mir::MachineOperand>{
                mir::MachineOperand{mir::PhysicalRegister{"edx", 32}}},
            std::vector<mir::MachineOperand>{target_reg});
        function.add_instruction(mov_into_target);
      }
    } break;
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
      function.add_instruction(move_instr);
    } break;
    case Opcode::NEG: {
      const auto target_reg = mir::MachineOperand{mir::VirtualRegister{
          ir_instruction.get_result().value().numeral, 32}};
      const auto src =
          std::visit(ir_op_to_m_op, ir_instruction.get_operands().at(0).value);
      const auto move_instr = arena.create<mir::MachineInstruction>(
          mir::MachineInstruction::MachineOpcode::MOV_RR,
          std::vector<mir::MachineOperand>{src},
          std::vector<mir::MachineOperand>{target_reg});
      function.add_instruction(move_instr);

      const auto neg_r_instruction = arena.create<mir::MachineInstruction>(
          mir::MachineInstruction::MachineOpcode::NEG_R,
          std::vector<mir::MachineOperand>{target_reg});
      function.add_instruction(neg_r_instruction);

    } break;
    case Opcode::RET: {
      const auto src =
          std::visit(ir_op_to_m_op, ir_instruction.get_operands().at(0).value);
      const auto mov_inst = arena.create<mir::MachineInstruction>(
          is_register(src) ? mir::MachineInstruction::MachineOpcode::MOV_RR
                           : mir::MachineInstruction::MachineOpcode::MOV_RI,
          std::vector<mir::MachineOperand>{src},
          std::vector<mir::MachineOperand>{
              mir::MachineOperand{mir::PhysicalRegister{"eax", 32}}});
      function.add_instruction(mov_inst);

      const auto ret_inst = arena.create<mir::MachineInstruction>(
          mir::MachineInstruction::MachineOpcode::RET);
      function.add_instruction(ret_inst);
    } break;
    case Opcode::LT: {
      const auto src =
          std::visit(ir_op_to_m_op, ir_instruction.get_operands().at(1).value);
      const auto dst =
          std::visit(ir_op_to_m_op, ir_instruction.get_operands().at(0).value);
      const auto cmp = arena.create<mir::MachineInstruction>(
          mir::MachineInstruction::MachineOpcode::CMP,
          std::vector<mir::MachineOperand>{dst, src});
      const auto jl = arena.create<mir::MachineInstruction>(
          mir::MachineInstruction::MachineOpcode::JL,
          std::vector<mir::MachineOperand>{
              mir::MachineOperand{mir::Immediate{static_cast<int32_t>(
                  bb->get_successor_true().value()->get_id())}}});
      // False branch is fallthrough
      function.add_instruction(cmp);
      function.add_instruction(jl);
    } break;
    case Opcode::JMP: {
      const auto src =
          std::visit(ir_op_to_m_op, ir_instruction.get_operands().at(0).value);
      const auto jmp = arena.create<mir::MachineInstruction>(
          mir::MachineInstruction::MachineOpcode::JMP,
          std::vector<mir::MachineOperand>{src});
      function.add_instruction(jmp);
    } break;
    default:
      break;
    }
  }
}
