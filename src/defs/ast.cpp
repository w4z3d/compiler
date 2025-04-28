#include "ast.hpp"

void Declaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void FunctionDeclaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void TranslationUnit::accept(ASTVisitor &visitor) { visitor.visit(*this); }
