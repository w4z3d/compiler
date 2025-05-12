#ifndef COMPILER_IR_BUILDER_H
#define COMPILER_IR_BUILDER_H

#include "../alloc/arena.hpp"
#include "../defs/ast.hpp"
#include "../report/report_builder.hpp"
#include "cfg.hpp"
#include <stack>

class IRBuilder : public ASTVisitor {
private:
  IntermediateRepresentation &representation;
  BasicBlock *current_block = nullptr;
  std::shared_ptr<DiagnosticEmitter> diagnostics;
  std::shared_ptr<SourceManager> source_manager;
  std::size_t temp_counter = 0;
  std::size_t block_counter = 0;
  std::stack<Var> temp_var_stack{};
  arena::Arena arena;

public:
  explicit IRBuilder(IntermediateRepresentation &representation,
                     std::shared_ptr<DiagnosticEmitter> diagnostics,
                     std::shared_ptr<SourceManager> source_manager)
      : representation(representation), diagnostics(std::move(diagnostics)),
        source_manager(std::move(source_manager)), arena(arena::Arena{}) {}
  Var gen_temp() { return Var{temp_counter++}; }
  BasicBlock *push_new_block() {
    current_block = arena.create<BasicBlock>(block_counter++);
    return current_block;
  }
  void visit(Typedef &typedef_) override;
  void visit(Declaration &decl) override;
  void visit(FunctionDeclaration &decl) override;
  void visit(ParameterDeclaration &decl) override;
  void visit(StructDeclaration &decl) override;
  void visit(Statement &stmt) override;
  void visit(CompoundStmt &stmt) override;
  void visit(ReturnStmt &stmt) override;
  void visit(AssertStmt &stmt) override;
  void visit(VariableDeclarationStatement &stmt) override;
  void visit(UnaryMutationStatement &stmt) override;
  void visit(AssignmentStatement &stmt) override;
  void visit(ExpressionStatement &stmt) override;
  void visit(IfStatement &stmt) override;
  void visit(ForStatement &stmt) override;
  void visit(WhileStatement &stmt) override;
  void visit(ErrorStatement &stmt) override;
  void visit(Expression &expr) override;
  void visit(NumericExpr &expr) override;
  void visit(CallExpr &expr) override;
  void visit(StringLiteralExpr &expr) override;
  void visit(CharLiteralExpr &expr) override;
  void visit(BoolConstExpr &expr) override;
  void visit(NullExpr &expr) override;
  void visit(VarExpr &expr) override;
  void visit(ParenthesisExpression &expr) override;
  void visit(BinaryOperatorExpression &expr) override;
  void visit(UnaryOperatorExpression &expr) override;
  void visit(ArrayAccessExpr &expr) override;
  void visit(PointerAccessExpr &expr) override;
  void visit(FieldAccessExpr &expr) override;
  void visit(AllocExpression &expr) override;
  void visit(AllocArrayExpression &expr) override;
  void visit(TernaryExpression &expr) override;
  void visit(TypeAnnotation &type) override;
  void visit(BuiltinTypeAnnotation &type) override;
  void visit(NamedTypeAnnotation &type) override;
  void visit(StructTypeAnnotation &type) override;
  void visit(PointerTypeAnnotation &type) override;
  void visit(ArrayTypeAnnotation &type) override;
  void visit(LValue &val) override;
  void visit(VariableLValue &val) override;
  void visit(ArrayAccessLValue &val) override;
  void visit(PointerAccessLValue &val) override;
  void visit(FieldAccessLValue &val) override;
  void visit(DereferenceLValue &val) override;
  void visit(TranslationUnit &unit) override;
};

#endif // COMPILER_IR_BUILDER_H
