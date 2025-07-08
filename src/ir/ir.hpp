#ifndef COMPILER_IR_H
#define COMPILER_IR_H

#include "../defs/ast.hpp"
#include <format>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

// TODO: types
struct Var {
  std::size_t numeral;

public:
  [[nodiscard]] std::string to_string() const {
    return std::format("t{}", numeral);
  }
  bool operator==(const Var &other) const { return numeral == other.numeral; }
};

namespace std {
template <> struct hash<Var> {
  std::size_t operator()(const Var &s) const noexcept {
    return std::hash<size_t>{}(s.numeral);
  }
};
} // namespace std

struct grr {
  std::string operator()(Var v) const { return v.to_string(); }
  std::string operator()(int32_t v) const { return std::format("{}", v); }
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
  NEG,
  SHR,
  SHL,
  XOR,
  B_OR,
  B_AND,
  L_OR,
  L_AND,
  // Non bin_op
  INC,
  DEC,

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
  CALL,

  // E
  STORE,
  PHI
};

static Opcode from_unary_op(UnaryOperator un_op) {
  switch (un_op) {
  case UnaryOperator::Neg:
    return Opcode::NEG;
  case UnaryOperator::LogicalNot:
  case UnaryOperator::BitwiseNot:
  case UnaryOperator::Deref:
  case UnaryOperator::Unknown:
    throw std::runtime_error("not implemented yet");
  }
}

static Opcode from_unary_mut_op(UnaryMutationStatement::Op op) {
  switch (op) {
  case UnaryMutationStatement::Op::PostDecrement:
    return Opcode::DEC;
  case UnaryMutationStatement::Op::PostIncrement:
    return Opcode::INC;
  }
}
static Opcode from_assmt_op(AssignmentOperator a_op) {
  switch (a_op) {
  case AssignmentOperator::Plus:
    return Opcode::ADD;
  case AssignmentOperator::Minus:
    return Opcode::SUB;
  case AssignmentOperator::Mult:
    return Opcode::MUL;
  case AssignmentOperator::Div:
    return Opcode::DIV;
  case AssignmentOperator::Modulo:
    return Opcode::MOD;
  case AssignmentOperator::Equals:
    throw std::runtime_error("hmmm, ist halt jetzt konvention ?");
  case AssignmentOperator::LShift:
    return Opcode::SHL;
  case AssignmentOperator::RShift:
    return Opcode::SHR;
  case AssignmentOperator::BitwiseAnd:
    return Opcode::B_AND;
  case AssignmentOperator::BitwiseXor:
    return Opcode::XOR;
  case AssignmentOperator::BitwiseOr:
    return Opcode::B_OR;
  case AssignmentOperator::Unknown:
    throw std::runtime_error("not implemented yet");
  }
}

static Opcode from_binary_op(BinaryOperator bin_op) {
  switch (bin_op) {
  case BinaryOperator::Add:
    return Opcode::ADD;
  case BinaryOperator::Sub:
    return Opcode::SUB;
  case BinaryOperator::Div:
    return Opcode::DIV;
  case BinaryOperator::Mult:
    return Opcode::MUL;
  case BinaryOperator::Modulo:
    return Opcode::MOD;
  case BinaryOperator::Equal:
    return Opcode::EQ;
  case BinaryOperator::NotEqual:
    return Opcode::NE;
  case BinaryOperator::LessThan:
    return Opcode::LT;
  case BinaryOperator::LessThanOrEqual:
    return Opcode::LE;
  case BinaryOperator::GreaterThan:
    return Opcode::GT;
  case BinaryOperator::GreaterThanOrEqual:
    return Opcode::GE;
  case BinaryOperator::LogicalAnd:
    return Opcode::L_AND;
  case BinaryOperator::LogicalOr:
    return Opcode::L_OR;
  case BinaryOperator::BitwiseAnd:
    return Opcode::B_AND;
  case BinaryOperator::BitwiseOr:
    return Opcode::B_OR;
  case BinaryOperator::BitwiseXor:
    return Opcode::XOR;
  case BinaryOperator::ShiftLeft:
    return Opcode::SHL;
  case BinaryOperator::ShiftRight:
    return Opcode::SHR;
  case BinaryOperator::FieldAccess:
  case BinaryOperator::PointerAccess: // Muss das überhaupt noch binop sein???
  case BinaryOperator::Unknown:
    throw std::runtime_error("not implemented yet");
  }
}

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
  case Opcode::NEG:
    return "NEG";
  case Opcode::INC:
    return "INC";
  case Opcode::DEC:
    return "DEC";
  case Opcode::B_AND:
    return "B_AND";
  case Opcode::B_OR:
    return "B_OR";
  case Opcode::L_AND:
    return "L_AND";
  case Opcode::L_OR:
    return "L_OR";
  case Opcode::XOR:
    return "XOR";
  case Opcode::SHL:
    return "SHL";
  case Opcode::SHR:
    return "SHR";
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
  case Opcode::CALL:
    return "CALL";
  }
}

class IRInstruction {
private:
  Opcode opcode;
  std::vector<Operand> operands;
  std::optional<Var> result;     // aka defined var
  std::unordered_set<Var> use{}; // used vars

public:
  IRInstruction(const Opcode op, const std::vector<Operand> &ops,
                std::optional<Var> result = std::nullopt)
      : opcode(op), operands(ops), result(result) {
    // set used vars
    for (const auto &v : ops) {
      if (const Var *s = std::get_if<Var>(&v.value)) {
        use.insert(*s);
      }
    }
  }

  [[nodiscard]] Opcode get_opcode() const { return opcode; }
  [[nodiscard]] const std::vector<Operand> &get_operands() const {
    return operands;
  }
  [[nodiscard]] const std::optional<Var> &get_result() const { return result; }
  [[nodiscard]] const std::unordered_set<Var> &get_used() const { return use; }
  [[nodiscard]] std::string to_string() const {
    std::string x = std::format(
        "{} <- {}", (result.has_value() ? result.value().to_string() : "/"),
        opcode_to_string(opcode));
    for (const auto &item : operands) {
      x += std::format(" {}", std::visit(grr(), item.value));
    }
    return x;
  }
};

#endif // COMPILER_IR_H
