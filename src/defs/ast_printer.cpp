#include "ast_printer.hpp"
void ClangStylePrintVisitor::visit(WhileStatement &stmt) {
  content +=
      color("WhileStatement", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(stmt))),
            YELLOW) +
      " " + formatRange(stmt.get_location()) + "\n";

  depth++;
  content += indent();
  stmt.get_condition()->accept(*this);
  content += indent();
  stmt.get_body()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(VariableDeclarationStatement &stmt) {
  content +=
      color("VarDeclStatement", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(stmt))),
            YELLOW) +
      " " + formatRange(stmt.get_location()) + " " +
      color(std::string(stmt.get_identifier()), MAGENTA) + " " +
      stmt.get_type()->toString() + "\n";

  if (stmt.get_initializer() != nullptr) {
    depth++;
    content += indent();
    stmt.get_initializer()->accept(*this);
    depth--;
  }
}

void ClangStylePrintVisitor::visit(AssignmentStatement &stmt) {
  content +=
      color("AssignmentStatement", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(stmt))),
            YELLOW) +
      " " + formatRange(stmt.get_location()) + " " +
      assmtOp2String(stmt.get_op()) + "\n";

  depth++;
  content += indent();
  stmt.get_lvalue()->accept(*this);
  content += indent();
  stmt.get_expr()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(UnaryMutationStatement &stmt) {
  content +=
      color("UnaryMutStatement", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(stmt))),
            YELLOW) +
      " " + formatRange(stmt.get_location()) + " " +
      (stmt.get_operation() == UnaryMutationStatement::Op::PostIncrement
           ? "PostIncrement"
           : "PostDecrement") +
      "\n";

  depth++;
  content += indent();
  stmt.get_target()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(ExpressionStatement &stmt) {
  content +=
      color("ExprStatement", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(stmt))),
            YELLOW) +
      " " + formatRange(stmt.get_location()) + "\n";

  depth++;
  content += indent();
  stmt.get_expression()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(CallExpr &expr) {
  content +=
      color("CallExpr", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + " " +
      std::string(expr.get_function_name()) + "\n";

  // Visit param expressions
  depth++;
  for (const auto &param : expr.get_params()) {
    content += indent();
    param->accept(*this);
  }
  depth--;
}
void ClangStylePrintVisitor::visit(NumericExpr &expr) {
  content +=
      color("IntegerLiteralExpr", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + " " + "'int' " +
      std::string(expr.get_value()) + "\n";
}

void ClangStylePrintVisitor::visit(StringLiteralExpr &expr) {
  content +=
      color("StringLiteralExpr", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + " " + "'string' " +
      std::string(expr.get_value()) + "\n";
}

void ClangStylePrintVisitor::visit(CharLiteralExpr &expr) {
  content +=
      color("CharLiteralExpr", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + " " + "'char' " +
      std::string(expr.get_value()) + "\n";
}

void ClangStylePrintVisitor::visit(BoolConstExpr &expr) {
  content +=
      color("BoolConstExpr", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + " " + "'bool' " +
      (expr.get_value() ? "true" : "false") + "\n";
}

void ClangStylePrintVisitor::visit(NullExpr &expr) {
  content +=
      color("NullExpr", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + " 'void' NULL \n";
}

void ClangStylePrintVisitor::visit(ParenthesisExpression &expr) {
  content +=
      color("ParenExpr", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + "\n";

  depth++;
  content += indent();
  expr.get_expression()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(VarExpr &expr) {
  content +=
      color("VarExpr", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + " " +
      std::string(expr.get_variable_name()) + "\n";
}

void ClangStylePrintVisitor::visit(UnaryOperatorExpression &expr) {
  content +=
      color("UnaryOperator", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + " " +
      unOp2String(expr.get_operator_kind()) + "\n";

  depth++;
  content += indent();
  expr.get_expression()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(BinaryOperatorExpression &expr) {
  content +=
      color("BinaryOperator", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + " " +
      binOp2String(expr.get_operator_kind()) + "\n";

  depth++;
  content += indent();
  expr.get_left_expression()->accept(*this);
  content += indent();
  expr.get_right_expression()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(TernaryExpression &expr) {
  content +=
      color("TernaryExpression", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + "\n";

  depth++;
  content += indent();
  expr.get_condition()->accept(*this);
  content += indent();
  expr.get_then()->accept(*this);
  content += indent();
  expr.get_else()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(ArrayAccessExpr &expr) {
  content +=
      color("ArrayAccess", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + "\n";

  depth++;
  content += indent();
  expr.get_array()->accept(*this);
  content += indent();
  expr.get_index()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(FieldAccessExpr &expr) {
  content +=
      color("FieldAccess", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + " " +
      std::string(expr.get_field()) + "\n";

  depth++;
  content += indent();
  expr.get_struct()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(PointerAccessExpr &expr) {
  content +=
      color("PointerAccess", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + " " +
      std::string(expr.get_field()) + "\n";

  depth++;
  content += indent();
  expr.get_struct_pointer()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(AllocExpression &expr) {
  content +=
      color("AllocExpression", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + " " +
      std::string(expr.get_type()->toString()) + "\n";
}

void ClangStylePrintVisitor::visit(AllocArrayExpression &expr) {
  content +=
      color("AllocArrayExpression", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(expr))),
            YELLOW) +
      " " + formatRange(expr.get_location()) + " " +
      std::string(expr.get_type()->toString()) + "\n";

  depth++;
  content += indent();
  expr.get_size()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(BuiltinTypeAnnotation &type) {
  content +=
      color("BuiltinTypeAnnotation", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(type))),
            YELLOW) +
      " " + formatRange(type.get_location()) + " " +
      builtin2String(type.get_type()) + "\n";
}

void ClangStylePrintVisitor::visit(NamedTypeAnnotation &type) {
  content +=
      color("NamedTypeAnnotation", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(type))),
            YELLOW) +
      " " + formatRange(type.get_location()) + " " +
      std::string(type.get_name()) + "\n";
}

void ClangStylePrintVisitor::visit(StructTypeAnnotation &type) {
  content +=
      color("StructTypeAnnotation", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(type))),
            YELLOW) +
      " " + formatRange(type.get_location()) + " Struct " +
      std::string(type.get_name()) + "\n";
}

void ClangStylePrintVisitor::visit(PointerTypeAnnotation &type) {
  content +=
      color("PointerTypeAnnotation", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(type))),
            YELLOW) +
      " " + formatRange(type.get_location()) + "\n";

  depth++;
  content += indent();
  type.get_type()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(ArrayTypeAnnotation &type) {
  content +=
      color("ArrayTypeAnnotation", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(type))),
            YELLOW) +
      " " + formatRange(type.get_location()) + "\n";

  depth++;
  content += indent();
  type.get_type()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(VariableLValue &val) {
  content +=
      color("VariableLValue", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(val))),
            YELLOW) +
      " " + formatRange(val.get_location()) + " " +
      std::string(val.get_name()) + "\n";
}

void ClangStylePrintVisitor::visit(DereferenceLValue &val) {
  content +=
      color("DereferenceLValue", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(val))),
            YELLOW) +
      " " + formatRange(val.get_location()) + "\n";
  depth++;
  content += indent();
  val.get_operand()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(FieldAccessLValue &val) {
  content +=
      color("FieldAccessLValue", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(val))),
            YELLOW) +
      " " + formatRange(val.get_location()) + " " +
      std::string(val.get_field()) + "\n";
  depth++;
  content += indent();
  val.get_base()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(PointerAccessLValue &val) {
  content +=
      color("PointerAccessLValue", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(val))),
            YELLOW) +
      " " + formatRange(val.get_location()) + " " +
      std::string(val.get_field()) + "\n";
  depth++;
  content += indent();
  val.get_base()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(ArrayAccessLValue &val) {
  content +=
      color("ArrayAccessLValue", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(val))),
            YELLOW) +
      " " + formatRange(val.get_location()) + "\n";
  depth++;
  content += indent();
  val.get_base()->accept(*this);
  content += indent();
  val.get_index()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(ForStatement &stmt) {
  content +=
      color("ForStatement", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(stmt))),
            YELLOW) +
      " " + formatRange(stmt.get_location()) + "\n";

  depth++;
  if (stmt.get_init() != nullptr) {
    content += indent();
    stmt.get_init()->accept(*this);
  }
  content += indent();
  stmt.get_condition()->accept(*this);
  if (stmt.get_increment() != nullptr) {
    content += indent();
    stmt.get_increment()->accept(*this);
  }
  content += indent();
  stmt.get_body()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(Typedef &typedef_) {
  content += color("Typedef", GREEN) + " " +
             color(std::format("{:#x}", reinterpret_cast<std::size_t>(
                                            std::addressof(typedef_))),
                   YELLOW) +
             " " + formatLocation(typedef_.get_location()) + " " +
             color(std::string(typedef_.get_name()), MAGENTA) + " " +
             typedef_.get_type()->toString() + "\n";
}

void ClangStylePrintVisitor::visit(TranslationUnit &unit) {
  content +=
      color("TranslationUnitDecl", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(unit))),
            YELLOW) +
      " " + formatLocation(unit.get_location()) + "\n";

  depth++;
  for (const auto &decl : unit.get_declarations()) {
    content += indent();
    decl->accept(*this);
  }
  depth--;
}

void ClangStylePrintVisitor::visit(FunctionDeclaration &decl) {
  content +=
      color("FunctionDecl", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(decl))),
            YELLOW) +
      " " + formatRange(decl.get_location()) + " " +
      color(std::format("line:{}", decl.get_location().line), CYAN) + " " +
      color(std::string(decl.get_name()), MAGENTA) + " " + "'" +
      decl.get_return_type()->toString() + " (";

  // Parameter types list
  bool first = true;
  for (const auto &param : decl.get_parameter_declarations()) {
    if (!first)
      content += ", ";
    content += param->get_type()->toString();
    first = false;
  }
  content += ")'\n";

  depth++;
  // Function parameters
  for (const auto &param : decl.get_parameter_declarations()) {
    content += indent();
    param->accept(*this);
  }

  // Function body
  if (decl.get_body() != nullptr) {
    content += indent();
    decl.get_body()->accept(*this);
  }
  depth--;
}

void ClangStylePrintVisitor::visit(ParameterDeclaration &decl) {
  content +=
      color("ParmVarDecl", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(decl))),
            YELLOW) +
      " " + formatRange(decl.get_location()) + " " +
      color(std::format("col:{}", decl.get_location().column), CYAN) + " " +
      color(std::string(decl.get_name()), MAGENTA) + " " + "'" +
      decl.get_type()->toString() + "'\n";
}

void ClangStylePrintVisitor::visit(StructDeclaration &decl) {
  content +=
      color("StructDecl", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(decl))),
            YELLOW) +
      " " + formatRange(decl.get_location()) + " " +
      color(std::format("col:{}", decl.get_location().column), CYAN) + " " +
      color(std::string(decl.get_name()), MAGENTA) + "\n";
  depth++;
  if (const auto &fields = decl.get_fields()) {
    for (const auto &statement : *fields) {
      content += indent();
      statement->accept(*this);
    }
  }
  depth--;
}

void ClangStylePrintVisitor::visit(CompoundStmt &stmt) {
  content +=
      color("CompoundStmt", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(stmt))),
            YELLOW) +
      " " + formatRange(stmt.get_location()) + "\n";

  depth++;
  for (const auto &statement : stmt.get_statements()) {
    content += indent();
    statement->accept(*this);
  }
  depth--;
}

void ClangStylePrintVisitor::visit(ReturnStmt &stmt) {
  content +=
      color("ReturnStmt", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(stmt))),
            YELLOW) +
      " " + formatRange(stmt.get_location()) + "\n";

  if (stmt.get_expression() != nullptr) {
    depth++;
    content += indent();
    stmt.get_expression()->accept(*this);
    depth--;
  }
}

void ClangStylePrintVisitor::visit(ErrorStatement &stmt) {
  content +=
      color("ErrorStatement", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(stmt))),
            YELLOW) +
      " " + formatRange(stmt.get_location()) + "\n";

  depth++;
  content += indent();
  stmt.get_expr()->accept(*this);
  depth--;
}

void ClangStylePrintVisitor::visit(AssertStmt &stmt) {
  content +=
      color("AssertStmt", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(stmt))),
            YELLOW) +
      " " + formatRange(stmt.get_location()) + "\n";

  if (stmt.get_expression() != nullptr) {
    depth++;
    content += indent();
    stmt.get_expression()->accept(*this);
    depth--;
  }
}

void ClangStylePrintVisitor::visit(IfStatement &stmt) {
  content +=
      color("IfStatement", GREEN) + " " +
      color(std::format("{:#x}",
                        reinterpret_cast<std::size_t>(std::addressof(stmt))),
            YELLOW) +
      " " + formatRange(stmt.get_location()) + "\n";

  depth++;
  content += indent();
  stmt.get_condition()->accept(*this);
  content += indent();
  stmt.get_then_branch()->accept(*this);
  if (stmt.get_else_branch() != nullptr) {
    content += indent();
    stmt.get_else_branch()->accept(*this);
  }
  depth--;
}
