#include "type_check.hpp"

namespace type_check {

void TypeVisitor::emit_error(const SourceLocation &loc,
                             const std::string &message) {
  diagnostics->error(loc, message).with_snippet(*source_manager);
}
// Im TypeChecker oder einem eigenen Pass:

void TypeVisitor::register_builtins() {
  // void print_int(int x)
  register_extern("print_int", types.get_void(), {types.get_int()});
  register_extern("println", types.get_void(), {});
  register_extern("print_bool", types.get_void(), {types.get_bool()});
  register_extern("print_char", types.get_void(), {types.get_int()});
  register_extern("print_int_ln", types.get_void(), {types.get_int()});

  // int read_int()
  register_extern("read_int", types.get_int(), {});
  register_extern("read_char", types.get_int(), {});

  // void exit(int code)
  register_extern("exit", types.get_void(), {types.get_int()});
  register_extern("abort", types.get_void(), {});
  register_extern("assert", types.get_void(), {types.get_bool()});
}

void TypeVisitor::register_extern(
    const std::string &name, const source_type::Type *ret_type,
    std::vector<const source_type::Type *> params) {

  std::vector<source_type::QualType> param_quals;
  param_quals.reserve(params.size());
  for (auto *p : params)
    param_quals.emplace_back(p);

  auto *func_type =
      types.get_function(source_type::QualType(ret_type), param_quals);
  auto *sym =
      symbol_table->create_function(name, {}, ret_type, func_type, true);
  symbol_table->define(sym);
}
const source_type::Type *
TypeVisitor::resolve_underlying(const source_type::Type *t) {
  return types.resolve_through_typedefs(t);
}

bool TypeVisitor::is_small_type(const source_type::Type *t) {
  if (!t)
    return false;
  t = resolve_underlying(t);
  if (t->is_builtin()) {
    auto *bt = static_cast<const source_type::BuiltinType *>(t);
    return !bt->is_void();
  }
  return t->is_pointer();
}

void TypeVisitor::visit(TranslationUnit &unit) {
  for (auto *decl : unit.get_declarations()) {
    if (auto *sd = dynamic_cast<StructDeclaration *>(decl)) {
      if (auto fields = sd->get_fields()) {
        std::vector<std::pair<std::string, source_type::QualType>> fs;
        for (auto *f : *fields) {
          auto *ft = types.resolve(f->get_type());
          if (ft && types.types_equal(ft, types.get_void()))
            diagnostics
                ->error(f->get_location(),
                        std::format("struct field '{}' cannot have "
                                    "type 'void'",
                                    f->get_identifier()))
                .with_snippet(*source_manager);
          fs.emplace_back(std::string(f->get_identifier()),
                          source_type::QualType(ft));
        }
        types.add_struct(std::string(sd->get_name()), std::move(fs));
      }
    }

    if (auto *td = dynamic_cast<Typedef *>(decl)) {
      auto *resolved = types.resolve(td->get_type());
      types.add_typedef(std::string(td->get_name()), resolved);
    }
    decl->accept(*this);
  }
}

void TypeVisitor::visit(FunctionDeclaration &decl) {
  auto return_type = types.resolve_qual(decl.get_return_type());
  current_return_type = return_type;
  has_return = false;

  std::vector<source_type::QualType> param_types;
  for (const auto &param : decl.get_parameter_declarations())
    param_types.emplace_back(types.resolve_qual(param->get_type()));

  auto *func_type = types.get_function(return_type, param_types);
  auto *func_sym = symbol_table->create_function(
      decl.get_name(), decl.get_location(), return_type.unqualified(),
      func_type, static_cast<bool>(decl.get_body()));
  symbol_table->define(func_sym);

  symbol_table->enter_scope(std::format("Scope_{}", decl.get_name()));

  for (const auto &param : decl.get_parameter_declarations()) {
    auto *param_type = types.resolve(param->get_type());
    if (param_type && types.types_equal(param_type, types.get_void()))
      diagnostics
          ->error(param->get_location(), "parameter cannot have type 'void'")
          .with_snippet(*source_manager);

    auto *param_sym = symbol_table->create_variable(
        param->get_name(), param->get_location(), param_type, true);
    symbol_table->define(param_sym);
    param->set_symbol(param_sym);
  }

  if (decl.get_body())
    decl.get_body()->accept(*this);

  if (!has_return &&
      !types.types_equal(return_type.unqualified(), types.get_void()))
    diagnostics
        ->error(decl.get_location(),
                std::format("non-void function '{}' must return a value",
                            decl.get_name()))
        .with_snippet(*source_manager)
        .suggest("add a return statement");

  symbol_table->exit_scope();
  current_return_type = {};
}

void TypeVisitor::visit(VariableDeclarationStatement &stmt) {
  auto var_type = types.resolve_qual(stmt.get_type());

  if (var_type.unqualified() &&
      types.types_equal(var_type.unqualified(), types.get_void()))
    diagnostics->error(stmt.get_location(), "variable cannot have type 'void'")
        .with_snippet(*source_manager);

  if (stmt.get_initializer()) {
    stmt.get_initializer()->accept(*this);
    auto *init_type = stmt.get_initializer()->get_resolved_type();
    if (init_type && var_type.unqualified() &&
        !types.types_equal(init_type, var_type.unqualified()))
      diagnostics
          ->error(stmt.get_location(),
                  std::format("initializer type mismatch: variable "
                              "declared as '{}', got '{}'",
                              var_type.to_string(), init_type->to_string()))
          .with_snippet(*source_manager);
  }

  auto *var_sym = symbol_table->create_variable(
      stmt.get_identifier(), stmt.get_location(), var_type.unqualified(),
      static_cast<bool>(stmt.get_initializer()));

  if (!symbol_table->define(var_sym)) {
    auto *prev = symbol_table->lookup(stmt.get_identifier());
    diagnostics
        ->error(
            stmt.get_location(),
            std::format("redefinition of variable '{}'", stmt.get_identifier()))
        .with_snippet(*source_manager)
        .note("previously defined here", prev->get_location(), *source_manager);
  }

  stmt.set_symbol(var_sym);
}

void TypeVisitor::visit(ParameterDeclaration &decl) {}
void TypeVisitor::visit(StructDeclaration &decl) {}
void TypeVisitor::visit(Typedef &typedef_) {}
void TypeVisitor::visit(Declaration &decl) {}

void TypeVisitor::visit(CompoundStmt &stmt) {
  symbol_table->enter_scope(
      std::format("compound_{}", stmt.get_location().start_line()));
  for (const auto &s : stmt.get_statements())
    s->accept(*this);
  symbol_table->exit_scope();
}

void TypeVisitor::visit(ReturnStmt &stmt) {
  has_return = true;

  if (stmt.get_expression()) {
    stmt.get_expression()->accept(*this);

    if (types.types_equal(current_return_type.unqualified(), types.get_void()))
      diagnostics
          ->error(stmt.get_location(), "returning a value from a void function")
          .with_snippet(*source_manager);
    else {
      auto *ret_type = stmt.get_expression()->get_resolved_type();
      if (ret_type &&
          !types.types_equal(ret_type, current_return_type.unqualified()))
        diagnostics
            ->error(stmt.get_location(),
                    std::format("return type mismatch: expected '{}', "
                                "got '{}'",
                                current_return_type.to_string(),
                                ret_type->to_string()))
            .with_snippet(*source_manager)
            .suggest(std::format("change return type to '{}'",
                                 ret_type->to_string()));
    }
  } else {
    if (!types.types_equal(current_return_type.unqualified(), types.get_void()))
      diagnostics
          ->error(stmt.get_location(),
                  std::format("non-void function must return a value "
                              "of type '{}'",
                              current_return_type.to_string()))
          .with_snippet(*source_manager);
  }
}

void TypeVisitor::visit(AssignmentStatement &stmt) {
  stmt.get_lvalue()->accept(*this);
  stmt.get_expr()->accept(*this);

  auto *lhs_type = lvalue_type(*stmt.get_lvalue());
  auto *rhs_type = stmt.get_expr()->get_resolved_type();
  lhs_type = resolve_underlying(lhs_type);
  rhs_type = resolve_underlying(rhs_type);

  if (stmt.get_op() != AssignmentOperator::Equals) {
    if (lhs_type && !types.types_equal(lhs_type, types.get_int()))
      diagnostics
          ->error(stmt.get_location(),
                  std::format("compound assignment requires 'int' on "
                              "left side, got '{}'",
                              lhs_type->to_string()))
          .with_snippet(*source_manager);
    if (rhs_type && !types.types_equal(rhs_type, types.get_int()))
      diagnostics
          ->error(stmt.get_location(),
                  std::format("compound assignment requires 'int' on "
                              "right side, got '{}'",
                              rhs_type->to_string()))
          .with_snippet(*source_manager);
  } else {
    if (lhs_type && rhs_type && !types.types_equal(lhs_type, rhs_type))
      diagnostics
          ->error(stmt.get_location(),
                  std::format("assignment type mismatch: left side is "
                              "'{}', right side is '{}'",
                              lhs_type->to_string(), rhs_type->to_string()))
          .with_snippet(*source_manager);
  }
}

void TypeVisitor::visit(UnaryMutationStatement &stmt) {
  stmt.get_target()->accept(*this);
  auto *t = resolve_underlying(lvalue_type(*stmt.get_target()));
  if (t && !types.types_equal(t, types.get_int()))
    diagnostics
        ->error(stmt.get_location(),
                std::format("increment/decrement requires 'int', got '{}'",
                            t->to_string()))
        .with_snippet(*source_manager);
}

void TypeVisitor::visit(IfStatement &stmt) {
  stmt.get_condition()->accept(*this);
  auto *cond = resolve_underlying(stmt.get_condition()->get_resolved_type());
  if (cond && !types.types_equal(cond, types.get_bool()))
    diagnostics
        ->error(stmt.get_condition()->get_location(),
                std::format("if condition must be 'bool', got '{}'",
                            cond->to_string()))
        .with_snippet(*source_manager);
  stmt.get_then_branch()->accept(*this);
  if (stmt.get_else_branch())
    stmt.get_else_branch()->accept(*this);
}

void TypeVisitor::visit(ForStatement &stmt) {
  symbol_table->enter_scope(
      std::format("for_{}", stmt.get_location().start_line()));
  if (stmt.get_init())
    stmt.get_init()->accept(*this);
  if (stmt.get_condition()) {
    stmt.get_condition()->accept(*this);
    auto *cond = resolve_underlying(stmt.get_condition()->get_resolved_type());
    if (cond && !types.types_equal(cond, types.get_bool()))
      diagnostics
          ->error(stmt.get_condition()->get_location(),
                  std::format("for condition must be 'bool', got '{}'",
                              cond->to_string()))
          .with_snippet(*source_manager);
  }
  if (stmt.get_increment())
    stmt.get_increment()->accept(*this);
  stmt.get_body()->accept(*this);
  symbol_table->exit_scope();
}

void TypeVisitor::visit(WhileStatement &stmt) {
  stmt.get_condition()->accept(*this);
  auto *cond = resolve_underlying(stmt.get_condition()->get_resolved_type());
  if (cond && !types.types_equal(cond, types.get_bool()))
    diagnostics
        ->error(stmt.get_condition()->get_location(),
                std::format("while condition must be 'bool', got '{}'",
                            cond->to_string()))
        .with_snippet(*source_manager);
  stmt.get_body()->accept(*this);
}

void TypeVisitor::visit(AssertStmt &stmt) {
  stmt.get_expression()->accept(*this);
  auto *t = resolve_underlying(stmt.get_expression()->get_resolved_type());
  if (t && !types.types_equal(t, types.get_bool()))
    diagnostics
        ->error(stmt.get_location(),
                std::format("assert condition must be 'bool', got '{}'",
                            t->to_string()))
        .with_snippet(*source_manager);
}

void TypeVisitor::visit(ErrorStatement &stmt) {
  stmt.get_expr()->accept(*this);
  auto *t = resolve_underlying(stmt.get_expr()->get_resolved_type());
  if (t && !types.types_equal(t, types.get_string()))
    diagnostics
        ->error(stmt.get_location(),
                std::format("error statement requires 'string', got '{}'",
                            t->to_string()))
        .with_snippet(*source_manager);
}

void TypeVisitor::visit(ExpressionStatement &stmt) {
  stmt.get_expression()->accept(*this);
}

void TypeVisitor::visit(Statement &stmt) {}

void TypeVisitor::visit(NumericExpr &expr) {
  expr.set_resolved_type(types.get_int());
}

void TypeVisitor::visit(BoolConstExpr &expr) {
  expr.set_resolved_type(types.get_bool());
}

void TypeVisitor::visit(CharLiteralExpr &expr) {
  expr.set_resolved_type(types.get_char());
}

void TypeVisitor::visit(StringLiteralExpr &expr) {
  expr.set_resolved_type(types.get_string());
}

void TypeVisitor::visit(NullExpr &expr) {
  expr.set_resolved_type(types.get_pointer(types.get_void()));
}

void TypeVisitor::visit(VarExpr &expr) {
  auto *sym = symbol_table->lookup(expr.get_variable_name());
  if (!sym) {
    diagnostics
        ->error(expr.get_location(), std::format("unresolved reference '{}'",
                                                 expr.get_variable_name()))
        .with_snippet(*source_manager);
    return;
  }
  if (!sym->is_initialized()) {
    diagnostics
        ->error(expr.get_location(),
                std::format("use of uninitialized variable '{}'",
                            expr.get_variable_name()))
        .with_snippet(*source_manager)
        .note("variable declared here", sym->get_location(), *source_manager)
        .suggest(std::format("initialize '{}' before use", sym->get_name()));
    return;
  }
  expr.set_symbol(sym);
  expr.set_resolved_type(sym->get_type());
}

void TypeVisitor::visit(ParenthesisExpression &expr) {
  expr.get_expression()->accept(*this);
  expr.set_resolved_type(expr.get_expression()->get_resolved_type());
}

void TypeVisitor::visit(BinaryOperatorExpression &expr) {
  expr.get_left_expression()->accept(*this);
  expr.get_right_expression()->accept(*this);

  auto *lhs =
      resolve_underlying(expr.get_left_expression()->get_resolved_type());
  auto *rhs =
      resolve_underlying(expr.get_right_expression()->get_resolved_type());

  if (!lhs || !rhs)
    return;

  switch (expr.get_operator_kind()) {
  case BinaryOperator::Add:
  case BinaryOperator::Sub:
  case BinaryOperator::Mult:
  case BinaryOperator::Div:
  case BinaryOperator::Modulo:
    if (!types.types_equal(lhs, types.get_int()))
      diagnostics
          ->error(expr.get_left_expression()->get_location(),
                  std::format("arithmetic operator expects 'int', "
                              "got '{}'",
                              lhs->to_string()))
          .with_snippet(*source_manager);
    if (!types.types_equal(rhs, types.get_int()))
      diagnostics
          ->error(expr.get_right_expression()->get_location(),
                  std::format("arithmetic operator expects 'int', "
                              "got '{}'",
                              rhs->to_string()))
          .with_snippet(*source_manager);
    expr.set_resolved_type(types.get_int());
    break;

  case BinaryOperator::BitwiseAnd:
  case BinaryOperator::BitwiseOr:
  case BinaryOperator::BitwiseXor:
  case BinaryOperator::ShiftLeft:
  case BinaryOperator::ShiftRight:
    if (!types.types_equal(lhs, types.get_int()))
      diagnostics
          ->error(expr.get_left_expression()->get_location(),
                  std::format("bitwise operator expects 'int', "
                              "got '{}'",
                              lhs->to_string()))
          .with_snippet(*source_manager);
    if (!types.types_equal(rhs, types.get_int()))
      diagnostics
          ->error(expr.get_right_expression()->get_location(),
                  std::format("bitwise operator expects 'int', "
                              "got '{}'",
                              rhs->to_string()))
          .with_snippet(*source_manager);
    expr.set_resolved_type(types.get_int());
    break;

  case BinaryOperator::Equal:
  case BinaryOperator::NotEqual:
    if (!is_small_type(lhs))
      diagnostics
          ->error(expr.get_left_expression()->get_location(),
                  std::format("equality not defined for type '{}'",
                              lhs->to_string()))
          .with_snippet(*source_manager);
    else if (!types.types_equal(lhs, rhs)) {
      bool null_ok = lhs->is_pointer() && rhs->is_pointer();
      if (!null_ok)
        diagnostics
            ->error(expr.get_location(),
                    std::format("comparing incompatible types "
                                "'{}' and '{}'",
                                lhs->to_string(), rhs->to_string()))
            .with_snippet(*source_manager);
    }
    expr.set_resolved_type(types.get_bool());
    break;

  case BinaryOperator::LessThan:
  case BinaryOperator::LessThanOrEqual:
  case BinaryOperator::GreaterThan:
  case BinaryOperator::GreaterThanOrEqual:
    if (!types.types_equal(lhs, types.get_int()) &&
        !types.types_equal(lhs, types.get_char()))
      diagnostics
          ->error(expr.get_left_expression()->get_location(),
                  std::format("comparison expects 'int' or 'char', "
                              "got '{}'",
                              lhs->to_string()))
          .with_snippet(*source_manager);
    if (!types.types_equal(lhs, rhs))
      diagnostics
          ->error(expr.get_location(),
                  std::format("comparing different types '{}' and '{}'",
                              lhs->to_string(), rhs->to_string()))
          .with_snippet(*source_manager);
    expr.set_resolved_type(types.get_bool());
    break;

  case BinaryOperator::LogicalAnd:
  case BinaryOperator::LogicalOr:
    if (!types.types_equal(lhs, types.get_bool()))
      diagnostics
          ->error(expr.get_left_expression()->get_location(),
                  std::format("logical operator expects 'bool', "
                              "got '{}'",
                              lhs->to_string()))
          .with_snippet(*source_manager);
    if (!types.types_equal(rhs, types.get_bool()))
      diagnostics
          ->error(expr.get_right_expression()->get_location(),
                  std::format("logical operator expects 'bool', "
                              "got '{}'",
                              rhs->to_string()))
          .with_snippet(*source_manager);
    expr.set_resolved_type(types.get_bool());
    break;

  case BinaryOperator::FieldAccess:
  case BinaryOperator::PointerAccess:
    break;

  case BinaryOperator::Unknown:
    diagnostics->error(expr.get_location(), "unknown binary operator")
        .with_snippet(*source_manager);
    break;
  }
}

void TypeVisitor::visit(UnaryOperatorExpression &expr) {
  expr.get_expression()->accept(*this);
  auto *inner = resolve_underlying(expr.get_expression()->get_resolved_type());
  if (!inner)
    return;

  switch (expr.get_operator_kind()) {
  case UnaryOperator::Neg:
    if (!types.types_equal(inner, types.get_int()))
      diagnostics
          ->error(expr.get_location(),
                  std::format("negation expects 'int', got '{}'",
                              inner->to_string()))
          .with_snippet(*source_manager);
    expr.set_resolved_type(types.get_int());
    break;

  case UnaryOperator::BitwiseNot:
    if (!types.types_equal(inner, types.get_int()))
      diagnostics
          ->error(expr.get_location(),
                  std::format("bitwise not expects 'int', got '{}'",
                              inner->to_string()))
          .with_snippet(*source_manager);
    expr.set_resolved_type(types.get_int());
    break;

  case UnaryOperator::LogicalNot:
    if (!types.types_equal(inner, types.get_bool()))
      diagnostics
          ->error(expr.get_location(),
                  std::format("logical not expects 'bool', got '{}'",
                              inner->to_string()))
          .with_snippet(*source_manager);
    expr.set_resolved_type(types.get_bool());
    break;

  case UnaryOperator::Deref:
    if (!inner->is_pointer()) {
      diagnostics
          ->error(expr.get_location(),
                  std::format("dereference expects pointer, got '{}'",
                              inner->to_string()))
          .with_snippet(*source_manager);
    } else {
      auto *pt = static_cast<const source_type::PointerType *>(inner);
      expr.set_resolved_type(pt->pointee.unqualified());
    }
    break;

  case UnaryOperator::Unknown:
    diagnostics->error(expr.get_location(), "unknown unary operator")
        .with_snippet(*source_manager);
    break;
  }
}

void TypeVisitor::visit(CallExpr &expr) {
  for (auto *arg : expr.get_params())
    arg->accept(*this);

  auto *sym = symbol_table->lookup(expr.get_function_name());
  if (!sym) {
    diagnostics
        ->error(expr.get_location(),
                std::format("unknown function '{}'", expr.get_function_name()))
        .with_snippet(*source_manager);
    return;
  }

  if (!sym->is_initialized()) {
    diagnostics
        ->error(expr.get_location(),
                std::format("call to declared but undefined function '{}'",
                            expr.get_function_name()))
        .with_snippet(*source_manager)
        .note("declared here", sym->get_location(), *source_manager)
        .suggest(std::format("provide a body for '{}'", sym->get_name()));
    return;
  }

  // Check if it's a FunctionSymbol with full type info
  if (sym->is_function()) {
    auto *func_sym = static_cast<FunctionSymbol *>(sym);
    if (func_sym->func_type) {
      auto &param_types = func_sym->func_type->param_types;

      if (expr.get_params().size() != param_types.size()) {
        diagnostics
            ->error(expr.get_location(),
                    std::format("function '{}' expects {} arguments, "
                                "got {}",
                                expr.get_function_name(), param_types.size(),
                                expr.get_params().size()))
            .with_snippet(*source_manager);
      } else {
        for (size_t i = 0; i < expr.get_params().size(); i++) {
          auto *arg_type = expr.get_params()[i]->get_resolved_type();
          if (arg_type &&
              !types.types_equal(arg_type, param_types[i].unqualified()))
            diagnostics
                ->error(expr.get_params()[i]->get_location(),
                        std::format("argument {} type mismatch: "
                                    "expected '{}', got '{}'",
                                    i + 1, param_types[i].to_string(),
                                    arg_type->to_string()))
                .with_snippet(*source_manager);
        }
      }
    }
  }

  expr.set_resolved_type(sym->get_type());
}

void TypeVisitor::visit(ArrayAccessExpr &expr) {
  expr.get_array()->accept(*this);
  expr.get_index()->accept(*this);

  auto *idx = resolve_underlying(expr.get_index()->get_resolved_type());
  if (idx && !types.types_equal(idx, types.get_int()))
    diagnostics
        ->error(expr.get_index()->get_location(),
                std::format("array index must be 'int', got '{}'",
                            idx->to_string()))
        .with_snippet(*source_manager);

  auto *arr = resolve_underlying(expr.get_array()->get_resolved_type());
  if (arr && arr->is_array()) {
    auto *at = static_cast<const source_type::ArrayType *>(arr);
    expr.set_resolved_type(at->element);
  } else if (arr) {
    diagnostics
        ->error(expr.get_array()->get_location(),
                std::format("subscript requires array type, got '{}'",
                            arr->to_string()))
        .with_snippet(*source_manager);
  }
}

void TypeVisitor::visit(FieldAccessExpr &expr) {
  expr.get_struct()->accept(*this);
  auto *base = resolve_underlying(expr.get_struct()->get_resolved_type());
  if (!base)
    return;

  if (!base->is_struct()) {
    diagnostics
        ->error(expr.get_struct()->get_location(),
                std::format("'.' requires struct type, got '{}'",
                            base->to_string()))
        .with_snippet(*source_manager);
    return;
  }

  auto *st = static_cast<const source_type::StructType *>(base);
  auto *field = st->field_type(std::string(expr.get_field()));
  if (field) {
    expr.set_resolved_type(field);
  } else {
    diagnostics
        ->error(expr.get_location(),
                std::format("struct '{}' has no field '{}'", st->name,
                            expr.get_field()))
        .with_snippet(*source_manager);
  }
}

void TypeVisitor::visit(PointerAccessExpr &expr) {
  expr.get_struct_pointer()->accept(*this);
  auto *base =
      resolve_underlying(expr.get_struct_pointer()->get_resolved_type());
  if (!base)
    return;

  if (!base->is_pointer()) {
    diagnostics
        ->error(expr.get_struct_pointer()->get_location(),
                std::format("'->' requires pointer type, got '{}'",
                            base->to_string()))
        .with_snippet(*source_manager);
    return;
  }

  auto *ptr = static_cast<const source_type::PointerType *>(base);
  auto *pointee = resolve_underlying(ptr->pointee.unqualified());
  if (!pointee || !pointee->is_struct()) {
    diagnostics
        ->error(expr.get_struct_pointer()->get_location(),
                std::format("'->' requires pointer to struct, got '{}'",
                            base->to_string()))
        .with_snippet(*source_manager);
    return;
  }

  auto *st = static_cast<const source_type::StructType *>(pointee);
  auto *field = st->field_type(std::string(expr.get_field()));
  if (field) {
    expr.set_resolved_type(field);
  } else {
    diagnostics
        ->error(expr.get_location(),
                std::format("struct '{}' has no field '{}'", st->name,
                            expr.get_field()))
        .with_snippet(*source_manager);
  }
}

void TypeVisitor::visit(AllocExpression &expr) {

  auto *alloc_type = types.resolve(expr.get_type());
  if (alloc_type)
    expr.set_resolved_type(types.get_pointer(alloc_type));
  expr.set_element_type(alloc_type);
}

void TypeVisitor::visit(AllocArrayExpression &expr) {
  expr.get_size()->accept(*this);
  auto *size_type = resolve_underlying(expr.get_size()->get_resolved_type());
  if (size_type && !types.types_equal(size_type, types.get_int()))
    diagnostics
        ->error(expr.get_size()->get_location(),
                std::format("alloc_array size must be 'int', got '{}'",
                            size_type->to_string()))
        .with_snippet(*source_manager);

  auto *elem_type = types.resolve(expr.get_type());
  if (elem_type)
    expr.set_resolved_type(types.get_array(elem_type));
  expr.set_element_type(elem_type);
}

void TypeVisitor::visit(TernaryExpression &expr) {
  expr.get_condition()->accept(*this);
  expr.get_then()->accept(*this);
  expr.get_else()->accept(*this);

  auto *cond = resolve_underlying(expr.get_condition()->get_resolved_type());
  if (cond && !types.types_equal(cond, types.get_bool()))
    diagnostics
        ->error(expr.get_condition()->get_location(),
                std::format("ternary condition must be 'bool', got '{}'",
                            cond->to_string()))
        .with_snippet(*source_manager);

  auto *then_type = expr.get_then()->get_resolved_type();
  auto *else_type = expr.get_else()->get_resolved_type();
  if (then_type && else_type && !types.types_equal(then_type, else_type))
    diagnostics
        ->error(expr.get_location(),
                std::format("ternary branches must have same type: "
                            "'{}' vs '{}'",
                            then_type->to_string(), else_type->to_string()))
        .with_snippet(*source_manager);

  if (then_type)
    expr.set_resolved_type(then_type);
}

void TypeVisitor::visit(Expression &expr) {}

const source_type::Type *TypeVisitor::lvalue_type(const LValue &val) {
  switch (val.get_kind()) {
  case LValue::Kind::Variable: {
    auto *v = dynamic_cast<const VariableLValue *>(&val);
    if (v && v->get_symbol())
      return v->get_symbol()->get_type();
    return nullptr;
  }
  case LValue::Kind::Dereference: {
    auto *d = dynamic_cast<const DereferenceLValue *>(&val);
    if (!d)
      return nullptr;
    auto *inner = resolve_underlying(lvalue_type(*d->get_operand()));
    if (inner && inner->is_pointer()) {
      auto *pt = static_cast<const source_type::PointerType *>(inner);
      return pt->pointee.unqualified();
    }
    return nullptr;
  }
  case LValue::Kind::Field: {
    auto *f = dynamic_cast<const FieldAccessLValue *>(&val);
    if (!f)
      return nullptr;
    auto *base = resolve_underlying(lvalue_type(*f->get_base()));
    if (base && base->is_struct()) {
      auto *st = static_cast<const source_type::StructType *>(base);
      return st->field_type(std::string(f->get_field()));
    }
    return nullptr;
  }
  case LValue::Kind::Pointer: {
    auto *p = dynamic_cast<const PointerAccessLValue *>(&val);
    if (!p)
      return nullptr;
    auto *base = resolve_underlying(lvalue_type(*p->get_base()));
    if (base && base->is_pointer()) {
      auto *pt = static_cast<const source_type::PointerType *>(base);
      auto *pointee = resolve_underlying(pt->pointee.unqualified());
      if (pointee && pointee->is_struct()) {
        auto *st = static_cast<const source_type::StructType *>(pointee);
        return st->field_type(std::string(p->get_field()));
      }
    }
    return nullptr;
  }
  case LValue::Kind::Array: {
    auto *a = dynamic_cast<const ArrayAccessLValue *>(&val);
    if (!a)
      return nullptr;
    auto *base = resolve_underlying(lvalue_type(*a->get_base()));
    if (base && base->is_array()) {
      auto *at = static_cast<const source_type::ArrayType *>(base);
      return at->element;
    }
    return nullptr;
  }
  }
  return nullptr;
}

void TypeVisitor::visit(LValue &val) {}

void TypeVisitor::visit(VariableLValue &val) {
  auto *sym = symbol_table->lookup(val.get_name());
  if (!sym) {
    diagnostics
        ->error(val.get_location(),
                std::format("unresolved reference '{}'", val.get_name()))
        .with_snippet(*source_manager);
  } else {
    sym->set_initialized(true);
    val.set_symbol(sym);
  }
}

void TypeVisitor::visit(ArrayAccessLValue &val) {
  val.get_base()->accept(*this);
  val.get_index()->accept(*this);
  auto *idx = resolve_underlying(val.get_index()->get_resolved_type());
  if (idx && !types.types_equal(idx, types.get_int()))
    diagnostics
        ->error(val.get_index()->get_location(),
                std::format("array index must be 'int', got '{}'",
                            idx->to_string()))
        .with_snippet(*source_manager);
}

void TypeVisitor::visit(PointerAccessLValue &val) {
  val.get_base()->accept(*this);
}

void TypeVisitor::visit(FieldAccessLValue &val) {
  val.get_base()->accept(*this);
}

void TypeVisitor::visit(DereferenceLValue &val) {
  val.get_operand()->accept(*this);
}

void TypeVisitor::visit(TypeAnnotation &) {}
void TypeVisitor::visit(BuiltinTypeAnnotation &) {}
void TypeVisitor::visit(NamedTypeAnnotation &) {}
void TypeVisitor::visit(StructTypeAnnotation &) {}
void TypeVisitor::visit(PointerTypeAnnotation &) {}
void TypeVisitor::visit(ArrayTypeAnnotation &) {}

} // namespace type_check
