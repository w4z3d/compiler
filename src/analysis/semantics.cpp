#include "semantics.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include "symbol.hpp"

void semantic::SemanticVisitor::visit(TranslationUnit &unit) {
  for (const auto &declaration : unit.get_declarations()) {
    declaration->accept(*this);
  }
}

void semantic::SemanticVisitor::visit(FunctionDeclaration &decl) {
  symbol_table.define(FunctionSymbol{decl.get_name(), decl.get_location()});

  // Enter scope and handle statements
  symbol_table.enter_scope(std::format("Scope_{}", decl.get_name()));

  // Add parameter symbols
  for (const auto &param : decl.get_parameter_declarations()) {
    symbol_table.define(
        VariableSymbol{param->get_name(), param->get_location()});
  }

  const auto body = decl.get_body();
  if (body) {
    body->accept(*this);
  }

  // Exit scope again
  symbol_table.exit_scope();
}

void semantic::SemanticVisitor::visit(CompoundStmt &stmt) {
  for (const auto &statement : stmt.get_statements()) {
    statement->accept(*this);
  }
}

void semantic::SemanticVisitor::visit(AssignmentStatement &stmt) {
  stmt.get_lvalue()->accept(*this);
  stmt.get_expr()->accept(*this);
}

void semantic::SemanticVisitor::visit(VariableLValue &val) {
  const auto lookup = symbol_table.lookup(val.get_name());
  if (lookup) {
    symbol_table.define(VariableSymbol{val.get_name(), val.get_location()});
  }
}

void semantic::SemanticVisitor::visit(VariableDeclarationStatement &stmt) {
  stmt.get_initializer()->accept(*this);
  if (!symbol_table.define(
          VariableSymbol{stmt.get_identifier(), stmt.get_location()})) {
    spdlog::error("Variable {} already defined", stmt.get_identifier());
  }
}

void semantic::SemanticVisitor::visit(VarExpr &expr) {
  const auto lookup = symbol_table.lookup(expr.get_variable_name());
  if (!lookup) {
    spdlog::error("Unresolved reference {}", expr.get_variable_name());
  }
}

void semantic::SemanticVisitor::visit(CallExpr &expr) {
  const auto lookup = symbol_table.lookup(expr.get_function_name());
  if (!lookup) {
    spdlog::error("Unresolved method reference {}", expr.get_function_name());
  }
}

void semantic::SemanticVisitor::visit(IfStatement &stmt) {
  stmt.get_condition()->accept(*this);

  symbol_table.enter_scope(
      std::format("Scope_if_{}", stmt.get_location().line));
  stmt.get_then_branch()->accept(*this);
  symbol_table.exit_scope();

  if (stmt.get_else_branch()) {
    symbol_table.enter_scope(
        std::format("Scope_else_{}_test", stmt.get_location().line));
    stmt.get_else_branch()->accept(*this);
    symbol_table.exit_scope();
  }
}

void semantic::SemanticVisitor::visit(ReturnStmt &stmt) {
  stmt.get_expression()->accept(*this);
}

void semantic::SemanticVisitor::visit(BinaryOperatorExpression &expr) {
  expr.get_left_expression()->accept(*this);
  expr.get_right_expression()->accept(*this);
}

void semantic::SemanticVisitor::visit(FieldAccessLValue &val) {}
void semantic::SemanticVisitor::visit(Typedef &typedef_) {
  ASTVisitor::visit(typedef_);
}
void semantic::SemanticVisitor::visit(Declaration &decl) {
  ASTVisitor::visit(decl);
}
void semantic::SemanticVisitor::visit(ParameterDeclaration &decl) {
  ASTVisitor::visit(decl);
}
void semantic::SemanticVisitor::visit(StructDeclaration &decl) {
  ASTVisitor::visit(decl);
}
void semantic::SemanticVisitor::visit(Statement &stmt) {
  ASTVisitor::visit(stmt);
}
void semantic::SemanticVisitor::visit(AssertStmt &stmt) {
  ASTVisitor::visit(stmt);
}
void semantic::SemanticVisitor::visit(UnaryMutationStatement &stmt) {
  ASTVisitor::visit(stmt);
}
void semantic::SemanticVisitor::visit(ExpressionStatement &stmt) {
  stmt.get_expression()->accept(*this);
}
void semantic::SemanticVisitor::visit(ForStatement &stmt) {
  ASTVisitor::visit(stmt);
}
void semantic::SemanticVisitor::visit(WhileStatement &stmt) {
  ASTVisitor::visit(stmt);
}
void semantic::SemanticVisitor::visit(ErrorStatement &stmt) {
  ASTVisitor::visit(stmt);
}
void semantic::SemanticVisitor::visit(Expression &expr) {
  ASTVisitor::visit(expr);
}
void semantic::SemanticVisitor::visit(NumericExpr &expr) {
  ASTVisitor::visit(expr);
}
void semantic::SemanticVisitor::visit(StringLiteralExpr &expr) {
  ASTVisitor::visit(expr);
}
void semantic::SemanticVisitor::visit(CharLiteralExpr &expr) {
  ASTVisitor::visit(expr);
}
void semantic::SemanticVisitor::visit(BoolConstExpr &expr) {
  ASTVisitor::visit(expr);
}
void semantic::SemanticVisitor::visit(NullExpr &expr) {
  ASTVisitor::visit(expr);
}
void semantic::SemanticVisitor::visit(ParenthesisExpression &expr) {
  ASTVisitor::visit(expr);
}
void semantic::SemanticVisitor::visit(UnaryOperatorExpression &expr) {
  ASTVisitor::visit(expr);
}
void semantic::SemanticVisitor::visit(ArrayAccessExpr &expr) {
  ASTVisitor::visit(expr);
}
void semantic::SemanticVisitor::visit(PointerAccessExpr &expr) {
  ASTVisitor::visit(expr);
}
void semantic::SemanticVisitor::visit(FieldAccessExpr &expr) {
  ASTVisitor::visit(expr);
}
void semantic::SemanticVisitor::visit(AllocExpression &expr) {
  ASTVisitor::visit(expr);
}
void semantic::SemanticVisitor::visit(AllocArrayExpression &expr) {
  ASTVisitor::visit(expr);
}
void semantic::SemanticVisitor::visit(TernaryExpression &expr) {
  ASTVisitor::visit(expr);
}
void semantic::SemanticVisitor::visit(TypeAnnotation &type) { ASTVisitor::visit(type); }
void semantic::SemanticVisitor::visit(BuiltinTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void semantic::SemanticVisitor::visit(NamedTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void semantic::SemanticVisitor::visit(StructTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void semantic::SemanticVisitor::visit(PointerTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void semantic::SemanticVisitor::visit(ArrayTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void semantic::SemanticVisitor::visit(LValue &val) { ASTVisitor::visit(val); }
void semantic::SemanticVisitor::visit(ArrayAccessLValue &val) {
  ASTVisitor::visit(val);
}
void semantic::SemanticVisitor::visit(PointerAccessLValue &val) {
  ASTVisitor::visit(val);
}
void semantic::SemanticVisitor::visit(DereferenceLValue &val) {
  ASTVisitor::visit(val);
}
