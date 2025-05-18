#include "generator.hpp"
#include <sstream>

std::string X86Generator::add_assembly_prolouge() {
  std::ostringstream out;
  out << ".intel_syntax noprefix" << std::endl;
  out << ".global main" << std::endl;
  out << ".global _main" << std::endl;
  out << ".text" << std::endl;
  out << "main:" << std::endl;
  out << "call _main:" << std::endl;
  out << "mov rdi, rax" << std::endl;
  out << "mov rax, 0x3C" << std::endl;
  out << "syscall" << std::endl;
  out << "_main:" << std::endl;
  return out.str();
}

std::string X86Generator::generate_program(mir::MIRProgram program) {
  return "";
}
std::string X86Generator::translate_basic_block(mir::MachineBasicBlock block) {
  return "";
}
std::string X86Generator::translate_function(mir::MachineFunction function) {
  return "";
}

std::string
translate_add_rr_instruction(const mir::MachineInstruction &instruction) {
  std::ostringstream out{};
  const auto dst =
      std::get<mir::PhysicalRegister>(instruction.get_ins().at(0).get_op());
  const auto src =
      std::get<mir::PhysicalRegister>(instruction.get_ins().at(1).get_op());

  return std::format("add\t{}, {}\t#{}", dst.get_name(), src.get_name(),
                     mir::to_string(instruction));
}

std::string
X86Generator::translate_instruction(mir::MachineInstruction instruction) {
  std::ostringstream out;

  switch (instruction.get_opcode()) {
  case mir::MachineInstruction::MachineOpcode::ADD_RR:
    out << translate_add_rr_instruction(instruction) << std::endl;
    break;
  default:
    out << "# Hier könnte ihr opcode stehen" << std::endl;
    break;
  }

  return out.str();
}
