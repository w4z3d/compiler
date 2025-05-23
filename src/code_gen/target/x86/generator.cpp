#include "generator.hpp"
#include <iostream>
std::string X86Generator::add_assembly_prolouge() {
  std::ostringstream out;
  out << ".intel_syntax noprefix" << std::endl;
  out << ".global main" << std::endl;
  out << ".global _main" << std::endl;
  out << ".text" << std::endl;
  out << "main:" << std::endl;
  out << "call _main" << std::endl;
  out << "mov rdi, rax" << std::endl;
  out << "mov rax, 0x3C" << std::endl;
  out << "syscall" << std::endl;
  out << "_main:" << std::endl;
  return out.str();
}

std::string X86Generator::generate_program(mir::MIRProgram program) {
  std::ostringstream out{};
  out << add_assembly_prolouge();
  for (const auto &function : program.get_functions()) {
    out << translate_function(function.second);
  }
  return out.str();
}
std::string X86Generator::translate_function(mir::MachineFunction function) {
  std::ostringstream out{};
  auto frame_size = function.get_frame_size() * 4;
  out << "push rbp" << std::endl;
  out << "mov rbp, rsp" << std::endl;
  if (frame_size)
    out << std::format(
               "sub\trsp, {}\t # Mach halt stack größer keine Ahnung man",
               frame_size)
        << std::endl;
  for (const auto &item : function.get_instructions()) {
    out << translate_instruction(item);
  }
  return out.str();
}

std::string X86Generator::translate_add_rr_instruction(
    mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  const auto dst =
      std::get<mir::PhysicalRegister>(instruction->get_ins().at(0).get_op());
  const auto src_op = instruction->get_ins().at(1).get_op();

  std::string src_op_str{};
  if (std::holds_alternative<mir::StackSlot>(src_op)) {
    src_op_str = std::format("DWORD PTR [rbp - {}]",
                             std::get<mir::StackSlot>(src_op).offset);
  } else {
    src_op_str = std::get<mir::PhysicalRegister>(src_op).get_name();
  }

  return std::format("add\t{}, {}\t #{}", dst.get_name(), src_op_str,
                     mir::to_string(*instruction));
}

std::string
X86Generator::translate_instruction(mir::MachineInstruction *instruction) {
  std::ostringstream out{};

  switch (instruction->get_opcode()) {
  case mir::MachineInstruction::MachineOpcode::ADD_RR:
    out << translate_add_rr_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::SUB_RR:
    out << translate_sub_rr_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::MUL_RR:
    out << translate_mul_rr_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::DIV_RR:
  case mir::MachineInstruction::MachineOpcode::MOD_RR:
    out << translate_div_rr_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::NEG_R:
    out << translate_neg_r_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::LOAD_REG_MEM:
    out << translate_load_reg_mem_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::STORE_MEM_REG:
    out << translate_store_mem_reg_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::STORE_MEM_IMM:
    out << translate_store_mem_imm_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::MOV_RR:
    out << translate_mov_rr_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::MOV_RI:
    out << translate_mov_ri_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::RET:
    out << translate_ret_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::JMP:
    out << translate_jmp_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::CMP:
    out << translate_cmp_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::JL:
    out << translate_jl_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::DEF_LABEL:
    out << translate_pseudo_label_instruction(instruction) << std::endl;
    break;
  default:
    out << "# Hier könnte ihr opcode stehen" << std::endl;
    break;
  }

  return out.str();
}
std::string X86Generator::translate_load_reg_mem_instruction(
    mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  const auto dst =
      std::get<mir::PhysicalRegister>(instruction->get_outs().at(0).get_op());
  const auto src =
      std::get<mir::StackSlot>(instruction->get_ins().at(0).get_op());
  out << std::format("mov\t{}, DWORD PTR [rbp - {}]\t #{}", dst.get_name(),
                     src.offset, mir::to_string(*instruction));

  return out.str();
}
std::string X86Generator::translate_store_mem_reg_instruction(
    mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  const auto src =
      std::get<mir::PhysicalRegister>(instruction->get_ins().at(0).get_op());
  const auto dst =
      std::get<mir::StackSlot>(instruction->get_outs().at(0).get_op());
  out << std::format("mov\tDWORD PTR [rbp - {}], {}\t #{}", dst.offset,
                     src.get_name(), mir::to_string(*instruction));

  return out.str();
}
std::string X86Generator::translate_store_mem_imm_instruction(
    mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  const auto src =
      std::get<mir::Immediate>(instruction->get_ins().at(0).get_op());
  const auto dst =
      std::get<mir::StackSlot>(instruction->get_outs().at(0).get_op());
  out << std::format("mov\tDWORD PTR [rbp - {}], {}\t #{}", dst.offset,
                     src.value, mir::to_string(*instruction));

  return out.str();
}
std::string X86Generator::translate_mov_rr_instruction(
    mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  const auto dst =
      std::get<mir::PhysicalRegister>(instruction->get_outs().at(0).get_op());
  const auto src =
      std::get<mir::PhysicalRegister>(instruction->get_ins().at(0).get_op());
  out << std::format("mov\t{}, {}\t #{}", dst.get_name(), src.get_name(),
                     mir::to_string(*instruction));
  return out.str();
}
std::string
X86Generator::translate_ret_instruction(mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  out << "mov rsp, rbp" << std::endl;
  out << "pop rbp" << std::endl;
  out << "ret" << std::endl;
  return out.str();
}
std::string X86Generator::translate_mov_ri_instruction(
    mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  const auto dst =
      std::get<mir::PhysicalRegister>(instruction->get_outs().at(0).get_op());
  const auto src =
      std::get<mir::Immediate>(instruction->get_ins().at(0).get_op());
  out << std::format("mov\t{}, {}\t\t #{}", dst.get_name(), src.value,
                     mir::to_string(*instruction));
  return out.str();
}
std::string X86Generator::translate_div_rr_instruction(
    mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  const auto divisor = instruction->get_ins().at(0).get_op();

  std::string divisor_string{};
  if (std::holds_alternative<mir::StackSlot>(divisor)) {
    divisor_string = std::format("DWORD PTR [rbp - {}]",
                                 std::get<mir::StackSlot>(divisor).offset);
  } else {
    divisor_string = std::get<mir::PhysicalRegister>(divisor).get_name();
  }
  out << "cdq" << std::endl;
  out << std::format("idiv\t{}\t\t #{}", divisor_string,
                     mir::to_string(*instruction));
  return out.str();
}

std::string X86Generator::translate_sub_rr_instruction(
    mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  const auto dst =
      std::get<mir::PhysicalRegister>(instruction->get_ins().at(0).get_op());
  const auto src_op = instruction->get_ins().at(1).get_op();

  std::string src_op_str{};
  if (std::holds_alternative<mir::StackSlot>(src_op)) {
    src_op_str = std::format("DWORD PTR [rbp - {}]",
                             std::get<mir::StackSlot>(src_op).offset);
  } else {
    src_op_str = std::get<mir::PhysicalRegister>(src_op).get_name();
  }

  return std::format("sub\t{}, {}\t #{}", dst.get_name(), src_op_str,
                     mir::to_string(*instruction));
}
std::string X86Generator::translate_mul_rr_instruction(
    mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  const auto dst =
      std::get<mir::PhysicalRegister>(instruction->get_ins().at(0).get_op());
  const auto src_op = instruction->get_ins().at(1).get_op();

  std::string src_op_str{};
  if (std::holds_alternative<mir::StackSlot>(src_op)) {
    src_op_str = std::format("DWORD PTR [rbp - {}]",
                             std::get<mir::StackSlot>(src_op).offset);
  } else {
    src_op_str = std::get<mir::PhysicalRegister>(src_op).get_name();
  }

  return std::format("imul\t{}, {}\t #{}", dst.get_name(), src_op_str,
                     mir::to_string(*instruction));
}
std::string X86Generator::translate_neg_r_instruction(
    mir::MachineInstruction *instruction) {
  const auto dst =
      std::get<mir::PhysicalRegister>(instruction->get_ins().at(0).get_op());

  return std::format("neg\t{}\t\t\t #{}", dst.get_name(),
                     mir::to_string(*instruction));
}

std::string X86Generator::translate_pseudo_label_instruction(
    mir::MachineInstruction *instruction) {
  const auto label_id =
      std::get<mir::Immediate>(instruction->get_ins().at(0).get_op());
  return std::format("l{}:", label_id.value);
}
std::string
X86Generator::translate_jmp_instruction(mir::MachineInstruction *instruction) {
  const auto label_id =
      std::get<mir::Immediate>(instruction->get_ins().at(0).get_op());
  return std::format("jmp\tl{}\t\t\t #{}", label_id.value,
                     mir::to_string(*instruction));
}

// TODO: Change to cmp_rr and cmp_ri because we can also compare immediates.
// The code below could just be replaced with test reg, reg which is faster
// because it just ANDs the 2 regs together. fix this in the future pls @me
std::string
X86Generator::translate_cmp_instruction(mir::MachineInstruction *instruction) {
  const auto lhs =
      std::get<mir::PhysicalRegister>(instruction->get_ins().at(0).get_op());
  const auto rhs =
      std::get<mir::PhysicalRegister>(instruction->get_ins().at(1).get_op());
  return std::format("cmp\t{}, {}\t\t #{}", lhs.get_name(), rhs.get_name(),
                     mir::to_string(*instruction));
}

std::string
X86Generator::translate_jl_instruction(mir::MachineInstruction *instruction) {
  const auto label_id =
      std::get<mir::Immediate>(instruction->get_ins().at(0).get_op());
  return std::format("jl\tl{}\t\t\t #{}", label_id.value,
                     mir::to_string(*instruction));
}
