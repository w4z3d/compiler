#include "ast.hpp"

void Declaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void FunctionDeclaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ParameterDeclaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Statement::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void CompoundStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ReturnStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void AssertStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Expression::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void NumericExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void CallExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void StringLiteralExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void TranslationUnit::accept(ASTVisitor &visitor) { visitor.visit(*this); }
