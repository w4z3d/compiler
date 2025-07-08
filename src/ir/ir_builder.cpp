#include "ir_builder.hpp"
#include "cfg.hpp"
#include "ir.hpp"
#include <iostream>
#include <optional>

void IRBuilder::write_variable(size_t symbol, Var value, BasicBlock *block) {
  current_definitions_in_block[symbol][block] = value;
}

Var IRBuilder::read_variable(size_t symbol_id, BasicBlock *for_block) {
  std::cout << "Searching for symbol " << symbol_id << " in block "
            << for_block->get_id() << std::endl;
  // First check for local defs maybe?=?!??!?!?!??!
  if (current_definitions_in_block.count(symbol_id) &&
      current_definitions_in_block[symbol_id].count(for_block)) {
    return current_definitions_in_block[symbol_id][for_block];
  }
  std::cout << " could not find " << symbol_id << " var in local values "
            << std::endl;
  return IRBuilder::read_Variable_recursive(symbol_id, for_block);
}

Var IRBuilder::read_Variable_recursive(size_t symbol_id, BasicBlock *block) {

  Var result_var{};
  // Check 2: Is block sealed? (Algorithm 2, first part for incomplete CFGs)
  if (sealed_blocks.find(block) == sealed_blocks.end()) { // Block is NOT sealed
    // Place operandless PHI (placeholder)
    if (!incomplete_phis[block].count(symbol_id)) {
      Var phi_result =
          gen_temp(); // Generate a new SSA variable for the PHI result
      // You would create a placeholder PHI IRInstruction here if your IR needs
      // it, otherwise, just tracking the 'phi_result' is enough for now.
      // Example: IRInstruction* phi_inst =
      // arena.create<IRInstruction>(Opcode::PHI, {}, phi_result);
      // for_block->add_instruction(*phi_inst); // Add to the beginning of the
      // block
      incomplete_phis[block][symbol_id] = phi_result;
      // write_variable for the current block being processed might also record
      // this.
      write_variable(symbol_id, phi_result, block);

      std::cout << " Block " << block->get_id()
                << " is not sealed, placing operandless phi with result var "
                << phi_result.to_string() << " sealing later " << std::endl;
    }
    result_var = incomplete_phis[block][symbol_id];
  } else { // Block IS sealed
    const auto &preds = predecessors_map[block];
    if (preds.empty()) {
      // No predecessors, variable must be undefined or global (handle as error
      // or special value) For now, let's generate a fresh temp to signify it's
      // an incoming undefined value This case is typically for the entry block
      // of a function for uninitialized variables/parameters. The paper
      // mentions "new Undef()" for trivial PHIs in start block.
      diagnostics->emit_warning(SourceLocation(),
                                "Variable with symbol_id " +
                                    std::to_string(symbol_id) +
                                    " may be uninitialized on a path.");
      result_var = gen_temp(); // Placeholder for now
    } else if (preds.size() == 1) {
      std::cout << "Single predecessor, searching for defs for " << symbol_id
                << " in " << preds[0]->get_id() << std::endl;
      // Single predecessor: recurse (Algorithm 2)
      result_var = read_variable(symbol_id, preds[0]);
    } else {
      std::cout << "Multiple predecessors, inserting Phi node and searching"
                << std::endl;
      // Multiple predecessors: insert PHI (Algorithm 2)
      Var phi_var_result = gen_temp();
      IRInstruction phi_instruction(Opcode::PHI, {},
                                    phi_var_result); // Operands added below

      // IMPORTANT: Break cycles (Algorithm 2)
      // Temporarily define the variable in for_block as the result of this new
      // PHI This is subtle: current_definitions_in_block is for the block
      // actively being built. If for_block is an already sealed predecessor,
      // we're calculating its outgoing value. The paper's
      // "writeVariable(variable, block, val)" in readVariableRecursive makes
      // 'val' (the PHI node) the definition *in that block*.
      write_variable(symbol_id, phi_var_result, block);

      std::vector<Operand> phi_operands;
      for (BasicBlock *pred_block : preds) {
        phi_operands.push_back({read_variable(symbol_id, pred_block)});
      }
      // Create the actual PHI instruction with operands
      // Need to re-create or modify the existing instruction if it was a
      // placeholder
      auto *actual_phi_inst = arena.create<IRInstruction>(
          Opcode::PHI, phi_operands, phi_var_result);
      block->add_instruction_front(
          std::move(*actual_phi_inst)); // PHIs go at the start

      result_var =
          try_remove_trivial_phi(actual_phi_inst, phi_var_result, block);
    }
  }
  write_variable(symbol_id, result_var, block);
  return result_var;
}
void IRBuilder::seal_block(BasicBlock *block) {
  std::cout << "Sealing block " << block->get_id() << std::endl;
  if (incomplete_phis.count(block)) {
    for (auto const &[symbol_id, phi_placeholder_var] :
         incomplete_phis[block]) {

      std::vector<Operand> phi_operands;
      const auto &preds = predecessors_map[block];

      for (BasicBlock *pred_block : preds) {
        phi_operands.push_back({read_variable(symbol_id, pred_block)});
      }

      auto *actual_phi_inst = arena.create<IRInstruction>(
          Opcode::PHI, phi_operands, phi_placeholder_var);
      block->add_instruction_front(
          std::move(*actual_phi_inst)); // Add to the beginning
      std::cout << "Placing phi while sealing block with tmp var" << std::endl;
      Var final_var =
          try_remove_trivial_phi(actual_phi_inst, phi_placeholder_var, block);
    }
  }
  sealed_blocks.insert(block);
}

Var IRBuilder::try_remove_trivial_phi(IRInstruction *phi_inst,
                                      Var phi_result_var,
                                      BasicBlock *in_block) {
  // phi_inst is the IRInstruction object for the PHI
  // phi_result_var is its defined SSA variable
  // in_block is the block containing the PHI
  if (phi_inst->get_opcode() != Opcode::PHI)
    return phi_result_var; // Should not happen

  Var same = {999999}; // Sentinel for not-yet-found common operand
  bool first_operand = true;

  for (const auto &op_wrapper : phi_inst->get_operands()) {
    Var current_operand_var = std::get<Var>(op_wrapper.value);

    if (current_operand_var.numeral ==
        phi_result_var.numeral) { // Operand is the PHI itself
      continue;
    }

    if (first_operand || same.numeral == (size_t)999999) {
      same = current_operand_var;
      first_operand = false;
    } else if (same.numeral != current_operand_var.numeral) {
      return phi_result_var; // Not trivial, merges at least two different
                             // values
    }
  }

  if (same.numeral ==
      (size_t)999999) { // PHI is all self-references or no non-self operands
    // This means it's unreachable or in start block (uses undef)
    // Replace with a special UNDEF Var if you have one, or handle as
    // error/uninitialized For now, we can't easily remove the instruction or
    // its uses without use-def chains. The paper says "phi.replaceBy(same)"
    // which means updating all users. This simplification pass is powerful but
    // needs more IR infrastructure. Let's log it for now.
    std::cout << "Trivial PHI (all self-references or undef): "
              << phi_result_var.to_string() << " in block "
              << in_block->get_id() << std::endl;
    // To truly remove it, you'd mark phi_result_var as equivalent to 'undef'
    // and then have a later pass to propagate this.
    return phi_result_var; // Cannot simplify further without use-def chains.
  }

  // If we reach here, all non-self operands are 'same'.
  std::cout << "Trivial PHI " << phi_result_var.to_string()
            << " can be replaced by " << same.to_string() << " in block "
            << in_block->get_id() << std::endl;

  // TODO: Actual replacement of 'phi_result_var' with 'same' in all uses.
  // This requires iterating through all instructions that use 'phi_result_var'.
  // For now, we return 'same', and update the definition.
  // The PHI instruction itself should be marked as dead/removed.
  // in_block->remove_instruction(phi_inst); // Need a way to remove/mark dead

  // Recursively try to remove users of this PHI if they also became trivial
  // This also needs use-def chains.

  return same; // The new SSA Var for this definition point
}

void IRBuilder::visit(Typedef &typedef_) { ASTVisitor::visit(typedef_); }
void IRBuilder::visit(Declaration &decl) { ASTVisitor::visit(decl); }
void IRBuilder::visit(FunctionDeclaration &decl) {
  if (decl.get_body()) {
    sealed_blocks.clear();
    incomplete_phis.clear();
    predecessors_map.clear();
    find_reaching_def_cache.clear();
    for (const auto &param : decl.get_parameter_declarations()) {
      param->accept(*this);
    }

    auto cfg = CFG{push_new_block()};
    representation.add_cfg(cfg);
    decl.get_body()->accept(*this);
  }
}
void IRBuilder::visit(ParameterDeclaration &decl) {
  Var param_ssa = gen_temp();
  write_variable(decl.get_symbol()->get_id(), param_ssa, current_block);
}
void IRBuilder::visit(StructDeclaration &decl) { ASTVisitor::visit(decl); }
void IRBuilder::visit(Statement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(CompoundStmt &stmt) {
  for (const auto &statement : stmt.get_statements()) {
    statement->accept(*this);
  }
}
void IRBuilder::visit(ReturnStmt &stmt) {
  if (stmt.get_expression() == nullptr) {
    current_block->add_instruction(IRInstruction{Opcode::RET, {}});
  } else {
    stmt.get_expression()->accept(*this);
    auto var = temp_var_stack.top();
    temp_var_stack.pop();
    current_block->add_instruction(IRInstruction{Opcode::RET, {Operand{var}}});
  }
}
void IRBuilder::visit(AssertStmt &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(VariableDeclarationStatement &stmt) {
  if (stmt.get_initializer()) {
    stmt.get_initializer()->accept(
        *this); // Pushes result SSA Var onto temp_var_stack
    Var initializer_var = temp_var_stack.top();
    temp_var_stack.pop();
    write_variable(stmt.get_symbol()->get_id(), initializer_var, current_block);
    // No IRInstruction for the declaration itself unless it's global/static.
    // The assignment is handled by write_variable.
  } else {
    // Declaration without initializer. It's not yet defined for SSA.
    // If it's used later before assignment, read_variable will handle it
    // (likely creating a PHI or erroring).
  }
}
void IRBuilder::visit(UnaryMutationStatement &stmt) {
  auto res = gen_temp();
  if (stmt.get_target()->get_kind() == LValue::Kind::Variable) {
    const auto var_l_var = dynamic_cast<VariableLValue *>(stmt.get_target());
    const auto prev_var =
        read_variable(var_l_var->get_symbol()->get_id(), current_block);
    write_variable(var_l_var->get_symbol()->get_id(), res, current_block);
    symbol_to_var.emplace(var_l_var->get_symbol()->get_id(), res);
    current_block->add_instruction(IRInstruction{
        from_unary_mut_op(stmt.get_operation()), {Operand{prev_var}}, res});
  }
}
void IRBuilder::visit(AssignmentStatement &stmt) {
  stmt.get_expr()->accept(*this); // Pushes result SSA Var
  Var rhs_var = temp_var_stack.top();
  temp_var_stack.pop();

  // Assuming LValue is VariableLValue for L1 as per your IRBuilder
  if (stmt.get_lvalue()->get_kind() == LValue::Kind::Variable) {
    auto *var_lval = dynamic_cast<VariableLValue *>(stmt.get_lvalue());
    size_t symbol_id = var_lval->get_symbol()->get_id();

    if (stmt.get_op() == AssignmentOperator::Equals) {
      write_variable(symbol_id, rhs_var, current_block);
      // No direct IR instruction for 'var = rhs_var' if rhs_var is already an
      // SSA var. The 'write_variable' updates the current definition. If
      // rhs_var was, say, a constant, the STORE to rhs_var already happened.
    } else { // Compound assignment e.g. x += y
      Var lhs_current_var =
          read_variable(symbol_id, current_block); // Read current value of x
      Opcode op = from_assmt_op(stmt.get_op());    // e.g., ADD
      Var result_var = gen_temp();
      current_block->add_instruction(IRInstruction{
          op, {Operand{lhs_current_var}, Operand{rhs_var}}, result_var});
      write_variable(symbol_id, result_var, current_block);
    }
  } else {
    // Handle other LValue types (struct/array access) later. They involve
    // memory operations.
  }
}
void IRBuilder::visit(ExpressionStatement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(IfStatement &stmt) {

  BasicBlock *condition_eval = current_block;
  stmt.get_condition()->accept(*this);
  const auto condition_temp = temp_var_stack.top();
  temp_var_stack.pop();

  auto *then_block = arena.create<BasicBlock>(block_counter++);
  BasicBlock *else_block = nullptr; // Will be created if an else branch exists.
  auto *merge_block = arena.create<BasicBlock>(block_counter++);

  condition_eval->set_successor_true(then_block);
  predecessors_map[then_block].push_back(condition_eval);
  if (stmt.get_else_branch()) {
    else_block = arena.create<BasicBlock>(block_counter++);
    condition_eval->set_successor_false(else_block);
    predecessors_map[else_block].push_back(condition_eval);
  } else {
    condition_eval->set_successor_false(merge_block);
    predecessors_map[merge_block].push_back(condition_eval);
  }

  // Seal the entry points of the branches *before* processing them
  seal_block(then_block);
  current_block = then_block;
  stmt.get_then_branch()->accept(*this);

  // After then_branch is processed, its current_definitions_in_block are the
  // values flowing to merge_block
  seal_block(then_block); // Seal again to finalize its outgoing definitions
                          // based on its content
  if (current_block) {    // If then_branch doesn't end in return/error
    current_block->set_successor_true(
        merge_block); // Assuming unconditional jump to merge
    predecessors_map[merge_block].push_back(current_block);
  }

  if (else_block) {
    seal_block(else_block);
    current_block = else_block;
    stmt.get_else_branch()->accept(*this);
    seal_block(else_block); // Finalize its outgoing definitions
    if (current_block) {
      current_block->set_successor_true(merge_block);
      predecessors_map[merge_block].push_back(current_block);
    }
  }

  seal_block(merge_block); // Now merge_block can resolve its PHIs
  current_block = merge_block;
}
void IRBuilder::visit(ForStatement &stmt) {
  auto *condition_block = arena.create<BasicBlock>(block_counter++);
  auto *body_block = arena.create<BasicBlock>(block_counter++);
  auto *increment_block = stmt.get_increment()
                              ? arena.create<BasicBlock>(block_counter++)
                              : nullptr;
  auto *exit_loop_block = arena.create<BasicBlock>(block_counter++);

  current_block->set_successor_true(condition_block);
  predecessors_map[condition_block].push_back(current_block);

  stmt.get_init()->accept(*this);

  current_block = condition_block;
  stmt.get_condition()->accept(*this);
  auto condition = temp_var_stack.top();
  temp_var_stack.pop();

  condition_block->set_successor_true(body_block);
  predecessors_map[body_block].push_back(condition_block);
  seal_block(body_block);

  condition_block->set_successor_false(exit_loop_block);
  predecessors_map[exit_loop_block].push_back(condition_block);

  current_block = body_block;
  stmt.get_body()->accept(*this);
  auto next_block = increment_block ? increment_block : condition_block;

  current_block->add_instruction(IRInstruction(
      Opcode::JMP, {Operand{static_cast<std::uint32_t>(next_block->get_id())}},
      std::nullopt));

  current_block->set_successor_true(next_block);
  predecessors_map[next_block].push_back(current_block);
  seal_block(next_block);

  if (increment_block) {
    current_block = increment_block;
    stmt.get_increment()->accept(*this);

    current_block->add_instruction(IRInstruction(
        Opcode::JMP,
        {Operand{static_cast<std::uint32_t>(condition_block->get_id())}},
        std::nullopt));

    current_block->set_successor_true(condition_block);
    predecessors_map[condition_block].push_back(current_block);
    seal_block(increment_block);
  }
  seal_block(exit_loop_block);
  seal_block(condition_block);
  current_block = exit_loop_block;
}
void IRBuilder::visit(WhileStatement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(ErrorStatement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(Expression &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(NumericExpr &expr) {
  const auto temp = gen_temp();
  temp_var_stack.push(temp);
  auto num = expr.try_parse<std::uint32_t>().value();
  if (expr.get_base() == NumericExpr::Base::Hexadecimal && num > 0x7FFFFFFF) {
    num = ~num + 1;
  }
  current_block->add_instruction(
      IRInstruction{Opcode::STORE, {Operand{num}}, temp});
}
void IRBuilder::visit(CallExpr &expr) {
  const auto temp = gen_temp();
  temp_var_stack.push(temp);
  current_block->add_instruction(IRInstruction{Opcode::CALL, {}, temp});
}
void IRBuilder::visit(StringLiteralExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(CharLiteralExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(BoolConstExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(NullExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(VarExpr &expr) {
  Var ssa_var = read_variable(expr.get_symbol()->get_id(), current_block);
  std::cout << " Variable: " << expr.get_variable_name() << " stored at "
            << ssa_var.to_string() << " Current block is "
            << current_block->get_id() << std::endl;
  temp_var_stack.push(ssa_var);
}
void IRBuilder::visit(ParenthesisExpression &expr) {
  expr.get_expression()->accept(*this);
}
void IRBuilder::visit(BinaryOperatorExpression &expr) {
  expr.get_right_expression()->accept(*this);
  expr.get_left_expression()->accept(*this);
  Opcode op = from_binary_op(expr.get_operator_kind());
  Var left = temp_var_stack.top();
  temp_var_stack.pop();
  Var right = temp_var_stack.top();
  temp_var_stack.pop();
  Var temp = gen_temp();
  temp_var_stack.push(temp);
  current_block->add_instruction(
      IRInstruction{op, {Operand{left}, Operand{right}}, temp});
}
void IRBuilder::visit(UnaryOperatorExpression &expr) {
  expr.get_expression()->accept(*this);
  Opcode op = from_unary_op(expr.get_operator_kind());
  Var expression = temp_var_stack.top();
  temp_var_stack.pop();
  Var temp = gen_temp();
  temp_var_stack.push(temp);
  current_block->add_instruction(
      IRInstruction{op, {Operand{expression}}, temp});
}
void IRBuilder::visit(ArrayAccessExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(PointerAccessExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(FieldAccessExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(AllocExpression &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(AllocArrayExpression &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(TernaryExpression &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(TypeAnnotation &type) { ASTVisitor::visit(type); }
void IRBuilder::visit(BuiltinTypeAnnotation &type) { ASTVisitor::visit(type); }
void IRBuilder::visit(NamedTypeAnnotation &type) { ASTVisitor::visit(type); }
void IRBuilder::visit(StructTypeAnnotation &type) { ASTVisitor::visit(type); }
void IRBuilder::visit(PointerTypeAnnotation &type) { ASTVisitor::visit(type); }
void IRBuilder::visit(ArrayTypeAnnotation &type) { ASTVisitor::visit(type); }
void IRBuilder::visit(LValue &val) { ASTVisitor::visit(val); }
void IRBuilder::visit(VariableLValue &val) {
  std::cout << val.get_name() << std::endl;
}
void IRBuilder::visit(ArrayAccessLValue &val) { ASTVisitor::visit(val); }
void IRBuilder::visit(PointerAccessLValue &val) { ASTVisitor::visit(val); }
void IRBuilder::visit(FieldAccessLValue &val) { ASTVisitor::visit(val); }
void IRBuilder::visit(DereferenceLValue &val) { ASTVisitor::visit(val); }
void IRBuilder::visit(TranslationUnit &unit) {
  for (const auto &decl : unit.get_declarations()) {
    decl->accept(*this);
  }
}
