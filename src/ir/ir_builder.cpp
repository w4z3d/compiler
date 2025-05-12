#include "ir_builder.hpp"
void IRBuilder::visit(Typedef &typedef_) { ASTVisitor::visit(typedef_); }
void IRBuilder::visit(Declaration &decl) { ASTVisitor::visit(decl); }
void IRBuilder::visit(FunctionDeclaration &decl) {
  auto cfg = CFG{push_new_block()};
  if (decl.get_body()) {
    decl.get_body()->accept(*this);
  }
  representation.add_cfg(cfg);
}
void IRBuilder::visit(ParameterDeclaration &decl) { ASTVisitor::visit(decl); }
void IRBuilder::visit(StructDeclaration &decl) { ASTVisitor::visit(decl); }
void IRBuilder::visit(Statement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(CompoundStmt &stmt) {
  for (const auto &statement : stmt.get_statements()) {
    statement->accept(*this);
  }
}
void IRBuilder::visit(ReturnStmt &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(AssertStmt &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(VariableDeclarationStatement &stmt) {

  // TODO: Add Map (or symbol table) for storing var references and local temp
  // variables.

  stmt.get_initializer()->accept(*this);
  const auto result = temp_var_stack.top();
  //temp_var_stack.pop();
  const auto temp = gen_temp();
  current_block->add_instruction(
      IRInstruction{Opcode::STORE, {Operand{result}}, temp});
}
void IRBuilder::visit(UnaryMutationStatement &stmt) { ASTVisitor::visit(stmt); }
void IRBuilder::visit(AssignmentStatement &stmt) { ASTVisitor::visit(stmt); }
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
void IRBuilder::visit(VarExpr &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(ParenthesisExpression &expr) { ASTVisitor::visit(expr); }
void IRBuilder::visit(BinaryOperatorExpression &expr) {
  ASTVisitor::visit(expr);
}
void IRBuilder::visit(UnaryOperatorExpression &expr) {
  ASTVisitor::visit(expr);
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
