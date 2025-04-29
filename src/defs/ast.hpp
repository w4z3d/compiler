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
  std::vector<Statement*> statements;
public:
  CompoundStmt(std::vector<Statement*> stmts, SourceLocation loc = {})
      : Statement(Statement::Kind::Compound, "Compound", loc), statements(std::move(stmts)) {}
  const std::vector<Statement*> &getStatements() const { return statements; }
  void accept(class ASTVisitor &visitor) override;
};

class ReturnStmt : public Statement {
private:
  Expression* expr;
public:
  ReturnStmt(SourceLocation loc = {})
      : Statement(Statement::Kind::Return, "Return", loc) {}
  inline void setExpression(Expression *expression) {
    this->expr = expression;
  }
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
  std::vector<ParameterDeclaration*> parameters;
  CompoundStmt* body = nullptr;

public:
  FunctionDeclaration(std::string_view name, std::string_view ret_type,
                      SourceLocation loc = {})
      : Declaration(Kind::Function, std::move(name), std::move(loc)),
        name(name), ret_type(ret_type) {}
  void accept(ASTVisitor &visitor) override;

  inline void addParameterDeclaration(ParameterDeclaration *paramDecl) {
      parameters.push_back(paramDecl);
  }
  inline void setBody(CompoundStmt *stmt) {
    body = stmt;
  }
  const std::vector<ParameterDeclaration*> &getParamDecls() const { return parameters; }
  CompoundStmt* getBody() const {return body;};
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

class PrintVisitor : public ASTVisitor {
private:
  std::string content;
  int offset = 0;

public:
  PrintVisitor() : content("") {}

  const std::string_view get_content() const { return content; }

  void addOffset() {
    for (int i = 0; i < offset; i++) {
      content += " ";
    }
  }

  void visit(NumericExpression &expr) {
    content += std::format("─NumericExpression {:#12x} File: {} "
                           "Value: {} Loc: {}:{}\n",
                           reinterpret_cast<std::size_t>(std::addressof(expr)),
                           expr.getLocation().file_name, expr.get_value(), expr.getLocation().line,
                           expr.getLocation().column);
  }

  void visit(CompoundStmt &stmt) {
    content += std::format("─CompoundStatement {:#12x} File: {} Name: {} "
                           "Loc: {}:{}\n",
                           reinterpret_cast<std::size_t>(std::addressof(stmt)),
                           stmt.getLocation().file_name, stmt.get_name(),
                           stmt.getLocation().line, stmt.getLocation().column);
    offset++;
    for (const auto &statement : stmt.getStatements()) {
      addOffset();
      content += "├";
      statement->accept(*this);
    }
    offset--;
  }
  void visit(ReturnStmt &stmt) {
    content += std::format("┬ReturnStatement {:#12x} File: {} Name: {} "
                           "Loc: {}:{}\n",
                           reinterpret_cast<std::size_t>(std::addressof(stmt)),
                           stmt.getLocation().file_name, stmt.get_name(),
                           stmt.getLocation().line, stmt.getLocation().column);
    offset++;
    addOffset();
    content += "├";
    (stmt.getExpression())->accept(*this);
    offset--;
  }

  void visit(FunctionDeclaration &decl) {
    content += std::format("─FunctionDeclaration {:#12x} File: {} Name: {} "
                           "Return type: {} Loc: {}:{}\n",
                           reinterpret_cast<std::size_t>(std::addressof(decl)),
                           decl.getLocation().file_name, decl.get_name(),
                           decl.get_return_type(), decl.getLocation().line,
                           decl.getLocation().column);
    offset++;
    for (const auto &param : decl.getParamDecls()) {
      addOffset();
      content += "├";
      param->accept(*this);
    }
    if (decl.getBody() != nullptr) {
      offset++;
      addOffset();
      content += "├";
      decl.getBody()->accept(*this);
      offset--;
    }
    offset--;
  }
  void visit(ParameterDeclaration &decl) {
    content += std::format("─ParamDeclaration {:#12x} File: {} Name: {} "
                           "Type: {} Loc: {}:{}\n",
                           reinterpret_cast<std::size_t>(std::addressof(decl)),
                           decl.getLocation().file_name, decl.get_name(),
                           decl.get_type(), decl.getLocation().line,
                           decl.getLocation().column);
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
