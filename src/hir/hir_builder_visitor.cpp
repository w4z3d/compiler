#include "hir_builder_visitor.hpp"
#include "hir_builder.hpp"
#include "hir_type.hpp"
#include <cassert>
#include <cstdint>
#include <stdexcept>

const source_type::StructType *
HIRBuilderVisitor::get_struct_from_source_type(const source_type::Type *t) {
  t = src_types.resolve_through_typedefs(t);
  assert(t && t->is_struct() && "expected struct type");
  return static_cast<const source_type::StructType *>(t);
}

const source_type::StructType *
HIRBuilderVisitor::get_struct_through_pointer(const source_type::Type *t) {
  t = src_types.resolve_through_typedefs(t);
  assert(t && t->is_pointer() && "expected pointer type");
  auto *pt = static_cast<const source_type::PointerType *>(t);
  auto *pointee = src_types.resolve_through_typedefs(pt->pointee.unqualified());
  assert(pointee && pointee->is_struct() && "expected pointer to struct");
  return static_cast<const source_type::StructType *>(pointee);
}

FieldInfo
HIRBuilderVisitor::find_struct_field(const source_type::StructType *st,
                                     std::string_view field_name) {
  for (size_t i = 0; i < st->fields.size(); i++) {
    if (st->fields[i].first == field_name) {
      auto *src_type = st->fields[i].second.unqualified();
      return {i, lower_type(src_type), src_type};
    }
  }
  assert(false && "unknown field");
  return {0, nullptr, nullptr};
}

const source_type::Type *
HIRBuilderVisitor::get_source_lvalue_type(LValue *lval) {
  switch (lval->get_kind()) {

  case LValue::Kind::Variable: {
    auto *var = static_cast<VariableLValue *>(lval);
    assert(var->get_symbol() && "null symbol in lvalue");
    return src_types.resolve_through_typedefs(var->get_symbol()->get_type());
  }

  case LValue::Kind::Array: {
    auto *arr = static_cast<ArrayAccessLValue *>(lval);
    auto *base_type = get_source_lvalue_type(arr->get_base());
    base_type = src_types.resolve_through_typedefs(base_type);
    if (base_type->is_array()) {
      auto *at = static_cast<const source_type::ArrayType *>(base_type);
      return at->element;
    }
    // Pointer arithmetic fallback
    if (base_type->is_pointer()) {
      auto *pt = static_cast<const source_type::PointerType *>(base_type);
      return pt->pointee.unqualified();
    }
    assert(false && "array access on non-array, non-pointer");
    return nullptr;
  }

  case LValue::Kind::Field: {
    auto *field_lval = static_cast<FieldAccessLValue *>(lval);
    auto *base_type = get_source_lvalue_type(field_lval->get_base());
    auto *st = get_struct_from_source_type(base_type);
    for (auto &[name, qt] : st->fields) {
      if (name == field_lval->get_field())
        return src_types.resolve_through_typedefs(qt.unqualified());
    }
    assert(false && "unknown field in field access lvalue");
    return nullptr;
  }

  case LValue::Kind::Pointer: {
    auto *ptr_lval = static_cast<PointerAccessLValue *>(lval);
    auto *base_type = get_source_lvalue_type(ptr_lval->get_base());
    auto *st = get_struct_through_pointer(base_type);
    for (auto &[name, qt] : st->fields) {
      if (name == ptr_lval->get_field())
        return src_types.resolve_through_typedefs(qt.unqualified());
    }
    assert(false && "unknown field in pointer access lvalue");
    return nullptr;
  }

  case LValue::Kind::Dereference: {
    auto *deref = static_cast<DereferenceLValue *>(lval);
    auto *base_type = get_source_lvalue_type(deref->get_operand());
    base_type = src_types.resolve_through_typedefs(base_type);
    assert(base_type->is_pointer() && "deref of non-pointer");
    auto *pt = static_cast<const source_type::PointerType *>(base_type);
    return src_types.resolve_through_typedefs(pt->pointee.unqualified());
  }
  }

  assert(false && "unhandled lvalue kind in get_source_lvalue_type");
  return nullptr;
}
hir::Value *HIRBuilderVisitor::resolve_lvalue_to_ptr(LValue *lval) {
  switch (lval->get_kind()) {

  case LValue::Kind::Variable: {
    auto *var = static_cast<VariableLValue *>(lval);
    assert(var->get_symbol() && "null symbol");
    return read_variable(var->get_symbol()->get_id(), current_block);
  }

  case LValue::Kind::Array: {
    auto *arr = static_cast<ArrayAccessLValue *>(lval);
    auto *base_ptr = resolve_lvalue_to_ptr(arr->get_base());
    auto *base_src_type = get_source_lvalue_type(arr->get_base());

    arr->get_index()->accept(*this);
    auto *index = pop_stack();

    if (base_src_type->is_array() || base_src_type->is_pointer()) {

      auto *at = static_cast<const source_type::ArrayType *>(base_src_type);
      auto *elem_ir_type = lower_type(at->element);
      return builder.build_gep(elem_ir_type, base_ptr, {index});
    }
  }

  case LValue::Kind::Field: {
    auto *field_lval = static_cast<FieldAccessLValue *>(lval);

    // Recurse: get pointer to the struct
    auto *base = resolve_lvalue_to_ptr(field_lval->get_base());

    // Get struct info from source types
    auto *base_src_type = get_source_lvalue_type(field_lval->get_base());
    auto *st = get_struct_from_source_type(base_src_type);
    auto field = find_struct_field(st, field_lval->get_field());

    auto *index = module.const_int(types.i64(), (int64_t)field.index);
    return builder.build_gep(types.i64(), base, {index});
  }

  case LValue::Kind::Pointer: {
    auto *ptr_lval = static_cast<PointerAccessLValue *>(lval);

    // Recurse: get the pointer value
    auto *base = resolve_lvalue_to_ptr(ptr_lval->get_base());

    // base is a pointer to a struct — look through the pointer
    // in source types to find the struct
    auto *base_src_type = get_source_lvalue_type(ptr_lval->get_base());
    auto *st = get_struct_through_pointer(base_src_type);
    auto field = find_struct_field(st, ptr_lval->get_field());

    auto *index = module.const_int(types.i64(), (int64_t)field.index);
    return builder.build_gep(types.i64(), base, {index});
  }

  case LValue::Kind::Dereference: {
    auto *deref = static_cast<DereferenceLValue *>(lval);
    auto *base = resolve_lvalue_to_ptr(deref->get_operand());
    return builder.build_load(types.ptr(), base);
  }
  }

  assert(false && "unhandled lvalue kind");
  return nullptr;
}
hir::Value *HIRBuilderVisitor::apply_compound_op(AssignmentOperator op,
                                                 hir::Value *lhs,
                                                 hir::Value *rhs) {
  switch (op) {
  case AssignmentOperator::Plus:
    return builder.build_add(lhs, rhs);
  case AssignmentOperator::Minus:
    return builder.build_sub(lhs, rhs);
  case AssignmentOperator::Mult:
    return builder.build_mul(lhs, rhs);
  case AssignmentOperator::Div:
    return builder.build_sdiv(lhs, rhs);
  case AssignmentOperator::Modulo:
    return builder.build_srem(lhs, rhs);
  case AssignmentOperator::LShift:
    return builder.build_shl(lhs, rhs);
  case AssignmentOperator::RShift:
    return builder.build_ashr(lhs, rhs);
  case AssignmentOperator::BitwiseAnd:
    return builder.build_and(lhs, rhs);
  case AssignmentOperator::BitwiseOr:
    return builder.build_or(lhs, rhs);
  case AssignmentOperator::BitwiseXor:
    return builder.build_xor(lhs, rhs);
  default:
    assert(false && "unhandled compound assignment op");
    return nullptr;
  }
}
hir::type::Type *
HIRBuilderVisitor::lower_type(const source_type::Type *ast_type) {
  assert(ast_type && "null ast type");
  switch (ast_type->kind) {
  case source_type::Type::Kind::Builtin: {
    const auto *b = dynamic_cast<const source_type::BuiltinType *>(ast_type);
    switch (b->builtin) {
    case source_type::BuiltinType::Builtin::Int:
      return types.i32();
    case source_type::BuiltinType::Builtin::Bool:
      return types.i1();
    case source_type::BuiltinType::Builtin::Char:
      return types.i8();
    case source_type::BuiltinType::Builtin::Void:
      return types.void_t();
    case source_type::BuiltinType::Builtin::String:
      return types.ptr();
    default:
      throw std::runtime_error("Leck eier");
    }
  }
  case source_type::Type::Kind::Pointer:
    return types.ptr();
  case source_type::Type::Kind::Array: {
    const auto *a = dynamic_cast<const source_type::ArrayType *>(ast_type);
    return types.get_array(a->length, lower_type(a->element));
  }
  case source_type::Type::Kind::Struct: {
    const auto *s = dynamic_cast<const source_type::StructType *>(ast_type);
    std::vector<std::pair<std::string, hir::type::Type *>> fields{};
    fields.reserve(s->fields.size());
    for (const auto &[name, field] : s->fields) {
      fields.emplace_back(name, lower_type(field.unqualified()));
    }
    return types.get_struct(s->name, fields);
  }
  case source_type::Type::Kind::Named: {
    const auto *n = dynamic_cast<const source_type::NamedType *>(ast_type);
    assert(n->underlying && "unresolved named type");
    return lower_type(n->underlying);
  }
  default:
    throw std::runtime_error("cannot lower type: unknown kind");
  }
}

hir::type::Type *HIRBuilderVisitor::type_of_symbol(size_t symbol_id) {
  auto sym = symbol_table->lookup_by_id(symbol_id);
  assert(sym && "symbol not found");
  return lower_type(sym->get_type());
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
  auto *sym = stmt.get_symbol();

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
  std::cerr << "VarExpr " << symbol->get_name()
            << " val_type: " << (void *)val->type << " ("
            << val->type->to_string() << ")" << std::endl;

  value_stack.push(val);
}
void HIRBuilderVisitor::visit(AssignmentStatement &stmt) {
  stmt.get_expr()->accept(*this);
  auto *rhs_var = pop_stack();

  if (stmt.get_lvalue()->get_kind() == LValue::Kind::Variable) {
    auto *var_lval = static_cast<VariableLValue *>(stmt.get_lvalue());
    size_t symbol_id = var_lval->get_symbol()->get_id();
    std::cerr << "Assign " << var_lval->get_symbol()->get_name()
              << " val_type: " << (void *)rhs_var->type << " ("
              << rhs_var->type->to_string() << ")" << std::endl;
    if (stmt.get_op() == AssignmentOperator::Equals) {
      write_variable(symbol_id, current_block, rhs_var);
    } else {
      auto *old_val = read_variable(symbol_id, current_block);
      auto *new_val = apply_compound_op(stmt.get_op(), old_val, rhs_var);
      write_variable(symbol_id, current_block, new_val);
    }
    return;
  }

  auto *ptr = resolve_lvalue_to_ptr(stmt.get_lvalue());

  if (stmt.get_op() == AssignmentOperator::Equals) {
    builder.build_store(rhs_var, ptr);
  } else {
    auto *lval_src_type = get_source_lvalue_type(stmt.get_lvalue());
    auto *ir_type = lower_type(lval_src_type);
    auto *old_val = builder.build_load(ir_type, ptr);
    auto *new_val = apply_compound_op(stmt.get_op(), old_val, rhs_var);
    builder.build_store(new_val, ptr);
  }
}

void HIRBuilderVisitor::visit(Typedef &typedef_) {
  ASTVisitor::visit(typedef_);
}
void HIRBuilderVisitor::visit(FunctionDeclaration &decl) {
  if (decl.is_extern()) {
    std::vector<hir::type::Type *> param_types;
    for (const auto &param : decl.get_parameter_declarations())
      param_types.push_back(lower_type(param->get_symbol()->get_type()));

    auto *fs =
        dynamic_cast<FunctionSymbol *>(symbol_table->lookup(decl.get_name()));
    auto *ret_type = lower_type(fs->func_type->return_type.unqualified());

    auto *fn =
        module.add_function(std::string(decl.get_name()),
                            types.get_function(ret_type, param_types, false));
    fn->is_extern = true;
    return;
  }
  // Build the function type from the declaration
  std::vector<hir::type::Type *> param_types;
  for (const auto &param : decl.get_parameter_declarations()) {
    auto sym = param->get_symbol();
    param_types.push_back(lower_type(sym->get_type()));
  }

  const auto fs =
      dynamic_cast<FunctionSymbol *>(symbol_table->lookup(decl.get_name()))
          ->func_type->return_type;

  const auto ret_type = lower_type(fs.unqualified());

  hir::type::FunctionType *fn_type =
      types.get_function(ret_type, param_types, false);

  current_function = module.add_function(std::string(decl.get_name()), fn_type);

  if (decl.is_extern())
    return;

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

  if (decl.get_body())
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
  auto *val = module.const_int(types.i32(), static_cast<int64_t>(num));

  // Debug:
  std::cerr << "NumericExpr: " << num << " type: " << val->type->to_string()
            << " type_ptr: " << val->type << std::endl;

  value_stack.push(val);
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
  std::cerr << "BinOp " << (int)expr.get_operator_kind()
            << " lhs_type: " << (void *)lhs->type << " ("
            << lhs->type->to_string() << ")"
            << " rhs_type: " << (void *)rhs->type << " ("
            << rhs->type->to_string() << ")" << std::endl;
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
  case BinaryOperator::BitwiseAnd:
    result = builder.build_and(lhs, rhs);
    break;
  case BinaryOperator::BitwiseOr:
    result = builder.build_or(lhs, rhs);
    break;
  case BinaryOperator::BitwiseXor:
    result = builder.build_xor(lhs, rhs);
    break;
  case BinaryOperator::ShiftLeft:
    result = builder.build_shl(lhs, rhs);
    break;
  case BinaryOperator::ShiftRight:
    result = builder.build_ashr(lhs, rhs);
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

  auto *base_src_type =
      src_types.resolve_through_typedefs(expr.get_array()->get_resolved_type());

  auto *at = static_cast<const source_type::ArrayType *>(base_src_type);
  auto *elem_ir_type = lower_type(at->element);

  auto *ptr = builder.build_gep(elem_ir_type, base, {index});
  auto *val = builder.build_load(elem_ir_type, ptr);

  value_stack.push(val);
}

void HIRBuilderVisitor::visit(FieldAccessExpr &expr) {
  expr.get_struct()->accept(*this);
  auto *base = pop_stack();

  auto *base_src_type = src_types.resolve_through_typedefs(
      expr.get_struct()->get_resolved_type());
  assert(base_src_type && base_src_type->is_struct());
  auto *st = static_cast<const source_type::StructType *>(base_src_type);

  auto field = find_struct_field(st, expr.get_field());

  auto *gep = builder.build_gep(
      types.i64(), base, {module.const_int(types.i64(), (int64_t)field.index)});
  auto *val = builder.build_load(field.ir_type, gep);
  value_stack.push(val);
}

void HIRBuilderVisitor::visit(PointerAccessExpr &expr) {
  expr.get_struct_pointer()->accept(*this);
  auto *base_ptr = pop_stack();

  auto *ptr_src_type = src_types.resolve_through_typedefs(
      expr.get_struct_pointer()->get_resolved_type());
  assert(ptr_src_type && ptr_src_type->is_pointer());
  auto *pt = static_cast<const source_type::PointerType *>(ptr_src_type);
  auto *pointee = src_types.resolve_through_typedefs(pt->pointee.unqualified());
  assert(pointee && pointee->is_struct());
  auto *st = static_cast<const source_type::StructType *>(pointee);

  auto field = find_struct_field(st, expr.get_field());

  auto *gep =
      builder.build_gep(types.i64(), base_ptr,
                        {module.const_int(types.i64(), (int64_t)field.index)});
  auto *val = builder.build_load(field.ir_type, gep);
  value_stack.push(val);
}
void HIRBuilderVisitor::visit(AllocExpression &expr) {
  hir::type::Type *alloc_type = lower_type(expr.get_element_type());
  size_t size = alloc_type->size_of();

  auto *malloc = module.get_function("malloc");
  if (!malloc) {
    malloc = module.add_function(
        "malloc", types.get_function(types.ptr(), {types.i32()}, false));
    malloc->is_extern = true;
  }
  hir::Value *size_val = module.const_int(types.i32(), (int64_t)size);
  hir::Value *ptr = builder.build_call(malloc, {size_val});

  value_stack.push(ptr);
}
void HIRBuilderVisitor::visit(AllocArrayExpression &expr) {
  auto *elem_src_type = src_types.resolve(expr.get_type());
  auto *elem_ir_type = lower_type(elem_src_type);

  expr.get_size()->accept(*this);
  auto *size_val = pop_stack();

  auto *size_const =
      module.const_int(types.i32(), (int64_t)elem_ir_type->size_of());
  auto *mul = builder.build_mul(size_const, size_val);

  auto *malloc = module.get_function("malloc");
  if (!malloc) {
    malloc = module.add_function(
        "malloc", types.get_function(types.get_array(0, elem_ir_type),
                                     {types.i32()}, false));
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
void HIRBuilderVisitor::visit(PointerAccessLValue &val) {}
void HIRBuilderVisitor::visit(FieldAccessLValue &val) {
  ASTVisitor::visit(val);
}
void HIRBuilderVisitor::visit(DereferenceLValue &val) {
  ASTVisitor::visit(val);
}
