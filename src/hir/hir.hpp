#ifndef SRC_HIR_HIR_HPP
#define SRC_HIR_HIR_HPP

#include "hir_type.hpp"
#include <cassert>
#include <cstdint>
#include <list>
#include <string>
#include <vector>
namespace hir {

struct Value;
struct Instruction;
struct BasicBlock;
struct Function;
struct Module;
struct PhiNode;

struct Use {

  Value *value = nullptr;
  Instruction *user = nullptr;
  size_t operand_index = 0;

  void set(Value *new_value);
};

struct Value {
  type::Type *type;
  std::string name;
  size_t id;

  std::vector<Use *> users;

  explicit Value(type::Type *type, std::string_view name = "")
      : type(type), name(name), id(0) {}
  virtual ~Value() = default;

  [[nodiscard]] bool has_users() const { return !users.empty(); }
  [[nodiscard]] bool is_void() const { return type->is_void(); }
  [[nodiscard]] virtual std::string to_string() const {
    return name.empty() ? "%" + std::to_string(id) : "%" + name;
  }
  [[nodiscard]] virtual std::string ref_string() const { return to_string(); }
  void replace_all_uses_with(Value *other);
};

struct ConstantInt : Value {
  uint64_t raw_bits;
  ConstantInt(type::IntegerType *type, uint64_t raw_bits)
      : Value(type), raw_bits(raw_bits) {}

  [[nodiscard]] int64_t signed_value() const {
    return static_cast<int64_t>(raw_bits);
  }
  [[nodiscard]] uint64_t unsigned_value() const { return raw_bits; }

  [[nodiscard]] bool is_zero() const { return raw_bits == 0; }
  [[nodiscard]] bool is_one() const { return raw_bits == 1; }

  [[nodiscard]] std::string to_string() const override {
    return std::to_string(signed_value());
  }
};

struct Undef : Value {
  explicit Undef(type::Type *type) : Value(type, "undef") {}
  [[nodiscard]] std::string to_string() const override { return "undef"; }
};

enum class Opcode {
  Add,
  Sub,
  Mul,
  SDiv,
  UDiv,
  SRem,
  URem,

  And,
  Or,
  Xor,
  Shl,
  LShr,
  AShr,
  Not,

  Neg,

  ICmp,

  Alloca,
  Load,
  Store,
  GetElementPtr, // Struct field pointer

  // Conversions
  Trunc, // integer narrowing (from 64 reg to 32 eg)
  ZExt,  // Zero extend
  SExt,  // Sign Extend
  PtrToInt,
  IntToPtr,

  // Control Flow
  Br,
  CondBr,
  Ret,
  Call,

  Phi
};

enum class ICmpPredicate { EQ, NE, SLT, SLE, SGT, SGE, ULT, ULE, UGT, UGE };

struct Instruction : Value {
  Opcode opcode;
  std::vector<Use> operands;
  BasicBlock *parent = nullptr;

  std::optional<ICmpPredicate> predicate;

  // Optional type argument — used by Alloca (allocated type),
  // Load (loaded type), GEP (base type), conversions (dest type)
  type::Type *type_arg = nullptr;

  Instruction(Opcode opcode, type::Type *result_type,
              std::vector<Value *> ops = {}, BasicBlock *parent = nullptr);

  [[nodiscard]] Value *operand(size_t i) const { return operands[i].value; }
  [[nodiscard]] size_t operand_count() const { return operands.size(); }

  void set_operand(size_t i, Value *value) { operands[i].set(value); };

  [[nodiscard]] bool is_terminator() const {
    return opcode == Opcode::Br || opcode == Opcode::CondBr ||
           opcode == Opcode::Ret;
  }

  [[nodiscard]] bool has_side_effects() const {
    return opcode == Opcode::Call || opcode == Opcode::Store ||
           opcode == Opcode::Alloca;
  }

  //
  // Remove this instruction from the parent block.
  // The caller of this function has to ensure that there are no uses left
  void erase_from_parent();
  [[nodiscard]] std::string ref_string() const override {
    return name.empty() ? "%" + std::to_string(id) : "%" + name;
  }
  [[nodiscard]] std::string to_string() const override;
};

// Phi node — adds incoming-block metadata to each operand
struct PhiNode : Instruction {
  std::vector<BasicBlock *> incoming_blocks;

  explicit PhiNode(type::Type *t, BasicBlock *parent = nullptr)
      : Instruction(Opcode::Phi, t, {}, parent) {}

  void add_incoming(Value *val, BasicBlock *from);
  Value *incoming_from(BasicBlock *bb) const;
  void remove_incoming(BasicBlock *bb);
  void replace_incoming_block(BasicBlock *old_bb, BasicBlock *new_bb);
};

struct BasicBlock : Value {
  std::string label;
  std::list<Instruction *> instructions;
  Function *parent;

  std::vector<BasicBlock *> predecessors;

  explicit BasicBlock(std::string label = "", Function *parent = nullptr)
      : Value(nullptr), label(std::move(label)), parent(parent) {}

  [[maybe_unused]] Instruction *push_back(Instruction *i);
  [[maybe_unused]] Instruction *insert_before(Instruction *before,
                                              Instruction *value);

  [[nodiscard]] Instruction *terminator();
  [[nodiscard]] const Instruction *terminator() const;

  [[nodiscard]] std::vector<BasicBlock *> successors() const;

  void remove_predecessor(BasicBlock *bb);

  [[nodiscard]] bool is_entry() const;
  [[nodiscard]] bool has_single_predecessor() const;
  [[nodiscard]] auto begin() { return instructions.begin(); };
  [[nodiscard]] auto end() { return instructions.end(); };
  [[nodiscard]] std::string to_string() const override {
    return label.empty() ? "bb" + std::to_string(id) : label;
  }
};
struct Argument : Value {
  Function *parent;
  size_t arg_index;
  Argument(type::Type *type, std::string_view name, Function *parent,
           size_t index)
      : Value(type, name), parent(parent), arg_index(index) {}
};

struct Function : Value {
  type::FunctionType *function_type;
  std::vector<Argument *> arguments; // One per param type
  std::list<BasicBlock *> blocks;
  bool is_extern = false;
  Module *parent = nullptr;

  Function(const std::string &name, type::FunctionType *type, Module *parent);

  [[nodiscard]] BasicBlock *entry() {
    assert(!blocks.empty());
    return blocks.front();
  }

  [[maybe_unused]] BasicBlock *append_block(const std::string &label = "");

  // Sequential ids to all values for printing
  void renumber();

  // Maybe IR invariant checking
  [[nodiscard]] std::vector<std::string> verify() const;
  [[nodiscard]] std::string to_dot() const;
  [[nodiscard]] auto begin() { return blocks.begin(); }
  [[nodiscard]] auto end() { return blocks.end(); }
};

struct Module {
  std::string name;
  type::TypeContext types;
  std::vector<std::unique_ptr<Function>> functions;
  explicit Module(std::string name = "") : name(std::move(name)) {}

  [[maybe_unused]] Function *add_function(std::string name,
                                          type::FunctionType *type);
  [[nodiscard]] Function *get_function(std::string_view name) const;

  // Constants owned by the module deduplicated by type + value
  [[nodiscard]] ConstantInt *const_int(type::IntegerType *type, int64_t value);
  [[nodiscard]] ConstantInt *const_bool(bool value);
  [[nodiscard]] Undef *get_undef(type::Type *type);
  [[nodiscard]] std::string to_string() const;
  [[nodiscard]] std::string to_dot() const;

public:
  // Dedupe caches
  std::unordered_map<uint64_t, std::unique_ptr<ConstantInt>> int_constants;
  std::vector<std::unique_ptr<Undef>> undefs;
};

} // namespace hir
#endif // SRC_HIR_HIR_HPP
