#ifndef ANALYSIS_SYMBOL_H
#define ANALYSIS_SYMBOL_H

#include "../alloc/arena.hpp"
#include "../defs/source_location.hpp"
#include "../type/source_type.hpp"
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>

class Symbol {
public:
  enum class Kind { Variable, Function, Struct };

private:
  size_t id;
  std::string name;
  Kind kind;
  bool initialized;
  SourceLocation location;
  const source_type::Type *type;

public:
  Symbol(std::string_view name, SourceLocation loc, Kind kind, size_t id,
         bool initialized, const source_type::Type *type)
      : id(id), name(name), kind(kind), initialized(initialized),
        location(std::move(loc)), type(type) {}

  virtual ~Symbol() = default;

  [[nodiscard]] std::string_view get_name() const { return name; }
  [[nodiscard]] const SourceLocation &get_location() const { return location; }
  [[nodiscard]] Kind get_kind() const { return kind; }
  [[nodiscard]] size_t get_id() const { return id; }
  [[nodiscard]] const source_type::Type *get_type() const { return type; }
  [[nodiscard]] bool is_initialized() const { return initialized; }

  void set_initialized(bool init) { initialized = init; }

  [[nodiscard]] bool is_variable() const { return kind == Kind::Variable; }
  [[nodiscard]] bool is_function() const { return kind == Kind::Function; }
  [[nodiscard]] bool is_struct() const { return kind == Kind::Struct; }

  [[nodiscard]] std::string to_string() const {
    return std::format("[{} id={} kind={}]", name, id, (int)kind);
  }
};

class FunctionSymbol : public Symbol {
public:
  const source_type::FunctionType *func_type;

  FunctionSymbol(std::string_view name, SourceLocation loc, size_t id,
                 bool initialized, const source_type::Type *return_type,
                 const source_type::FunctionType *func_type = nullptr)
      : Symbol(name, loc, Kind::Function, id, initialized, return_type),
        func_type(func_type) {}
};

class StructSymbol : public Symbol {
public:
  const source_type::StructType *struct_type;

  StructSymbol(std::string_view name, SourceLocation loc, size_t id,
               bool initialized, const source_type::StructType *st)
      : Symbol(name, loc, Kind::Struct, id, initialized, st), struct_type(st) {}
};

class Scope {
  struct StringHash {
    using is_transparent = void;
    size_t operator()(std::string_view s) const noexcept {
      return std::hash<std::string_view>{}(s);
    }
  };
  struct StringEqual {
    using is_transparent = void;
    bool operator()(std::string_view a, std::string_view b) const noexcept {
      return a == b;
    }
  };

  std::unordered_map<std::string, Symbol *, StringHash, StringEqual> symbols;
  Scope *parent;
  std::string scope_name;

public:
  explicit Scope(std::string name = "unnamed", Scope *parent = nullptr)
      : parent(parent), scope_name(std::move(name)) {}

  [[nodiscard]] Scope *get_parent() const { return parent; }
  [[nodiscard]] std::string_view get_name() const { return scope_name; }

  // Returns false if already defined in THIS scope
  bool define(Symbol *sym) {
    auto [it, inserted] = symbols.emplace(std::string(sym->get_name()), sym);
    return inserted;
  }

  [[nodiscard]] Symbol *lookup_local(std::string_view name) {
    auto it = symbols.find(name);
    return it != symbols.end() ? it->second : nullptr;
  }

  [[nodiscard]] Symbol *lookup(std::string_view name) {
    if (auto *sym = lookup_local(name))
      return sym;
    return parent ? parent->lookup(name) : nullptr;
  }
};

class SymbolTable {
  Scope *current_scope;
  arena::Arena arena;
  size_t id_counter = 0;
  std::unordered_map<size_t, Symbol *> id_to_symbol;

public:
  explicit SymbolTable() : arena(arena::Arena{}) {
    current_scope = arena.create<Scope>("top_level");
  }

  void enter_scope(const std::string &name = "anonymous") {
    current_scope = arena.create<Scope>(name, current_scope);
  }

  bool exit_scope() {
    if (auto *p = current_scope->get_parent()) {
      current_scope = p;
      return true;
    }
    return false;
  }

  Symbol *create_variable(std::string_view name, SourceLocation loc,
                          const source_type::Type *type,
                          bool initialized = false) {
    auto *sym =
        arena.create<Symbol>(name, std::move(loc), Symbol::Kind::Variable,
                             next_id(), initialized, type);
    return sym;
  }

  FunctionSymbol *create_function(std::string_view name, SourceLocation loc,
                                  const source_type::Type *return_type,
                                  const source_type::FunctionType *func_type,
                                  bool has_body = false) {
    auto *sym = arena.create<FunctionSymbol>(name, std::move(loc), next_id(),
                                             has_body, return_type, func_type);
    return sym;
  }

  StructSymbol *create_struct(std::string_view name, SourceLocation loc,
                              const source_type::StructType *struct_type,
                              bool is_complete = false) {
    auto *sym = arena.create<StructSymbol>(name, std::move(loc), next_id(),
                                           is_complete, struct_type);
    return sym;
  }

  bool define(Symbol *sym) {
    if (!current_scope->define(sym))
      return false;
    id_to_symbol[sym->get_id()] = sym;
    return true;
  }

  [[nodiscard]] Symbol *lookup(std::string_view name) {
    return current_scope->lookup(name);
  }

  [[nodiscard]] Symbol *lookup_by_id(size_t id) {
    auto it = id_to_symbol.find(id);
    return it != id_to_symbol.end() ? it->second : nullptr;
  }

  size_t next_id() { return id_counter++; }

  [[nodiscard]] std::string dump() const {
    std::string out;
    dump_scope(out, current_scope, 0);
    return out;
  }

private:
  static void dump_scope(std::string &out, Scope *scope, int level) {
    std::string indent(level * 2, ' ');
    out += std::format("{}Scope: {}\n", indent, scope->get_name());

    // Note: would need Scope to expose iteration
    // for (auto &[name, sym] : scope->symbols())
    //     out += std::format("{}  {}\n", indent, sym->to_string());

    if (auto *parent = scope->get_parent())
      dump_scope(out, parent, level + 1);
  }
};

#endif
