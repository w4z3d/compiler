#ifndef DEFS_AST_H
#define DEFS_AST_H

#include <memory>
#include <string>
#include <vector>
struct SourceLocation {
  std::string file_name;
  int line;
  int column;

  SourceLocation(std::string file_name = "", int line = 0, int column = 0)
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
  std::string name;

public:
  Declaration(Kind k, std::string n, SourceLocation loc = {})
      : ASTNode(std::move(loc)), kind(k), name(std::move(n)) {}

  Kind getKind() const { return kind; }
  const std::string &getName() const { return name; }
  virtual void accept(class ASTVisitor &visitor) override;
};

class FunctionDeclaration : public Declaration {
private:
  std::string name;

}

class TranslationUnit : public ASTNode {
private:
  std::vector<std::shared_ptr<Declaration>> declarations;

public:
  inline void addDeclaration(std::shared_ptr<Declaration> declaration) {
    declarations.push_back(declaration);
  }

  const std::vector<std::shared_ptr<Declaration>> &getDeclarations() const {
    return declarations;
  }

  void accept(ASTVisitor &visitor) override;
};

#endif // !DEFS_AST_H
