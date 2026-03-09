#include "hir.hpp"
#include <algorithm>
#include <iostream>
#include <ranges>
#include <sstream>

namespace hir {

// ============================================================
// Use
// ============================================================
void Use::set(Value *new_value) {
  // Unlink from old value's users list
  if (value) {
    auto &u = value->users;
    u.erase(std::remove(u.begin(), u.end(), this), u.end());
  }
  value = new_value;
  // Link into new value's users list
  if (value)
    value->users.push_back(this);
}

// ============================================================
// Value
// ============================================================
void Value::replace_all_uses_with(Value *other) {
  assert(other != this && "cannot replace with self");
  assert(other && "Tried to replace user with nullptr");
  // Copy because set() modifies users during iteration
  auto users_copy = users;
  for (Use *use : users_copy) {
    assert(use && "Nullptr in value user");
    use->set(other);
  }
}

// ============================================================
// Instruction
// ============================================================
Instruction::Instruction(Opcode op, type::Type *result_type,
                         std::vector<Value *> ops, BasicBlock *par)
    : Value(result_type), opcode(op), parent(par) {
  operands.resize(ops.size());
  for (size_t i = 0; i < ops.size(); i++) {
    operands[i].user = this;
    operands[i].operand_index = i;
    operands[i].set(ops[i]); // this is fine — operands already resized
  }
}

void Instruction::erase_from_parent() {
  assert(parent && "instruction has no parent block");
  assert(users.empty() && "cannot erase instruction that still has users");

  // Unlink all operand uses before removal
  for (auto &use : operands) {
    use.set(nullptr);
  }

  auto &instrs = parent->instructions;
  instrs.erase(std::remove(instrs.begin(), instrs.end(), this), instrs.end());
  parent = nullptr;
}

std::string Instruction::to_string() const {
  std::ostringstream oss;

  if (!is_void()) {
    oss << Value::to_string() << " = ";
  }

  switch (opcode) {
  case Opcode::Add:
    oss << "add" << type->to_string();
    break;
  case Opcode::Sub:
    oss << "sub";
    break;
  case Opcode::Mul:
    oss << "mul";
    break;
  case Opcode::SDiv:
    oss << "sdiv";
    break;
  case Opcode::UDiv:
    oss << "udiv";
    break;
  case Opcode::SRem:
    oss << "srem";
    break;
  case Opcode::URem:
    oss << "urem";
    break;
  case Opcode::And:
    oss << "and";
    break;
  case Opcode::Or:
    oss << "or";
    break;
  case Opcode::Xor:
    oss << "xor";
    break;
  case Opcode::Shl:
    oss << "shl";
    break;
  case Opcode::LShr:
    oss << "lshr";
    break;
  case Opcode::AShr:
    oss << "ashr";
    break;
  case Opcode::Not:
    oss << "not";
    break;
  case Opcode::Neg:
    oss << "neg";
    break;
  case Opcode::ICmp:
    oss << "icmp ";
    if (predicate) {
      switch (*predicate) {
      case ICmpPredicate::EQ:
        oss << "eq";
        break;
      case ICmpPredicate::NE:
        oss << "ne";
        break;
      case ICmpPredicate::SLT:
        oss << "slt";
        break;
      case ICmpPredicate::SLE:
        oss << "sle";
        break;
      case ICmpPredicate::SGT:
        oss << "sgt";
        break;
      case ICmpPredicate::SGE:
        oss << "sge";
        break;
      case ICmpPredicate::ULT:
        oss << "ult";
        break;
      case ICmpPredicate::ULE:
        oss << "ule";
        break;
      case ICmpPredicate::UGT:
        oss << "ugt";
        break;
      case ICmpPredicate::UGE:
        oss << "uge";
        break;
      }
    }
    break;
  case Opcode::Alloca:
    oss << "alloca " << type_arg->to_string();
    break;
  case Opcode::Load:
    oss << "load " << type->to_string();
    break;
  case Opcode::Store:
    oss << "store " << operands[0].value->type->to_string();
    break;
  case Opcode::GetElementPtr:
    oss << "gep " << type_arg->to_string();
    break;
  case Opcode::Trunc:
    oss << "trunc " << type->to_string();
    break;
  case Opcode::ZExt:
    oss << "zext " << type->to_string();
    break;
  case Opcode::SExt:
    oss << "sext " << type->to_string();
    break;
  case Opcode::PtrToInt:
    oss << "ptrtoint " << type->to_string();
    break;
  case Opcode::IntToPtr:
    oss << "inttoptr";
    break;
  case Opcode::Br:
    oss << "br";
    break;
  case Opcode::CondBr:
    oss << "condbr";
    break;
  case Opcode::Ret:
    oss << "ret";
    break;
  case Opcode::Call:
    oss << "call";
    break;
  case Opcode::Phi: {
    oss << "phi " << type->to_string();
    const auto *node = dynamic_cast<const PhiNode *>(this);
    bool first = true;
    for (size_t i = 0; i < node->operands.size(); i++) {
      if (!first)
        oss << ",";
      first = false;
      oss << " [" << node->operands[i].value->ref_string() << ", "
          << node->incoming_blocks[i]->ref_string() << "]";
    }
    return oss.str();
  }
  }

  // Print operands
  for (size_t i = 0; i < operands.size(); i++) {
    oss << (i == 0 ? " " : ", ");
    if (operands[i].value) {
      oss << operands[i].value->ref_string();
    } else {
      oss << "<null operand " << i << ">"; // tells you which slot is null
    }
  }

  return oss.str();
}

// ============================================================
// PhiNode
// ============================================================
void PhiNode::add_incoming(Value *val, BasicBlock *from) {
  assert(val && from && "null incoming value or block");

  operands.push_back(Use{});
  Use &use = operands.back();
  use.user = this;
  use.operand_index = operands.size() - 1;
  use.set(val); // now registers address inside the vector

  incoming_blocks.push_back(from);
}

Value *PhiNode::incoming_from(BasicBlock *bb) const {
  for (size_t i = 0; i < incoming_blocks.size(); i++) {
    if (incoming_blocks[i] == bb)
      return operands[i].value;
  }
  return nullptr;
}

void PhiNode::remove_incoming(BasicBlock *bb) {
  for (size_t i = 0; i < incoming_blocks.size(); i++) {
    if (incoming_blocks[i] == bb) {
      operands[i].set(nullptr);
      operands.erase(operands.begin() + i);
      incoming_blocks.erase(incoming_blocks.begin() + i);
      return;
    }
  }
}

void PhiNode::replace_incoming_block(BasicBlock *old_bb, BasicBlock *new_bb) {
  for (auto &bb : incoming_blocks) {
    if (bb == old_bb) {
      bb = new_bb;
      return;
    }
  }
}

// ============================================================
// BasicBlock
// ============================================================
Instruction *BasicBlock::push_back(Instruction *i) {
  i->parent = this;
  instructions.push_back(i);
  return i;
}

Instruction *BasicBlock::insert_before(Instruction *before,
                                       Instruction *value) {
  auto it = std::find(instructions.begin(), instructions.end(), before);
  assert(it != instructions.end() && "instruction not found in block");
  value->parent = this;
  instructions.insert(it, value);
  return value;
}

Instruction *BasicBlock::terminator() {
  if (instructions.empty())
    return nullptr;
  auto *last = instructions.back();
  return last->is_terminator() ? last : nullptr;
}

const Instruction *BasicBlock::terminator() const {
  if (instructions.empty())
    return nullptr;
  auto *last = instructions.back();
  return last->is_terminator() ? last : nullptr;
}

std::vector<BasicBlock *> BasicBlock::successors() const {
  const auto *term = terminator();
  if (!term)
    return {};

  std::vector<BasicBlock *> succs;
  switch (term->opcode) {
  case Opcode::Br:
    // operand(0) is the target block
    succs.push_back(dynamic_cast<BasicBlock *>(term->operand(0)));
    break;
  case Opcode::CondBr:
    // operand(0) = condition, operand(1) = true bb, operand(2) = false bb
    succs.push_back(dynamic_cast<BasicBlock *>(term->operand(1)));
    succs.push_back(dynamic_cast<BasicBlock *>(term->operand(2)));
    break;
  case Opcode::Ret:
  default:
    break;
  }
  return succs;
}

void BasicBlock::remove_predecessor(BasicBlock *bb) {
  predecessors.erase(std::remove(predecessors.begin(), predecessors.end(), bb),
                     predecessors.end());

  // Update phi nodes — remove the incoming from this predecessor
  for (auto *instr : instructions) {
    if (instr->opcode != Opcode::Phi)
      break; // phis are always first
    dynamic_cast<PhiNode *>(instr)->remove_incoming(bb);
  }
}

bool BasicBlock::is_entry() const {
  if (!parent)
    return false;
  return parent->blocks.front() == this;
}

bool BasicBlock::has_single_predecessor() const {
  return predecessors.size() == 1;
}

// ============================================================
// Function
// ============================================================
Function::Function(const std::string &name, type::FunctionType *function_type,
                   Module *parent)
    : Value(function_type, name), function_type(function_type), parent(parent) {
  // Create entry block
  auto *entry_bb = new BasicBlock("entry", this);
  blocks.push_back(entry_bb);

  // Create argument values
  for (size_t i = 0; i < function_type->param_types.size(); i++) {
    auto *arg = new Argument(function_type->param_types[i],
                             "arg" + std::to_string(i), this, i);
    arguments.push_back(arg);
  }
}

BasicBlock *Function::append_block(const std::string &label) {
  auto *bb = new BasicBlock(
      label.empty() ? "l" + std::to_string(block_id++) : label, this);
  blocks.push_back(bb);
  return bb;
}

void Function::renumber() {
  size_t counter = 0;

  // Number arguments first
  for (auto *arg : arguments) {
    arg->id = counter++;
  }

  // Number all instructions
  for (auto *bb : blocks) {
    bb->id = counter++;
    for (auto *instr : bb->instructions) {
      if (!instr->is_void()) {
        instr->id = counter++;
      }
    }
  }
}

std::vector<std::string> Function::verify() const {
  std::vector<std::string> errors;

  for (const auto *bb : blocks) {
    if (bb->instructions.empty()) {
      errors.push_back("block '" + bb->label + "' has no instructions");
      continue;
    }

    // Check terminator
    if (!bb->terminator()) {
      errors.push_back("block '" + bb->label +
                       "' does not end with a terminator");
    }

    // Check phis are at the top
    bool seen_non_phi = false;
    for (const auto *instr : bb->instructions) {
      if (instr->opcode == Opcode::Phi) {
        if (seen_non_phi) {
          errors.push_back("phi node not at top of block '" + bb->label + "'");
        }
      } else {
        seen_non_phi = true;
      }
    }

    // Check each instruction
    for (const auto *instr : bb->instructions) {
      // Check operands are non-null
      for (size_t i = 0; i < instr->operand_count(); i++) {
        if (!instr->operand(i)) {
          errors.push_back("instruction '" + instr->to_string() +
                           "' has null operand " + std::to_string(i));
        }
      }

      // Check phi incoming block count matches operand count
      if (instr->opcode == Opcode::Phi) {
        const auto *phi = dynamic_cast<const PhiNode *>(instr);
        if (phi->incoming_blocks.size() != phi->operand_count()) {
          errors.emplace_back("phi node has mismatched incoming "
                              "blocks and operands");
        }
      }
    }
  }

  return errors;
}

// ============================================================
// Module
// ============================================================
Function *Module::add_function(std::string name,
                               type::FunctionType *function_type) {
  auto fn = std::make_unique<Function>(name, function_type, this);
  auto *ptr = fn.get();
  functions.push_back(std::move(fn));
  return ptr;
}

Function *Module::get_function(std::string_view name) const {
  for (const auto &fn : functions) {
    if (fn->name == name)
      return fn.get();
  }
  return nullptr;
}

ConstantInt *Module::const_int(type::IntegerType *type, int64_t value) {
  // Deduplicate by combining type width and value into a single key
  uint64_t key = (static_cast<uint64_t>(type->width) << 56) |
                 (static_cast<uint64_t>(value) & 0x00FFFFFFFFFFFFFF);

  auto it = int_constants.find(key);
  if (it != int_constants.end())
    return it->second.get();

  auto c = std::make_unique<ConstantInt>(type, static_cast<uint64_t>(value));
  auto *ptr = c.get();
  int_constants[key] = std::move(c);
  return ptr;
}

ConstantInt *Module::const_bool(bool value) {
  return const_int(types.i1(), value ? 1 : 0);
}

Undef *Module::get_undef(type::Type *type) {
  // Check if we already have an undef for this type
  for (const auto &u : undefs) {
    if (u->type == type)
      return u.get();
  }
  auto u = std::make_unique<Undef>(type);
  auto *ptr = u.get();
  undefs.push_back(std::move(u));
  return ptr;
}

std::string Module::to_string() const {
  std::ostringstream oss;
  oss << "; Module: " << name << "\n\n";

  for (const auto &fn : functions) {
    const_cast<Function *>(fn.get())->renumber();

    if (fn->is_extern) {
      oss << "declare " << fn->function_type->return_type->to_string() << " @"
          << fn->name << "()\n";
      continue;
    }

    oss << "define " << fn->function_type->return_type->to_string() << " @"
        << fn->name << "(";

    for (size_t i = 0; i < fn->arguments.size(); i++) {
      if (i > 0)
        oss << ", ";
      oss << fn->arguments[i]->type->to_string() << " "
          << fn->arguments[i]->to_string();
    }
    oss << ") {\n";

    for (const auto *bb : fn->blocks) {
      oss << bb->label << ":\n";
      for (const auto *instr : bb->instructions) {
        oss << "  " << instr->to_string() << "\n";
      }
    }

    oss << "}\n\n";
  }

  return oss.str();
}
static std::string escape_dot(const std::string &s) {
  std::string out;
  for (char c : s) {
    switch (c) {
    case '"':
      out += "\\\"";
      break;
    case '<':
      out += "\\<";
      break;
    case '>':
      out += "\\>";
      break;
    case '{':
      out += "\\{";
      break;
    case '}':
      out += "\\}";
      break;
    case '|':
      out += "\\|";
      break;
    case '\\':
      out += "\\\\";
      break;
    default:
      out += c;
      break;
    }
  }
  return out;
}

static std::string block_node_id(const BasicBlock *bb) {
  return "bb_" + std::to_string(reinterpret_cast<uintptr_t>(bb));
}

static std::string opcode_color(Opcode op) {
  switch (op) {
  // Control flow — blue family
  case Opcode::Br:
  case Opcode::CondBr:
  case Opcode::Ret:
    return "#dbeafe"; // light blue

  // Memory — orange family
  case Opcode::Alloca:
  case Opcode::Load:
  case Opcode::Store:
  case Opcode::GetElementPtr:
    return "#ffedd5"; // light orange

  // Comparisons — purple family
  case Opcode::ICmp:
    return "#f3e8ff"; // light purple

  // Phi — green family
  case Opcode::Phi:
    return "#dcfce7"; // light green

  // Calls — yellow family
  case Opcode::Call:
    return "#fef9c3"; // light yellow

  // Arithmetic + everything else — neutral
  default:
    return "#f8fafc"; // near white
  }
}

std::string Function::to_dot() const {
  const_cast<Function *>(this)->renumber();
  std::ostringstream dot;

  dot << "  subgraph cluster_" << name << " {\n";
  dot << "    label=\"" << escape_dot(name) << "\";\n\n";

  // Nodes
  for (const auto *bb : blocks) {
    std::string node_id = block_node_id(bb);
    std::ostringstream label;

    label << (bb->label.empty() ? "entry" : bb->label) << ":\\n";
    for (const auto *instr : bb->instructions) {
      label << escape_dot(instr->to_string()) << "\\n";
    }

    dot << "    " << node_id << " [shape=box label=\"" << label.str()
        << "\"];\n";
  }

  dot << "\n";

  // Edges
  for (const auto *bb : blocks) {
    auto succs = bb->successors();
    std::string src = block_node_id(bb);

    if (succs.size() == 1) {
      dot << "    " << src << " -> " << block_node_id(succs[0]) << ";\n";
    } else if (succs.size() == 2) {
      dot << "    " << src << " -> " << block_node_id(succs[0])
          << " [label=\"true\"];\n";
      dot << "    " << src << " -> " << block_node_id(succs[1])
          << " [label=\"false\"];\n";
    }
  }

  dot << "  }\n";
  return dot.str();
}

std::string Module::to_dot() const {
  std::ostringstream dot;

  dot << "digraph IR {\n";
  dot << "  rankdir=TB;\n";
  dot << "  node [shape=box fontname=\"Courier\" fontsize=11];\n\n";

  for (const auto &fn : functions) {
    dot << const_cast<Function *>(fn.get())->to_dot();
    dot << "\n";
  }

  dot << "}\n";
  return dot.str();
}
} // namespace hir
