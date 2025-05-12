
#ifndef DEFS_AST_PRINTER_H
#define DEFS_AST_PRINTER_H
#include "ast.hpp"

class ClangStylePrintVisitor : public ASTVisitor {
private:
  std::string content;
  int depth = 0;
  bool use_colors = true;

  // ANSI color codes for terminal output
  const std::string RESET = "\033[0m";
  const std::string GREEN = "\u001b[32m";
  const std::string YELLOW = "\033[33m";
  const std::string BLUE = "\033[34m";
  const std::string MAGENTA = "\033[35m";
  const std::string CYAN = "\033[36m";

  std::string color(const std::string &text, const std::string &color_code) {
    if (use_colors) {
      return color_code + text + RESET;
    }
    return text;
  }

  [[nodiscard]] std::string indent() const {
    if (depth == 0) {
      return "";
    }
    std::string result;
    for (int i = 0; i < depth - 1; i++) {
      result += "| ";
    }
    result += "|-";
    return result;
  }

  static std::string formatLocation(const SourceLocation &loc) {
    if (loc.file_name.empty()) {
      return "<invalid sloc>";
    }
    return std::format("<{}>", loc.file_name);
  }

  static std::string formatRange(const SourceLocation &loc) {
    if (loc.file_name.empty()) {
      return "<invalid sloc>";
    }
    return std::format("<{}:{}:{} - {}:{}:{}>", loc.file_name,
                       std::get<0>(loc.begin), std::get<1>(loc.begin),
                       loc.file_name, std::get<0>(loc.end),
                       std::get<1>(loc.end));
  }

public:
  explicit ClangStylePrintVisitor(bool colored = true) : use_colors(colored) {}

  [[nodiscard]] std::string_view get_content() const { return content; }

  void visit(Typedef &typedef_) override;
  void visit(TranslationUnit &unit) override;
  void visit(FunctionDeclaration &decl) override;
  void visit(ParameterDeclaration &decl) override;
  void visit(StructDeclaration &decl) override;
  void visit(CompoundStmt &stmt) override;
  void visit(ReturnStmt &stmt) override;
  void visit(ErrorStatement &stmt) override;
  void visit(AssertStmt &stmt) override;
  void visit(IfStatement &stmt) override;
  void visit(ForStatement &stmt) override;
  void visit(WhileStatement &stmt) override;
  void visit(VariableDeclarationStatement &stmt) override;
  void visit(AssignmentStatement &stmt) override;
  void visit(UnaryMutationStatement &stmt) override;
  void visit(ExpressionStatement &stmt) override;
  void visit(CallExpr &expr) override;
  void visit(NumericExpr &expr) override;
  void visit(StringLiteralExpr &expr) override;
  void visit(CharLiteralExpr &expr) override;
  void visit(BoolConstExpr &expr) override;
  void visit(NullExpr &expr) override;
  void visit(ParenthesisExpression &expr) override;
  void visit(VarExpr &expr) override;
  void visit(UnaryOperatorExpression &expr) override;
  void visit(BinaryOperatorExpression &expr) override;
  void visit(TernaryExpression &expr) override;
  void visit(ArrayAccessExpr &expr) override;
  void visit(FieldAccessExpr &expr) override;
  void visit(PointerAccessExpr &expr) override;
  void visit(AllocExpression &expr) override;
  void visit(AllocArrayExpression &expr) override;
  void visit(BuiltinTypeAnnotation &type) override;
  void visit(NamedTypeAnnotation &type) override;
  void visit(StructTypeAnnotation &type) override;
  void visit(PointerTypeAnnotation &type) override;
  void visit(ArrayTypeAnnotation &type) override;
  void visit(VariableLValue &val) override;
  void visit(DereferenceLValue &val) override;
  void visit(FieldAccessLValue &val) override;
  void visit(PointerAccessLValue &val) override;
  void visit(ArrayAccessLValue &val) override;
};
#endif // !DEFS_AST_PRINTER_H
