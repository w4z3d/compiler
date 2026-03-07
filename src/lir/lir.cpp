#include "lir.hpp"
#include <format>

namespace lir {

std::string Register::to_string() const {
  if (kind == Virtual)
    return std::format("%v{}", id);
  return std::format("%p{}", id);
}

std::string Operand::to_string() const {
  switch (kind) {
  case Reg:
    return reg.to_string();
  case Imm:
    return std::format("${}", imm);
  case StackSlot:
    return std::format("[stack:{}]", slot);
  case Block:
    return block ? block->label : "<null>";
  }
  return "<?>";
}

std::string_view opcode_name(Opcode op) {
  switch (op) {
  case Opcode::Mov:
    return "mov";
  case Opcode::Copy:
    return "copy";
  case Opcode::Add:
    return "add";
  case Opcode::Sub:
    return "sub";
  case Opcode::Mul:
    return "mul";
  case Opcode::SDiv:
    return "sdiv";
  case Opcode::Neg:
    return "neg";
  case Opcode::And:
    return "and";
  case Opcode::Or:
    return "or";
  case Opcode::Xor:
    return "xor";
  case Opcode::Shl:
    return "shl";
  case Opcode::LShr:
    return "lshr";
  case Opcode::AShr:
    return "ashr";
  case Opcode::Cmp:
    return "cmp";
  case Opcode::CSet:
    return "cset";
  case Opcode::Load:
    return "load";
  case Opcode::Store:
    return "store";
  case Opcode::Jump:
    return "jmp";
  case Opcode::CondJump:
    return "cjmp";
  case Opcode::Call:
    return "call";
  case Opcode::Ret:
    return "ret";
  }
  return "???";
}

std::string_view predicate_name(CmpPredicate pred) {
  switch (pred) {
  case CmpPredicate::EQ:
    return "eq";
  case CmpPredicate::NE:
    return "ne";
  case CmpPredicate::SLT:
    return "slt";
  case CmpPredicate::SLE:
    return "sle";
  case CmpPredicate::SGT:
    return "sgt";
  case CmpPredicate::SGE:
    return "sge";
  case CmpPredicate::ULT:
    return "ult";
  case CmpPredicate::ULE:
    return "ule";
  case CmpPredicate::UGT:
    return "ugt";
  case CmpPredicate::UGE:
    return "uge";
  }
  return "???";
}

std::string Instruction::to_string() const {
  std::string result = "  ";

  if (opcode == Opcode::Jump) {
    assert(!operands.empty() && operands[0].is_block());
    return result + std::format("jmp {}", operands[0].to_string());
  }

  if (opcode == Opcode::CondJump) {
    assert(operands.size() >= 2 && predicate);
    return result + std::format("cjmp.{} {}, {}", predicate_name(*predicate),
                                operands[0].to_string(),
                                operands[1].to_string());
  }

  if (opcode == Opcode::Ret) {
    if (operands.empty())
      return result + "ret";
    return result + std::format("ret {}", operands[0].to_string());
  }

  if (opcode == Opcode::Store) {
    assert(operands.size() >= 2);
    return result + std::format("store {}, {}", operands[0].to_string(),
                                operands[1].to_string());
  }

  if (opcode == Opcode::Call) {
    std::string args;
    for (unsigned i = num_defs; i < operands.size(); ++i) {
      if (i > num_defs)
        args += ", ";
      args += operands[i].to_string();
    }
    if (num_defs > 0) {
      return result + std::format("{} = call {}({})", operands[0].to_string(),
                                  callee, args);
    }
    return result + std::format("call {}({})", callee, args);
  }

  if (opcode == Opcode::Cmp) {
    assert(operands.size() >= 2 && predicate);
    return result + std::format("cmp.{} {}, {}", predicate_name(*predicate),
                                operands[0].to_string(),
                                operands[1].to_string());
  }

  if (opcode == Opcode::CSet) {
    assert(num_defs == 1 && predicate);
    return result + std::format("{} = cset.{}", operands[0].to_string(),
                                predicate_name(*predicate));
  }

  if (num_defs > 0) {
    result += operands[0].to_string() + " = ";
  }

  result += std::string(opcode_name(opcode));

  for (unsigned i = num_defs; i < operands.size(); ++i) {
    result += (i == num_defs) ? " " : ", ";
    result += operands[i].to_string();
  }

  return result;
}

std::string BasicBlock::to_string() const {
  std::string result = std::format("{}:\n", label);
  for (const auto *instr : instructions) {
    result += instr->to_string() + "\n";
  }
  return result;
}

std::string Function::to_string() const {
  std::string result = std::format("function {}(", name);

  for (size_t i = 0; i < param_regs.size(); ++i) {
    if (i > 0)
      result += ", ";
    result += param_regs[i].to_string();
  }
  result += ")";
  if (!has_return_value)
    result += " -> void";
  result += " {\n";

  for (const auto *mbb : blocks) {
    result += mbb->to_string();
  }

  result += "}\n";
  return result;
}

Function *Module::add_function(std::string name) {
  auto *fn = new Function();
  fn->name = std::move(name);
  functions.push_back(fn);
  return fn;
}

Function *Module::get_function(std::string_view name) {
  for (auto *fn : functions) {
    if (fn->name == name)
      return fn;
  }
  return nullptr;
}

std::string Module::to_string() const {
  std::string result;
  for (const auto *fn : functions) {
    result += fn->to_string();
    result += "\n";
  }
  return result;
}

} // namespace lir
