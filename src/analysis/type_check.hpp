#ifndef COMPILER_TYPE_CHECK_H
#define COMPILER_TYPE_CHECK_H

#include "../defs/ast.hpp"
#include "../report/report_builder.hpp"
#include "symbol.hpp"

namespace type_check {

class TypeVisitor : public ASTVisitor {
private:
  std::shared_ptr<DiagnosticEmitter> diagnostics;
  std::shared_ptr<SourceManager> source_manager;
  std::shared_ptr<SymbolTable> symbol_table;

  std::unordered_map<std::string, std::shared_ptr<type::StructType>>
      struct_types;
  std::unordered_map<std::string, const type::Type *> typedef_types;

  const type::Type *current_return_type = nullptr;

  const type::Type *resolve_type(const TypeAnnotation *annotation);

  const type::Type *resolve_underlying(const type::Type *t);

  static bool is_small_type(const type::Type *t);

  bool types_equal(const type::Type *a, const type::Type *b);

  void type_error(const SourceLocation &loc, const std::string &message);

  static const type::Type *expr_type(const Expression &expr);

  const type::Type *lvalue_type(const LValue &val);

public:
  explicit TypeVisitor(std::shared_ptr<DiagnosticEmitter> diagnostics,
                       std::shared_ptr<SourceManager> source_manager,
                       std::shared_ptr<SymbolTable> symbol_table)
      : diagnostics(std::move(diagnostics)),
        source_manager(std::move(source_manager)),
        symbol_table(std::move(symbol_table)) {}

  void visit(Typedef &typedef_) override;
  void visit(Declaration &decl) override;
  void visit(FunctionDeclaration &decl) override;
  void visit(ParameterDeclaration &decl) override;
  void visit(StructDeclaration &decl) override;
  void visit(Statement &stmt) override;
  void visit(CompoundStmt &stmt) override;
  void visit(ReturnStmt &stmt) override;
  void visit(AssertStmt &stmt) override;
  void visit(VariableDeclarationStatement &stmt) override;
  void visit(UnaryMutationStatement &stmt) override;
  void visit(AssignmentStatement &stmt) override;
  void visit(ExpressionStatement &stmt) override;
  void visit(IfStatement &stmt) override;
  void visit(ForStatement &stmt) override;
  void visit(WhileStatement &stmt) override;
  void visit(ErrorStatement &stmt) override;
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
  void visit(TypeAnnotation &type) override;
  void visit(BuiltinTypeAnnotation &type) override;
  void visit(NamedTypeAnnotation &type) override;
  void visit(StructTypeAnnotation &type) override;
  void visit(PointerTypeAnnotation &type) override;
  void visit(ArrayTypeAnnotation &type) override;
  void visit(LValue &val) override;
  void visit(VariableLValue &val) override;
  void visit(ArrayAccessLValue &val) override;
  void visit(PointerAccessLValue &val) override;
  void visit(FieldAccessLValue &val) override;
  void visit(DereferenceLValue &val) override;
  void visit(TranslationUnit &unit) override;
};

} // namespace type_check

#endif // COMPILER_TYPE_CHECK_H
