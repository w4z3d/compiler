#ifndef SRC_TYPE_SOURCE_TYPE_HPP
#define SRC_TYPE_SOURCE_TYPE_HPP

#include <string>
#include <utility>
#include <vector>

namespace source_type {

class QualType;

class Type {
public:
  enum class Kind { Builtin, Pointer, Array, Struct, Named, Enum, Function };
  Kind kind;

  explicit Type(Kind kind) : kind(kind) {}
  virtual ~Type() = default;

  [[nodiscard]] virtual std::string to_string() const = 0;
  [[nodiscard]] virtual bool equals(const Type &other) const = 0;

  [[nodiscard]] bool is_builtin() const { return kind == Kind::Builtin; }
  [[nodiscard]] bool is_pointer() const { return kind == Kind::Pointer; }
  [[nodiscard]] bool is_array() const { return kind == Kind::Array; }
  [[nodiscard]] bool is_struct() const { return kind == Kind::Struct; }
  [[nodiscard]] bool is_named() const { return kind == Kind::Named; }
  [[nodiscard]] bool is_enum() const { return kind == Kind::Enum; }
  [[nodiscard]] bool is_function() const { return kind == Kind::Function; }
};

class QualType {
  const Type *type = nullptr;
  unsigned qual_flags = 0;

public:
  static constexpr unsigned CONST = 1 << 0;
  static constexpr unsigned VOLATILE = 1 << 1;
  static constexpr unsigned RESTRICT = 1 << 2;

  QualType() = default;
  QualType(const Type *t, unsigned q = 0) : type(t), qual_flags(q) {}

  [[nodiscard]] const Type *unqualified() const { return type; }
  [[nodiscard]] unsigned quals() const { return qual_flags; }
  [[nodiscard]] bool is_null() const { return type == nullptr; }
  [[nodiscard]] bool is_const() const { return qual_flags & CONST; }
  [[nodiscard]] bool is_volatile() const { return qual_flags & VOLATILE; }

  [[nodiscard]] QualType with_const() const {
    return {type, qual_flags | CONST};
  }

  bool operator==(const QualType &o) const {
    return type == o.type && qual_flags == o.qual_flags;
  }
  bool operator!=(const QualType &o) const { return !(*this == o); }

  [[nodiscard]] std::string to_string() const {
    std::string s;
    if (is_const())
      s += "const ";
    if (is_volatile())
      s += "volatile ";
    if (type)
      s += type->to_string();
    else
      s += "<null>";
    return s;
  }
};

class BuiltinType : public Type {
public:
  enum class Builtin {
    Void,
    Bool,
    Char,
    Short,
    Int,
    Long,
    UChar,
    UShort,
    UInt,
    ULong,
    Float,
    Double,
    String,
  };

  Builtin builtin;

  explicit BuiltinType(Builtin b) : Type(Kind::Builtin), builtin(b) {}

  [[nodiscard]] std::string to_string() const override {
    switch (builtin) {
    case Builtin::Void:
      return "void";
    case Builtin::Bool:
      return "bool";
    case Builtin::Char:
      return "char";
    case Builtin::Short:
      return "short";
    case Builtin::Int:
      return "int";
    case Builtin::Long:
      return "long";
    case Builtin::UChar:
      return "unsigned char";
    case Builtin::UShort:
      return "unsigned short";
    case Builtin::UInt:
      return "unsigned int";
    case Builtin::ULong:
      return "unsigned long";
    case Builtin::Float:
      return "float";
    case Builtin::Double:
      return "double";
    case Builtin::String:
      return "string";
    }
    std::unreachable();
  }

  [[nodiscard]] bool equals(const Type &other) const override {
    if (other.kind != Kind::Builtin)
      return false;
    return static_cast<const BuiltinType &>(other).builtin == builtin;
  }

  [[nodiscard]] bool is_void() const { return builtin == Builtin::Void; }
  [[nodiscard]] bool is_bool() const { return builtin == Builtin::Bool; }
  [[nodiscard]] bool is_integer() const {
    return builtin >= Builtin::Char && builtin <= Builtin::ULong;
  }
  [[nodiscard]] bool is_floating() const {
    return builtin == Builtin::Float || builtin == Builtin::Double;
  }
  [[nodiscard]] bool is_signed() const {
    return builtin >= Builtin::Char && builtin <= Builtin::Long;
  }
  [[nodiscard]] bool is_unsigned() const {
    return builtin >= Builtin::UChar && builtin <= Builtin::ULong;
  }
};

class PointerType : public Type {
public:
  QualType pointee;

  explicit PointerType(QualType pointee)
      : Type(Kind::Pointer), pointee(pointee) {}

  explicit PointerType(const Type *t) : Type(Kind::Pointer), pointee(t) {}

  [[nodiscard]] std::string to_string() const override {
    return pointee.to_string() + "*";
  }

  [[nodiscard]] bool equals(const Type &other) const override {
    if (other.kind != Kind::Pointer)
      return false;
    return pointee == static_cast<const PointerType &>(other).pointee;
  }
};

class ArrayType : public Type {
public:
  const Type *element;
  size_t length; // 0 = unsized

  explicit ArrayType(const Type *elem, size_t len = 0)
      : Type(Kind::Array), element(elem), length(len) {}

  [[nodiscard]] std::string to_string() const override {
    if (length > 0)
      return element->to_string() + "[" + std::to_string(length) + "]";
    return element->to_string() + "[]";
  }

  [[nodiscard]] bool equals(const Type &other) const override {
    if (other.kind != Kind::Array)
      return false;
    auto &o = static_cast<const ArrayType &>(other);
    return element == o.element && length == o.length;
  }
};

class StructType : public Type {
public:
  std::string name;
  std::vector<std::pair<std::string, QualType>> fields;
  bool is_complete = false;

  explicit StructType(std::string name)
      : Type(Kind::Struct), name(std::move(name)) {}
  explicit StructType(std::string name,
                      std::vector<std::pair<std::string, QualType>> fields)
      : Type(Kind::Struct), name(std::move(name)), fields(std::move(fields)) {}

  [[nodiscard]] std::string to_string() const override {
    return "struct " + name;
  }

  // Nominal equality — two structs are equal iff same name
  [[nodiscard]] bool equals(const Type &other) const override {
    if (other.kind != Kind::Struct)
      return false;
    return name == static_cast<const StructType &>(other).name;
  }

  [[nodiscard]] const Type *field_type(const std::string &field) const {
    for (auto &[n, qt] : fields) {
      if (n == field)
        return qt.unqualified();
    }
    return nullptr;
  }

  [[nodiscard]] QualType field_qualtype(const std::string &field) const {
    for (auto &[n, qt] : fields) {
      if (n == field)
        return qt;
    }
    return {};
  }
};

class EnumType : public Type {
public:
  std::string name;
  std::vector<std::pair<std::string, int>> enumerators;

  explicit EnumType(std::string name)
      : Type(Kind::Enum), name(std::move(name)) {}

  [[nodiscard]] std::string to_string() const override {
    return "enum " + name;
  }

  [[nodiscard]] bool equals(const Type &other) const override {
    if (other.kind != Kind::Enum)
      return false;
    return name == static_cast<const EnumType &>(other).name;
  }
};

class FunctionType : public Type {
public:
  QualType return_type;
  std::vector<QualType> param_types;
  bool is_variadic;

  FunctionType(QualType ret, std::vector<QualType> params,
               bool variadic = false)
      : Type(Kind::Function), return_type(ret), param_types(std::move(params)),
        is_variadic(variadic) {}

  [[nodiscard]] std::string to_string() const override {
    std::string s = return_type.to_string() + "(";
    for (size_t i = 0; i < param_types.size(); i++) {
      if (i)
        s += ", ";
      s += param_types[i].to_string();
    }
    if (is_variadic) {
      if (!param_types.empty())
        s += ", ";
      s += "...";
    }
    return s + ")";
  }

  [[nodiscard]] bool equals(const Type &other) const override {
    if (other.kind != Kind::Function)
      return false;
    auto &o = static_cast<const FunctionType &>(other);
    if (return_type != o.return_type)
      return false;
    if (is_variadic != o.is_variadic)
      return false;
    if (param_types.size() != o.param_types.size())
      return false;
    for (size_t i = 0; i < param_types.size(); i++)
      if (param_types[i] != o.param_types[i])
        return false;
    return true;
  }
};

class NamedType : public Type {
public:
  std::string name;
  const Type *underlying;

  explicit NamedType(std::string name, const Type *underlying = nullptr)
      : Type(Kind::Named), name(std::move(name)), underlying(underlying) {}

  [[nodiscard]] std::string to_string() const override { return name; }

  [[nodiscard]] bool equals(const Type &other) const override {
    if (underlying)
      return underlying->equals(other);
    if (other.kind != Kind::Named)
      return false;
    return name == static_cast<const NamedType &>(other).name;
  }
};

} // namespace source_type

#endif
