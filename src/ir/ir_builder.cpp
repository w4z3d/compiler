#include "ir_builder.hpp"

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
    current_block->add_instruction(
        IRInstruction{Opcode::RET, {}});
  } else {
    stmt.get_expression()->accept(*this);
    auto var = temp_var_stack.top();
    temp_var_stack.pop();
    current_block->add_instruction(
        IRInstruction{Opcode::RET, {Operand{var}}});
  }
}
void IRBuilder::visit(AssertStmt &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(VariableDeclarationStatement &stmt) {
  stmt.get_initializer()->accept(*this);
  symbol_to_var.emplace(stmt.get_symbol()->get_id(), temp_var_stack.top());
}
void IRBuilder::visit(UnaryMutationStatement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(AssignmentStatement &stmt) {
  stmt.get_expr()->accept(*this);
  stmt.get_lvalue()->accept(*this);
  auto to = temp_var_stack.top();
  temp_var_stack.pop();
  auto from = temp_var_stack.top();
  temp_var_stack.pop();
  if (stmt.get_op() == AssignmentOperator::Equals) {
    current_block->add_instruction(
        IRInstruction{Opcode::STORE, {Operand{from}}, to});
  } else {
    auto op = from_assmt_op(stmt.get_op());
    current_block->add_instruction(
        IRInstruction{op, {Operand{from}, Operand{to}}, to});
  }
}
void IRBuilder::visit(ExpressionStatement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(IfStatement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(ForStatement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(WhileStatement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(ErrorStatement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(Expression &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(NumericExpr &expr) {
  const auto temp = gen_temp();
  temp_var_stack.push(temp);
  const auto num = expr.try_parse<std::uint32_t>();
  current_block->add_instruction(
      IRInstruction{Opcode::STORE, {Operand{num.value()}}, temp});
}
void IRBuilder::visit(CallExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(StringLiteralExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(CharLiteralExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(BoolConstExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(NullExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(VarExpr &expr) {
  auto it = symbol_to_var.find(expr.get_symbol()->get_id());
  if ( it == symbol_to_var.end()) {
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
void IRBuilder::visit(VariableLValue &val) {
  auto var = symbol_to_var.find(val.get_symbol()->get_id());
  if (var == symbol_to_var.end()) {
    throw std::runtime_error("namensanalyse verkackt...");
  }
  temp_var_stack.push(var->second);
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
