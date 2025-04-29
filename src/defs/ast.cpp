#include "ast.hpp"

void Declaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void FunctionDeclaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ParameterDeclaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Statement::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void CompoundStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ReturnStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Expression::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void NumericExpression::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void TranslationUnit::accept(ASTVisitor &visitor) { visitor.visit(*this); }
