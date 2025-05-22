#include "ir_builder.hpp"
#include "cfg.hpp"
#include "ir.hpp"
#include <optional>

void IRBuilder::visit(Typedef &typedef_) { ASTVisitor::visit(typedef_); }
void IRBuilder::visit(Declaration &decl) { ASTVisitor::visit(decl); }
void IRBuilder::visit(FunctionDeclaration &decl) {
  if (decl.get_body()) {
    auto cfg = CFG{push_new_block()};
    representation.add_cfg(cfg);
    decl.get_body()->accept(*this);
  }
}
void IRBuilder::visit(ParameterDeclaration &decl) { ASTVisitor::visit(decl); }
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
  if (stmt.get_initializer() == nullptr)
    return;
  stmt.get_initializer()->accept(*this);
  auto var = gen_temp();
  auto init = temp_var_stack.top();
  symbol_to_var.emplace(stmt.get_symbol()->get_id(), init); /*/*/
  temp_var_stack.pop();
}
void IRBuilder::visit(UnaryMutationStatement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(AssignmentStatement &stmt) {
  stmt.get_expr()->accept(*this);
  auto from = temp_var_stack.top(); // var of expr result
  temp_var_stack.pop();
  if (stmt.get_lvalue()->get_kind() == LValue::Kind::Variable) {
    const auto var_l_val = dynamic_cast<VariableLValue *>(stmt.get_lvalue());
    if (stmt.get_op() == AssignmentOperator::Equals) {
      symbol_to_var.insert_or_assign(var_l_val->get_symbol()->get_id(), from);
    } else {
      auto op = from_assmt_op(stmt.get_op());
      const auto var_it = symbol_to_var.find(var_l_val->get_symbol()->get_id());
      if (var_it == symbol_to_var.end()) {
        throw std::runtime_error("namensanalyse goes brrrrr. Variable ist "
                                 "nicht init, wird aber verwendet...");
      } else {
        const auto new_var = gen_temp();

        auto old_var = var_it->second;
        current_block->add_instruction(
            IRInstruction{op, {Operand{old_var}, Operand{from}}, new_var});
        symbol_to_var.insert_or_assign(var_l_val->get_symbol()->get_id(),
                                       new_var);
      }
    }
  } else {
    throw std::runtime_error(
        "Was auch immer du gemacht hast, bei L1 geht das noch nicht.");
  }
}
void IRBuilder::visit(ExpressionStatement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(IfStatement &stmt) {
  BasicBlock *condition_eval = current_block;

  auto *then_block = arena.create<BasicBlock>(block_counter++);
  BasicBlock *else_block = nullptr; // Will be created if an else branch exists.
  auto *merge_block = arena.create<BasicBlock>(block_counter++);
  stmt.get_condition()->accept(*this);

  const auto condition_temp = temp_var_stack.top();
  temp_var_stack.pop();

  condition_eval->set_successor_true(then_block);
  if (stmt.get_else_branch()) {
    else_block = arena.create<BasicBlock>(block_counter++);
    condition_eval->set_successor_false(else_block);
  } else {
    condition_eval->set_successor_false(merge_block);
  }

  current_block = then_block;
  stmt.get_then_branch()->accept(*this);

  if (current_block) {
    Operand merge_operand;

    merge_operand.value = static_cast<std::uint32_t>(merge_block->get_id());
    current_block->add_instruction(
        IRInstruction(Opcode::JMP, {merge_operand}, std::nullopt));
    current_block->set_successor_true(merge_block);
    current_block->set_successor_false(std::nullopt);
  }

  if (stmt.get_else_branch()) {
    current_block = else_block;
    stmt.get_else_branch()->accept(*this);
    if (current_block) {
      Operand merge_operand;

      merge_operand.value = static_cast<std::uint32_t>(merge_block->get_id());
      current_block->add_instruction(
          IRInstruction(Opcode::JMP, {merge_operand}, std::nullopt));
      current_block->set_successor_true(merge_block);
      current_block->set_successor_false(std::nullopt);
    }
  }

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
  stmt.get_init()->accept(*this);

  current_block->add_instruction(IRInstruction(
      Opcode::JMP,
      {Operand{static_cast<std::uint32_t>(condition_block->get_id())}},
      std::nullopt));

  current_block = condition_block;
  stmt.get_condition()->accept(*this);
  auto condition = temp_var_stack.top();
  temp_var_stack.pop();

  condition_block->set_successor_true(body_block);
  condition_block->set_successor_false(exit_loop_block);

  current_block = body_block;
  stmt.get_body()->accept(*this);
  auto next_block = increment_block ? increment_block : condition_block;
  current_block->add_instruction(IRInstruction(
      Opcode::JMP, {Operand{static_cast<std::uint32_t>(next_block->get_id())}},
      std::nullopt));
  current_block->set_successor_true(next_block);
  if (increment_block) {
    current_block = increment_block;
    stmt.get_increment()->accept(*this);

    current_block->add_instruction(IRInstruction(
        Opcode::JMP,
        {Operand{static_cast<std::uint32_t>(condition_block->get_id())}},
        std::nullopt));
    current_block->set_successor_true(condition_block);
  }
  current_block = exit_loop_block;
}
void IRBuilder::visit(WhileStatement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(ErrorStatement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(Expression &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(NumericExpr &expr) {
  const auto temp = gen_temp();
  temp_var_stack.push(temp);
   auto num = expr.try_parse<std::uint32_t>().value();
  if(expr.get_base() == NumericExpr::Base::Hexadecimal && num > 0x7FFFFFFF) {
     num = ~num + 1;
  }
  current_block->add_instruction(
      IRInstruction{Opcode::STORE, {Operand{num}}, temp});
}
void IRBuilder::visit(CallExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(StringLiteralExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(CharLiteralExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(BoolConstExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(NullExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(VarExpr &expr) {
  auto it = symbol_to_var.find(expr.get_symbol()->get_id());
  if (it == symbol_to_var.end()) {
    throw std::runtime_error("Da hat jmd Namensanalyse verkackt...");
  }
  temp_var_stack.push(it->second);
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
void IRBuilder::visit(VariableLValue &val) { ASTVisitor::visit(val); }
void IRBuilder::visit(ArrayAccessLValue &val) { ASTVisitor::visit(val); }
void IRBuilder::visit(PointerAccessLValue &val) { ASTVisitor::visit(val); }
void IRBuilder::visit(FieldAccessLValue &val) { ASTVisitor::visit(val); }
void IRBuilder::visit(DereferenceLValue &val) { ASTVisitor::visit(val); }
void IRBuilder::visit(TranslationUnit &unit) {
  for (const auto &decl : unit.get_declarations()) {
    decl->accept(*this);
  }
}
