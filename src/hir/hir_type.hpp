#ifndef SRC_HIR_HIR_TYPE_HPP
#define SRC_HIR_HIR_TYPE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace hir::type {

struct IntegerType;

struct Type {
  enum class Kind { Void, Integer, Float, Pointer, Struct, Array, Function };

  [[nodiscard]] virtual Kind kind() const = 0;
  [[nodiscard]] virtual std::string to_string() const = 0;
  virtual ~Type() = default;

  [[nodiscard]] bool is_void() const { return kind() == Kind::Void; };
  [[nodiscard]] bool is_integer() const { return kind() == Kind::Integer; };
  [[nodiscard]] bool is_float() const { return kind() == Kind::Float; };
  [[nodiscard]] bool is_pointer() const { return kind() == Kind::Pointer; };
  [[nodiscard]] bool is_struct() const { return kind() == Kind::Struct; };
  [[nodiscard]] bool is_array() const { return kind() == Kind::Array; };
  [[nodiscard]] bool is_function() const { return kind() == Kind::Function; };

  [[nodiscard]] inline virtual size_t size_of() const = 0;
};

struct VoidType : Type {
  [[nodiscard]] Kind kind() const override { return Kind::Void; }
  [[nodiscard]] std::string to_string() const override { return "void"; }
  [[nodiscard]] inline size_t size_of() const override { return 8; };
};

struct IntegerType : Type {
  uint8_t width; // powers of 2

  explicit IntegerType(uint8_t width) : width(width) {}
  [[nodiscard]] Kind kind() const override { return Kind::Integer; }
  [[nodiscard]] std::string to_string() const override {
    return "i" + std::to_string(width);
  }
  [[nodiscard]] inline size_t size_of() const override {
    return (width + 7) / 8;
  };
};

struct PointerType : Type {
  [[nodiscard]] Kind kind() const override { return Kind::Pointer; }
  [[nodiscard]] std::string to_string() const override { return "ptr"; }
  [[nodiscard]] inline size_t size_of() const override { return 8; };
};

struct FloatType : Type {
  uint8_t width; // powers of 2

  explicit FloatType(uint8_t width) : width(width) {}

  [[nodiscard]] Kind kind() const override { return Kind::Float; }
  [[nodiscard]] std::string to_string() const override {
    return "f" + std::to_string(width);
  }
  [[nodiscard]] inline size_t size_of() const override {
    return (width + 7) / 8;
  };
};

struct ArrayType : Type {
  size_t count; // Array sizes must be known at compile time
  Type *inner_type;

  explicit ArrayType(size_t count, Type *inner_type)
      : count(count), inner_type(inner_type) {}

  [[nodiscard]] Kind kind() const override { return Kind::Array; };
  [[nodiscard]] std::string to_string() const override {
    return "[" + std::to_string(count) + "x" + inner_type->to_string() + "]";
  }
  [[nodiscard]] inline size_t size_of() const override {
    return count * inner_type->size_of();
  };
};
struct FunctionType : Type {
  Type *return_type;
  std::vector<Type *> param_types;
  bool is_variadic = false;
  FunctionType(Type *ret, std::vector<Type *> params)
      : return_type(ret), param_types(std::move(params)) {}
  [[nodiscard]] Kind kind() const override { return Kind::Function; }
  [[nodiscard]] std::string to_string() const override;
  [[nodiscard]] inline size_t size_of() const override { return 0; };
};

struct StructType : Type {
  std::string name;
  std::vector<std::pair<std::string, Type *>> fields;
  std::vector<size_t> offsets; // filled by layout pass
  size_t size = 0;
  explicit StructType(std::string n,
                      std::vector<std::pair<std::string, Type *>> &fields)
      : name(std::move(n)), fields(std::move(fields)) {}
  [[nodiscard]] Kind kind() const override { return Kind::Struct; }
  [[nodiscard]] std::string to_string() const override { return "%" + name; }
  [[nodiscard]] inline size_t size_of() const override {
    size_t size = 0;
    for (auto &[name, field] : fields) {
      size += field->size_of();
    }
    return size;
  };
};

struct TypeContext {
  [[nodiscard]] VoidType *void_t();

  [[nodiscard]] IntegerType *i1();
  [[nodiscard]] IntegerType *i8();
  [[nodiscard]] IntegerType *i16();
  [[nodiscard]] IntegerType *i32();
  [[nodiscard]] IntegerType *i64();

  [[nodiscard]] PointerType *ptr();

  [[nodiscard]] ArrayType *get_array(size_t n, Type *inner_type);

  [[nodiscard]] StructType *
  get_struct(std::string_view name,
             std::vector<std::pair<std::string, Type *>> &fields);
  [[nodiscard]] FunctionType *
  get_function(Type *return_type, std::vector<Type *> params, bool variadic);

private:
  VoidType void_type_;
  PointerType pointer_type_;
  std::unordered_map<uint8_t, IntegerType> integer_types_;
  std::vector<std::unique_ptr<ArrayType>> array_types_;
  std::vector<std::unique_ptr<StructType>> struct_types;
  std::vector<std::unique_ptr<FunctionType>> function_types;

  IntegerType *get_integer(uint8_t width);
};
} // namespace hir::type

#endif // SRC_HIR_HIR_TYPE_HPP
