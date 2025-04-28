#ifndef DEFS_AST_H
#define DEFS_AST_H

#include "spdlog/fmt/bundled/base.h"
#include <algorithm>
#include <format>
#include <string>
#include <string_view>
#include <vector>
struct SourceLocation {
  std::string_view file_name;
  int line;
  int column;

  SourceLocation(std::string_view file_name = "", int line = 0, int column = 0)
      : file_name(file_name), line(line), column(column) {}
};

class ASTNode {
protected:
  SourceLocation location;

public:
  explicit ASTNode(SourceLocation loc = {}) : location(std::move(loc)) {}
  virtual ~ASTNode() = default;

  const SourceLocation &getLocation() const { return location; }

  virtual void accept(class ASTVisitor &visitor) = 0;
};

// ==== Declarations ====

class Declaration : public ASTNode {
public:
  enum class Kind { Function, Parameter, Class, Struct, Const };

private:
  Kind kind;
  std::string_view name;

public:
  Declaration(Kind k, std::string_view n, SourceLocation loc = {})
      : ASTNode(std::move(loc)), kind(k), name(std::move(n)) {}

  Kind get_kindd() const { return kind; }
  const std::string_view get_name() const { return name; }
  void accept(class ASTVisitor &visitor) override;
};

class ParameterDeclaration : public Declaration {
private:
  std::string_view name;
  std::string_view type;

public:
  ParameterDeclaration(std::string_view name, std::string_view type,
                       SourceLocation loc = {})
      : Declaration(Kind::Parameter, std::move(name), std::move(loc)),
        name(name), type(type) {}
  void accept(ASTVisitor &visitor) override;
};

class FunctionDeclaration : public Declaration {
private:
  std::string_view name;
  std::string_view ret_type;

public:
  FunctionDeclaration(std::string_view name, std::string_view ret_type,
                      SourceLocation loc = {})
      : Declaration(Kind::Function, std::move(name), std::move(loc)),
        name(name), ret_type(ret_type) {}
  void accept(ASTVisitor &visitor) override;

  const std::string_view get_return_type() const { return ret_type; }
};

class TranslationUnit : public ASTNode {
private:
  std::vector<Declaration *> declarations;

public:
  TranslationUnit(SourceLocation loc = {}) : ASTNode(std::move(loc)) {}

  inline void addDeclaration(Declaration *declaration) {
    declarations.push_back(declaration);
  }

  const std::vector<Declaration *> &getDeclarations() const {
    return declarations;
  }

  void accept(ASTVisitor &visitor) override;
};

class ASTVisitor {
public:
  virtual void visit(Declaration &decl) {}
  virtual void visit(FunctionDeclaration &decl) {}
  virtual void visit(TranslationUnit &unit) {}
};

class PrintVisitor : public ASTVisitor {
private:
  std::string content;

public:
  PrintVisitor() : content("") {}

  const std::string_view get_content() const { return content; }

  void visit(FunctionDeclaration &decl) {
    content += std::format(
        "─FunctionDeclaration {:#12x} File: {} Name: {} "
        "Return type: {} Loc: {}:{}\n" reinterpret_cast<std::size_t>(
            std::addressof(decl)),
        decl.getLocation().file_name, decl.get_name(), decl.get_return_type(),
        decl.getLocation().line, decl.getLocation().column);
  }
  void visit(TranslationUnit &unit) {
    content += std::format("┌TranslationUnit {:#12x} File: {}\n",
                           reinterpret_cast<std::size_t>(std::addressof(unit)),
                           unit.getLocation().file_name);
    for (const auto &decl : unit.getDeclarations()) {
      content += "├";
      decl->accept(*this);
    }
  }
};

#endif // !DEFS_AST_H
