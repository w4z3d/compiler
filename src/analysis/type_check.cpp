#include "type_check.hpp"
#include <format>

// ============================================================================
// Helpers
// ============================================================================

const type::Type *
type_check::TypeVisitor::resolve_type(const TypeAnnotation *annotation) {
  return from_type_annotation(annotation);
}

const type::Type *
type_check::TypeVisitor::resolve_underlying(const type::Type *t) {
  if (!t)
    return nullptr;
  if (t->kind == type::Type::Kind::Named) {
    auto *named = dynamic_cast<const type::NamedType *>(t);
    if (named) {
      auto it = typedef_types.find(named->name);
      if (it != typedef_types.end())
        return resolve_underlying(it->second);
      if (named->type)
        return resolve_underlying(named->type);
    }
  }
  return t;
}

bool type_check::TypeVisitor::is_small_type(const type::Type *t) {
  if (!t)
    return false;
  switch (t->kind) {
  case type::Type::Kind::Builtin: {
    auto *bt = dynamic_cast<const type::BuiltinType *>(t);
    return bt && bt->get_kind() != type::BuiltinType::BuiltinKind::Void;
  }
  case type::Type::Kind::Pointer:
    return true;
  default:
    return false;
  }
}

bool type_check::TypeVisitor::types_equal(const type::Type *a,
                                          const type::Type *b) {
  a = resolve_underlying(a);
  b = resolve_underlying(b);
  if (!a || !b)
    return false;
  return a->equals(*b);
}

void type_check::TypeVisitor::type_error(const SourceLocation &loc,
                                         const std::string &message) {
  diagnostics->emit_error(loc, message);
  diagnostics->add_source_context(source_manager->get_line(loc.start_line()));
}

const type::Type *type_check::TypeVisitor::expr_type(const Expression &expr) {
  auto rt = expr.get_resolved_type();
  return rt.get();
}

const type::Type *type_check::TypeVisitor::lvalue_type(const LValue &val) {
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
    auto *inner = lvalue_type(*d->get_operand());
    inner = resolve_underlying(inner);
    if (inner && inner->kind == type::Type::Kind::Pointer) {
      auto *ptr = dynamic_cast<const type::PointerType *>(inner);
      return ptr ? ptr->to : nullptr;
    }
    return nullptr;
  }
  case LValue::Kind::Field: {
    auto *f = dynamic_cast<const FieldAccessLValue *>(&val);
    if (!f)
      return nullptr;
    auto *base = lvalue_type(*f->get_base());
    base = resolve_underlying(base);
    if (base && base->kind == type::Type::Kind::Struct) {
      auto *st = dynamic_cast<const type::StructType *>(base);
      if (st) {
        for (const auto &[fname, ftype] : st->fields) {
          if (fname == f->get_field())
            return ftype;
        }
      }
    }
    return nullptr;
  }
  case LValue::Kind::Pointer: {
    auto *p = dynamic_cast<const PointerAccessLValue *>(&val);
    if (!p)
      return nullptr;
    auto *base = lvalue_type(*p->get_base());
    base = resolve_underlying(base);
    if (base && base->kind == type::Type::Kind::Pointer) {
      auto *ptr = dynamic_cast<const type::PointerType *>(base);
      if (ptr) {
        auto *pointee = resolve_underlying(ptr->to);
        if (pointee && pointee->kind == type::Type::Kind::Struct) {
          auto *st = dynamic_cast<const type::StructType *>(pointee);
          if (st) {
            for (const auto &[fname, ftype] : st->fields) {
              if (fname == p->get_field())
                return ftype;
            }
          }
        }
      }
    }
    return nullptr;
  }
  case LValue::Kind::Array: {
    auto *a = dynamic_cast<const ArrayAccessLValue *>(&val);
    if (!a)
      return nullptr;
    auto *base = lvalue_type(*a->get_base());
    base = resolve_underlying(base);
    if (base && base->kind == type::Type::Kind::Array) {
      auto *arr = dynamic_cast<const type::ArrayType *>(base);
      return arr ? arr->elementType : nullptr;
    }
    return nullptr;
  }
  }
  return nullptr;
}

void type_check::TypeVisitor::visit(TranslationUnit &unit) {
  // First pass: register all struct types and typedefs
  for (auto *decl : unit.get_declarations()) {
    if (auto *sd = dynamic_cast<StructDeclaration *>(decl)) {
      auto st = std::make_shared<type::StructType>(std::string(sd->get_name()));
      std::cout << " Struct decl " << st->toString() << std::endl;
      if (auto sd_fields = sd->get_fields()) {
        std::vector<std::pair<std::string, const type::Type *>> fields;
        for (auto *field : *sd_fields) {
          fields.emplace_back(std::string(field->get_identifier()),
                              resolve_type(field->get_type()));
        }
        st->set_fields(std::move(fields));
      }
      struct_types[std::string(sd->get_name())] = st;
    } else if (auto *td = dynamic_cast<Typedef *>(decl)) {
      typedef_types[std::string(td->get_name())] = resolve_type(td->get_type());
    }
  }

  // Second pass: type check everything
  for (auto *decl : unit.get_declarations()) {
    decl->accept(*this);
  }
}

void type_check::TypeVisitor::visit(Typedef &typedef_) {
  // Already registered in the first pass
}

void type_check::TypeVisitor::visit(Declaration &decl) {
  ASTVisitor::visit(decl);
}

void type_check::TypeVisitor::visit(FunctionDeclaration &decl) {
  auto *prev_return = current_return_type;
  current_return_type = resolve_type(decl.get_return_type());

  for (auto *param : decl.get_parameter_declarations()) {
    param->accept(*this);
  }

  if (decl.get_body()) {
    decl.get_body()->accept(*this);
  }

  current_return_type = prev_return;
}

void type_check::TypeVisitor::visit(ParameterDeclaration &decl) {
  auto *param_type = resolve_type(decl.get_type());
  if (param_type && param_type->equals(*type::void_t())) {
    type_error(decl.get_location(), "parameter cannot have void type");
  }
}

void type_check::TypeVisitor::visit(StructDeclaration &decl) {
  // Already registered in the first pass; check fields for void
  if (auto fields = decl.get_fields()) {
    for (auto *field : *fields) {
      auto *ft = resolve_type(field->get_type());
      if (ft && ft->equals(*type::void_t())) {
        type_error(field->get_location(),
                   std::format("struct field '{}' cannot have void type",
                               field->get_identifier()));
      }
    }
  }
}

void type_check::TypeVisitor::visit(Statement &stmt) {
  ASTVisitor::visit(stmt);
}

void type_check::TypeVisitor::visit(CompoundStmt &stmt) {
  for (auto *s : stmt.get_statements()) {
    s->accept(*this);
  }
}

void type_check::TypeVisitor::visit(ReturnStmt &stmt) {
  if (stmt.get_expression()) {
    stmt.get_expression()->accept(*this);

    if (current_return_type && current_return_type->equals(*type::void_t())) {
      type_error(stmt.get_location(), "returning a value from a void function");
    } else if (current_return_type) {
      auto *et = expr_type(*stmt.get_expression());
      if (et && !types_equal(et, current_return_type)) {
        type_error(stmt.get_location(),
                   std::format("return type mismatch: expected '{}', got '{}'",
                               current_return_type->toString(),
                               et->toString()));
      }
    }
  } else {
    if (current_return_type && !current_return_type->equals(*type::void_t())) {
      type_error(stmt.get_location(),
                 std::format("non-void function must return a value of "
                             "type '{}'",
                             current_return_type->toString()));
    }
  }
}

void type_check::TypeVisitor::visit(AssertStmt &stmt) {
  stmt.get_expression()->accept(*this);
  auto *et = expr_type(*stmt.get_expression());
  if (et && !et->equals(*type::bool_t())) {
    type_error(
        stmt.get_location(),
        std::format("assert condition must be bool, got '{}'", et->toString()));
  }
}

void type_check::TypeVisitor::visit(VariableDeclarationStatement &stmt) {
  auto *declared_type = resolve_type(stmt.get_type());

  if (declared_type && declared_type->equals(*type::void_t())) {
    type_error(stmt.get_location(), "variable cannot have void type");
  }

  if (stmt.get_initializer()) {
    stmt.get_initializer()->accept(*this);
    auto *init_type = expr_type(*stmt.get_initializer());
    if (init_type && declared_type && !types_equal(init_type, declared_type)) {
      type_error(stmt.get_location(),
                 std::format("initializer type mismatch: variable "
                             "declared as '{}', got '{}'",
                             declared_type->toString(), init_type->toString()));
    }
  }
}

void type_check::TypeVisitor::visit(UnaryMutationStatement &stmt) {
  stmt.get_target()->accept(*this);
  auto *t = lvalue_type(*stmt.get_target());
  t = resolve_underlying(t);
  if (t && !t->equals(*type::int_t())) {
    type_error(stmt.get_location(),
               std::format("increment/decrement requires int type, got '{}'",
                           t->toString()));
  }
}

void type_check::TypeVisitor::visit(AssignmentStatement &stmt) {
  stmt.get_lvalue()->accept(*this);
  stmt.get_expr()->accept(*this);

  auto *lhs_type = lvalue_type(*stmt.get_lvalue());
  auto *rhs_type = expr_type(*stmt.get_expr());

  if (stmt.get_op() != AssignmentOperator::Equals) {
    lhs_type = resolve_underlying(lhs_type);
    rhs_type = resolve_underlying(rhs_type);
    if (lhs_type && !lhs_type->equals(*type::int_t())) {
      type_error(stmt.get_location(),
                 std::format("compound assignment requires int type "
                             "on left side, got '{}'",
                             lhs_type->toString()));
    }
    if (rhs_type && !rhs_type->equals(*type::int_t())) {
      type_error(stmt.get_location(),
                 std::format("compound assignment requires int type "
                             "on right side, got '{}'",
                             rhs_type->toString()));
    }
  } else {
    // Plain assignment: types must match
    if (lhs_type && rhs_type && !types_equal(lhs_type, rhs_type)) {
      type_error(
          stmt.get_location(),
          std::format(
              "assignment type mismatch: left side is '{}', right side is '{}'",
              lhs_type->toString(), rhs_type->toString()));
    }
  }
}

void type_check::TypeVisitor::visit(ExpressionStatement &stmt) {
  stmt.get_expression()->accept(*this);
}

void type_check::TypeVisitor::visit(IfStatement &stmt) {
  stmt.get_condition()->accept(*this);
  auto *cond_type = expr_type(*stmt.get_condition());
  if (cond_type && !cond_type->equals(*type::bool_t())) {
    type_error(stmt.get_location(),
               std::format("if condition must be bool, got '{}'",
                           cond_type->toString()));
  }
  stmt.get_then_branch()->accept(*this);
  if (stmt.get_else_branch())
    stmt.get_else_branch()->accept(*this);
}

void type_check::TypeVisitor::visit(ForStatement &stmt) {
  if (stmt.get_init())
    stmt.get_init()->accept(*this);
  stmt.get_condition()->accept(*this);
  auto *cond_type = expr_type(*stmt.get_condition());
  if (cond_type && !cond_type->equals(*type::bool_t())) {
    type_error(stmt.get_location(),
               std::format("for loop condition must be bool, got '{}'",
                           cond_type->toString()));
  }
  if (stmt.get_increment())
    stmt.get_increment()->accept(*this);
  stmt.get_body()->accept(*this);
}

void type_check::TypeVisitor::visit(WhileStatement &stmt) {
  stmt.get_condition()->accept(*this);
  auto *cond_type = expr_type(*stmt.get_condition());
  if (cond_type && !cond_type->equals(*type::bool_t())) {
    type_error(stmt.get_location(),
               std::format("while condition must be bool, got '{}'",
                           cond_type->toString()));
  }
  stmt.get_body()->accept(*this);
}

void type_check::TypeVisitor::visit(ErrorStatement &stmt) {
  stmt.get_expr()->accept(*this);
  auto *et = expr_type(*stmt.get_expr());
  if (et && !et->equals(*type::string_t())) {
    type_error(stmt.get_location(),
               std::format("error statement requires string, got '{}'",
                           et->toString()));
  }
}

// ============================================================================
// Expressions
// ============================================================================

void type_check::TypeVisitor::visit(Expression &expr) {
  ASTVisitor::visit(expr);
}

void type_check::TypeVisitor::visit(NumericExpr &expr) {
  expr.set_resolved_type(std::shared_ptr<type::Type>(
      const_cast<type::BuiltinType *>(type::int_t()), [](type::Type *) {}));
}

void type_check::TypeVisitor::visit(StringLiteralExpr &expr) {
  expr.set_resolved_type(std::shared_ptr<type::Type>(
      const_cast<type::BuiltinType *>(type::string_t()), [](type::Type *) {}));
}

void type_check::TypeVisitor::visit(CharLiteralExpr &expr) {
  expr.set_resolved_type(std::shared_ptr<type::Type>(
      const_cast<type::BuiltinType *>(type::char_t()), [](type::Type *) {}));
}

void type_check::TypeVisitor::visit(BoolConstExpr &expr) {
  expr.set_resolved_type(std::shared_ptr<type::Type>(
      const_cast<type::BuiltinType *>(type::bool_t()), [](type::Type *) {}));
}

void type_check::TypeVisitor::visit(NullExpr &expr) {
  // null has pointer type (polymorphic — compatible with any pointer)
  // Use a special "null pointer" type; for simplicity, use PointerType(nullptr)
  auto null_type = std::make_shared<type::PointerType>(nullptr);
  expr.set_resolved_type(null_type);
}

void type_check::TypeVisitor::visit(VarExpr &expr) {
  if (expr.get_symbol()) {
    auto *t = expr.get_symbol()->get_type();
    if (t) {
      expr.set_resolved_type(std::shared_ptr<type::Type>(
          const_cast<type::Type *>(t), [](type::Type *) {}));
    }
  }
}

void type_check::TypeVisitor::visit(ParenthesisExpression &expr) {
  expr.get_expression()->accept(*this);
  auto rt = expr.get_expression()->get_resolved_type();
  if (rt)
    expr.set_resolved_type(rt);
}

void type_check::TypeVisitor::visit(BinaryOperatorExpression &expr) {
  expr.get_left_expression()->accept(*this);
  expr.get_right_expression()->accept(*this);

  auto *lhs = expr_type(*expr.get_left_expression());
  auto *rhs = expr_type(*expr.get_right_expression());
  lhs = resolve_underlying(lhs);
  rhs = resolve_underlying(rhs);

  if (!lhs || !rhs)
    return; // earlier error

  auto set_int = [&] {
    expr.set_resolved_type(std::shared_ptr<type::Type>(
        const_cast<type::BuiltinType *>(type::int_t()), [](type::Type *) {}));
  };
  auto set_bool = [&] {
    expr.set_resolved_type(std::shared_ptr<type::Type>(
        const_cast<type::BuiltinType *>(type::bool_t()), [](type::Type *) {}));
  };

  switch (expr.get_operator_kind()) {
  // Arithmetic: int x int -> int
  case BinaryOperator::Add:
  case BinaryOperator::Sub:
  case BinaryOperator::Mult:
  case BinaryOperator::Div:
  case BinaryOperator::Modulo:
    if (!lhs->equals(*type::int_t()))
      type_error(expr.get_left_expression()->get_location(),
                 std::format("arithmetic operator expects int, got '{}'",
                             lhs->toString()));
    if (!rhs->equals(*type::int_t()))
      type_error(expr.get_right_expression()->get_location(),
                 std::format("arithmetic operator expects int, got '{}'",
                             rhs->toString()));
    set_int();
    break;

  // Bitwise: int x int -> int
  case BinaryOperator::BitwiseAnd:
  case BinaryOperator::BitwiseOr:
  case BinaryOperator::BitwiseXor:
  case BinaryOperator::ShiftLeft:
  case BinaryOperator::ShiftRight:
    if (!lhs->equals(*type::int_t()))
      type_error(expr.get_left_expression()->get_location(),
                 std::format("bitwise operator expects int, got '{}'",
                             lhs->toString()));
    if (!rhs->equals(*type::int_t()))
      type_error(expr.get_right_expression()->get_location(),
                 std::format("bitwise operator expects int, got '{}'",
                             rhs->toString()));
    set_int();
    break;

  // Equality: small x small -> bool (same type)
  case BinaryOperator::Equal:
  case BinaryOperator::NotEqual:
    if (!is_small_type(lhs))
      type_error(expr.get_left_expression()->get_location(),
                 std::format("equality comparison not defined for type '{}'",
                             lhs->toString()));
    else if (!types_equal(lhs, rhs)) {
      // Allow comparing any pointer with null
      bool null_compare = (lhs->kind == type::Type::Kind::Pointer &&
                           rhs->kind == type::Type::Kind::Pointer);
      if (!null_compare)
        type_error(expr.get_location(),
                   std::format("equality comparison between incompatible types "
                               "'{}' and '{}'",
                               lhs->toString(), rhs->toString()));
    }
    set_bool();
    break;

  // Ordered comparison: int x int -> bool (or char x char)
  case BinaryOperator::LessThan:
  case BinaryOperator::LessThanOrEqual:
  case BinaryOperator::GreaterThan:
  case BinaryOperator::GreaterThanOrEqual:
    if (!lhs->equals(*type::int_t()) && !lhs->equals(*type::char_t()))
      type_error(expr.get_left_expression()->get_location(),
                 std::format("ordered comparison expects int or char, got '{}'",
                             lhs->toString()));
    if (!rhs->equals(*type::int_t()) && !rhs->equals(*type::char_t()))
      type_error(expr.get_right_expression()->get_location(),
                 std::format("ordered comparison expects int or char, got '{}'",
                             rhs->toString()));
    if (!types_equal(lhs, rhs))
      type_error(expr.get_location(),
                 std::format("ordered comparison between different types '{}' "
                             "and '{}'",
                             lhs->toString(), rhs->toString()));
    set_bool();
    break;

  // Logical: bool x bool -> bool
  case BinaryOperator::LogicalAnd:
  case BinaryOperator::LogicalOr:
    if (!lhs->equals(*type::bool_t()))
      type_error(expr.get_left_expression()->get_location(),
                 std::format("logical operator expects bool, got '{}'",
                             lhs->toString()));
    if (!rhs->equals(*type::bool_t()))
      type_error(expr.get_right_expression()->get_location(),
                 std::format("logical operator expects bool, got '{}'",
                             rhs->toString()));
    set_bool();
    break;

  case BinaryOperator::FieldAccess:
  case BinaryOperator::PointerAccess:
    // Handled by FieldAccessExpr/PointerAccessExpr
    break;

  case BinaryOperator::Unknown:
    type_error(expr.get_location(), "unknown binary operator");
    break;
  }
}

void type_check::TypeVisitor::visit(UnaryOperatorExpression &expr) {
  expr.get_expression()->accept(*this);
  auto *inner = expr_type(*expr.get_expression());
  inner = resolve_underlying(inner);

  if (!inner)
    return;

  switch (expr.get_operator_kind()) {
  case UnaryOperator::Neg:
    if (!inner->equals(*type::int_t()))
      type_error(
          expr.get_location(),
          std::format("negation expects int, got '{}'", inner->toString()));
    expr.set_resolved_type(std::shared_ptr<type::Type>(
        const_cast<type::BuiltinType *>(type::int_t()), [](type::Type *) {}));
    break;

  case UnaryOperator::BitwiseNot:
    if (!inner->equals(*type::int_t()))
      type_error(
          expr.get_location(),
          std::format("bitwise not expects int, got '{}'", inner->toString()));
    expr.set_resolved_type(std::shared_ptr<type::Type>(
        const_cast<type::BuiltinType *>(type::int_t()), [](type::Type *) {}));
    break;

  case UnaryOperator::LogicalNot:
    if (!inner->equals(*type::bool_t()))
      type_error(
          expr.get_location(),
          std::format("logical not expects bool, got '{}'", inner->toString()));
    expr.set_resolved_type(std::shared_ptr<type::Type>(
        const_cast<type::BuiltinType *>(type::bool_t()), [](type::Type *) {}));
    break;

  case UnaryOperator::Deref:
    if (inner->kind != type::Type::Kind::Pointer)
      type_error(expr.get_location(),
                 std::format("dereference expects pointer type, got '{}'",
                             inner->toString()));
    else {
      auto *ptr = dynamic_cast<const type::PointerType *>(inner);
      if (ptr && ptr->to) {
        expr.set_resolved_type(std::shared_ptr<type::Type>(
            const_cast<type::Type *>(ptr->to), [](type::Type *) {}));
      }
    }
    break;

  case UnaryOperator::Unknown:
    type_error(expr.get_location(), "unknown unary operator");
    break;
  }
}

void type_check::TypeVisitor::visit(CallExpr &expr) {
  // Visit all argument expressions first
  for (auto *arg : expr.get_params()) {
    arg->accept(*this);
  }

  // Look up the function symbol
  auto lookup = symbol_table->lookup(expr.get_function_name());
  if (!lookup)
    return; // semantic analysis already reported the error

  auto &sym = lookup->get();
  auto *func_type = sym.get_type();
  func_type = resolve_underlying(func_type);

  if (func_type && func_type->kind == type::Type::Kind::Function) {
    auto *ft = dynamic_cast<const type::FunctionType *>(func_type);
    if (ft) {
      // Check argument count
      if (expr.get_params().size() != ft->paramTypes.size()) {
        type_error(expr.get_location(),
                   std::format("function '{}' expects {} arguments, got {}",
                               expr.get_function_name(), ft->paramTypes.size(),
                               expr.get_params().size()));
      } else {
        // Check argument types
        for (size_t i = 0; i < expr.get_params().size(); ++i) {
          auto *arg_type = expr_type(*expr.get_params()[i]);
          if (arg_type && ft->paramTypes[i] &&
              !types_equal(arg_type, ft->paramTypes[i])) {
            type_error(expr.get_params()[i]->get_location(),
                       std::format("argument {} type mismatch: expected '{}', "
                                   "got '{}'",
                                   i + 1, ft->paramTypes[i]->toString(),
                                   arg_type->toString()));
          }
        }
      }

      // Set return type
      if (ft->returnType) {
        expr.set_resolved_type(std::shared_ptr<type::Type>(
            const_cast<type::Type *>(ft->returnType), [](type::Type *) {}));
      }
      return;
    }
  }

  // If the symbol type is not a FunctionType, use the return type from the
  // symbol directly (FunctionSymbol stores return_type as its type)
  if (func_type) {
    expr.set_resolved_type(std::shared_ptr<type::Type>(
        const_cast<type::Type *>(func_type), [](type::Type *) {}));
  }
}

void type_check::TypeVisitor::visit(ArrayAccessExpr &expr) {
  expr.get_array()->accept(*this);
  expr.get_index()->accept(*this);

  auto *idx = expr_type(*expr.get_index());
  idx = resolve_underlying(idx);
  if (idx && !idx->equals(*type::int_t())) {
    type_error(
        expr.get_index()->get_location(),
        std::format("array index must be int, got '{}'", idx->toString()));
  }

  auto *arr = expr_type(*expr.get_array());
  arr = resolve_underlying(arr);
  if (arr && arr->kind == type::Type::Kind::Array) {
    auto *at = dynamic_cast<const type::ArrayType *>(arr);
    if (at && at->elementType) {
      expr.set_resolved_type(std::shared_ptr<type::Type>(
          const_cast<type::Type *>(at->elementType), [](type::Type *) {}));
    }
  } else if (arr) {
    type_error(expr.get_array()->get_location(),
               std::format("subscript operator requires array type, got '{}'",
                           arr->toString()));
  }
}

void type_check::TypeVisitor::visit(PointerAccessExpr &expr) {
  expr.get_struct_pointer()->accept(*this);

  auto *base = expr_type(*expr.get_struct_pointer());
  base = resolve_underlying(base);

  if (!base)
    return;

  if (base->kind != type::Type::Kind::Pointer) {
    type_error(
        expr.get_struct_pointer()->get_location(),
        std::format("'->' requires pointer type, got '{}'", base->toString()));
    return;
  }

  auto *ptr = dynamic_cast<const type::PointerType *>(base);
  if (!ptr || !ptr->to)
    return;

  auto *pointee = resolve_underlying(ptr->to);
  if (!pointee || pointee->kind != type::Type::Kind::Struct) {
    type_error(expr.get_struct_pointer()->get_location(),
               std::format("'->' requires pointer to struct, got pointer "
                           "to '{}'",
                           pointee ? pointee->toString() : "unknown"));
    return;
  }

  auto *st = dynamic_cast<const type::StructType *>(pointee);
  if (!st)
    return;

  for (const auto &[fname, ftype] : st->fields) {
    if (fname == expr.get_field()) {
      if (ftype)
        expr.set_resolved_type(std::shared_ptr<type::Type>(
            const_cast<type::Type *>(ftype), [](type::Type *) {}));
      return;
    }
  }

  type_error(expr.get_location(), std::format("struct '{}' has no field '{}'",
                                              st->name, expr.get_field()));
}

void type_check::TypeVisitor::visit(FieldAccessExpr &expr) {
  expr.get_struct()->accept(*this);

  auto *base = expr_type(*expr.get_struct());
  base = resolve_underlying(base);

  if (!base)
    return;

  if (base->kind != type::Type::Kind::Struct) {
    type_error(
        expr.get_struct()->get_location(),
        std::format("'.' requires struct type, got '{}'", base->toString()));
    return;
  }

  auto *st = dynamic_cast<const type::StructType *>(base);
  if (!st)
    return;

  for (const auto &[fname, ftype] : st->fields) {
    if (fname == expr.get_field()) {
      if (ftype)
        expr.set_resolved_type(std::shared_ptr<type::Type>(
            const_cast<type::Type *>(ftype), [](type::Type *) {}));
      return;
    }
  }

  type_error(expr.get_location(), std::format("struct '{}' has no field '{}'",
                                              st->name, expr.get_field()));
}

void type_check::TypeVisitor::visit(AllocExpression &expr) {
  auto *alloc_type = resolve_type(expr.get_type());
  if (alloc_type) {
    auto ptr_type = std::make_shared<type::PointerType>(alloc_type);
    expr.set_resolved_type(ptr_type);
  }
}

void type_check::TypeVisitor::visit(AllocArrayExpression &expr) {
  expr.get_size()->accept(*this);

  auto *size_type = expr_type(*expr.get_size());
  size_type = resolve_underlying(size_type);
  if (size_type && !size_type->equals(*type::int_t())) {
    type_error(expr.get_size()->get_location(),
               std::format("alloc_array size must be int, got '{}'",
                           size_type->toString()));
  }

  auto *elem_type = resolve_type(expr.get_type());
  if (elem_type) {
    auto arr_type = std::make_shared<type::ArrayType>(elem_type);
    expr.set_resolved_type(arr_type);
  }
}

void type_check::TypeVisitor::visit(TernaryExpression &expr) {
  expr.get_condition()->accept(*this);
  expr.get_then()->accept(*this);
  expr.get_else()->accept(*this);

  auto *cond = expr_type(*expr.get_condition());
  cond = resolve_underlying(cond);
  if (cond && !cond->equals(*type::bool_t())) {
    type_error(expr.get_condition()->get_location(),
               std::format("ternary condition must be bool, got '{}'",
                           cond->toString()));
  }

  auto *then_type = expr_type(*expr.get_then());
  auto *else_type = expr_type(*expr.get_else());
  if (then_type && else_type && !types_equal(then_type, else_type)) {
    type_error(expr.get_location(),
               std::format("ternary branches must have same type: '{}' "
                           "vs '{}'",
                           then_type->toString(), else_type->toString()));
  }

  // Result type is the type of the then branch
  auto rt = expr.get_then()->get_resolved_type();
  if (rt)
    expr.set_resolved_type(rt);
}

// ============================================================================
// Type annotations — no-ops, actual resolution happens in resolve_type
// ============================================================================

void type_check::TypeVisitor::visit(TypeAnnotation &type) {}
void type_check::TypeVisitor::visit(BuiltinTypeAnnotation &type) {}
void type_check::TypeVisitor::visit(NamedTypeAnnotation &type) {}
void type_check::TypeVisitor::visit(StructTypeAnnotation &type) {}
void type_check::TypeVisitor::visit(PointerTypeAnnotation &type) {}
void type_check::TypeVisitor::visit(ArrayTypeAnnotation &type) {}

// ============================================================================
// LValues — visited for side effects (accept already called in statements)
// ============================================================================

void type_check::TypeVisitor::visit(LValue &val) {}
void type_check::TypeVisitor::visit(VariableLValue &val) {}
void type_check::TypeVisitor::visit(ArrayAccessLValue &val) {
  val.get_index()->accept(*this);
  auto *idx = expr_type(*val.get_index());
  idx = resolve_underlying(idx);
  if (idx && !idx->equals(*type::int_t())) {
    type_error(
        val.get_index()->get_location(),
        std::format("array index must be int, got '{}'", idx->toString()));
  }
}
void type_check::TypeVisitor::visit(PointerAccessLValue &val) {}
void type_check::TypeVisitor::visit(FieldAccessLValue &val) {}
void type_check::TypeVisitor::visit(DereferenceLValue &val) {}
