#ifndef CODEGEN_INSTRUCTION_SELECTION_H
#define CODEGEN_INSTRUCTION_SELECTION_H
#include "../ir/cfg.hpp"
#include "../ir/ir.hpp"
#include "../report/report_builder.hpp"
#include <cstdint>
#include <format>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <variant>
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

struct Register {
  std::string canonical_name;
  int bits;
};

struct InstructionSelector {
private:
  std::shared_ptr<DiagnosticEmitter> diagnostics;

  std::unordered_map<Var, size_t> var_to_color{};
  std::vector<Register> GPR32{Register{"eax", 32}, Register{"ecx", 32},
                              Register{"edx", 32}, Register{"edi", 32},
                              Register{"ebp", 32}, Register{"esp", 32}};
  std::vector<Register> GPR64{};

  Register get_32bit_register_for_color(size_t color) const {
    if (color >= GPR32.size()) {
      diagnostics->emit_error(SourceLocation{},
                              "Error while generating instructions, tried to "
                              "allocate a register that doesn't exist.");
      diagnostics->suggest_fix(
          "Seems like the variable didn't get spilled onto the stack.");
      return GPR32.at(0);
    }
    return GPR32.at(color);
  }

  std::string get_reg_name_for_var(Var var) const {
    return get_32bit_register_for_color(var_to_color.at(var)).canonical_name;
  }

public:
  explicit InstructionSelector(const std::unordered_map<Var, size_t> &color_map,
                               std::shared_ptr<DiagnosticEmitter> diagnostics)
      : var_to_color(color_map), diagnostics(std::move(diagnostics)) {}

  std::string
  generate_function_body(IntermediateRepresentation &representation) {
    std::ostringstream out{};
    out << ".intel_syntax noprefix" << std::endl;
    out << ".global main" << std::endl;
    out << ".global _main" << std::endl;
    out << ".text" << std::endl;
    out << "main:" << std::endl;
    out << "call _main" << std::endl;
    out << "mov\trdi, rax" << std::endl;
    out << "mov\trax, 0x3C" << std::endl;
    out << "syscall" << std::endl;
    out << "_main:" << std::endl;
    for (const auto &cfg : representation.get_cfgs()) {
      for (const auto &instruction :
           cfg.get_entry_block()->get_instructions()) {
        out << match_and_parse(instruction);
      }
    }
    return out.str();
  }

  std::string match_and_parse(const IRInstruction &instruction) {

    const auto operand_to_string = overloaded{
        [this](Var v) -> std::string {
          return get_32bit_register_for_color(this->var_to_color.at(v))
              .canonical_name;
        },
        [](uint32_t val) -> std::string { return std::format("{}", val); }};

    std::ostringstream out{};
    switch (instruction.get_opcode()) {

    case Opcode::SUB: {
      const auto target_reg =
          get_32bit_register_for_color(
              var_to_color.at(instruction.get_result().value()))
              .canonical_name;

      const auto lhs =
          std::visit(operand_to_string, instruction.get_operands().at(0).value);
      const auto rhs =
          std::visit(operand_to_string, instruction.get_operands().at(1).value);
      if (target_reg == lhs) {
        out << std::format("sub\t{}, {}\t # {}", lhs, rhs,
                           instruction.to_string())
            << std::endl;
      } else if (target_reg == rhs) {
        out << std::format("sub\t{}, {}\t # {}", rhs, lhs,
                           instruction.to_string())
            << std::endl;
      } else {
        out << std::format("sub\t{}, {}\t # Moving to target reg", target_reg,
                           lhs)
            << std::endl;
        out << std::format("sub\t{}, {}\t # {}", target_reg, rhs,
                           instruction.to_string())
            << std::endl;
      }
    } break;
    case Opcode::MOD: {
      const auto target_reg =
          get_32bit_register_for_color(
              var_to_color.at(instruction.get_result().value()))
              .canonical_name;

      const auto lhs =
          std::visit(operand_to_string, instruction.get_operands().at(0).value);
      const auto rhs =
          std::visit(operand_to_string, instruction.get_operands().at(1).value);
      if (target_reg == lhs) {
        out << std::format("idiv\t{}\t # {}", lhs, rhs, instruction.to_string())
            << std::endl;
      } else if (target_reg == rhs) {
        out << std::format("idiv\t{}\t # {}", rhs, lhs, instruction.to_string())
            << std::endl;
      } else {
        out << std::format("mov\t{}, {}\t # Moving to target reg", target_reg,
                           lhs)
            << std::endl;
        out << std::format("idiv\t{}\t # {}", target_reg, rhs,
                           instruction.to_string())
            << std::endl;
      }
    } break;
    case Opcode::DIV: {

      const auto target_reg =
          get_32bit_register_for_color(
              var_to_color.at(instruction.get_result().value()))
              .canonical_name;

      const auto lhs =
          std::visit(operand_to_string, instruction.get_operands().at(0).value);
      const auto rhs =
          std::visit(operand_to_string, instruction.get_operands().at(1).value);
      if (target_reg == lhs) {
        out << std::format("idiv\t{}\t # {}", lhs, rhs, instruction.to_string())
            << std::endl;
      } else if (target_reg == rhs) {
        out << std::format("idiv\t{}\t # {}", rhs, lhs, instruction.to_string())
            << std::endl;
      } else {
        out << std::format("mov\t{}, {}\t # Moving to target reg", target_reg,
                           lhs)
            << std::endl;
        out << std::format("idiv\t{}, {}\t # {}", target_reg, rhs,
                           instruction.to_string())
            << std::endl;
      }
    } break;
    case Opcode::MUL: {
      const auto target_reg =
          get_32bit_register_for_color(
              var_to_color.at(instruction.get_result().value()))
              .canonical_name;

      const auto lhs =
          std::visit(operand_to_string, instruction.get_operands().at(0).value);
      const auto rhs =
          std::visit(operand_to_string, instruction.get_operands().at(1).value);
      if (target_reg == lhs) {
        out << std::format("imul\t{}, {}\t # {}", lhs, rhs,
                           instruction.to_string())
            << std::endl;
      } else if (target_reg == rhs) {
        out << std::format("imul\t{}, {}\t # {}", rhs, lhs,
                           instruction.to_string())
            << std::endl;
      } else {
        out << std::format("mov\t{}, {}\t # Moving to target reg", target_reg,
                           lhs)
            << std::endl;
        out << std::format("imul\t{}, {}\t # {}", target_reg, rhs,
                           instruction.to_string())
            << std::endl;
      }
    } break;
    case Opcode::ADD: {
      const auto target_reg =
          get_32bit_register_for_color(
              var_to_color.at(instruction.get_result().value()))
              .canonical_name;

      const auto lhs =
          std::visit(operand_to_string, instruction.get_operands().at(0).value);
      const auto rhs =
          std::visit(operand_to_string, instruction.get_operands().at(1).value);
      if (target_reg == lhs) {
        out << std::format("add\t{}, {}\t # {}", lhs, rhs,
                           instruction.to_string())
            << std::endl;
      } else if (target_reg == rhs) {
        out << std::format("add\t{}, {}\t # {}", rhs, lhs,
                           instruction.to_string())
            << std::endl;
      } else {
        out << std::format("mov\t{}, {}\t # Moving to target reg", target_reg,
                           lhs)
            << std::endl;
        out << std::format("add\t{}, {}\t # {}", target_reg, rhs,
                           instruction.to_string())
            << std::endl;
      }

    } break;
    case Opcode::NEG: {
      const auto target_reg =
          get_32bit_register_for_color(
              var_to_color.at(instruction.get_result().value()))
              .canonical_name;
      const auto operand =
          std::visit(operand_to_string, instruction.get_operands().at(0).value);
      out << std::format("neg\t{}\t\t # {}", operand, instruction.to_string())
          << std::endl;
      if (operand != target_reg) {
        out << std::format("mov\t{}, {} # Move into target reg", target_reg,
                           operand)
            << std::endl;
      }
    } break;
    case Opcode::LT:
      break;
    case Opcode::LE:
      break;
    case Opcode::GT:
      break;
    case Opcode::GE:
      break;
    case Opcode::EQ:
      break;
    case Opcode::NE:
      break;
    case Opcode::JMP:
      break;
    case Opcode::STORE:
      out << std::format(
                 "mov\t{}, {}   \t # {}",
                 instruction.get_result()
                     ? get_reg_name_for_var(instruction.get_result().value())
                     : "WHAT THE HELLY?!?!?!",
                 std::visit(operand_to_string,
                            instruction.get_operands().at(0).value),
                 instruction.to_string())
          << std::endl;
      break;
    case Opcode::RET:
      out << std::format("mov\teax, {}\t # Move into (sub register of) rax",
                         std::visit(operand_to_string,
                                    instruction.get_operands().at(0).value))
          << std::endl;
      out << std::format("ret\t\t\t # {}", instruction.to_string())
          << std::endl;
      break;
    case Opcode::PHI:
      break;
    }
    return out.str();
  }
};

#endif // !CODEGEN_INSTRUCTION_SELECTION_H
