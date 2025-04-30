#ifndef DEFS_AST_H
#define DEFS_AST_H

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
  explicit ASTNode(SourceLocation loc = {}) : location(loc) {}
  virtual ~ASTNode() = default;

  [[nodiscard]] const SourceLocation &get_location() const { return location; }

  virtual void accept(class ASTVisitor &visitor) = 0;
};

// ==== Expressions ====
// TODO: there are still a lot more expressions
class Expression : public ASTNode {
public:
  enum class Kind { Numeric, String, Call };

private:
  Kind kind;
  std::string_view name;

public:
  Expression(Kind k, std::string_view n, SourceLocation loc = {})
      : ASTNode(loc), kind(k), name(n) {}
  [[nodiscard]] Kind get_kind() const { return kind; }
  [[nodiscard]] std::string_view get_name() const { return name; }
  void accept(class ASTVisitor &visitor) override;
};

class NumericExpr : public Expression {
private:
  std::string_view value;

public:
  explicit NumericExpr(std::string_view value, SourceLocation loc = {})
      : Expression(Expression::Kind::Numeric, "Numeric", loc), value(value) {}
  [[nodiscard]] std::string_view get_value() const { return value; }
  void accept(class ASTVisitor &visitor) override;
};

class StringLiteralExpr : public Expression {
private:
  std::string_view value;

public:
  explicit StringLiteralExpr(std::string_view value, SourceLocation loc = {})
      : Expression(Expression::Kind::String, "StringLiteral", loc),
        value(value) {}
  [[nodiscard]] std::string_view get_value() const { return value; }
  void accept(class ASTVisitor &visitor) override;
};

class CallExpr : public Expression {
private:
  std::string_view function_name;
  std::vector<Expression *> params{};

public:
  explicit CallExpr(std::string_view name, SourceLocation loc = {})
      : Expression(Kind::Call, "Call", loc), function_name(name) {}

  [[nodiscard]] const std::vector<Expression *> &get_params() const {
    return params;
  }

  [[nodiscard]] std::string_view get_function_name() const {
    return function_name;
  }
  void add_param(Expression *param) { params.push_back(param); }
  void accept(class ASTVisitor &visitor) override;
};

// ==== Statements ====
// TODO: add all possible statements
class Statement : public ASTNode {
public:
  enum class Kind {
    Compound,
    Return,
    Assert /*If, While, Expression, Declaration*/
  };

private:
  Kind kind;
  std::string_view name;

public:
  Statement(Kind k, std::string_view n, SourceLocation loc = {})
      : ASTNode(loc), kind(k), name(n) {}
  [[nodiscard]] Kind get_kind() const { return kind; }
  [[nodiscard]] std::string_view get_name() const { return name; }
  void accept(class ASTVisitor &visitor) override;
};

class AssertStmt : public Statement {
private:
  Expression *expression{};

public:
  explicit AssertStmt(SourceLocation loc = {})
      : Statement(Kind::Assert, "Assert", loc) {}

  void set_expression(Expression *expression) { this->expression = expression; }
  [[nodiscard]] Expression *get_expression() const { return this->expression; }
  void accept(class ASTVisitor &visitor) override;
};

class CompoundStmt : public Statement {
private:
  std::vector<Statement *> statements;

public:
  explicit CompoundStmt(std::vector<Statement *> stmts, SourceLocation loc = {})
      : Statement(Statement::Kind::Compound, "Compound", loc),
        statements(std::move(stmts)) {}
  [[nodiscard]] const std::vector<Statement *> &getStatements() const {
    return statements;
  }

  void add_statement(Statement *statement) {
    this->statements.push_back(statement);
  }

  void accept(class ASTVisitor &visitor) override;
};

class ReturnStmt : public Statement {
private:
  Expression *expr{};

public:
  explicit ReturnStmt(SourceLocation loc = {})
      : Statement(Statement::Kind::Return, "Return", loc) {}
  void set_expression(Expression *expression) { this->expr = expression; }
  [[nodiscard]] Expression *get_expression() const { return expr; }
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
      : ASTNode(loc), kind(k), name(n) {}

  [[nodiscard]] Kind get_kind() const { return kind; }
  [[nodiscard]] std::string_view get_name() const { return name; }
  void accept(class ASTVisitor &visitor) override;
};

class ParameterDeclaration : public Declaration {
private:
  std::string_view name;
  std::string_view type;

public:
  ParameterDeclaration(std::string_view name, std::string_view type,
                       SourceLocation loc = {})
      : Declaration(Kind::Parameter, name, loc), name(name), type(type) {}
  [[nodiscard]] std::string_view get_type() const { return type; }
  void accept(ASTVisitor &visitor) override;
};

class FunctionDeclaration : public Declaration {
private:
  std::string_view name;
  std::string_view ret_type;
  std::vector<ParameterDeclaration *> parameters;
  CompoundStmt *body{};

public:
  FunctionDeclaration(std::string_view name, std::string_view ret_type,
                      SourceLocation loc = {})
      : Declaration(Kind::Function, name, loc), name(name), ret_type(ret_type) {
  }
  void accept(ASTVisitor &visitor) override;

  void add_parameter_declaration(ParameterDeclaration *paramDecl) {
    parameters.push_back(paramDecl);
  }
  void set_body(CompoundStmt *stmt) { body = stmt; }

  [[nodiscard]] const std::vector<ParameterDeclaration *> &
  getParamDecls() const {
    return parameters;
  }
  [[nodiscard]] CompoundStmt *get_body() const { return body; };
  [[nodiscard]] std::string_view get_return_type() const { return ret_type; }
};

class TranslationUnit : public ASTNode {
private:
  std::vector<Declaration *> declarations;

public:
  explicit TranslationUnit(SourceLocation loc = {}) : ASTNode(loc) {}

  inline void add_declaration(Declaration *declaration) {
    declarations.push_back(declaration);
  }

  [[nodiscard]] const std::vector<Declaration *> &get_declarations() const {
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
  virtual void visit(AssertStmt &stmt) {}

  virtual void visit(Expression &expr) {}
  virtual void visit(NumericExpr &expr) {}
  virtual void visit(CallExpr &expr) {}
  virtual void visit(StringLiteralExpr &epxr) {}

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

  [[nodiscard]] std::string indent() const {
    if (depth == 0) {
      return "";
    }
    std::string result;
    for (int i = 0; i < depth - 1; i++) {
      result += "| ";
    }
    result += "|-";
    return result;
  }

  static std::string formatLocation(const SourceLocation &loc) {
    if (loc.file_name.empty()) {
      return "<invalid sloc>";
    }
    return std::format("<{}>", loc.file_name);
  }

  static std::string formatRange(const SourceLocation &loc) {
    if (loc.file_name.empty()) {
      return "<invalid sloc>";
    }
    return std::format("<line:{}, col:{}>", loc.line, loc.column);
  }

public:
  explicit ClangStylePrintVisitor(bool colored = true) : use_colors(colored) {}

  [[nodiscard]] std::string_view get_content() const { return content; }

  void visit(TranslationUnit &unit) override {
    content +=
        color("TranslationUnitDecl", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(unit))),
              YELLOW) +
        " " + formatLocation(unit.get_location()) + "\n";

    depth++;
    for (const auto &decl : unit.get_declarations()) {
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
        " " + formatRange(decl.get_location()) + " " +
        color(std::format("line:{}", decl.get_location().line), CYAN) + " " +
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
    if (decl.get_body() != nullptr) {
      content += indent();
      decl.get_body()->accept(*this);
    }
    depth--;
  }

  void visit(ParameterDeclaration &decl) override {
    content +=
        color("ParmVarDecl", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(decl))),
              YELLOW) +
        " " + formatRange(decl.get_location()) + " " +
        color(std::format("col:{}", decl.get_location().column), CYAN) + " " +
        color(std::string(decl.get_name()), MAGENTA) + " " + "'" +
        std::string(decl.get_type()) + "'\n";
  }

  void visit(CompoundStmt &stmt) override {
    content +=
        color("CompoundStmt", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(stmt))),
              YELLOW) +
        " " + formatRange(stmt.get_location()) + "\n";

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
        " " + formatRange(stmt.get_location()) + "\n";

    if (stmt.get_expression() != nullptr) {
      depth++;
      content += indent();
      stmt.get_expression()->accept(*this);
      depth--;
    }
  }

  void visit(AssertStmt &stmt) override {
    content +=
        color("AssertStmt", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(stmt))),
              YELLOW) +
        " " + formatRange(stmt.get_location()) + "\n";

    if (stmt.get_expression() != nullptr) {
      depth++;
      content += indent();
      stmt.get_expression()->accept(*this);
      depth--;
    }
  }

  void visit(CallExpr &expr) override {
    content +=
        color("CallExpr", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(expr))),
              YELLOW) +
        " " + formatRange(expr.get_location()) + " " + "'int' " +
        std::string(expr.get_function_name()) + "\n";

    // Visit param expressions
    depth++;
    for (const auto &param : expr.get_params()) {
      content += indent();
      param->accept(*this);
    }
    depth--;
  }

  void visit(NumericExpr &expr) override {
    content +=
        color("IntegerLiteralExpr", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(expr))),
              YELLOW) +
        " " + formatRange(expr.get_location()) + " " + "'int' " +
        std::string(expr.get_value()) + "\n";
  }

  void visit(StringLiteralExpr &expr) override {
    content +=
        color("StringLiteralExpr", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(expr))),
              YELLOW) +
        " " + formatRange(expr.get_location()) + " " + "'string' " +
        std::string(expr.get_value()) + "\n";
  }
};
#endif // !DEFS_AST_H
