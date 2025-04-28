#ifndef DEFS_AST_H
#define DEFS_AST_H

#include <algorithm>
#include <string>
#include <vector>
struct SourceLocation {
  std::string_view file_name;
  int line;
  int column;

  SourceLocation(std::string_view file_name = "", int line = 0, int column = 0)
      : file_name(file_name), line(line), column(column) {}
};

class ASTNode {
protected:
  SourceLocation location;

public:
  explicit ASTNode(SourceLocation loc = {}) : location(std::move(loc)) {}
  virtual ~ASTNode() = default;

  const SourceLocation &getLocation() const { return location; }

  virtual void accept(class ASTVisitor &visitor) = 0;
};

// ==== Declarations ====

class Declaration : public ASTNode {
public:
  enum class Kind { Function, Class, Struct, Const };

private:
  Kind kind;
  std::string_view name;

public:
  Declaration(Kind k, std::string_view n, SourceLocation loc = {})
      : ASTNode(std::move(loc)), kind(k), name(std::move(n)) {}

  Kind getKind() const { return kind; }
  const std::string_view getName() const { return name; }
  void accept(class ASTVisitor &visitor) override;
};

class FunctionDeclaration : public Declaration {
private:
  std::string_view name;
  std::string_view ret_type;

public:
  FunctionDeclaration(std::string_view name, std::string_view ret_type,
                      SourceLocation loc = {})
      : Declaration(Kind::Function, std::move(name), std::move(loc)),
        name(name), ret_type(ret_type) {}
  void accept(ASTVisitor &visitor) override;
};

class TranslationUnit : public ASTNode {
private:
  std::vector<Declaration *> declarations;

public:
  TranslationUnit(SourceLocation loc = {}) : ASTNode(std::move(loc)) {}

  inline void addDeclaration(Declaration *declaration) {
    declarations.push_back(declaration);
  }

  const std::vector<Declaration *> &getDeclarations() const {
    return declarations;
  }

  void accept(ASTVisitor &visitor) override;
};

class ASTVisitor {
public:
  virtual void visit(Declaration &decl) {}
  virtual void visit(FunctionDeclaration &decl) {}
  virtual void visit(TranslationUnit &unit) {}
};

#endif // !DEFS_AST_H
