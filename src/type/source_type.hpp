#ifndef SRC_TYPE_SOURCE_TYPE_HPP
#define SRC_TYPE_SOURCE_TYPE_HPP

#include <string>
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

struct PointerType : Type {};

struct ArrayType : Type {};

struct StructType : Type {};

struct NamedType : Type {};
struct EnumType : Type {};
struct FunctionType : Type {};
} // namespace source_type
//
#endif
