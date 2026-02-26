#ifndef DEFS_TYPE_H
#define DEFS_TYPE_H
#include <sstream> // For easily building strings
#include <string>
#include <utility> // For std::pair
#include <vector>

// Potentially needed for PointerType, ArrayType etc. to call toString on their
// members Make sure to include the full header where these types are defined.

namespace type {

class Type {
public:
  enum class Kind { Builtin, Pointer, Array, Struct, Named, Function };
  constexpr explicit Type(Kind kind) : kind(kind) {}
  Kind kind;

  [[nodiscard]] virtual bool equals(const Type &other) const {
    return kind == other.kind;
  }

  // Add pure virtual toString method
  [[nodiscard]] virtual std::string toString() const { return ""; }
};

// --- BuiltinType ---
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
    if (other.kind != Kind::Builtin)
      return false;
    const auto *o = dynamic_cast<const BuiltinType *>(&other);
    return o && builtinKind == o->builtinKind;
  }

  [[nodiscard]] std::string toString() const override {
    switch (builtinKind) {
    case BuiltinKind::Int:
      return "int";
    case BuiltinKind::Char:
      return "char";
    case BuiltinKind::String:
      return "string";
    case BuiltinKind::Bool:
      return "bool";
    case BuiltinKind::Void:
      return "void";
    default:
      return "UnknownBuiltin";
    }
  }
};

// --- PointerType ---
class PointerType : public Type {
public:
  const Type *to;

  explicit PointerType(const Type *to) : Type(Kind::Pointer), to(to) {}

  [[nodiscard]] bool equals(const Type &other) const override {
    if (other.kind != Kind::Pointer)
      return false;
    const auto &otherPtr = dynamic_cast<const PointerType &>(other);
    if (!to || !otherPtr.to)
      return !to && !otherPtr.to; // Both null or both not null
    return to->equals(*otherPtr.to);
  }

  [[nodiscard]] std::string toString() const override {
    if (to) {
      return to->toString() + "*";
    }
    return "nullptr*";
  }
};

// --- ArrayType ---
class ArrayType : public Type {
public:
  const Type *elementType;
  size_t length = 0;

  explicit ArrayType(const Type *elem) : Type(Kind::Array), elementType(elem) {}

  [[nodiscard]] bool equals(const Type &other) const override {
    if (other.kind != Kind::Array)
      return false;
    const auto &otherArray = dynamic_cast<const ArrayType &>(other);
    if (!elementType || !otherArray.elementType)
      return !elementType && !otherArray.elementType;
    return elementType->equals(*otherArray.elementType);
  }
  void set_len(size_t len) { length = len; }
  [[nodiscard]] size_t get_len() const { return length; }

  [[nodiscard]] std::string toString() const override {
    std::string len_str;
    if (length > 0 || (elementType != nullptr)) {
      len_str = std::to_string(length);
    }
    if (elementType) {
      return elementType->toString() + "[" + len_str + "]";
    }
    return "nullptr_element_type[" + len_str + "]";
  }
};

// --- StructType ---
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

  [[nodiscard]] std::string toString() const override {
    std::ostringstream oss;
    oss << "struct " << name << " { ";
    for (size_t i = 0; i < fields.size(); ++i) {
      oss << fields[i].first << ": ";
      if (fields[i].second) {
        oss << fields[i].second->toString();
      } else {
        oss << "nullptr_field_type";
      }
      if (i < fields.size() - 1) {
        oss << "; ";
      }
    }
    oss << " }";
    return oss.str();
  }
};

// --- NamedType ---
class NamedType : public Type {
public:
  std::string name;
  Type *type{}; // Pointer to the actual type definition (could be nullptr if
                // unresolved)

  explicit NamedType(std::string name)
      : Type(Kind::Named), name(std::move(name)) {}

  void set_type(Type *t) { type = t; }
  [[nodiscard]] bool equals(const Type &other) const override {
    return other.kind == Kind::Named &&
           name == dynamic_cast<const NamedType &>(other).name;
  }

  [[nodiscard]] std::string toString() const override {
    if (type) {
      return name + " (alias for " + type->toString() + ")";
    }
    return name;
  }
};

// --- FunctionType ---
class FunctionType : public Type {
public:
  std::string name; // Name of the function
  const Type *returnType;
  std::vector<const Type *> paramTypes;

  FunctionType(std::string name, const Type *returnType,
               std::vector<const Type *> paramTypes)
      : Type(Kind::Function), name(std::move(name)), returnType(returnType),
        paramTypes(std::move(paramTypes)) {}

  [[nodiscard]] bool equals(const Type &other) const override {
    if (other.kind != Kind::Function)
      return false;
    const auto &otherFunc = dynamic_cast<const FunctionType &>(other);
    if (name != otherFunc.name)
      return false;
    if (!returnType || !otherFunc.returnType) { // check nullity
      if (returnType != otherFunc.returnType)
        return false; // one is null, other isn't
    } else if (!returnType->equals(*otherFunc.returnType)) {
      return false;
    }
    if (paramTypes.size() != otherFunc.paramTypes.size())
      return false;
    for (size_t i = 0; i < paramTypes.size(); ++i) {
      if (!paramTypes[i] || !otherFunc.paramTypes[i]) { // check nullity
        if (paramTypes[i] != otherFunc.paramTypes[i])
          return false;
      } else if (!paramTypes[i]->equals(*otherFunc.paramTypes[i])) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] std::string toString() const override {
    std::ostringstream oss;
    oss << name << "("; // Using the 'name' field as the function's name
    for (size_t i = 0; i < paramTypes.size(); ++i) {
      if (paramTypes[i]) {
        oss << paramTypes[i]->toString();
      } else {
        oss << "nullptr_param_type";
      }
      if (i < paramTypes.size() - 1) {
        oss << ", ";
      }
    }
    oss << ") -> ";
    if (returnType) {
      oss << returnType->toString();
    } else {
      oss << "nullptr_return_type";
    }
    return oss.str();
  }
};

inline const BuiltinType *int_t() {
  static BuiltinType t{BuiltinType::BuiltinKind::Int};
  return &t;
}
inline const BuiltinType *bool_t() {
  static BuiltinType t{BuiltinType::BuiltinKind::Bool};
  return &t;
}
inline const BuiltinType *char_t() {
  static BuiltinType t{BuiltinType::BuiltinKind::Char};
  return &t;
};
inline const BuiltinType *string_t() {
  static BuiltinType t{BuiltinType::BuiltinKind::String};
  return &t;
};
inline const BuiltinType *void_t() {
  static BuiltinType t{BuiltinType::BuiltinKind::Void};
  return &t;
};

} // namespace type

#endif
#ifndef SRC_DEFS_TYPE_HPP
#define SRC_DEFS_TYPE_HPP

#endif // SRC_DEFS_TYPE_HPP
