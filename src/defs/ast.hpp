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

// ==== Expressions ====
// TODO: there are still a lot more expressions
class Expression : public ASTNode {
public:
  enum class Kind { Numeric };

private:
  Kind kind;
  std::string_view name;

public:
  Expression(Kind k, std::string_view n, SourceLocation loc = {})
      : ASTNode(loc), kind(k), name(n) {}
  Kind get_kind() const { return kind; }
  const std::string_view get_name() const { return name; }
  void accept(class ASTVisitor &visitor) override;
};

class NumericExpression : public Expression {
private:
  std::string_view value;

public:
  NumericExpression(std::string_view value, SourceLocation loc = {})
      : Expression(Expression::Kind::Numeric, "Numeric", loc), value(value) {}
  const std::string_view get_value() const { return value; }
  void accept(class ASTVisitor &visitor) override;
};

// ==== Statements ====
// TODO: add all possible statements
class Statement : public ASTNode {
public:
  enum class Kind { Compound, Return, /*If, While, Expression, Declaration*/ };

private:
  Kind kind;
  std::string_view name;

public:
  Statement(Kind k, std::string_view n, SourceLocation loc = {})
      : ASTNode(std::move(loc)), kind(k), name(std::move(n)) {}
  Kind get_kind() const { return kind; }
  const std::string_view get_name() const { return name; }
  void accept(class ASTVisitor &visitor) override;
};

class CompoundStmt : public Statement {
private:
  std::vector<Statement *> statements;

public:
  CompoundStmt(std::vector<Statement *> stmts, SourceLocation loc = {})
      : Statement(Statement::Kind::Compound, "Compound", loc),
        statements(std::move(stmts)) {}
  const std::vector<Statement *> &getStatements() const { return statements; }
  void accept(class ASTVisitor &visitor) override;
};

class ReturnStmt : public Statement {
private:
  Expression *expr;

public:
  ReturnStmt(SourceLocation loc = {})
      : Statement(Statement::Kind::Return, "Return", loc) {}
  inline void setExpression(Expression *expression) { this->expr = expression; }
  inline Expression *getExpression() { return expr; }
  void accept(class ASTVisitor &visitor) override;
};

// ==== Declarations ====
// TODO: add all decls
class Declaration : public ASTNode {
public:
  enum class Kind { Function, Parameter, Struct, Const };

private:
  Kind kind;
  std::string_view name;

public:
  Declaration(Kind k, std::string_view n, SourceLocation loc = {})
      : ASTNode(std::move(loc)), kind(k), name(std::move(n)) {}

  Kind get_kind() const { return kind; }
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
  const std::string_view get_type() const { return type; }
  void accept(ASTVisitor &visitor) override;
};

class FunctionDeclaration : public Declaration {
private:
  std::string_view name;
  std::string_view ret_type;
  std::vector<ParameterDeclaration *> parameters;
  CompoundStmt *body = nullptr;

public:
  FunctionDeclaration(std::string_view name, std::string_view ret_type,
                      SourceLocation loc = {})
      : Declaration(Kind::Function, std::move(name), std::move(loc)),
        name(name), ret_type(ret_type) {}
  void accept(ASTVisitor &visitor) override;

  inline void addParameterDeclaration(ParameterDeclaration *paramDecl) {
    parameters.push_back(paramDecl);
  }
  inline void setBody(CompoundStmt *stmt) { body = stmt; }
  const std::vector<ParameterDeclaration *> &getParamDecls() const {
    return parameters;
  }
  CompoundStmt *getBody() const { return body; };
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
  virtual void visit(ParameterDeclaration &decl) {}

  virtual void visit(Statement &stmt) {}
  virtual void visit(CompoundStmt &stmt) {}
  virtual void visit(ReturnStmt &stmt) {}

  virtual void visit(Expression &expr) {}
  virtual void visit(NumericExpression &expr) {}

  virtual void visit(TranslationUnit &unit) {}
};

class ClangStylePrintVisitor : public ASTVisitor {
private:
  std::string content;
  int depth = 0;
  bool use_colors = true;

  // ANSI color codes for terminal output
  const std::string RESET = "\033[0m";
  const std::string GREEN = "\033[32m";
  const std::string YELLOW = "\033[33m";
  const std::string BLUE = "\033[34m";
  const std::string MAGENTA = "\033[35m";
  const std::string CYAN = "\033[36m";

  std::string color(const std::string &text, const std::string &color_code) {
    if (use_colors) {
      return color_code + text + RESET;
    }
    return text;
  }

  std::string indent() {
    if (depth == 0) {
      return "";
    }
    std::string result = "";
    for (int i = 0; i < depth - 1; i++) {
      result += "| ";
    }
    result += "|-";
    return result;
  }

  std::string formatLocation(const SourceLocation &loc) {
    if (loc.file_name.empty()) {
      return "<invalid sloc>";
    }
    return std::format("<{}>", loc.file_name);
  }

  std::string formatRange(const SourceLocation &loc) {
    if (loc.file_name.empty()) {
      return "<invalid sloc>";
    }
    return std::format("<line:{}, col:{}>", loc.line, loc.column);
  }

public:
  ClangStylePrintVisitor(bool colored = true)
      : content(""), use_colors(colored) {}

  const std::string_view get_content() const { return content; }

  void visit(TranslationUnit &unit) override {
    content +=
        color("TranslationUnitDecl", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(unit))),
              YELLOW) +
        " " + formatLocation(unit.getLocation()) + "\n";

    depth++;
    for (const auto &decl : unit.getDeclarations()) {
      content += indent();
      decl->accept(*this);
    }
    depth--;
  }

  void visit(FunctionDeclaration &decl) override {
    content +=
        color("FunctionDecl", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(decl))),
              YELLOW) +
        " " + formatRange(decl.getLocation()) + " " +
        color(std::format("line:{}", decl.getLocation().line), CYAN) + " " +
        color(std::string(decl.get_name()), MAGENTA) + " " + "'" +
        std::string(decl.get_return_type()) + " (";

    // Parameter types list
    bool first = true;
    for (const auto &param : decl.getParamDecls()) {
      if (!first)
        content += ", ";
      content += std::string(param->get_type());
      first = false;
    }
    content += ")'\n";

    depth++;
    // Function parameters
    for (const auto &param : decl.getParamDecls()) {
      content += indent();
      param->accept(*this);
    }

    // Function body
    if (decl.getBody() != nullptr) {
      content += indent();
      decl.getBody()->accept(*this);
    }
    depth--;
  }

  void visit(ParameterDeclaration &decl) override {
    content +=
        color("ParmVarDecl", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(decl))),
              YELLOW) +
        " " + formatRange(decl.getLocation()) + " " +
        color(std::format("col:{}", decl.getLocation().column), CYAN) + " " +
        color(std::string(decl.get_name()), MAGENTA) + " " + "'" +
        std::string(decl.get_type()) + "'\n";
  }

  void visit(CompoundStmt &stmt) override {
    content +=
        color("CompoundStmt", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(stmt))),
              YELLOW) +
        " " + formatRange(stmt.getLocation()) + "\n";

    depth++;
    for (const auto &statement : stmt.getStatements()) {
      content += indent();
      statement->accept(*this);
    }
    depth--;
  }

  void visit(ReturnStmt &stmt) override {
    content +=
        color("ReturnStmt", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(stmt))),
              YELLOW) +
        " " + formatRange(stmt.getLocation()) + "\n";

    if (stmt.getExpression() != nullptr) {
      depth++;
      content += indent();
      stmt.getExpression()->accept(*this);
      depth--;
    }
  }

  void visit(NumericExpression &expr) override {
    content +=
        color("IntegerLiteral", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(expr))),
              YELLOW) +
        " " + formatRange(expr.getLocation()) + " " + "'int' " +
        std::string(expr.get_value()) + "\n";
  }
};
#endif // !DEFS_AST_H
