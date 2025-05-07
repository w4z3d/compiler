#ifndef ANALYSIS_SEMANTICS_H
#define ANALYSIS_SEMANTICS_H

#include "../defs/ast.hpp"
namespace semantic {

class SemanticVisitor : public ASTVisitor {
  void visit(TranslationUnit &unit) override;
  void visit(AssignmentStatement &stmt) override;
  void visit(Declaration &decl) override;
  void visit(ParameterDeclaration &decl) override;
  void visit(BinaryOperatorExpression &expr) override;
  void visit(UnaryOperatorExpression &expr) override;
  void visit(VariableDeclarationStatement &stmt) override;
  void visit(ExpressionStatement &stmt) override;
  void visit(CallExpr &expr) override;
  void visit(FunctionDeclaration &decl) override;
};

} // namespace semantic
#endif // !ANALYSIS_SEMANTICS_H
