#ifndef COMPILER_TYPE_CHECK_H
#define COMPILER_TYPE_CHECK_H

#include "../defs/ast.hpp"
#include "../report/report_builder.hpp"
#include "../type/source_type_registry.hpp"
#include "symbol.hpp"

namespace type_check {

class TypeVisitor : public ASTVisitor {
private:
  source_type::QualType current_return_type;
  bool has_return = false;

  arena::Arena arena;
  source_type::TypeRegistry &types;

  std::shared_ptr<DiagnosticEmitter> diagnostics;
  std::shared_ptr<SourceManager> source_manager;
  std::shared_ptr<SymbolTable> symbol_table;

  // ── Helpers ──────────────────────────────────────────
  void emit_error(const SourceLocation &loc, const std::string &message);
  const source_type::Type *resolve_underlying(const source_type::Type *t);
  bool is_small_type(const source_type::Type *t);
  const source_type::Type *lvalue_type(const LValue &val);
  void register_builtins();
  void register_extern(const std::string &name,
                       const source_type::Type *ret_type,
                       std::vector<const source_type::Type *> params);

public:
  explicit TypeVisitor(std::shared_ptr<DiagnosticEmitter> diagnostics,
                       std::shared_ptr<SourceManager> source_manager,
                       std::shared_ptr<SymbolTable> symbol_table,
                       source_type::TypeRegistry &types)
      : diagnostics(std::move(diagnostics)),
        source_manager(std::move(source_manager)),
        symbol_table(std::move(symbol_table)), arena(arena::Arena{}),
        types(types) {}

  // ── Top Level ────────────────────────────────────────
  void visit(TranslationUnit &unit) override;

  // ── Declarations ─────────────────────────────────────
  void visit(Declaration &decl) override;
  void visit(FunctionDeclaration &decl) override;
  void visit(ParameterDeclaration &decl) override;
  void visit(StructDeclaration &decl) override;
  void visit(Typedef &typedef_) override;
  void visit(VariableDeclarationStatement &stmt) override;

  // ── Statements ───────────────────────────────────────
  void visit(Statement &stmt) override;
  void visit(CompoundStmt &stmt) override;
  void visit(ReturnStmt &stmt) override;
  void visit(AssertStmt &stmt) override;
  void visit(UnaryMutationStatement &stmt) override;
  void visit(AssignmentStatement &stmt) override;
  void visit(ExpressionStatement &stmt) override;
  void visit(IfStatement &stmt) override;
  void visit(ForStatement &stmt) override;
  void visit(WhileStatement &stmt) override;
  void visit(ErrorStatement &stmt) override;

  // ── Expressions ──────────────────────────────────────
  void visit(Expression &expr) override;
  void visit(NumericExpr &expr) override;
  void visit(CallExpr &expr) override;
  void visit(StringLiteralExpr &expr) override;
  void visit(CharLiteralExpr &expr) override;
  void visit(BoolConstExpr &expr) override;
  void visit(NullExpr &expr) override;
  void visit(VarExpr &expr) override;
  void visit(ParenthesisExpression &expr) override;
  void visit(BinaryOperatorExpression &expr) override;
  void visit(UnaryOperatorExpression &expr) override;
  void visit(ArrayAccessExpr &expr) override;
  void visit(PointerAccessExpr &expr) override;
  void visit(FieldAccessExpr &expr) override;
  void visit(AllocExpression &expr) override;
  void visit(AllocArrayExpression &expr) override;
  void visit(TernaryExpression &expr) override;

  // ── Type Annotations (no-ops) ────────────────────────
  void visit(TypeAnnotation &type) override;
  void visit(BuiltinTypeAnnotation &type) override;
  void visit(NamedTypeAnnotation &type) override;
  void visit(StructTypeAnnotation &type) override;
  void visit(PointerTypeAnnotation &type) override;
  void visit(ArrayTypeAnnotation &type) override;

  // ── LValues ──────────────────────────────────────────
  void visit(LValue &val) override;
  void visit(VariableLValue &val) override;
  void visit(ArrayAccessLValue &val) override;
  void visit(PointerAccessLValue &val) override;
  void visit(FieldAccessLValue &val) override;
  void visit(DereferenceLValue &val) override;
};

} // namespace type_check

#endif
