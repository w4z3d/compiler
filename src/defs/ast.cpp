#include "ast.hpp"

void ast::PrintVisitor::visit(BinaryExpression &expression) {
  content += "BinaryExpression(";
  expression.lhs->accept(*this);
  content += " " + expression.op + " ";
  expression.rhs->accept(*this);
  content += ")";
}

void ast::PrintVisitor::visit(LiteralExpr &expression) {
  content += expression.value;
}
