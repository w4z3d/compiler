#ifndef ANALYSIS_SYMBOL_H
#define ANALYSIS_SYMBOL_H

#include "../alloc/arena.hpp"
#include "../defs/source_location.hpp"
#include "spdlog/spdlog.h"
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

class Symbol {
public:
  enum class Kind { Variable, Function, Struct };

private:
  size_t id;
  std::string name;
  Kind kind;
  bool initialized = false;
  bool constant = false;

  SourceLocation location;

public:
  explicit Symbol(std::string_view name, SourceLocation loc, Kind kind)
      : name(name), location(std::move(loc)), kind(kind) {}

  [[nodiscard]] const std::string &get_name() const { return name; }

  [[nodiscard]] const SourceLocation &get_source_location() const {
    return location;
  }

  [[nodiscard]] Kind get_kind() const { return kind; }

  size_t get_id() const {
    return id;
  }

  void set_id(size_t id_) {
    id = id_;
  }

  [[nodiscard]] std::string to_string() const {
    return std::format("[{}{}, <{}:{}:{} - {}:{}:{}>]", name, id, location.file_name,
                       std::get<0>(location.begin), std::get<1>(location.begin),
                       location.file_name, std::get<0>(location.end),
                       std::get<1>(location.end));
  }
};

class VariableSymbol : public Symbol {
public:
  explicit VariableSymbol(std::string_view name, SourceLocation loc)
      : Symbol(name, loc, Kind::Variable) {}
};

class FunctionSymbol : public Symbol {
public:
  explicit FunctionSymbol(std::string_view name, SourceLocation loc)
      : Symbol(name, loc, Kind::Function) {}
};

class StructSymbol : public Symbol {
public:
  explicit StructSymbol(std::string_view name, SourceLocation loc)
      : Symbol(name, loc, Kind::Struct) {}
};

class Scope {
private:
  struct StringHash {
    using is_transparent = void;
    std::size_t operator()(std::string_view str) const noexcept {
      return std::hash<std::string_view>{}(str);
    }
  };

  struct StringEqual {
    using is_transparent = void;
    bool operator()(std::string_view lhs, std::string_view rhs) const noexcept {
      return lhs == rhs;
    }
  };

  std::unordered_map<std::string, Symbol, StringHash, StringEqual> symbols;
  Scope *parent;
  std::string scope_name;

public:
  explicit Scope(std::string name = "unnamed", Scope *parent = nullptr)
      : parent(parent), scope_name(std::move(name)) {}

  [[nodiscard]] Scope *get_parent() const { return parent; }

  bool define(const Symbol &symbol) {
    if (symbols.find(symbol.get_name()) != symbols.end()) {
      return false;
    }
    symbols.emplace(symbol.get_name(), symbol);
    return true;
  }

  [[nodiscard]] std::optional<Symbol>
  lookup_local(std::string_view name) const {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
      return it->second;
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<Symbol>
  lookup(const std::string_view name) const {
    auto symbol = lookup_local(name);

    if (symbol) {
      return symbol;
    }

    auto parent = get_parent();
    if (parent) {
      return parent->lookup(name);
    }

    return std::nullopt;
  }

  [[nodiscard]] std::unordered_map<std::string, Symbol, StringHash, StringEqual>
  get_scoped_symbols() const {
    return symbols;
  }

  [[nodiscard]] std::string_view get_name() const { return scope_name; }
};

class SymbolTable {
private:
  Scope *current_scope;
  arena::Arena arena;
  size_t id_counter = 0;

public:
  explicit SymbolTable() : arena(arena::Arena{}) {
    current_scope = arena.create<Scope>("top_level");
  }

  void enter_scope(const std::string &name = "anonymous") {
    auto new_scope = arena.create<Scope>(name, current_scope);
    spdlog::debug("Entering scope {} with parent {}", new_scope->get_name(),
                  current_scope->get_name());
    current_scope = new_scope;
  }

  bool exit_scope() {

    spdlog::debug("Exiting Scope {}", current_scope->get_name());
    auto parent = current_scope->get_parent();

    if (parent) {
      spdlog::debug("Exiting scope {} with parent {}",
                    current_scope->get_name(), parent->get_name());
      current_scope = parent;
      return true;
    }
    spdlog::debug("New Scope {}", current_scope->get_name());
    return false;
  }

  bool define(Symbol &symbol) {
    bool v = current_scope->define(symbol);
    if (v) {
      symbol.set_id(id_counter++);
    }
    return v;
  }

  [[nodiscard]] std::optional<Symbol> lookup(std::string_view name) const {
    return current_scope->lookup(name);
  }

  [[nodiscard]] std::string dump() const {
    std::string content{};
    return dump_scope(content, current_scope, 0);
  }

private:
  static std::string dump_scope(std::string &content, Scope *scope, int level) {
    std::string indent(level * 2, ' ');
    content += std::format("Scope: {} \n", scope->get_name());

    auto symbols = scope->get_scoped_symbols();
    for (const auto &pair : symbols) {
      content += std::format("{}  {} \n", indent, pair.second.to_string());
    }

    auto parent = scope->get_parent();
    if (parent) {
      content += std::format("{} Parent:\n", indent);
      dump_scope(content, parent, level + 1);
    }

    return content;
  }
};

#endif // !ANALYSIS_SYMBOL_H
