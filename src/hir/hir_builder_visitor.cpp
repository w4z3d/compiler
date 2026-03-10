#include "hir_builder_visitor.hpp"
#include "hir_builder.hpp"
#include <cassert>
#include <cstdint>
hir::type::Type *HIRBuilderVisitor::lower_type(const type::Type *ast_type) {
  assert(ast_type && "null ast type");
  switch (ast_type->kind) {
  case type::Type::Kind::Builtin: {
    const auto *b = dynamic_cast<const type::BuiltinType *>(ast_type);
    switch (b->get_kind()) {
    case type::BuiltinType::BuiltinKind::Int:
      return types.i32();
    case type::BuiltinType::BuiltinKind::Bool:
      return types.i1();
    case type::BuiltinType::BuiltinKind::Char:
      return types.i8();
    case type::BuiltinType::BuiltinKind::Void:
      return types.void_t();
    case type::BuiltinType::BuiltinKind::String:
      return types.ptr();
    }
  }
  case type::Type::Kind::Pointer:
    return types.ptr();
  case type::Type::Kind::Array: {
    const auto *a = dynamic_cast<const type::ArrayType *>(ast_type);
    return types.get_array(a->get_len(), lower_type(a->elementType));
  }
  case type::Type::Kind::Struct: {
    const auto *s = dynamic_cast<const type::StructType *>(ast_type);
    return types.get_struct(s->name);
  }
  case type::Type::Kind::Named: {
    const auto *n = dynamic_cast<const type::NamedType *>(ast_type);

    assert(n->type && "unresolved named type");
    return lower_type(n->type);
  }
  default:
    throw std::runtime_error("cannot lower type: unknown kind");
  }
}
hir::type::Type *HIRBuilderVisitor::type_of_symbol(size_t symbol_id) {
  auto sym = symbol_table->lookup_by_id(symbol_id);
  assert(sym && "symbol not found");
  return lower_type(sym->get().get_type());
}
// Write a variable definition for a symbol in a block
void HIRBuilderVisitor::write_variable(size_t symbol_id, hir::BasicBlock *bb,
                                       hir::Value *val) {
  current_defs[symbol_id][bb] = val;
}

// Read the current SSA value for a symbol in a block
hir::Value *HIRBuilderVisitor::read_variable(size_t symbol_id,
                                             hir::BasicBlock *bb) {
  if (current_defs.count(symbol_id) && current_defs[symbol_id].count(bb)) {
    return current_defs[symbol_id][bb];
  }
  // Not found locally — search predecessors
  return read_variable_recursive(symbol_id, bb);
}

hir::Value *HIRBuilderVisitor::read_variable_recursive(size_t symbol_id,
                                                       hir::BasicBlock *bb) {
  hir::Value *result = nullptr;

  if (!sealed_blocks.count(bb)) {
    auto *saved_block = current_block;
    builder.set_insert_point(bb);

    hir::PhiNode *phi = builder.build_phi(type_of_symbol(symbol_id));

    builder.set_insert_point(saved_block);

    incomplete_phis[bb][symbol_id] = phi;
    write_variable(symbol_id, bb, phi);
    result = phi;
  } else if (bb->predecessors.size() == 1) {
    const auto &preds = bb->predecessors;
    result = read_variable(symbol_id, preds[0]);
  } else {
    const auto &preds = bb->predecessors;
    auto *saved_block = current_block;
    builder.set_insert_point(bb);

    hir::PhiNode *phi = builder.build_phi(type_of_symbol(symbol_id));
    builder.set_insert_point(saved_block);
    result = phi;
    write_variable(symbol_id, bb, phi);
    result = add_phi_operands(symbol_id, phi, bb->predecessors);
    write_variable(symbol_id, bb, result);
  }

  write_variable(symbol_id, bb, result);
  return result;
}

hir::Value *HIRBuilderVisitor::add_phi_operands(
    size_t var, hir::PhiNode *phi,
    const std::vector<hir::BasicBlock *> &predecessors) {
  phi->operands.reserve(predecessors.size());
  for (const auto &pred : predecessors) {
    phi->add_incoming(read_variable(var, pred), pred);
  }
  return try_remove_trivial_phi(phi);
}

void HIRBuilderVisitor::seal_block(hir::BasicBlock *bb) {
  if (sealed_blocks.count(bb))
    return;

  if (incomplete_phis.count(bb)) {
    for (auto &[symbol_id, phi] : incomplete_phis[bb]) {
      hir::Value *result = add_phi_operands(symbol_id, phi, bb->predecessors);
      write_variable(symbol_id, bb, result);
    }
    incomplete_phis.erase(bb);
  }

  sealed_blocks.insert(bb);
}

hir::Value *HIRBuilderVisitor::try_remove_trivial_phi(hir::PhiNode *phi) {
  hir::Value *same = nullptr;

  for (auto &use : phi->operands) {
    hir::Value *op = use.value;
    if (op == same || op == phi) // Unique value or self reference
      continue;
    if (same != nullptr)
      return phi; // merges two values, not trivial
    same = op;
  }

  if (same == nullptr)
    same =
        module.get_undef(phi->type); // Phi is unreachable or in the start block

  std::vector<hir::PhiNode *> phi_users;
  for (auto *use : phi->users) {
    if (use->user != phi && use->user->opcode == hir::Opcode::Phi) {
      if (auto *p = dynamic_cast<hir::PhiNode *>(use->user))
        phi_users.push_back(p);
    }
  }
  phi->replace_all_uses_with(same);
  phi->erase_from_parent();
  // Update current_defs to replace the removed phi
  // Recursively try to simplify phi users
  for (auto *user : phi_users) {
    try_remove_trivial_phi(user);
  }

  return same;
}
void HIRBuilderVisitor::visit(IfStatement &stmt) {
  stmt.get_condition()->accept(*this);
  hir::Value *cond = pop_stack();

  if (cond->type != types.i1()) {
    cond = builder.build_icmp(hir::ICmpPredicate::NE, cond,
                              module.const_int(types.i32(), 0), "cond");
  }

  hir::BasicBlock *then_bb = current_function->append_block();
  hir::BasicBlock *else_bb = current_function->append_block();
  hir::BasicBlock *merge_bb = current_function->append_block();

  then_bb->predecessors.push_back(current_block);
  else_bb->predecessors.push_back(current_block);

  builder.build_cond_br(cond, then_bb, else_bb);

  set_insert_point(then_bb);
  seal_block(then_bb); // then_bb has exactly one predecessor, seal immediately
  stmt.get_then_branch()->accept(*this);
  // Block may have changed due to nested control flow
  // so use current_block rather than then_bb for the trailing branch
  if (!current_block->terminator()) {
    merge_bb->predecessors.push_back(current_block);
    builder.build_br(merge_bb);
  }

  set_insert_point(else_bb);
  seal_block(else_bb); // else_bb also has exactly one predecessor
  if (stmt.get_else_branch()) {
    stmt.get_else_branch()->accept(*this);
  }
  if (!current_block->terminator()) {
    merge_bb->predecessors.push_back(current_block);
    builder.build_br(merge_bb);
  }

  set_insert_point(merge_bb);
  seal_block(merge_bb);
}

void HIRBuilderVisitor::visit(VariableDeclarationStatement &stmt) {
  auto sym = stmt.get_symbol().get();

  if (stmt.get_initializer()) {
    stmt.get_initializer()->accept(*this);
    hir::Value *init_val = pop_stack();
    write_variable(sym->get_id(), current_block, init_val);
  } else {
    hir::type::Type *hir_type = lower_type(sym->get_type());
    write_variable(sym->get_id(), current_block, module.get_undef(hir_type));
  }
}
void HIRBuilderVisitor::visit(VarExpr &expr) {
  auto symbol = expr.get_symbol();
  assert(symbol && "Symbol is nullptr");
  // Always use SSA — read_variable handles cross-block cases
  // and inserts phi nodes where needed
  hir::Value *val = read_variable(symbol->get_id(), current_block);
  value_stack.push(val);
}
void HIRBuilderVisitor::visit(AssignmentStatement &stmt) {
  stmt.get_expr()->accept(*this); // Pushes result SSA Var
  const auto rhs_var = pop_stack();

  if (stmt.get_lvalue()->get_kind() == LValue::Kind::Variable) {
    auto *var_lval = dynamic_cast<VariableLValue *>(stmt.get_lvalue());
    size_t symbol_id = var_lval->get_symbol()->get_id();

    if (stmt.get_op() == AssignmentOperator::Equals) {
      write_variable(symbol_id, current_block, rhs_var);
    } else {
      hir::Value *lhs = read_variable(symbol_id, current_block);
      hir::Value *result;
      switch (stmt.get_op()) {
      case AssignmentOperator::Plus:
        result = builder.build_add(lhs, rhs_var);
        break;
      case AssignmentOperator::Minus:
        result = builder.build_sub(lhs, rhs_var);
        break;
      case AssignmentOperator::Mult:
        result = builder.build_mul(lhs, rhs_var);
        break;
      case AssignmentOperator::Div:
        result = builder.build_sdiv(lhs, rhs_var);
        break;
      case AssignmentOperator::Modulo:
        result = builder.build_srem(lhs, rhs_var);
        break;
      case AssignmentOperator::LShift:
        result = builder.build_shl(lhs, rhs_var);
        break;
      case AssignmentOperator::RShift:
        result = builder.build_ashr(lhs, rhs_var);
        break;
      case AssignmentOperator::BitwiseAnd:
        result = builder.build_and(lhs, rhs_var);
        break;
      case AssignmentOperator::BitwiseOr:
        result = builder.build_or(lhs, rhs_var);
        break;
      case AssignmentOperator::BitwiseXor:
        result = builder.build_xor(lhs, rhs_var);
        break;
      default:
        assert(false && "unhandled compound assignment");
      }
      write_variable(symbol_id, current_block, result);
    }
  } else if (stmt.get_lvalue()->get_kind() == LValue::Kind::Array) {
    auto *arr_lval = dynamic_cast<ArrayAccessLValue *>(stmt.get_lvalue());

    auto *base_lval = arr_lval->get_base();
    while (base_lval->get_kind() == LValue::Kind::Array) {
      base_lval = dynamic_cast<ArrayAccessLValue *>(base_lval)->get_base();
    }
    if (auto *var_lval = dynamic_cast<VariableLValue *>(base_lval)) {

      auto sym = var_lval->get_symbol();

      assert(sym && "Symbol pointer is null");

      auto *arr_type = dynamic_cast<const type::ArrayType *>(sym->get_type());
      auto *elem_type = lower_type(arr_type->elementType);

      hir::Value *base =
          read_variable(var_lval->get_symbol()->get_id(), current_block);
      arr_lval->get_index()->accept(*this);
      auto *index = pop_stack();

      auto *ptr = builder.build_gep(elem_type, base, {index});
      builder.build_store(rhs_var, ptr);
    } else {
      std::cerr << "Array LVar does not contain Var L Var" << std::endl;
    }
  }
}

void HIRBuilderVisitor::visit(Typedef &typedef_) {
  ASTVisitor::visit(typedef_);
}
void HIRBuilderVisitor::visit(FunctionDeclaration &decl) {
  // Build the function type from the declaration
  std::vector<hir::type::Type *> param_types;
  for (const auto &param : decl.get_parameter_declarations()) {
    auto sym = param->get_symbol();
    param_types.push_back(lower_type(sym->get_type()));
  }

  const auto ret_type =
      lower_type(from_type_annotation(decl.get_return_type()));

  hir::type::FunctionType *fn_type =
      types.get_function(ret_type, param_types, false);

  current_function = module.add_function(std::string(decl.get_name()), fn_type);

  hir::BasicBlock *entry = current_function->entry();
  set_insert_point(entry);

  seal_block(entry);

  for (size_t i = 0; i < decl.get_parameter_declarations().size(); i++) {
    auto sym = decl.get_parameter_declarations()[i]->get_symbol();
    assert(sym && "parameter has no symbol!");

    hir::Argument *arg = current_function->arguments[i];
    arg->name = std::string(sym->get_name());

    write_variable(sym->get_id(), entry, arg);
  }

  decl.get_body()->accept(*this);
  if (!current_block->terminator()) {
    if (ret_type->is_void()) {
      builder.build_ret(nullptr);
    } else {
      diagnostics->emit_warning({}, "non-void function may not return a value");
      builder.build_ret(module.get_undef(ret_type));
    }
  }

  reset_ssa_state();
}
void HIRBuilderVisitor::visit(TranslationUnit &unit) {
  for (const auto &decl : unit.get_declarations()) {
    decl->accept(*this);
  }
}
void HIRBuilderVisitor::visit(ParameterDeclaration &decl) {}
void HIRBuilderVisitor::visit(StructDeclaration &decl) {
  ASTVisitor::visit(decl);
}
void HIRBuilderVisitor::visit(CompoundStmt &stmt) {
  assert(current_function && "no current function");
  assert(current_block && "no current block");

  for (const auto &s : stmt.get_statements()) {
    s->accept(*this);
    if (current_block->terminator())
      break;
  }
}
void HIRBuilderVisitor::visit(ReturnStmt &stmt) {
  if (stmt.get_expression()) {
    stmt.get_expression()->accept(*this);
    hir::Value *val = pop_stack();
    builder.build_ret(val);
  } else {
    builder.build_ret(nullptr);
  }
}
void HIRBuilderVisitor::visit(AssertStmt &stmt) { ASTVisitor::visit(stmt); }
void HIRBuilderVisitor::visit(UnaryMutationStatement &stmt) {
  auto *lval = dynamic_cast<VariableLValue *>(stmt.get_target());
  assert(lval && "only variable lvalues supported for now");

  size_t symbol_id = lval->get_symbol()->get_id();
  hir::Value *current = read_variable(symbol_id, current_block);

  hir::Value *one = module.const_int(types.i32(), 1);
  hir::Value *result;

  switch (stmt.get_operation()) {
  case UnaryMutationStatement::Op::PostIncrement:
    result = builder.build_add(current, one);
    break;
  case UnaryMutationStatement::Op::PostDecrement:
    result = builder.build_sub(current, one);
    break;
  }

  write_variable(symbol_id, current_block, result);
}
void HIRBuilderVisitor::visit(ExpressionStatement &stmt) {
  stmt.get_expression()->accept(*this);
}
void HIRBuilderVisitor::visit(ForStatement &stmt) {
  if (stmt.get_init()) {
    stmt.get_init()->accept(*this);
  }

  hir::BasicBlock *cond_bb = current_function->append_block();
  hir::BasicBlock *body_bb = current_function->append_block();
  hir::BasicBlock *inc_bb = current_function->append_block();
  hir::BasicBlock *merge_bb = current_function->append_block();

  cond_bb->predecessors.push_back(current_block);
  builder.build_br(cond_bb);

  cond_bb->predecessors.push_back(inc_bb); // back edge from increment
  set_insert_point(cond_bb);

  if (stmt.get_condition()) {
    stmt.get_condition()->accept(*this);
    hir::Value *cond = pop_stack();

    if (cond->type != types.i1()) {
      cond = builder.build_icmp(hir::ICmpPredicate::NE, cond,
                                module.const_int(types.i32(), 0));
    }

    body_bb->predecessors.push_back(cond_bb);
    merge_bb->predecessors.push_back(cond_bb);
    builder.build_cond_br(cond, body_bb, merge_bb);
  } else {
    body_bb->predecessors.push_back(cond_bb);
    builder.build_br(body_bb);
  }

  set_insert_point(body_bb);
  seal_block(body_bb); // only one predecessor: cond_bb
  stmt.get_body()->accept(*this);
  if (!current_block->terminator()) {
    inc_bb->predecessors.push_back(current_block);
    builder.build_br(inc_bb);
  }

  set_insert_point(inc_bb);
  seal_block(
      inc_bb); // only one predecessor: body (or current_block after body)
  if (stmt.get_increment()) {
    stmt.get_increment()->accept(*this);
  }
  builder.build_br(cond_bb);

  seal_block(cond_bb);

  set_insert_point(merge_bb);
  seal_block(merge_bb);
}
void HIRBuilderVisitor::visit(WhileStatement &stmt) {
  auto *condition_block = current_function->append_block();
  auto *body_block = current_function->append_block();
  auto *merge_block = current_function->append_block();

  condition_block->predecessors.push_back(current_block);
  builder.build_br(condition_block);

  set_insert_point(condition_block);
  stmt.get_condition()->accept(*this);
  auto *cond = pop_stack();

  body_block->predecessors.push_back(condition_block);
  merge_block->predecessors.push_back(condition_block);
  builder.build_cond_br(cond, body_block, merge_block);

  set_insert_point(body_block);
  seal_block(body_block);
  stmt.get_body()->accept(*this);
  if (!current_block->terminator()) {
    condition_block->predecessors.push_back(current_block);
    builder.build_br(condition_block);
  }
  seal_block(condition_block);

  set_insert_point(merge_block);
  seal_block(merge_block);
}
void HIRBuilderVisitor::visit(ErrorStatement &stmt) { ASTVisitor::visit(stmt); }
void HIRBuilderVisitor::visit(NumericExpr &expr) {

  auto num = expr.try_parse<std::uint32_t>().value();
  if (expr.get_base() == NumericExpr::Base::Hexadecimal && num > 0x7FFFFFFF) {
    num = ~num + 1;
  }

  value_stack.push(module.const_int(builder.context.i32(), num));
}
void HIRBuilderVisitor::visit(CallExpr &expr) {
  const auto symbol = symbol_table->lookup(expr.get_function_name());
  assert(symbol && "Could not find function symbol");
  std::vector<hir::Value *> args;
  for (const auto &arg : expr.get_params()) {
    arg->accept(*this);
    args.push_back(pop_stack());
  }

  hir::Function *callee = module.get_function(expr.get_function_name());
  assert(callee && "unknown function");

  hir::Value *result = builder.build_call(callee, args);

  if (!result->type->is_void())
    value_stack.push(result);
}
void HIRBuilderVisitor::visit(StringLiteralExpr &expr) {
  ASTVisitor::visit(expr);
  // TODO: Alloc char array, then immediately load. (Idk if this is stack
  // allocatable but iirc not)
}
void HIRBuilderVisitor::visit(CharLiteralExpr &expr) {
  ASTVisitor::visit(expr);
}
void HIRBuilderVisitor::visit(BoolConstExpr &expr) {
  auto *bool_ = module.const_bool(expr.get_value());
  value_stack.push(bool_);
}
void HIRBuilderVisitor::visit(NullExpr &expr) {
  value_stack.push(module.const_int(types.i64(), 0));
}
void HIRBuilderVisitor::visit(ParenthesisExpression &expr) {
  expr.get_expression()->accept(*this);
}
void HIRBuilderVisitor::visit(BinaryOperatorExpression &expr) {
  expr.get_left_expression()->accept(*this);
  auto *lhs = pop_stack();

  expr.get_right_expression()->accept(*this);
  auto *rhs = pop_stack();

  hir::Value *result;
  switch (expr.get_operator_kind()) {
  case BinaryOperator::Add:
    result = builder.build_add(lhs, rhs);
    break;
  case BinaryOperator::Sub:
    result = builder.build_sub(lhs, rhs);
    break;
  case BinaryOperator::Div:
    result = builder.build_sdiv(lhs, rhs);
    break;
  case BinaryOperator::Mult:
    result = builder.build_mul(lhs, rhs);
    break;
  case BinaryOperator::Modulo:
    result = builder.build_srem(lhs, rhs);
    break;
  case BinaryOperator::LessThan:
    result = builder.build_icmp(hir::ICmpPredicate::SLT, lhs, rhs);
    break;
  case BinaryOperator::GreaterThan:
    result = builder.build_icmp(hir::ICmpPredicate::SGT, lhs, rhs);
    break;
  case BinaryOperator::GreaterThanOrEqual:
    result = builder.build_icmp(hir::ICmpPredicate::SGE, lhs, rhs);
    break;
  case BinaryOperator::LessThanOrEqual:
    result = builder.build_icmp(hir::ICmpPredicate::SLE, lhs, rhs);
    break;
  case BinaryOperator::Equal:
    result = builder.build_icmp(hir::ICmpPredicate::EQ, lhs, rhs);
    break;
  case BinaryOperator::NotEqual:
    result = builder.build_icmp(hir::ICmpPredicate::NE, lhs, rhs);
    break;
  case BinaryOperator::LogicalAnd:
    result = builder.build_and(lhs, rhs);
    break;
  case BinaryOperator::LogicalOr:
    result = builder.build_or(lhs, rhs);
    break;
  default:

    break;
  }

  value_stack.push(result);
}
void HIRBuilderVisitor::visit(UnaryOperatorExpression &expr) {
  expr.get_expression()->accept(*this);
  auto *expr_val = pop_stack();

  hir::Value *result;
  switch (expr.get_operator_kind()) {
  case UnaryOperator::Neg:
    result = builder.build_neg(expr_val);
    break;
  case UnaryOperator::Deref:
    result = builder.build_load(expr_val->type, expr_val);
    break;
  case UnaryOperator::BitwiseNot:
  case UnaryOperator::LogicalNot:
    result = builder.build_not(expr_val);
    break;
  default:
    result = module.get_undef(expr_val->type);
    break;
  }
  value_stack.push(result);
}
void HIRBuilderVisitor::visit(ArrayAccessExpr &expr) {
  expr.get_array()->accept(*this);
  auto *base = pop_stack();

  expr.get_index()->accept(*this);
  auto *index = pop_stack();

  hir::type::Type *elem_type;
  if (auto *arr_ty = dynamic_cast<hir::type::ArrayType *>(base->type)) {
    elem_type = arr_ty->inner_type;
  }
  auto *ptr = builder.build_gep(elem_type, base, {index});
  auto *val = builder.build_load(elem_type, ptr);

  value_stack.push(val);
}
void HIRBuilderVisitor::visit(PointerAccessExpr &expr) {
  ASTVisitor::visit(expr);
}
void HIRBuilderVisitor::visit(FieldAccessExpr &expr) {
  ASTVisitor::visit(expr);
}
void HIRBuilderVisitor::visit(AllocExpression &expr) {
  hir::type::Type *alloc_type =
      lower_type(from_type_annotation(expr.get_type()));
  size_t size = alloc_type->size_of();

  auto *malloc = module.get_function("malloc");
  if (!malloc) {
    malloc = module.add_function(
        "malloc", types.get_function(types.ptr(), {types.ptr()}, false));
    malloc->is_extern = true;
  }
  hir::Value *size_val = module.const_int(types.i32(), size);
  hir::Value *ptr = builder.build_call(malloc, {size_val});

  value_stack.push(ptr);
}
void HIRBuilderVisitor::visit(AllocArrayExpression &expr) {
  auto *ty = lower_type(from_type_annotation(expr.get_type()));
  expr.get_size()->accept(*this);
  auto *size_val = pop_stack();

  auto *size_const = module.const_int(types.i32(), ty->size_of());
  auto *mul = builder.build_mul(size_const, size_val);

  auto *malloc = module.get_function("malloc");
  if (!malloc) {
    malloc = module.add_function(
        "malloc", types.get_function(types.ptr(), {types.ptr()}, false));
    malloc->is_extern = true;
  }
  hir::Value *ptr = builder.build_call(malloc, {mul});

  value_stack.push(ptr);
}
void HIRBuilderVisitor::visit(TernaryExpression &expr) {
  expr.get_condition()->accept(*this);
  const auto condition = pop_stack();

  hir::BasicBlock *then_bb = current_function->append_block();
  hir::BasicBlock *else_bb = current_function->append_block();
  hir::BasicBlock *merge_bb = current_function->append_block();

  then_bb->predecessors.push_back(current_block);
  else_bb->predecessors.push_back(current_block);

  builder.build_cond_br(condition, then_bb, else_bb);

  set_insert_point(then_bb);
  seal_block(then_bb); // then_bb has exactly one predecessor, seal immediately
  expr.get_then()->accept(*this);
  // Block may have changed due to nested control flow
  // so use current_block rather than then_bb for the trailing branch
  if (!current_block->terminator()) {
    merge_bb->predecessors.push_back(current_block);
    builder.build_br(merge_bb);
  }

  set_insert_point(else_bb);
  seal_block(else_bb); // else_bb also has exactly one predecessor
  if (expr.get_else()) {
    expr.get_else()->accept(*this);
  }
  if (!current_block->terminator()) {
    merge_bb->predecessors.push_back(current_block);
    builder.build_br(merge_bb);
  }

  // Seal merge_bb only after both branches are done
  // so all predecessors are known
  set_insert_point(merge_bb);
  seal_block(merge_bb);
}
void HIRBuilderVisitor::visit(BuiltinTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void HIRBuilderVisitor::visit(NamedTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void HIRBuilderVisitor::visit(StructTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void HIRBuilderVisitor::visit(PointerTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void HIRBuilderVisitor::visit(ArrayTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void HIRBuilderVisitor::visit(VariableLValue &val) { ASTVisitor::visit(val); }
void HIRBuilderVisitor::visit(ArrayAccessLValue &val) {
  ASTVisitor::visit(val);
}
void HIRBuilderVisitor::visit(PointerAccessLValue &val) {
  ASTVisitor::visit(val);
}
void HIRBuilderVisitor::visit(FieldAccessLValue &val) {
  ASTVisitor::visit(val);
}
void HIRBuilderVisitor::visit(DereferenceLValue &val) {
  ASTVisitor::visit(val);
}
