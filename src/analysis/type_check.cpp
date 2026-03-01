#include "type_check.hpp"
#include <memory>

void type_check::TypeVisitor::visit(Typedef &typedef_) {
  known_types.insert(from_type_annotation(typedef_.get_type()));
}
void type_check::TypeVisitor::visit(Declaration &decl) {
  ASTVisitor::visit(decl);
}
void type_check::TypeVisitor::visit(FunctionDeclaration &decl) {
  ASTVisitor::visit(decl);
}
void type_check::TypeVisitor::visit(ParameterDeclaration &decl) {
  ASTVisitor::visit(decl);
}
void type_check::TypeVisitor::visit(StructDeclaration &decl) {
  auto struct_type =
      arena.create<type::StructType>(std::string(decl.get_name()));
  known_types.insert(struct_type);
}
void type_check::TypeVisitor::visit(Statement &stmt) {
  ASTVisitor::visit(stmt);
}
void type_check::TypeVisitor::visit(CompoundStmt &stmt) {
  ASTVisitor::visit(stmt);
}
void type_check::TypeVisitor::visit(ReturnStmt &stmt) {
  ASTVisitor::visit(stmt);
}
void type_check::TypeVisitor::visit(AssertStmt &stmt) {
  ASTVisitor::visit(stmt);
}
void type_check::TypeVisitor::visit(VariableDeclarationStatement &stmt) {
  ASTVisitor::visit(stmt);
}
void type_check::TypeVisitor::visit(UnaryMutationStatement &stmt) {
  ASTVisitor::visit(stmt);
}
void type_check::TypeVisitor::visit(AssignmentStatement &stmt) {
  ASTVisitor::visit(stmt);
}
void type_check::TypeVisitor::visit(ExpressionStatement &stmt) {
  ASTVisitor::visit(stmt);
}
void type_check::TypeVisitor::visit(IfStatement &stmt) {
  ASTVisitor::visit(stmt);
}
void type_check::TypeVisitor::visit(ForStatement &stmt) {
  ASTVisitor::visit(stmt);
}
void type_check::TypeVisitor::visit(WhileStatement &stmt) {
  ASTVisitor::visit(stmt);
}
void type_check::TypeVisitor::visit(ErrorStatement &stmt) {
  ASTVisitor::visit(stmt);
}
void type_check::TypeVisitor::visit(Expression &expr) {
  ASTVisitor::visit(expr);
}
void type_check::TypeVisitor::visit(NumericExpr &expr) {
  // expr.set_resolved_type(std::make_shared<type::Type>(type::INT_T));
}
void type_check::TypeVisitor::visit(CallExpr &expr) { ASTVisitor::visit(expr); }
void type_check::TypeVisitor::visit(StringLiteralExpr &expr) {
  ASTVisitor::visit(expr);
}
void type_check::TypeVisitor::visit(CharLiteralExpr &expr) {
  ASTVisitor::visit(expr);
}
void type_check::TypeVisitor::visit(BoolConstExpr &expr) {
  ASTVisitor::visit(expr);
}
void type_check::TypeVisitor::visit(NullExpr &expr) { ASTVisitor::visit(expr); }
void type_check::TypeVisitor::visit(VarExpr &expr) { ASTVisitor::visit(expr); }
void type_check::TypeVisitor::visit(ParenthesisExpression &expr) {
  ASTVisitor::visit(expr);
}
void type_check::TypeVisitor::visit(BinaryOperatorExpression &expr) {
  ASTVisitor::visit(expr);
}
void type_check::TypeVisitor::visit(UnaryOperatorExpression &expr) {
  ASTVisitor::visit(expr);
}
void type_check::TypeVisitor::visit(ArrayAccessExpr &expr) {
  ASTVisitor::visit(expr);
}
void type_check::TypeVisitor::visit(PointerAccessExpr &expr) {
  ASTVisitor::visit(expr);
}
void type_check::TypeVisitor::visit(FieldAccessExpr &expr) {
  ASTVisitor::visit(expr);
}
void type_check::TypeVisitor::visit(AllocExpression &expr) {
  ASTVisitor::visit(expr);
}
void type_check::TypeVisitor::visit(AllocArrayExpression &expr) {
  ASTVisitor::visit(expr);
}
void type_check::TypeVisitor::visit(TernaryExpression &expr) {
  ASTVisitor::visit(expr);
}
void type_check::TypeVisitor::visit(TypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void type_check::TypeVisitor::visit(BuiltinTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void type_check::TypeVisitor::visit(NamedTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void type_check::TypeVisitor::visit(StructTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void type_check::TypeVisitor::visit(PointerTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void type_check::TypeVisitor::visit(ArrayTypeAnnotation &type) {
  ASTVisitor::visit(type);
}
void type_check::TypeVisitor::visit(LValue &val) { ASTVisitor::visit(val); }
void type_check::TypeVisitor::visit(VariableLValue &val) {
  ASTVisitor::visit(val);
}
void type_check::TypeVisitor::visit(ArrayAccessLValue &val) {
  ASTVisitor::visit(val);
}
void type_check::TypeVisitor::visit(PointerAccessLValue &val) {
  ASTVisitor::visit(val);
}
void type_check::TypeVisitor::visit(FieldAccessLValue &val) {
  ASTVisitor::visit(val);
}
void type_check::TypeVisitor::visit(DereferenceLValue &val) {
  ASTVisitor::visit(val);
}
void type_check::TypeVisitor::visit(TranslationUnit &unit) {
  ASTVisitor::visit(unit);
}
