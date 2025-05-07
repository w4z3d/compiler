#ifndef ANALYSIS_SEMANTICS_H
#define ANALYSIS_SEMANTICS_H

#include "../defs/ast.hpp"
#include "symbol.hpp"
namespace semantic {

class SemanticVisitor : public ASTVisitor {
private:
  SymbolTable symbol_table{};

public:
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
};

} // namespace semantic
#endif // !ANALYSIS_SEMANTICS_H
