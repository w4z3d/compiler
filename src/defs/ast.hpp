#ifndef DEFS_AST_H
#define DEFS_AST_H

#include "../analysis/symbol.hpp"
#include "source_location.hpp"
#include "token.hpp"
#include <charconv>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
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
BinaryOperator binOpFromToken(token::TokenKind token);
UnaryOperator unOpFromToken(token::TokenKind token);
AssignmentOperator assmtOpFromToken(token::TokenKind token);
int precedenceFromBinOp(BinaryOperator binOp);
int precedenceFromUnOp(UnaryOperator unOp);
int precedenceFromAssmtOp(AssignmentOperator assmtOp);

std::string builtin2String(Builtin type);
Builtin builtinFromToken(token::TokenKind token);

class ASTNode {
protected:
  SourceLocation location;

public:
  explicit ASTNode(SourceLocation loc = {}) : location(std::move(loc)) {}
  virtual ~ASTNode() = default;

  [[nodiscard]] const SourceLocation &get_location() const { return location; }
  void set_source_location(std::string_view file, std::tuple<int, int> begin,
                           std::tuple<int, int> end) {
    this->location = SourceLocation{file, begin, end};
  }

  virtual void accept(class ASTVisitor &visitor) = 0;
};

// ==== Types ====
class TypeAnnotation : public ASTNode {
public:
  explicit TypeAnnotation(SourceLocation loc = {}) : ASTNode(std::move(loc)) {}
  [[nodiscard]] virtual std::string toString() const = 0;
  void accept(class ASTVisitor &visitor) override = 0;
};

class BuiltinTypeAnnotation : public TypeAnnotation {
private:
  Builtin type;

public:
  explicit BuiltinTypeAnnotation(Builtin type, SourceLocation loc = {})
      : TypeAnnotation(std::move(loc)), type(type) {}
  [[nodiscard]] Builtin get_type() const { return type; };
  void accept(class ASTVisitor &visitor) override;
  [[nodiscard]] std::string toString() const override {
    return builtin2String(type);
  }
};

class NamedTypeAnnotation : public TypeAnnotation {
private:
  std::string_view name; // typedef name
public:
  explicit NamedTypeAnnotation(std::string_view name, SourceLocation loc = {})
      : TypeAnnotation(std::move(loc)), name(name) {}
  [[nodiscard]] std::string_view get_name() const { return name; }
  [[nodiscard]] std::string toString() const override {
    return std::string(name);
  }
  void accept(class ASTVisitor &visitor) override;
};

class StructTypeAnnotation : public TypeAnnotation {
private:
  std::string_view name;

public:
  explicit StructTypeAnnotation(std::string_view name, SourceLocation loc = {})
      : TypeAnnotation(std::move(loc)), name(name) {}
  [[nodiscard]] std::string_view get_name() const { return name; }
  [[nodiscard]] std::string toString() const override {
    return std::format("struct {}", name);
  }
  void accept(class ASTVisitor &visitor) override;
};

class PointerTypeAnnotation : public TypeAnnotation {
private:
  TypeAnnotation *type;

public:
  explicit PointerTypeAnnotation(TypeAnnotation *type, SourceLocation loc = {})
      : TypeAnnotation(std::move(loc)), type(type) {}
  [[nodiscard]] TypeAnnotation *get_type() const { return type; }
  [[nodiscard]] std::string toString() const override {
    return std::format("Pointer to <{}>", type->toString());
  }
  void accept(class ASTVisitor &visitor) override;
};

class ArrayTypeAnnotation : public TypeAnnotation {
private:
  TypeAnnotation *type;

public:
  explicit ArrayTypeAnnotation(TypeAnnotation *type, SourceLocation loc = {})
      : TypeAnnotation(std::move(loc)), type(type) {}
  [[nodiscard]] TypeAnnotation *get_type() const { return type; }
  [[nodiscard]] std::string toString() const override {
    return std::format("Array of <{}>", type->toString());
  }
  void accept(class ASTVisitor &visitor) override;
};

// ==== Expressions ====
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
    Ternary,
    BinOp,
    UnOp,
    Paren,
    ArrayAccess,
    FieldAccess,
    PointerAccess,
    Alloc,
    Alloc_array
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

class AllocExpression : public Expression {
private:
  TypeAnnotation *type;

public:
  explicit AllocExpression(TypeAnnotation *type, SourceLocation loc = {})
      : Expression(Expression::Kind::Alloc, "AllocExpr", loc), type(type) {}
  [[nodiscard]] TypeAnnotation *get_type() const { return type; };
  void accept(class ASTVisitor &visitor) override;
};

class AllocArrayExpression : public Expression {
private:
  TypeAnnotation *type;
  Expression *size;

public:
  AllocArrayExpression(TypeAnnotation *type, Expression *size,
                       SourceLocation loc = {})
      : Expression(Expression::Kind::Alloc_array, "AllocArrayExpr", loc),
        type(type), size(size) {}
  [[nodiscard]] TypeAnnotation *get_type() const { return type; };
  [[nodiscard]] Expression *get_size() const { return size; };
  void accept(class ASTVisitor &visitor) override;
};

class PointerAccessExpr : public Expression {
private:
  Expression *struct_pointer;
  std::string_view field;

public:
  PointerAccessExpr(Expression *struct_pointer, std::string_view field,
                    SourceLocation loc = {})
      : Expression(Expression::Kind::PointerAccess, "PointerAccessExpr", loc),
        struct_pointer(struct_pointer), field(field) {}
  [[nodiscard]] Expression *get_struct_pointer() const {
    return struct_pointer;
  };
  [[nodiscard]] std::string_view get_field() const { return field; };
  void accept(class ASTVisitor &visitor) override;
};

class FieldAccessExpr : public Expression {
private:
  Expression *struct_;
  std::string_view field;

public:
  FieldAccessExpr(Expression *struct_, std::string_view field,
                  SourceLocation loc = {})
      : Expression(Expression::Kind::FieldAccess, "FieldAccessExpr", loc),
        struct_(struct_), field(field) {}
  [[nodiscard]] Expression *get_struct() const { return struct_; };
  [[nodiscard]] std::string_view get_field() const { return field; };
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
public:
  enum class Base { Decimal, Hexadecimal };

private:
  std::string_view value;
  Base base;

public:
  explicit NumericExpr(std::string_view value, Base base,
                       SourceLocation loc = {})
      : Expression(Expression::Kind::Numeric, "NumericLiteral", loc),
        value(value), base(base) {}
  [[nodiscard]] std::string_view get_value() const { return value; }
  [[nodiscard]] Base get_base() const { return base; }
  void accept(class ASTVisitor &visitor) override;

  template <typename IntType> std::optional<IntType> try_parse() {
    int int_base;
    IntType result{};
    switch (base) {
    case Base::Decimal:
      int_base = 10;
    case Base::Hexadecimal:
      int_base = 16;
    };

    auto [ptr, ec] =
        std::from_chars(value.data(), value.data() + value.size(), result);

    // spdlog::info("String repr: {} errc: {}", value, static_cast<int>(ec));
    if (ec == std::errc()) {
      return result;
    }

    return std::nullopt;
  }
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

class TernaryExpression : public Expression {
private:
  Expression *condition;
  Expression *then;
  Expression *else_;

public:
  explicit TernaryExpression(Expression *condition, Expression *then,
                             Expression *else_, SourceLocation loc = {})
      : Expression(Expression::Kind::Ternary, "TernaryOperator", loc),
        condition(condition), then(then), else_(else_) {}
  [[nodiscard]] Expression *get_condition() const { return condition; }
  [[nodiscard]] Expression *get_then() const { return then; }
  [[nodiscard]] Expression *get_else() const { return else_; }
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
  std::shared_ptr<Symbol> resolved_symbol;

public:
  explicit VarExpr(std::string_view name, SourceLocation loc = {})
      : Expression(Expression::Kind::Var, "VarExpr", loc), variable_name(name) {
  }
  [[nodiscard]] std::string_view get_variable_name() const {
    return variable_name;
  }
  void set_symbol(std::shared_ptr<Symbol> sym) {
    resolved_symbol = std::move(sym);
  }
  [[nodiscard]] std::shared_ptr<Symbol> get_symbol() const {
    return resolved_symbol;
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
    if (std::string(value) == "true") {
      this->value = true;
    } else if (std::string(value) == "false") {
      this->value = false;
    } else {
      throw std::runtime_error(
          std::format("Bool Const with illegal value {}", value));
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
  enum class Kind {
    Variable,
    Pointer,
    Field,
    Array,
    Dereference,
  };

private:
  Kind kind;

public:
  explicit LValue(Kind kind, SourceLocation loc = {})
      : ASTNode(loc), kind(kind) {}
  void accept(class ASTVisitor &visitor) override = 0;

  [[nodiscard]] Kind get_kind() const { return kind; }
};

class VariableLValue : public LValue {
private:
  std::string_view name;
  std::shared_ptr<Symbol> resolved_symbol;

public:
  explicit VariableLValue(std::string_view name, SourceLocation loc = {})
      : LValue(Kind::Variable, loc), name(name) {}

  [[nodiscard]] std::string_view get_name() const { return name; }
  void set_symbol(std::shared_ptr<Symbol> sym) {
    resolved_symbol = std::move(sym);
  }
  [[nodiscard]] std::shared_ptr<Symbol> get_symbol() const {
    return resolved_symbol;
  }
  void accept(class ASTVisitor &visitor) override;
};

class DereferenceLValue : public LValue {
private:
  LValue *operand;

public:
  explicit DereferenceLValue(LValue *operand, SourceLocation loc = {})
      : LValue(Kind::Dereference, loc), operand(operand) {}

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
      : LValue(Kind::Field, loc), base(base), field(field) {}

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
      : LValue(Kind::Pointer, loc), base(base), field(field) {}

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
      : LValue(Kind::Array, loc), base(base), index(index) {}

  [[nodiscard]] LValue *get_base() const { return base; }
  [[nodiscard]] Expression *get_index() const { return index; }
  void accept(class ASTVisitor &visitor) override;
};

// ==== Statements ====
class Statement : public ASTNode {
public:
  explicit Statement(SourceLocation loc = {}) : ASTNode(loc) {}
  void accept(class ASTVisitor &visitor) override;
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
  TypeAnnotation *type;
  std::string_view identifier;
  Expression *initializer;
  std::shared_ptr<Symbol> resolved_symbol;

public:
  VariableDeclarationStatement(TypeAnnotation *type,
                               std::string_view identifier,
                               Expression *init = nullptr,
                               SourceLocation loc = {})
      : Statement(loc), type(type), identifier(identifier), initializer(init) {}

  [[nodiscard]] TypeAnnotation *get_type() const { return type; }
  [[nodiscard]] std::string_view get_identifier() const { return identifier; }
  [[nodiscard]] Expression *get_initializer() const { return initializer; }
  void set_symbol(const std::shared_ptr<Symbol> &sym) { resolved_symbol = sym; }
  [[nodiscard]] std::shared_ptr<Symbol> get_symbol() const {
    return resolved_symbol;
  }

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
  [[nodiscard]] const std::vector<Statement *> &get_statements() const {
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
  enum class Kind { Function, Parameter, Struct, Typedef };

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

class Typedef : public Declaration {
private:
  TypeAnnotation *type;
  std::string_view name;

public:
  Typedef(TypeAnnotation *type, std::string_view name, SourceLocation loc = {})
      : Declaration(Declaration::Kind::Typedef, name, loc), type(type),
        name(name) {}
  [[nodiscard]] TypeAnnotation *get_type() const { return type; }
  void accept(class ASTVisitor &visitor) override;
};

class StructDeclaration : public Declaration {
private:
  std::string_view name;
  std::optional<std::vector<VariableDeclarationStatement *>> fields;

public:
  explicit StructDeclaration(std::string_view name, SourceLocation loc = {})
      : Declaration(Kind::Struct, name, loc), name(name) {}
  StructDeclaration(std::string_view name,
                    std::vector<VariableDeclarationStatement *> fields,
                    SourceLocation loc = {})
      : Declaration(Kind::Struct, name, loc), name(name),
        fields(std::move(fields)) {}

  [[nodiscard]] std::optional<std::vector<VariableDeclarationStatement *>>
  get_fields() const {
    return fields;
  }
  void accept(ASTVisitor &visitor) override;
};

class ParameterDeclaration : public Declaration {
private:
  std::string_view name;
  TypeAnnotation *type;

public:
  ParameterDeclaration(std::string_view name, TypeAnnotation *type,
                       SourceLocation loc = {})
      : Declaration(Kind::Parameter, name, loc), name(name), type(type) {}
  [[nodiscard]] TypeAnnotation *get_type() const { return type; }
  void accept(ASTVisitor &visitor) override;
};

class FunctionDeclaration : public Declaration {
private:
  std::string_view name;
  TypeAnnotation *ret_type;
  std::vector<ParameterDeclaration *> parameters;
  CompoundStmt *body{};

public:
  FunctionDeclaration(std::string_view name, TypeAnnotation *ret_type,
                      SourceLocation loc = {})
      : Declaration(Kind::Function, name, loc), name(name), ret_type(ret_type) {
  }
  void accept(ASTVisitor &visitor) override;

  void add_parameter_declaration(ParameterDeclaration *paramDecl) {
    parameters.push_back(paramDecl);
  }
  void set_body(CompoundStmt *stmt) { body = stmt; }

  [[nodiscard]] const std::vector<ParameterDeclaration *> &
  get_parameter_declarations() const {
    return parameters;
  }
  [[nodiscard]] CompoundStmt *get_body() const { return body; };
  [[nodiscard]] TypeAnnotation *get_return_type() const { return ret_type; }
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
  virtual void visit(Typedef &typedef_) {}

  virtual void visit(Declaration &decl) {}
  virtual void visit(FunctionDeclaration &decl) {}
  virtual void visit(ParameterDeclaration &decl) {}
  virtual void visit(StructDeclaration &decl) {}

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
  virtual void visit(PointerAccessExpr &expr) {}
  virtual void visit(FieldAccessExpr &expr) {}
  virtual void visit(AllocExpression &expr) {}
  virtual void visit(AllocArrayExpression &expr) {}
  virtual void visit(TernaryExpression &expr) {}

  virtual void visit(TypeAnnotation &type) {}
  virtual void visit(BuiltinTypeAnnotation &type) {}
  virtual void visit(NamedTypeAnnotation &type) {}
  virtual void visit(StructTypeAnnotation &type) {}
  virtual void visit(PointerTypeAnnotation &type) {}
  virtual void visit(ArrayTypeAnnotation &type) {}

  virtual void visit(LValue &val) {}
  virtual void visit(VariableLValue &val) {}
  virtual void visit(ArrayAccessLValue &val) {}
  virtual void visit(PointerAccessLValue &val) {}
  virtual void visit(FieldAccessLValue &val) {}
  virtual void visit(DereferenceLValue &val) {}

  virtual void visit(TranslationUnit &unit) {}
};

#endif // !DEFS_AST_H
