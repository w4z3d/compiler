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
  // Check local definitions first
  auto def_it = current_defs.find(symbol_id);
  if (def_it != current_defs.end()) {
    auto bb_it = def_it->second.find(bb);
    if (bb_it != def_it->second.end()) {
      return bb_it->second;
    }
  }
  // Not found locally — search predecessors
  return read_variable_recursive(symbol_id, bb);
}

hir::Value *HIRBuilderVisitor::read_variable_recursive(size_t symbol_id,
                                                       hir::BasicBlock *bb) {
  hir::Value *result = nullptr;

  if (!sealed_blocks.count(bb)) {
    // Block not sealed yet — place incomplete phi
    if (!incomplete_phis[bb].count(symbol_id)) {
      // Create phi without incoming values yet
      // Save and restore insert point since we're inserting at block top
      auto *saved_block = current_block;
      builder.set_insert_point(bb);

      hir::PhiNode *phi = builder.build_phi(type_of_symbol(symbol_id));

      // Restore insert point
      builder.set_insert_point(saved_block);

      incomplete_phis[bb][symbol_id] = phi;
      write_variable(symbol_id, bb, phi);
    }
    result = incomplete_phis[bb][symbol_id];

  } else {
    const auto &preds = bb->predecessors;

    if (preds.empty()) {
      diagnostics->emit_warning(SourceLocation{},
                                "variable '" + std::to_string(symbol_id) +
                                    "' may be uninitialized");
      result = module.get_undef(type_of_symbol(symbol_id));

    } else if (preds.size() == 1) {
      // Single predecessor — just recurse
      result = read_variable(symbol_id, preds[0]);

    } else {
      // Multiple predecessors — insert phi and fill operands
      auto *saved_block = current_block;
      builder.set_insert_point(bb);

      hir::PhiNode *phi = builder.build_phi(type_of_symbol(symbol_id));
      builder.set_insert_point(saved_block);
      write_variable(symbol_id, bb, phi);

      phi->operands.reserve(preds.size());
      for (hir::BasicBlock *pred : preds) {
        hir::Value *incoming = read_variable(symbol_id, pred);
        phi->add_incoming(incoming, pred);
      }

      result = try_remove_trivial_phi(phi);
    }
  }

  write_variable(symbol_id, bb, result);
  return result;
}

void HIRBuilderVisitor::seal_block(hir::BasicBlock *bb) {
  if (sealed_blocks.count(bb))
    return;

  if (incomplete_phis.count(bb)) {
    for (auto &[symbol_id, phi] : incomplete_phis[bb]) {
      phi->operands.reserve(phi->operands.size() + bb->predecessors.size());
      for (hir::BasicBlock *pred : bb->predecessors) {
        hir::Value *incoming = read_variable(symbol_id, pred);
        phi->add_incoming(incoming, pred);
      }
      try_remove_trivial_phi(phi);
    }
    incomplete_phis.erase(bb);
  }

  sealed_blocks.insert(bb);
}

hir::Value *HIRBuilderVisitor::try_remove_trivial_phi(hir::PhiNode *phi) {
  hir::Value *same = nullptr;

  for (auto &use : phi->operands) {
    hir::Value *op = use.value;
    if (op == same || op == phi)
      continue;
    if (same != nullptr)
      return phi; // merges two distinct values, not trivial
    same = op;
  }

  if (same == nullptr)
    same = module.get_undef(phi->type);

  // Collect users before modifying
  std::vector<hir::Instruction *> users;
  for (auto *use : phi->users) {
    if (use->user != phi)
      users.push_back(use->user);
  }

  phi->replace_all_uses_with(same);
  phi->erase_from_parent();
  // Update current_defs to replace the removed phi
  for (auto &[sym_id, block_map] : current_defs) {
    for (auto &[bb, val] : block_map) {
      if (val == phi)
        val = same;
    }
  }
  // Recursively try to simplify phi users
  for (auto *user : users) {
    if (auto *user_phi = dynamic_cast<hir::PhiNode *>(user))
      try_remove_trivial_phi(user_phi);
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

  hir::BasicBlock *then_bb = current_function->append_block("if.then");
  hir::BasicBlock *else_bb = current_function->append_block("if.else");
  hir::BasicBlock *merge_bb = current_function->append_block("if.merge");

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

  // Seal merge_bb only after both branches are done
  // so all predecessors are known
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
    // Uninitialized — write undef so read_variable never goes to predecessors
    // looking for a definition that doesn't exist
    hir::type::Type *hir_type = lower_type(sym->get_type());
    write_variable(sym->get_id(), current_block, module.get_undef(hir_type));
  }
}
void HIRBuilderVisitor::visit(VarExpr &expr) {
  auto symbol = expr.get_symbol();
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

  // Add function to module — this also creates the entry block
  current_function = module.add_function(std::string(decl.get_name()), fn_type);

  // Entry block is already created by Function constructor
  hir::BasicBlock *entry = current_function->entry();
  set_insert_point(entry);

  // Seal entry block immediately — it has no predecessors
  seal_block(entry);

  // Write function arguments into SSA map
  for (size_t i = 0; i < decl.get_parameter_declarations().size(); i++) {
    auto sym = decl.get_parameter_declarations()[i]->get_symbol();
    assert(sym && "parameter has no symbol!");
    std::cerr << "param " << i << ": " << sym->get_name()
              << " id=" << sym->get_id() << "\n";

    hir::Argument *arg = current_function->arguments[i];
    arg->name = std::string(sym->get_name());

    write_variable(sym->get_id(), entry, arg);
  }

  // Visit the function body
  decl.get_body()->accept(*this);

  // If the last block has no terminator and return type is void,
  // insert an implicit ret
  if (!current_block->terminator()) {
    if (ret_type->is_void()) {
      builder.build_ret(nullptr);
    } else {
      // Non-void function missing return — type checker should have
      // caught this, but emit undef ret defensively
      diagnostics->emit_warning({}, "non-void function may not return a value");
      builder.build_ret(module.get_undef(ret_type));
    }
  }

  // Clean up SSA state for next function
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
  ASTVisitor::visit(stmt);
}
void HIRBuilderVisitor::visit(ForStatement &stmt) {
  // --- Initializer runs in current block ---
  if (stmt.get_init()) {
    stmt.get_init()->accept(*this);
  }

  hir::BasicBlock *cond_bb = current_function->append_block("for.cond");
  hir::BasicBlock *body_bb = current_function->append_block("for.body");
  hir::BasicBlock *inc_bb = current_function->append_block("for.inc");
  hir::BasicBlock *merge_bb = current_function->append_block("for.merge");

  // --- Jump from current block into condition ---
  cond_bb->predecessors.push_back(current_block);
  builder.build_br(cond_bb);

  // --- Condition block ---
  // cond_bb has two predecessors: entry and inc_bb (back edge)
  // so we cannot seal it yet — seal after inc_bb is built
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
    // No condition — infinite loop, always jump to body
    body_bb->predecessors.push_back(cond_bb);
    builder.build_br(body_bb);
  }

  // --- Body block ---
  set_insert_point(body_bb);
  seal_block(body_bb); // only one predecessor: cond_bb
  stmt.get_body()->accept(*this);
  if (!current_block->terminator()) {
    inc_bb->predecessors.push_back(current_block);
    builder.build_br(inc_bb);
  }

  // --- Increment block ---
  set_insert_point(inc_bb);
  seal_block(
      inc_bb); // only one predecessor: body (or current_block after body)
  if (stmt.get_increment()) {
    stmt.get_increment()->accept(*this);
    // Increment is a statement so nothing on stack to pop
  }
  builder.build_br(cond_bb);

  // --- Now seal cond_bb — all predecessors known ---
  seal_block(cond_bb);

  // --- Continue after loop ---
  set_insert_point(merge_bb);
  seal_block(merge_bb);
}
void HIRBuilderVisitor::visit(WhileStatement &stmt) { ASTVisitor::visit(stmt); }
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
  // Evaluate arguments left to right
  std::vector<hir::Value *> args;
  for (const auto &arg : expr.get_params()) {
    arg->accept(*this);
    args.push_back(pop_stack());
  }

  // Look up the callee function
  for (const auto &function : module.functions) {
    std::cout << "Function " << function->to_string() << " "
              << function->function_type->return_type->to_string() << std::endl;
    std::cout << "Searching for " << expr.get_function_name() << " "
              << symbol->get().get_type()->toString() << std::endl;
  }
  hir::Function *callee = module.get_function(expr.get_function_name());
  assert(callee && "unknown function");

  // Build the call
  hir::Value *result = builder.build_call(callee, args);

  // Push result if non-void (so the caller can use it)
  value_stack.push(result);
}
void HIRBuilderVisitor::visit(StringLiteralExpr &expr) {
  ASTVisitor::visit(expr);
}
void HIRBuilderVisitor::visit(CharLiteralExpr &expr) {
  ASTVisitor::visit(expr);
}
void HIRBuilderVisitor::visit(BoolConstExpr &expr) {
  auto *bool_ = module.const_bool(expr.get_value());
  value_stack.push(bool_);
}
void HIRBuilderVisitor::visit(NullExpr &expr) { ASTVisitor::visit(expr); }
void HIRBuilderVisitor::visit(ParenthesisExpression &expr) {
  ASTVisitor::visit(expr);
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
  ASTVisitor::visit(expr);
}
void HIRBuilderVisitor::visit(ArrayAccessExpr &expr) {
  ASTVisitor::visit(expr);
}
void HIRBuilderVisitor::visit(PointerAccessExpr &expr) {
  ASTVisitor::visit(expr);
}
void HIRBuilderVisitor::visit(FieldAccessExpr &expr) {
  ASTVisitor::visit(expr);
}
size_t HIRBuilderVisitor::size_of(hir::type::Type *type) {
  if (auto *int_type = dynamic_cast<hir::type::IntegerType *>(type)) {
    return int_type->width / 8; // bits to bytes
  } else if (type->is_pointer()) {
    return 8; // 64-bit pointers
  } else if (auto *struct_type = dynamic_cast<hir::type::StructType *>(type)) {
    size_t total = 0;
    for (auto *field_type : struct_type->fields) {
      // Simple: no padding/alignment
      total += size_of(field_type);
    }
    return total;
  } else if (auto *array_type = dynamic_cast<hir::type::ArrayType *>(type)) {
    return array_type->count * size_of(array_type->inner_type);
  }
  assert(false && "cannot compute size of type");
}
void HIRBuilderVisitor::visit(AllocExpression &expr) {
  hir::type::Type *alloc_type =
      lower_type(from_type_annotation(expr.get_type()));
  size_t size = size_of(alloc_type);

  // Generate: call malloc(size)
  auto *malloc_fn = module.add_function(
      "malloc", types.get_function(types.i32(), {types.i32()}, false));
  hir::Value *size_val = module.const_int(types.i32(), size);
  hir::Value *ptr = builder.build_call(malloc_fn, {size_val});

  value_stack.push(ptr);
}
void HIRBuilderVisitor::visit(AllocArrayExpression &expr) {
  ASTVisitor::visit(expr);
}
void HIRBuilderVisitor::visit(TernaryExpression &expr) {
  expr.get_condition()->accept(*this);
  const auto condition = pop_stack();

  hir::BasicBlock *then_bb = current_function->append_block("if.then");
  hir::BasicBlock *else_bb = current_function->append_block("if.else");
  hir::BasicBlock *merge_bb = current_function->append_block("if.merge");

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
