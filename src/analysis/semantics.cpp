#include "semantics.hpp"

void semantic::SemanticVisitor::visit(TranslationUnit &unit) {
  for (const auto &declaration : unit.get_declarations()) {
    declaration->accept(*this);
  }
}

void semantic::SemanticVisitor::visit(FunctionDeclaration &decl) {
  auto fs =
      FunctionSymbol{decl.get_name(), decl.get_location(),
                     symbol_table.next_id(), decl.get_body() ? true : false};
  symbol_table.define(fs);

  // Enter scope and handle statements
  symbol_table.enter_scope(std::format("Scope_{}", decl.get_name()));

  // Add parameter symbols
  for (const auto &param : decl.get_parameter_declarations()) {
    auto vs = VariableSymbol{param->get_name(), param->get_location(),
                             symbol_table.next_id(), true};
    symbol_table.define(vs);
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
#ifdef L1
  if (!has_return_statement) {
    diagnostics->emit_error(stmt.get_location(), "Missing return statement");
    diagnostics->add_source_context(
        source_manager->get_line(stmt.get_location().start_line()));
    diagnostics->suggest_fix("This is only present in L1");
  }
#endif
}

void semantic::SemanticVisitor::visit(AssignmentStatement &stmt) {
  if (stmt.get_lvalue()->get_kind() == LValue::Kind::Variable) {
    const auto var_l_val = dynamic_cast<VariableLValue *>(stmt.get_lvalue());
    auto lookup = symbol_table.lookup(var_l_val->get_name());
    if (!lookup) {
      diagnostics->emit_error(
          var_l_val->get_location(),
          std::format("Unresolved reference {}", var_l_val->get_name()));
      diagnostics->add_source_context(
          source_manager->get_line(var_l_val->get_location().start_line()));
    }
    if (!lookup->get().is_initialized() &&
        stmt.get_op() != AssignmentOperator::Equals) {
      diagnostics->emit_error(
          var_l_val->get_location(),
          std::format("Referencing uninitialized variable {} ",
                      var_l_val->get_name()));
      diagnostics->add_source_context(
          source_manager->get_line(var_l_val->get_location().start_line()));
      diagnostics->suggest_fix(
          std::format("Try initializing {}", lookup->get().get_name()));

      diagnostics->emit_note(
          lookup->get().get_source_location(),
          std::format("Variable {} declared here", lookup->get().get_name()));
      diagnostics->add_source_context(
          source_manager->get_snippet(lookup->get().get_source_location()));
    } else {
      lookup->get().set_initialized(true);
      var_l_val->set_symbol(std::make_shared<Symbol>(lookup.value()));
    }
  }
  stmt.get_expr()->accept(*this);
}

void semantic::SemanticVisitor::visit(VariableLValue &val) {}

void semantic::SemanticVisitor::visit(VariableDeclarationStatement &stmt) {
  bool initialized = false;
  if (stmt.get_initializer()) {
    stmt.get_initializer()->accept(*this);
    initialized = true;
  }
  auto vs = VariableSymbol{stmt.get_identifier(), stmt.get_location(),
                           symbol_table.next_id(), initialized};
  if (!symbol_table.define(vs)) {
    const auto previous_def = symbol_table.lookup(stmt.get_identifier());
    diagnostics->emit_error(
        stmt.get_location(),
        std::format("Redefinition of variable {} ", stmt.get_identifier()));
    diagnostics->add_source_context(
        source_manager->get_line(stmt.get_location().start_line()));
    diagnostics->emit_note(previous_def->get().get_source_location(),
                           "Previously defined here:");
    diagnostics->add_source_context(source_manager->get_line(
        previous_def->get().get_source_location().start_line()));
  } else {
    stmt.set_symbol(std::make_shared<Symbol>(vs));
  }
}

void semantic::SemanticVisitor::visit(VarExpr &expr) {
  const auto lookup = symbol_table.lookup(expr.get_variable_name());
  if (!lookup) {
    diagnostics->emit_error(
        expr.get_location(),
        std::format("Unresolved reference {}", expr.get_variable_name()));
    diagnostics->add_source_context(
        source_manager->get_line(expr.get_location().start_line()));
  } else if (!lookup->get().is_initialized()) {
    diagnostics->emit_error(
        expr.get_location(),
        std::format("Referencing uninitialized variable {} ",
                    expr.get_variable_name()));
    diagnostics->add_source_context(
        source_manager->get_line(expr.get_location().start_line()));
    diagnostics->suggest_fix(
        std::format("Try initializing {}", lookup->get().get_name()));

    diagnostics->emit_note(
        lookup->get().get_source_location(),
        std::format("Variable {} declared here", lookup->get().get_name()));
    diagnostics->add_source_context(
        source_manager->get_snippet(lookup->get().get_source_location()));
  } else {
    expr.set_symbol(std::make_shared<Symbol>(lookup.value()));
  }
}

void semantic::SemanticVisitor::visit(CallExpr &expr) {
  const auto lookup = symbol_table.lookup(expr.get_function_name());
  if (!lookup) {
    diagnostics->emit_error(expr.get_location(),
                            std::format("Unresolved method reference {}",
                                        expr.get_function_name()));
    diagnostics->add_source_context(
        source_manager->get_line(expr.get_location().start_line()));
  } else if (!lookup->get().is_initialized()) {
    diagnostics->emit_error(
        expr.get_location(),
        std::format("Referencing function declaration {} with no function body",
                    expr.get_function_name()));
    diagnostics->add_source_context(
        source_manager->get_line(expr.get_location().start_line()));
    diagnostics->suggest_fix(
        std::format("Try giving {} a body", lookup->get().get_name()));

    diagnostics->emit_note(
        lookup->get().get_source_location(),
        std::format("Function {} declared here", lookup->get().get_name()));
    diagnostics->add_source_context(
        source_manager->get_snippet(lookup->get().get_source_location()));
  }
  for (const auto &param : expr.get_params()) {
    param->accept(*this);
  }
}

void semantic::SemanticVisitor::visit(IfStatement &stmt) {
  stmt.get_condition()->accept(*this);

  symbol_table.enter_scope(
      std::format("Scope_if_{}", std::get<0>(stmt.get_location().begin)));
  stmt.get_then_branch()->accept(*this);
  symbol_table.exit_scope();

  if (stmt.get_else_branch()) {
    symbol_table.enter_scope(
        std::format("Scope_else_{}", std::get<0>(stmt.get_location().begin)));
    stmt.get_else_branch()->accept(*this);
    symbol_table.exit_scope();
  }
}

void semantic::SemanticVisitor::visit(ReturnStmt &stmt) {
#ifdef L1
  has_return_statement = true;
#endif
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
void semantic::SemanticVisitor::visit(StructDeclaration &decl) {
  auto ss =
      StructSymbol{decl.get_name(), decl.get_location(), symbol_table.next_id(),
                   decl.get_fields() ? true : false};
  symbol_table.define(ss);
}
void semantic::SemanticVisitor::visit(AssertStmt &stmt) {
  stmt.get_expression()->accept(*this);
}
void semantic::SemanticVisitor::visit(UnaryMutationStatement &stmt) {
  stmt.get_target()->accept(*this);
}
void semantic::SemanticVisitor::visit(ExpressionStatement &stmt) {
  stmt.get_expression()->accept(*this);
}
void semantic::SemanticVisitor::visit(ForStatement &stmt) {
  symbol_table.enter_scope(
      std::format("for_{}_head", std::get<0>(stmt.get_location().begin)));
  stmt.get_init()->accept(*this);
  stmt.get_condition()->accept(*this);
  stmt.get_increment()->accept(*this);

  symbol_table.enter_scope(
      std::format("for_{}_body", std::get<0>(stmt.get_location().begin)));
  stmt.get_body()->accept(*this);
  symbol_table.exit_scope();
  symbol_table.exit_scope();
}
void semantic::SemanticVisitor::visit(WhileStatement &stmt) {
  stmt.get_condition()->accept(*this);
  symbol_table.enter_scope(
      std::format("while_{}", std::get<0>(stmt.get_location().begin)));
  stmt.get_body()->accept(*this);
  symbol_table.exit_scope();
}
void semantic::SemanticVisitor::visit(ErrorStatement &stmt) {
  stmt.get_expr()->accept(*this);
}
void semantic::SemanticVisitor::visit(ParenthesisExpression &expr) {
  expr.get_expression()->accept(*this);
}
void semantic::SemanticVisitor::visit(UnaryOperatorExpression &expr) {
  expr.get_expression()->accept(*this);
}
void semantic::SemanticVisitor::visit(ArrayAccessExpr &expr) {
  expr.get_array()->accept(*this);
}
void semantic::SemanticVisitor::visit(PointerAccessExpr &expr) {
  expr.get_struct_pointer()->accept(*this);
}
void semantic::SemanticVisitor::visit(FieldAccessExpr &expr) {
  expr.get_struct()->accept(*this);
  // TODO: Check for struct reference? somehow i guess
}
void semantic::SemanticVisitor::visit(AllocExpression &expr) {
  // TODO: Typechecking here
}
void semantic::SemanticVisitor::visit(AllocArrayExpression &expr) {
  // TODO: Typechecking here too
}
void semantic::SemanticVisitor::visit(TernaryExpression &expr) {
  expr.get_condition()->accept(*this);
  expr.get_then()->accept(*this);
  expr.get_else()->accept(*this);
}
void semantic::SemanticVisitor::visit(ArrayAccessLValue &val) {
  val.get_base()->accept(*this);
  val.get_index()->accept(*this);
}
void semantic::SemanticVisitor::visit(PointerAccessLValue &val) {
  const auto lookup = symbol_table.lookup(val.get_field());
  if (!lookup) {
    diagnostics->emit_error(
        val.get_location(),
        std::format("Unresolved reference {}", val.get_field()));

    diagnostics->add_source_context(
        source_manager->get_line(val.get_location().start_line()));
  }
  val.get_base()->accept(*this);
}
void semantic::SemanticVisitor::visit(DereferenceLValue &val) {
  val.get_operand()->accept(*this);
}

void semantic::SemanticVisitor::visit(NumericExpr &expr) {
  const auto value = expr.try_parse<int>();
  if (!value) {
    diagnostics->emit_error(
        expr.get_location(),
        std::format("Integer literal out of bounds {}", expr.get_value()));

    diagnostics->add_source_context(
        source_manager->get_line(expr.get_location().start_line()));

    diagnostics->suggest_fix("The bounds are -2^31 < c < 2^31");
  }
}
