#ifndef SRC_TYPE_SOURCE_TYPE_HPP
#define SRC_TYPE_SOURCE_TYPE_HPP

#include <string>
#include <unordered_map>
#include <utility>
namespace source_type {
struct Type {
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

  bool operator==(const Type &other) const { return equals(other); }
};

struct BuiltinType : Type {
  enum class Builtin {
    Void,
    Bool,
    Char,
    Short,
    Int,
    Long,
    LongLong,
    UChar,
    UShort,
    UInt,
    ULong,
    Float,
    Double,
    String,
  };

  Builtin builtin;

  explicit BuiltinType(Builtin builtin)
      : Type(Kind::Builtin), builtin(builtin) {}

  [[nodiscard]] std::string to_string() const override {
    switch (builtin) {
    case Builtin::Int:
      return "int";
    case Builtin::Float:
      return "float";
    case Builtin::Double:
      return "double";
    case Builtin::Char:
      return "char";
    case Builtin::Short:
      return "short";
    case Builtin::Long:
      return "long";
    case Builtin::LongLong:
      return "longlong";
    case Builtin::Void:
      return "void";
    case Builtin::UChar:
      return "unsigned char";
    case Builtin::UInt:
      return "unsigned int";
    case Builtin::ULong:
      return "unsigned long";
    case Builtin::UShort:
      return "unsigned short";
    case Builtin::String:
      return "string";
    default:
      std::unreachable();
    }
  }

  // NOLINTBEGIN
  [[nodiscard]] bool equals(const Type &other) const override {
    return other.kind == Kind::Builtin &&
           (static_cast<const BuiltinType &>(other)).builtin == this->builtin;
  }
  // NOLINTEND
};

struct PointerType : Type {
  const Type &pointee;

  explicit PointerType(const Type &pointee)
      : Type(Kind::Pointer), pointee(pointee) {}

  [[nodiscard]] std::string to_string() const override {
    return pointee.to_string() + "*";
  }

  // NOLINTBEGIN
  [[nodiscard]] bool equals(const Type &other) const override {
    if (other.kind != Kind::Pointer)
      return false;
    auto &other_ptr = static_cast<const PointerType &>(other);
    return other_ptr.pointee.equals(pointee);
  }
  // NOLINTEND
};

struct ArrayType : Type {
  const Type &base;
  const size_t size;

  explicit ArrayType(const Type &base, size_t size)
      : Type(Kind::Array), base(base), size(size) {}

  [[nodiscard]] std::string to_string() const override {
    return base.to_string() + "[" + std::to_string(size) + "]";
  }

  // NOLINTBEGIN
  [[nodiscard]] bool equals(const Type &other) const override {
    if (other.kind != Kind::Array)
      return false;
    auto &other_arr = static_cast<const ArrayType &>(other);
    return base.equals(other_arr.base) && size == other_arr.size;
  }
  // NOLINTEND
};

struct StructType : Type {
  const std::string name;
  const std::unordered_map<const std::string, const Type &> field_type_map;

  size_t size;
  explicit StructType(
      std::string name,
      const std::unordered_map<const std::string, const Type &> &field_type_map,
      size_t size)
      : Type(Kind::Struct), name(std::move(name)),
        field_type_map(field_type_map), size(size) {}

  [[nodiscard]] std::string to_string() const override {
    return "struct " + name;
  }

  // NOLINTBEGIN
  [[nodiscard]] bool equals(const Type &other) const override {
    if (other.kind != Kind::Struct)
      return false;

    auto &other_struct = static_cast<const StructType &>(other);
    if (name != other_struct.name)
      return false;
    for (const auto &[name, field] : field_type_map) {
      auto it = other_struct.field_type_map.find(name);
      if (it == other_struct.field_type_map.end() ||
          !it->second.equals(field)) {
        return false;
      }
    }
    return true;
  }
  // NOLINTEND
};

struct NamedType : Type {
  const Type &underlying_type;
  const std::string name;

  explicit NamedType(const Type &underlying_type, std::string name)
      : Type(Kind::Named), name(std::move(name)),
        underlying_type(underlying_type) {}

  [[nodiscard]] std::string to_string() const override {
    return name + " alias " + underlying_type.to_string();
  }

  // NOLINTBEGIN
  [[nodiscard]] bool equals(const Type &other) const override {
    if (other.kind != Kind::Named)
      return false;
    auto &other_named = static_cast<const NamedType &>(other);
    return underlying_type.equals(other_named.underlying_type);
  }
  // NOLINTEND
};

// TODO: Implement for C compiler
struct EnumType : Type {};
struct FunctionType : Type {};
} // namespace source_type
//
#endif
