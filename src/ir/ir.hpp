#ifndef COMPILER_IR_H
#define COMPILER_IR_H

#include <format>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

struct BasicBlock;

// TODO: types
struct Var {
  std::size_t numeral;

public:
  [[nodiscard]] std::string to_string() const {
    return std::format("var_{}", numeral);
  }
};

struct grr {
  std::string operator()(Var v) const { return v.to_string(); }
  std::string operator()(uint32_t v) const { return std::format("i_{}", v); }
};

struct Operand {
  std::variant<Var, std::uint32_t> value;
};

enum class Opcode {
  // Arithmetic
  ADD,
  SUB,
  MUL,
  DIV,
  MOD,
  // Conditions
  LT,
  LE,
  GT,
  GE,
  EQ,
  NE,
  // Control Flow
  RET,
  JMP,
  // E
  STORE,
  PHI
};

static std::string opcode_to_string(Opcode opcode) {
  switch (opcode) {

  case Opcode::ADD:
    return "ADD";
  case Opcode::SUB:
    return "SUB";
  case Opcode::MUL:
    return "MUL";
  case Opcode::DIV:
    return "DIV";
  case Opcode::MOD:
    return "MOD";
  case Opcode::LT:
    return "LT";
  case Opcode::LE:
    return "LE";
  case Opcode::GT:
    return "GT";
  case Opcode::GE:
    return "GE";
  case Opcode::EQ:
    return "EQ";
  case Opcode::NE:
    return "NE";
  case Opcode::RET:
    return "RET";
  case Opcode::JMP:
    return "JMP";
  case Opcode::STORE:
    return "STORE";
  case Opcode::PHI:
    return "PHI";
  }
}

class IRInstruction {
private:
  const Opcode opcode;
  const std::vector<Operand> operands;
  const std::optional<Var> result;

public:
  IRInstruction(const Opcode op, const std::vector<Operand> &ops,
                std::optional<Var> result = std::nullopt)
      : opcode(op), operands(ops), result(result) {}

  [[nodiscard]] Opcode get_opcode() const { return opcode; }
  [[nodiscard]] const std::vector<Operand> &get_operands() const {
    return operands;
  }
  [[nodiscard]] std::string to_string() const {
    std::string x = std::format("{} <- {}", result.value().to_string(),
                                opcode_to_string(opcode));
    for (const auto &item : operands) {
      x += std::format(" {}", std::visit(grr(), item.value));
    }
    return x;
  }
};

#endif // COMPILER_IR_H