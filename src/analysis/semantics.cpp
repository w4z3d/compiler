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
