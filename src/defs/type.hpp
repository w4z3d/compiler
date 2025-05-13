#ifndef DEFS_TYPE_H
#define DEFS_TYPE_H

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class Type {
public:
  enum class Kind { Builtin, Pointer, Array, Struct, Named, Function };
  constexpr explicit Type(Kind kind) : kind(kind) {}
  Kind kind;
  virtual ~Type() = default;
  [[nodiscard]] virtual bool equals(const Type &other) const = 0;
};

class BuiltinType : public Type {
public:
  enum class BuiltinKind { Int, Char, String, Bool, Void };

private:
  BuiltinKind builtinKind;

public:
  constexpr explicit BuiltinType(BuiltinKind k)
      : Type(Kind::Builtin), builtinKind(k) {}
  [[nodiscard]] BuiltinKind get_kind() const { return builtinKind; }
  [[nodiscard]] bool equals(const Type &other) const override {
    return other.kind == Kind::Builtin &&
           builtinKind == dynamic_cast<const BuiltinType &>(other).builtinKind;
  }
};

constexpr BuiltinType INT_T{BuiltinType::BuiltinKind::Int};
constexpr BuiltinType BOOL_T{BuiltinType::BuiltinKind::Bool};
constexpr BuiltinType CHAR_T{BuiltinType::BuiltinKind::Char};
constexpr BuiltinType STRING_T{BuiltinType::BuiltinKind::String};
constexpr BuiltinType VOID_T{BuiltinType::BuiltinKind::Void};

class PointerType : public Type {
public:
  const Type *to;

  explicit PointerType(const Type *to) : Type(Kind::Pointer), to(to) {}

  [[nodiscard]] bool equals(const Type &other) const override {
    return other.kind == Kind::Pointer &&
           to->equals(*dynamic_cast<const PointerType &>(other).to);
  }
};

class ArrayType : public Type {
public:
  const Type *elementType;
  size_t length = 0;

  explicit ArrayType(const Type *elem) : Type(Kind::Array), elementType(elem) {}

  [[nodiscard]] bool equals(const Type &other) const override {
    return other.kind == Kind::Array &&
           elementType->equals(
               *dynamic_cast<const ArrayType &>(other).elementType);
  }
  void set_len(size_t len) { length = len; }
  [[nodiscard]] size_t get_len() const { return length; }
};

class StructType : public Type {
public:
  std::vector<std::pair<std::string, const Type *>> fields;
  std::string name;

  StructType(std::string name,
             std::vector<std::pair<std::string, const Type *>> fields)
      : Type(Kind::Struct), fields(std::move(fields)), name(std::move(name)) {}

  [[nodiscard]] bool equals(const Type &other) const override {
    return other.kind == Kind::Struct &&
           name == dynamic_cast<const StructType &>(other).name;
  }
};

class NamedType : public Type {
public:
  std::string name;
  Type *type;

  explicit NamedType(std::string name, Type *type)
      : Type(Kind::Named), name(std::move(name)), type(type) {}

  [[nodiscard]] bool equals(const Type &other) const override {
    return other.kind == Kind::Named &&
           name == dynamic_cast<const NamedType &>(other).name;
  }
};

class FunctionType : public Type {
public:
  std::string name;
  const Type *returnType;
  std::vector<const Type *> paramTypes;

  FunctionType(std::string name, const Type *returnType,
               std::vector<const Type *> paramTypes)
      : Type(Kind::Function), name(std::move(name)), returnType(returnType),
        paramTypes(std::move(paramTypes)) {}

  [[nodiscard]] bool equals(const Type &other) const override {
    if (other.kind != Kind::Function)
      return false;
    return name == dynamic_cast<const FunctionType &>(other).name;
  }
};

#endif // DEFS_TYPE_H