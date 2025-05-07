#ifndef DEFS_AST_H
#define DEFS_AST_H

#include "token.hpp"
#include <format>
#include <string>
#include <string_view>
#include <vector>

enum class BinaryOperator {
  Add,
  Sub,
  Mult,
  Div,
  Modulo,
  Equal,
  NotEqual,
  LessThan,
  LessThanOrEqual,
  GreaterThan,
  GreaterThanOrEqual,
  LogicalAnd,
  LogicalOr,
  BitwiseAnd,
  BitwiseOr,
  BitwiseXor,
  ShiftLeft,
  ShiftRight,
  FieldAccess,
  PointerAccess,
  Unknown
};

enum class UnaryOperator { LogicalNot, BitwiseNot, Neg, Deref, Unknown };

enum class AssignmentOperator {
  Plus,
  Minus,
  Mult,
  Div,
  Modulo,
  LShift,
  RShift,
  BitwiseAnd,
  BitwiseXor,
  BitwiseOr,
  Equals,
  Unknown
};

enum class Builtin { Int, Bool, String, Char, Void, Unknown };

std::string binOp2String(BinaryOperator binOp);
std::string unOp2String(UnaryOperator unOp);
std::string assmtOp2String(AssignmentOperator assmtOp);
std::string builtin2String(Builtin type);
BinaryOperator binOpFromToken(token::TokenKind token);
UnaryOperator unOpFromToken(token::TokenKind token);
AssignmentOperator assmtOpFromToken(token::TokenKind token);
Builtin builtinFromToken(token::TokenKind token);

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

// ==== Types ====
class Type : public ASTNode {
public:
  explicit Type(SourceLocation loc = {}) : ASTNode(loc) {}
  virtual ~Type() = default;
  virtual std::string toString() const = 0;
  void accept(class ASTVisitor &visitor) override = 0;
};

class BuiltinType : public Type {
private:
  Builtin type;

public:
  explicit BuiltinType(Builtin type, SourceLocation loc = {})
      : Type(loc), type(type) {}
  [[nodiscard]] Builtin get_type() const { return type; };
  void accept(class ASTVisitor &visitor) override;
  [[nodiscard]] std::string toString() const override {
    return builtin2String(type);
  }
};

class NamedType : public Type {
private:
  std::string_view name; // typedef name
public:
  explicit NamedType(std::string_view name, SourceLocation loc = {})
      : Type(loc), name(name) {}
  [[nodiscard]] std::string_view get_name() const { return name; }
  [[nodiscard]] std::string toString() const override {
    return std::string(name);
  }
  void accept(class ASTVisitor &visitor) override;
};

class StructType : public Type {
private:
  std::string_view name;

public:
  explicit StructType(std::string_view name, SourceLocation loc = {})
      : Type(loc), name(name) {}
  [[nodiscard]] std::string_view get_name() const { return name; }
  [[nodiscard]] std::string toString() const override {
    return std::format("struct {}", name);
  }
  void accept(class ASTVisitor &visitor) override;
};

class PointerType : public Type {
private:
  Type *type;

public:
  explicit PointerType(Type *type, SourceLocation loc = {})
      : Type(loc), type(type) {}
  [[nodiscard]] Type *get_type() const { return type; }
  [[nodiscard]] std::string toString() const override {
    return std::format("Pointer to <{}>", type->toString());
  }
  void accept(class ASTVisitor &visitor) override;
};

class ArrayType : public Type {
private:
  Type *type;

public:
  explicit ArrayType(Type *type, SourceLocation loc = {})
      : Type(loc), type(type) {}
  [[nodiscard]] Type *get_type() const { return type; }
  [[nodiscard]] std::string toString() const override {
    return std::format("Array of <{}>", type->toString());
  }
  void accept(class ASTVisitor &visitor) override;
};

// ==== Expressions ====
// TODO: there are still a lot more expressions
class Expression : public ASTNode {
public:
  enum class Kind {
    Numeric,
    String,
    Call,
    Char,
    BoolConst,
    Null,
    Var,
    BinOp,
    UnOp,
    Paren,
    ArrayAccess
  };

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

class ArrayAccessExpr : public Expression {
private:
  Expression *array;
  Expression *index;

public:
  ArrayAccessExpr(Expression *array, Expression *index, SourceLocation loc = {})
      : Expression(Expression::Kind::ArrayAccess, "ArrayAccessExpr", loc),
        array(array), index(index) {}
  [[nodiscard]] Expression *get_array() const { return array; };
  [[nodiscard]] Expression *get_index() const { return index; };
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

class ParenthesisExpression : public Expression {
private:
  Expression *expression;

public:
  explicit ParenthesisExpression(Expression *expr, SourceLocation loc = {})
      : Expression(Expression::Kind::Paren, "ParenExpr", loc),
        expression(expr) {}
  [[nodiscard]] Expression *get_expression() const { return expression; }
  void accept(class ASTVisitor &visitor) override;
};

class BinaryOperatorExpression : public Expression {
private:
  Expression *leftExpression;
  Expression *rightExpression;
  BinaryOperator binaryOperator;

public:
  explicit BinaryOperatorExpression(Expression *leftExpr, Expression *rightExpr,
                                    BinaryOperator binOp,
                                    SourceLocation loc = {})
      : Expression(Expression::Kind::BinOp, "BinaryOperator", loc),
        leftExpression(leftExpr), rightExpression(rightExpr),
        binaryOperator(binOp) {}
  [[nodiscard]] BinaryOperator get_operator_kind() const {
    return binaryOperator;
  }
  [[nodiscard]] Expression *get_left_expression() const {
    return leftExpression;
  }
  [[nodiscard]] Expression *get_right_expression() const {
    return rightExpression;
  }
  void accept(class ASTVisitor &visitor) override;
};

class UnaryOperatorExpression : public Expression {
private:
  Expression *expression;
  UnaryOperator unaryOperator;

public:
  explicit UnaryOperatorExpression(Expression *expr, UnaryOperator unOperator,
                                   SourceLocation loc = {})
      : Expression(Expression::Kind::UnOp, "UnaryOperator", loc),
        expression(expr), unaryOperator(unOperator) {}
  [[nodiscard]] UnaryOperator get_operator_kind() const {
    return unaryOperator;
  }
  [[nodiscard]] Expression *get_expression() const { return expression; }
  void accept(class ASTVisitor &visitor) override;
};

class VarExpr : public Expression {
private:
  std::string_view variable_name;

public:
  explicit VarExpr(std::string_view name, SourceLocation loc = {})
      : Expression(Expression::Kind::Var, "VarExpr", loc), variable_name(name) {
  }
  [[nodiscard]] std::string_view get_variable_name() const {
    return variable_name;
  }
  void accept(class ASTVisitor &visitor) override;
};

class NullExpr : public Expression {
public:
  explicit NullExpr(SourceLocation loc = {})
      : Expression(Expression::Kind::Null, "NullExpr", loc) {}
  [[nodiscard]] std::string_view get_value() const { return "NULL"; }
  void accept(class ASTVisitor &visitor) override;
};

class CharLiteralExpr : public Expression {
private:
  std::string_view value;

public:
  explicit CharLiteralExpr(std::string_view value, SourceLocation loc = {})
      : Expression(Expression::Kind::Char, "CharLiteral", loc), value(value) {}
  [[nodiscard]] std::string_view get_value() const { return value; }
  void accept(class ASTVisitor &visitor) override;
};

class BoolConstExpr : public Expression {
private:
  bool value;

public:
  explicit BoolConstExpr(std::string_view value, SourceLocation loc = {})
      : Expression(Expression::Kind::BoolConst, "BoolConst", loc) {
    if (value.compare("true")) {
      this->value = true;
    } else if (value.compare("false")) {
      this->value = false;
    } else {
      throw std::runtime_error("Bool Const with unknown value.");
    }
  }
  [[nodiscard]] bool get_value() const { return value; }
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

// ==== LValues ====
class LValue : public ASTNode {
public:
  explicit LValue(SourceLocation loc = {}) : ASTNode(loc) {}
  virtual ~LValue() = default;
  void accept(class ASTVisitor &visitor) override = 0;
};

class VariableLValue : public LValue {
private:
  std::string_view name;

public:
  explicit VariableLValue(std::string_view name, SourceLocation loc = {})
      : LValue(loc), name(name) {}

  [[nodiscard]] std::string_view get_name() const { return name; }
  void accept(class ASTVisitor &visitor) override;
};

class DereferenceLValue : public LValue {
private:
  LValue *operand;

public:
  explicit DereferenceLValue(LValue *operand, SourceLocation loc = {})
      : LValue(loc), operand(operand) {}

  [[nodiscard]] LValue *get_operand() const { return operand; }
  void accept(class ASTVisitor &visitor) override;
};

class FieldAccessLValue : public LValue {
private:
  LValue *base;
  std::string_view field;

public:
  FieldAccessLValue(LValue *base, std::string_view field,
                    SourceLocation loc = {})
      : LValue(loc), base(base), field(field) {}

  [[nodiscard]] LValue *get_base() const { return base; }
  [[nodiscard]] std::string_view get_field() const { return field; }
  void accept(class ASTVisitor &visitor) override;
};

class PointerAccessLValue : public LValue {
private:
  LValue *base;
  std::string_view field;

public:
  PointerAccessLValue(LValue *base, std::string_view field,
                      SourceLocation loc = {})
      : LValue(loc), base(base), field(field) {}

  [[nodiscard]] LValue *get_base() const { return base; }
  [[nodiscard]] std::string_view get_field() const { return field; }
  void accept(class ASTVisitor &visitor) override;
};

class ArrayAccessLValue : public LValue {
private:
  LValue *base;
  Expression *index;

public:
  ArrayAccessLValue(LValue *base, Expression *index, SourceLocation loc = {})
      : LValue(loc), base(base), index(index) {}

  [[nodiscard]] LValue *get_base() const { return base; }
  [[nodiscard]] Expression *get_index() const { return index; }
  void accept(class ASTVisitor &visitor) override;
};

// ==== Statements ====
// TODO: add all possible statements
class Statement : public ASTNode {
public:
  explicit Statement(SourceLocation loc = {}) : ASTNode(loc) {}
  virtual ~Statement() = default;
  virtual void accept(class ASTVisitor &visitor) override = 0;
};

class ErrorStatement : public Statement {
private:
  Expression *expr; // numerical

public:
  explicit ErrorStatement(Expression *expr, SourceLocation loc = {})
      : Statement(loc), expr(expr) {}

  [[nodiscard]] Expression *get_expr() const { return expr; }
  void accept(ASTVisitor &visitor) override;
};

class WhileStatement : public Statement {
private:
  Expression *condition;
  Statement *body;

public:
  WhileStatement(Expression *cond, Statement *body, SourceLocation loc = {})
      : Statement(loc), condition(cond), body(body) {}

  [[nodiscard]] Expression *get_condition() const { return condition; }
  [[nodiscard]] Statement *get_body() const { return body; }

  void accept(ASTVisitor &visitor) override;
};

class IfStatement : public Statement {
private:
  Expression *condition;
  Statement *then_branch;
  Statement *else_branch; // nullptr if no else

public:
  IfStatement(Expression *cond, Statement *then_stmt,
              Statement *else_stmt = nullptr, SourceLocation loc = {})
      : Statement(loc), condition(cond), then_branch(then_stmt),
        else_branch(else_stmt) {}

  [[nodiscard]] Expression *get_condition() const { return condition; }
  [[nodiscard]] Statement *get_then_branch() const { return then_branch; }
  [[nodiscard]] Statement *get_else_branch() const { return else_branch; }

  void accept(ASTVisitor &visitor) override;
};

class ForStatement : public Statement {
private:
  Statement *init; // nullptr if empty
  Expression *condition;
  Statement *increment; // nullptr if empty
  Statement *body;

public:
  ForStatement(Statement *init, Expression *cond, Statement *incr,
               Statement *body, SourceLocation loc = {})
      : Statement(loc), init(init), condition(cond), increment(incr),
        body(body) {}

  [[nodiscard]] Statement *get_init() const { return init; }
  [[nodiscard]] Expression *get_condition() const { return condition; }
  [[nodiscard]] Statement *get_increment() const { return increment; }
  [[nodiscard]] Statement *get_body() const { return body; }

  void accept(ASTVisitor &visitor) override;
};

class AssignmentStatement : public Statement {
private:
  LValue *lValue;
  AssignmentOperator op;
  Expression *expr;

public:
  AssignmentStatement(LValue *lValue, AssignmentOperator op, Expression *expr,
                      SourceLocation loc = {})
      : Statement(loc), lValue(lValue), op(op), expr(expr) {}

  [[nodiscard]] LValue *get_lvalue() const { return lValue; }
  [[nodiscard]] AssignmentOperator get_op() const { return op; }
  [[nodiscard]] Expression *get_expr() const { return expr; }

  void accept(ASTVisitor &visitor) override;
};

class UnaryMutationStatement : public Statement {
public:
  enum class Op { PostIncrement, PostDecrement };

private:
  LValue *target;
  Op operation;

public:
  UnaryMutationStatement(LValue *target, Op op, SourceLocation loc = {})
      : Statement(loc), target(target), operation(op) {}

  [[nodiscard]] LValue *get_target() const { return target; }
  [[nodiscard]] Op get_operation() const { return operation; }

  void accept(ASTVisitor &visitor) override;
};

class ExpressionStatement : public Statement {
private:
  Expression *expr;

public:
  explicit ExpressionStatement(Expression *expr, SourceLocation loc = {})
      : Statement(loc), expr(expr) {}

  [[nodiscard]] Expression *get_expression() const { return expr; }

  void accept(ASTVisitor &visitor) override;
};

class VariableDeclarationStatement : public Statement {
private:
  Type *type;
  std::string_view identifier;
  Expression *initializer;

public:
  VariableDeclarationStatement(Type *type, std::string_view identifier,
                               Expression *init = nullptr,
                               SourceLocation loc = {})
      : Statement(loc), type(type), identifier(identifier), initializer(init) {}

  [[nodiscard]] Type *get_type() const { return type; }
  [[nodiscard]] std::string_view get_identifier() const { return identifier; }
  [[nodiscard]] Expression *get_initializer() const { return initializer; }

  void accept(ASTVisitor &visitor) override;
};

class AssertStmt : public Statement {
private:
  Expression *expression{};

public:
  explicit AssertStmt(SourceLocation loc = {}) : Statement(loc) {}

  void set_expression(Expression *expr) { this->expression = expr; }
  [[nodiscard]] Expression *get_expression() const { return this->expression; }
  void accept(class ASTVisitor &visitor) override;
};

class CompoundStmt : public Statement {
private:
  std::vector<Statement *> statements;

public:
  explicit CompoundStmt(std::vector<Statement *> stmts, SourceLocation loc = {})
      : Statement(loc), statements(std::move(stmts)) {}
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
  explicit ReturnStmt(SourceLocation loc = {}) : Statement(loc) {}
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
  Type *type;

public:
  ParameterDeclaration(std::string_view name, Type *type,
                       SourceLocation loc = {})
      : Declaration(Kind::Parameter, name, loc), name(name), type(type) {}
  [[nodiscard]] Type *get_type() const { return type; }
  void accept(ASTVisitor &visitor) override;
};

class FunctionDeclaration : public Declaration {
private:
  std::string_view name;
  Type *ret_type;
  std::vector<ParameterDeclaration *> parameters;
  CompoundStmt *body{};

public:
  FunctionDeclaration(std::string_view name, Type *ret_type,
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
  [[nodiscard]] Type *get_return_type() const { return ret_type; }
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
  virtual void visit(VariableDeclarationStatement &stmt) {}
  virtual void visit(UnaryMutationStatement &stmt) {}
  virtual void visit(AssignmentStatement &stmt) {}
  virtual void visit(ExpressionStatement &stmt) {}
  virtual void visit(IfStatement &stmt) {}
  virtual void visit(ForStatement &stmt) {}
  virtual void visit(WhileStatement &stmt) {}
  virtual void visit(ErrorStatement &stmt) {}

  virtual void visit(Expression &expr) {}
  virtual void visit(NumericExpr &expr) {}
  virtual void visit(CallExpr &expr) {}
  virtual void visit(StringLiteralExpr &expr) {}
  virtual void visit(CharLiteralExpr &expr) {}
  virtual void visit(BoolConstExpr &expr) {}
  virtual void visit(NullExpr &expr) {}
  virtual void visit(VarExpr &expr) {}
  virtual void visit(ParenthesisExpression &expr) {}
  virtual void visit(BinaryOperatorExpression &expr) {}
  virtual void visit(UnaryOperatorExpression &expr) {}
  virtual void visit(ArrayAccessExpr &expr) {}

  virtual void visit(Type &type) {}
  virtual void visit(BuiltinType &type) {}
  virtual void visit(NamedType &type) {}
  virtual void visit(StructType &type) {}
  virtual void visit(PointerType &type) {}
  virtual void visit(ArrayType &type) {}

  virtual void visit(LValue &val) {}
  virtual void visit(VariableLValue &val) {}
  virtual void visit(ArrayAccessLValue &val) {}
  virtual void visit(PointerAccessLValue &val) {}
  virtual void visit(FieldAccessLValue &val) {}
  virtual void visit(DereferenceLValue &val) {}

  virtual void visit(TranslationUnit &unit) {}
};

class ClangStylePrintVisitor : public ASTVisitor {
private:
  std::string content;
  int depth = 0;
  bool use_colors = true;

  // ANSI color codes for terminal output
  const std::string RESET = "\033[0m";
  const std::string GREEN = "\u001b[32m";
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
        decl.get_return_type()->toString() + " (";

    // Parameter types list
    bool first = true;
    for (const auto &param : decl.getParamDecls()) {
      if (!first)
        content += ", ";
      content += param->get_type()->toString();
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
        decl.get_type()->toString() + "'\n";
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

  void visit(ErrorStatement &stmt) override {
    content +=
        color("ErrorStatement", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(stmt))),
              YELLOW) +
        " " + formatRange(stmt.get_location()) + "\n";

    depth++;
    content += indent();
    stmt.get_expr()->accept(*this);
    depth--;
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

  void visit(IfStatement &stmt) override {
    content +=
        color("IfStatement", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(stmt))),
              YELLOW) +
        " " + formatRange(stmt.get_location()) + "\n";

    depth++;
    content += indent();
    stmt.get_condition()->accept(*this);
    content += indent();
    stmt.get_then_branch()->accept(*this);
    if (stmt.get_else_branch() != nullptr) {
      content += indent();
      stmt.get_else_branch()->accept(*this);
    }
    depth--;
  }

  void visit(ForStatement &stmt) override {
    content +=
        color("ForStatement", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(stmt))),
              YELLOW) +
        " " + formatRange(stmt.get_location()) + "\n";

    depth++;
    if (stmt.get_init() != nullptr) {
      content += indent();
      stmt.get_init()->accept(*this);
    }
    content += indent();
    stmt.get_condition()->accept(*this);
    if (stmt.get_increment() != nullptr) {
      content += indent();
      stmt.get_increment()->accept(*this);
    }
    content += indent();
    stmt.get_body()->accept(*this);
    depth--;
  }

  void visit(WhileStatement &stmt) override {
    content +=
        color("WhileStatement", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(stmt))),
              YELLOW) +
        " " + formatRange(stmt.get_location()) + "\n";

    depth++;
    content += indent();
    stmt.get_condition()->accept(*this);
    content += indent();
    stmt.get_body()->accept(*this);
    depth--;
  }

  void visit(VariableDeclarationStatement &stmt) override {
    content +=
        color("VarDeclStatement", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(stmt))),
              YELLOW) +
        " " + formatRange(stmt.get_location()) + " " +
        color(std::string(stmt.get_identifier()), MAGENTA) + " " +
        stmt.get_type()->toString() + "\n";

    if (stmt.get_initializer() != nullptr) {
      depth++;
      content += indent();
      stmt.get_initializer()->accept(*this);
      depth--;
    }
  }

  void visit(AssignmentStatement &stmt) override {
    content +=
        color("AssignmentStatement", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(stmt))),
              YELLOW) +
        " " + formatRange(stmt.get_location()) + " " +
        assmtOp2String(stmt.get_op()) + "\n";

    depth++;
    content += indent();
    stmt.get_lvalue()->accept(*this);
    content += indent();
    stmt.get_expr()->accept(*this);
    depth--;
  }

  void visit(UnaryMutationStatement &stmt) override {
    content +=
        color("UnaryMutStatement", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(stmt))),
              YELLOW) +
        " " + formatRange(stmt.get_location()) + " " +
        (stmt.get_operation() == UnaryMutationStatement::Op::PostIncrement
             ? "PostIncrement"
             : "PostDecrement") +
        "\n";

    depth++;
    content += indent();
    stmt.get_target()->accept(*this);
    depth--;
  }

  void visit(ExpressionStatement &stmt) override {
    content +=
        color("ExprStatement", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(stmt))),
              YELLOW) +
        " " + formatRange(stmt.get_location()) + "\n";

    depth++;
    content += indent();
    stmt.get_expression()->accept(*this);
    depth--;
  }

  void visit(CallExpr &expr) override {
    content +=
        color("CallExpr", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(expr))),
              YELLOW) +
        " " + formatRange(expr.get_location()) + " " +
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

  void visit(CharLiteralExpr &expr) override {
    content +=
        color("CharLiteralExpr", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(expr))),
              YELLOW) +
        " " + formatRange(expr.get_location()) + " " + "'char' " +
        std::string(expr.get_value()) + "\n";
  }

  void visit(BoolConstExpr &expr) override {
    content +=
        color("BoolConstExpr", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(expr))),
              YELLOW) +
        " " + formatRange(expr.get_location()) + " " + "'bool' " +
        (expr.get_value() ? "true" : "false") + "\n";
  }

  void visit(NullExpr &expr) override {
    content +=
        color("NullExpr", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(expr))),
              YELLOW) +
        " " + formatRange(expr.get_location()) + " 'void' NULL \n";
  }

  void visit(ParenthesisExpression &expr) override {
    content +=
        color("ParenExpr", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(expr))),
              YELLOW) +
        " " + formatRange(expr.get_location()) + "\n";

    depth++;
    content += indent();
    expr.get_expression()->accept(*this);
    depth--;
  }

  void visit(VarExpr &expr) override {
    content +=
        color("VarExpr", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(expr))),
              YELLOW) +
        " " + formatRange(expr.get_location()) + " " +
        std::string(expr.get_variable_name()) + "\n";
  }

  void visit(UnaryOperatorExpression &expr) override {
    content +=
        color("UnaryOperator", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(expr))),
              YELLOW) +
        " " + formatRange(expr.get_location()) + " " +
        unOp2String(expr.get_operator_kind()) + "\n";

    depth++;
    content += indent();
    expr.get_expression()->accept(*this);
    depth--;
  }

  void visit(BinaryOperatorExpression &expr) override {
    content +=
        color("BinaryOperator", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(expr))),
              YELLOW) +
        " " + formatRange(expr.get_location()) + " " +
        binOp2String(expr.get_operator_kind()) + "\n";

    depth++;
    content += indent();
    expr.get_left_expression()->accept(*this);
    content += indent();
    expr.get_right_expression()->accept(*this);
    depth--;
  }

  void visit(ArrayAccessExpr &expr) override {
    content +=
        color("ArrayAccess", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(expr))),
              YELLOW) +
        " " + formatRange(expr.get_location()) + "\n";

    depth++;
    content += indent();
    expr.get_array()->accept(*this);
    content += indent();
    expr.get_index()->accept(*this);
    depth--;
  }

  void visit(BuiltinType &type) override {
    content +=
        color("BuiltinType", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(type))),
              YELLOW) +
        " " + formatRange(type.get_location()) + " " +
        builtin2String(type.get_type()) + "\n";
  }

  void visit(NamedType &type) override {
    content +=
        color("NamedType", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(type))),
              YELLOW) +
        " " + formatRange(type.get_location()) + " " +
        std::string(type.get_name()) + "\n";
  }

  void visit(StructType &type) override {
    content +=
        color("StructType", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(type))),
              YELLOW) +
        " " + formatRange(type.get_location()) + " Struct " +
        std::string(type.get_name()) + "\n";
  }

  void visit(PointerType &type) override {
    content +=
        color("PointerType", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(type))),
              YELLOW) +
        " " + formatRange(type.get_location()) + "\n";

    depth++;
    content += indent();
    type.get_type()->accept(*this);
    depth--;
  }

  void visit(ArrayType &type) override {
    content +=
        color("ArrayType", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(type))),
              YELLOW) +
        " " + formatRange(type.get_location()) + "\n";

    depth++;
    content += indent();
    type.get_type()->accept(*this);
    depth--;
  }

  void visit(VariableLValue &val) override {
    content +=
        color("VariableLValue", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(val))),
              YELLOW) +
        " " + formatRange(val.get_location()) + " " +
        std::string(val.get_name()) + "\n";
  }

  void visit(DereferenceLValue &val) override {
    content +=
        color("DereferenceLValue", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(val))),
              YELLOW) +
        " " + formatRange(val.get_location()) + "\n";
    depth++;
    content += indent();
    val.get_operand()->accept(*this);
    depth--;
  }

  void visit(FieldAccessLValue &val) override {
    content +=
        color("FieldAccessLValue", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(val))),
              YELLOW) +
        " " + formatRange(val.get_location()) + " " +
        std::string(val.get_field()) + "\n";
    depth++;
    content += indent();
    val.get_base()->accept(*this);
    depth--;
  }

  void visit(PointerAccessLValue &val) override {
    content +=
        color("PointerAccessLValue", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(val))),
              YELLOW) +
        " " + formatRange(val.get_location()) + " " +
        std::string(val.get_field()) + "\n";
    depth++;
    content += indent();
    val.get_base()->accept(*this);
    depth--;
  }

  void visit(ArrayAccessLValue &val) override {
    content +=
        color("ArrayAccessLValue", GREEN) + " " +
        color(std::format("{:#x}",
                          reinterpret_cast<std::size_t>(std::addressof(val))),
              YELLOW) +
        " " + formatRange(val.get_location()) + "\n";
    depth++;
    content += indent();
    val.get_base()->accept(*this);
    content += indent();
    val.get_index()->accept(*this);
    depth--;
  }
};
#endif // !DEFS_AST_H
