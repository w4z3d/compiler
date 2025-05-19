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
  out << "movq rdi, rax" << std::endl;
  out << "movq rax, 0x3C" << std::endl;
  out << "syscall" << std::endl;
  out << "_main:" << std::endl;
  return out.str();
}

std::string X86Generator::generate_program(mir::MIRProgram program) {
  std::ostringstream out{};
  out << add_assembly_prolouge();
  for(const auto function : program.get_functions()) {
    out << translate_function(function.second);
  }
  return out.str();
}
std::string X86Generator::translate_basic_block(mir::MachineBasicBlock *block) {
  std::ostringstream out{};
  for (const auto &item : block->get_instructions()) {
    out << translate_instruction(item);
  }
  return out.str();
}
std::string X86Generator::translate_function(mir::MachineFunction function) {
  std::ostringstream out{};
  out << "push rbp" << std::endl;
  out << "mov rbp, rsp" << std::endl;
  out << std::format("sub\trsp, {}\t # Mach halt stack größer keine Ahnung man", function.get_frame_size() + 4) << std::endl;
  out << translate_basic_block(function.get_entry_block());
  return out.str();
}

std::string
X86Generator::translate_add_rr_instruction(mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  const auto dst =
      std::get<mir::PhysicalRegister>(instruction->get_ins().at(0).get_op());
  const auto src_op = instruction->get_ins().at(1).get_op();

  std::string src_op_str{};
  if(std::holds_alternative<mir::StackSlot>(src_op)) {
    src_op_str = std::format("DWORD PTR [rbp - {}]", std::get<mir::StackSlot>(src_op).offset);
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
    out << translate_div_rr_instruction(instruction) << std::endl;
    break;
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
  case mir::MachineInstruction::MachineOpcode::MOV_RR:
     out << translate_mov_rr_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::MOV_RI:
    out << translate_mov_ri_instruction(instruction) << std::endl;
    break;
  case mir::MachineInstruction::MachineOpcode::RET:
    out << translate_ret_instruction(instruction) << std::endl;
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
  out << std::format("mov\t{}, DWORD PTR [rbp - {}]\t #{}", dst.get_name(), src.offset, mir::to_string(*instruction));

  return out.str();
}
std::string X86Generator::translate_store_mem_reg_instruction(
    mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  const auto src =
      std::get<mir::PhysicalRegister>(instruction->get_ins().at(0).get_op());
  const auto dst =
      std::get<mir::StackSlot>(instruction->get_outs().at(0).get_op());
  out << std::format("mov\tDWORD PTR [rbp - {}], {}\t #{}", dst.offset, src.get_name(), mir::to_string(*instruction));

  return out.str();
}
std::string X86Generator::translate_mov_rr_instruction(
    mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  const auto dst =
      std::get<mir::PhysicalRegister>(instruction->get_outs().at(0).get_op());
  const auto src =
      std::get<mir::PhysicalRegister>(instruction->get_ins().at(0).get_op());
  out << std::format("mov\t{}, {}\t #{}", dst.get_name(), src.get_name(), mir::to_string(*instruction));
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
  out << std::format("mov\t{}, {}\t\t #{}", dst.get_name(), src.value, mir::to_string(*instruction));
  return out.str();
}
std::string X86Generator::translate_div_rr_instruction(
    mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  const auto divisor =instruction->get_ins().at(0).get_op();

  std::string divisor_string{};
  if(std::holds_alternative<mir::StackSlot>(divisor)) {
    divisor_string = std::format("DWORD PTR [rbp - {}]", std::get<mir::StackSlot>(divisor).offset);
  } else {
    divisor_string = std::get<mir::PhysicalRegister>(divisor).get_name();
  }
  out << "cdq" << std::endl;
  out << std::format("idiv\t{}\t\t #{}", divisor_string, mir::to_string(*instruction));
  return out.str();
}
std::string X86Generator::translate_sub_rr_instruction(
    mir::MachineInstruction *instruction) {
  std::ostringstream out{};
  const auto dst =
      std::get<mir::PhysicalRegister>(instruction->get_ins().at(0).get_op());
  const auto src_op = instruction->get_ins().at(1).get_op();

  std::string src_op_str{};
  if(std::holds_alternative<mir::StackSlot>(src_op)) {
    src_op_str = std::format("DWORD PTR [rbp - {}]", std::get<mir::StackSlot>(src_op).offset);
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
  if(std::holds_alternative<mir::StackSlot>(src_op)) {
    src_op_str = std::format("DWORD PTR [rbp - {}]", std::get<mir::StackSlot>(src_op).offset);
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

  return std::format("neg\t{}\t\t\t #{}", dst.get_name(), mir::to_string(*instruction));
}
std::string X86Generator::translate_mod_rr_instruction(
    mir::MachineInstruction *instruction) {
  return std::string();
}
