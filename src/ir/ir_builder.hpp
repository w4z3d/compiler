#ifndef COMPILER_IR_BUILDER_H
#define COMPILER_IR_BUILDER_H

#include "../alloc/arena.hpp"
#include "../defs/ast.hpp"
#include "../report/report_builder.hpp"
#include "cfg.hpp"
#include "ir.hpp"
#include <map>
#include <stack>
#include <unordered_map>

class IRBuilder : public ASTVisitor {
private:
  arena::Arena arena;

  IntermediateRepresentation &representation;
  BasicBlock *current_block = nullptr;
  std::shared_ptr<DiagnosticEmitter> diagnostics;
  std::shared_ptr<SourceManager> source_manager;

  std::size_t temp_counter = 0;
  std::size_t block_counter = 0;
  std::stack<Var> temp_var_stack{};
  std::unordered_map<size_t, Var> symbol_to_var{};

  // Maps a source variable (Symbol ID) to its current SSA Var in the current
  // block being processed. Cleared or managed when moving to a new
  // current_block.
  std::unordered_map<size_t /*symbol_id*/,
                     std::unordered_map<BasicBlock *, Var>>
      current_definitions_in_block;

  // Tracks blocks that will not receive further predecessors.
  std::unordered_set<BasicBlock *> sealed_blocks;

  // For handling forward references in loops/unsealed blocks.
  // Key: Block where PHI is needed. Value: Map of Symbol ID to the PHI
  // IRInstruction* or its result Var. We'll store the result Var of the
  // placeholder PHI.
  std::unordered_map<BasicBlock *,
                     std::unordered_map<size_t /*symbol_id*/, Var>>
      incomplete_phis;

  // Helper to get predecessors (you'll need to build this mapping as you build
  // the CFG)
  std::unordered_map<BasicBlock *, std::vector<BasicBlock *>> predecessors_map;

  // Optional: Memoization for find_reaching_definition to avoid recomputing for
  // the same (symbol, block)
  std::map<std::pair<size_t, BasicBlock *>, Var> find_reaching_def_cache;

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

  void write_variable(size_t symbol, Var value, BasicBlock *block);
  Var read_variable(size_t symbol_id, BasicBlock *for_block);
  Var read_Variable_recursive(size_t symbol_id, BasicBlock *block);

  void seal_block(BasicBlock *block);

  Var try_remove_trivial_phi(IRInstruction *phi_inst, Var phi_result_var,
                             BasicBlock *in_block);
};

#endif // COMPILER_IR_BUILDER_H
