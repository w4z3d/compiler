#ifndef ANALYSIS_SEMANTICS_H
#define ANALYSIS_SEMANTICS_H

#include <memory>
#include <utility>

#include "../defs/ast.hpp"
#include "../report/report_builder.hpp"
#include "symbol.hpp"

namespace semantic {

class SemanticVisitor : public ASTVisitor {
private:
  std::shared_ptr<SymbolTable> symbol_table;
  std::shared_ptr<DiagnosticEmitter> diagnostics;
  std::shared_ptr<SourceManager> source_manager;

#ifdef L1
  bool has_return_statement = false;
#endif

public:
  explicit SemanticVisitor(std::shared_ptr<DiagnosticEmitter> diagnostics,
                           std::shared_ptr<SourceManager> source_manager,
                           std::shared_ptr<SymbolTable> symbol_table)
      : diagnostics(std::move(diagnostics)),
        source_manager(std::move(source_manager)),
        symbol_table(std::move(symbol_table)) {}

  void visit(TranslationUnit &unit) override;
  void visit(CompoundStmt &stmt) override;
  void visit(FunctionDeclaration &decl) override;
  void visit(AssignmentStatement &stmt) override;
  void visit(VariableLValue &val) override;
  void visit(VariableDeclarationStatement &stmt) override;
  void visit(VarExpr &expr) override;
  void visit(CallExpr &expr) override;
  void visit(IfStatement &stmt) override;
  void visit(ReturnStmt &stmt) override;
  void visit(BinaryOperatorExpression &expr) override;
  void visit(FieldAccessLValue &val) override;
  void visit(Typedef &typedef_) override;
  void visit(StructDeclaration &decl) override;
  void visit(AssertStmt &stmt) override;
  void visit(UnaryMutationStatement &stmt) override;
  void visit(ExpressionStatement &stmt) override;
  void visit(ForStatement &stmt) override;
  void visit(WhileStatement &stmt) override;
  void visit(ErrorStatement &stmt) override;
  void visit(ParenthesisExpression &expr) override;
  void visit(UnaryOperatorExpression &expr) override;
  void visit(ArrayAccessExpr &expr) override;
  void visit(PointerAccessExpr &expr) override;
  void visit(FieldAccessExpr &expr) override;
  void visit(AllocExpression &expr) override;
  void visit(AllocArrayExpression &expr) override;
  void visit(TernaryExpression &expr) override;
  void visit(ArrayAccessLValue &val) override;
  void visit(PointerAccessLValue &val) override;
  void visit(DereferenceLValue &val) override;

  // Literal checking
  void visit(NumericExpr &expr) override;
};

} // namespace semantic
#endif // !ANALYSIS_SEMANTICS_H
#ifndef SRC_ANALYSIS_SEMANTICS_HPP
#define SRC_ANALYSIS_SEMANTICS_HPP

#endif // SRC_ANALYSIS_SEMANTICS_HPP
