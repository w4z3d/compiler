#ifndef DEFS_TYPE_H
#define DEFS_TYPE_H

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include "ast.hpp"

namespace type {

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

  explicit StructType(std::string name)
      : Type(Kind::Struct), name(std::move(name)) {}

  void set_fields(std::vector<std::pair<std::string, const Type *>> f) {
    fields = std::move(f);
  }

  [[nodiscard]] bool equals(const Type &other) const override {
    return other.kind == Kind::Struct &&
           name == dynamic_cast<const StructType &>(other).name;
  }
};

class NamedType : public Type {
public:
  std::string name;
  Type *type{};

  explicit NamedType(std::string name)
      : Type(Kind::Named), name(std::move(name)) {}

  void set_type(Type *t) {
    type = t;
  }
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

static std::shared_ptr<Type> from_type(const TypeAnnotation *annotation) {
  if (auto builtin = dynamic_cast<const BuiltinTypeAnnotation *>(annotation)) {
    return from_type(builtin);
  } else if (auto struct_ =
                 dynamic_cast<const StructTypeAnnotation *>(annotation)) {
    return from_type(struct_);
  } else if (auto named =
                 dynamic_cast<const NamedTypeAnnotation *>(annotation)) {
    return from_type(named);
  } else if (auto pointer =
                 dynamic_cast<const PointerTypeAnnotation *>(annotation)) {
    return from_type(pointer);
  } else if (auto array =
                 dynamic_cast<const ArrayTypeAnnotation *>(annotation)) {
    return from_type(array);
  } else {
    throw std::runtime_error("Unknown type annotation");
  }
}
static std::shared_ptr<BuiltinType>
from_type(const BuiltinTypeAnnotation *type_annotation) {
  switch (type_annotation->get_type()) {
  case Builtin::Int:
    return std::make_shared<BuiltinType>(INT_T);
  case Builtin::Bool:
    return std::make_shared<BuiltinType>(BOOL_T);
  case Builtin::String:
    return std::make_shared<BuiltinType>(STRING_T);
  case Builtin::Char:
    return std::make_shared<BuiltinType>(CHAR_T);
  case Builtin::Void:
    return std::make_shared<BuiltinType>(VOID_T);
  case Builtin::Unknown:
    throw std::runtime_error("gg");
  }
}
static std::shared_ptr<StructType>
from_type(const StructTypeAnnotation *type_annotation) {
  return std::make_shared<StructType>(
      StructType{type_annotation->get_name().data()});
}
static std::shared_ptr<NamedType>
from_type(const NamedTypeAnnotation *type_annotation) {
  return std::make_shared<NamedType>(
      NamedType{type_annotation->get_name().data()});
}
static std::shared_ptr<ArrayType>
from_type(const ArrayTypeAnnotation *type_annotation) {
  return std::make_shared<ArrayType>(
      ArrayType{from_type(type_annotation->get_type()).get()});
}
static std::shared_ptr<PointerType>
from_type(const PointerTypeAnnotation *type_annotation) {
  return std::make_shared<PointerType>(
      PointerType{from_type(type_annotation->get_type()).get()});
}
static std::shared_ptr<FunctionType>
from_type(const FunctionDeclaration *func_decl) {
  std::vector<const Type *> params{};
  for (const auto &item : func_decl->get_parameter_declarations()) {
    params.push_back(from_type(item->get_type()).get());
  }
  return std::make_shared<FunctionType>(
      FunctionType{func_decl->get_name().data(),
                   from_type(func_decl->get_return_type()).get(), params});
}

} // namespace type

#endif // DEFS_TYPE_H