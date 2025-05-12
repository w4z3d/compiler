#ifndef DEFS_TYPE_H
#define DEFS_TYPE_H

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

// TODO: maybe add function type (-> return type + list of param types)
class Type {
public:
  enum class Kind { Builtin, Pointer, Array, Struct, Named };
  constexpr explicit Type(Kind kind) : kind(kind) {}
  Kind kind;
  virtual ~Type() = default;
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
};

constexpr BuiltinType INT{BuiltinType::BuiltinKind::Int};
constexpr BuiltinType BOOL{BuiltinType::BuiltinKind::Bool};
constexpr BuiltinType CHAR{BuiltinType::BuiltinKind::Char};
constexpr BuiltinType STRING{BuiltinType::BuiltinKind::String};
constexpr BuiltinType VOID{BuiltinType::BuiltinKind::Void};

class PointerType : public Type {
public:
  const Type *to;

  explicit PointerType(const Type *to) : Type(Kind::Pointer), to(to) {}

  [[nodiscard]] bool equals(const Type &other) const {
    return other.kind == Kind::Pointer &&
           to == dynamic_cast<const PointerType &>(other).to;
  }
};

class ArrayType : public Type {
public:
  const Type *elementType;
  size_t length;

  ArrayType(const Type *elem, size_t len)
      : Type(Kind::Array), elementType(elem), length(len) {
    throw std::runtime_error("Not implemented yet");
  }

  bool operator==(const ArrayType &other) const {
    return elementType == other.elementType && length == other.length;
  }
};

class StructType : public Type {
public:
  // das ist nicht so optimal ig
  std::vector<std::pair<std::string, const Type *>> fields;

  explicit StructType(std::vector<std::pair<std::string, const Type *>> fields)
      : Type(Kind::Struct), fields(std::move(fields)) {
    throw std::runtime_error("Not implemented yet");
  }

  bool operator==(const StructType &other) const { return false; }
};

class NamedType : public Type {
public:
  std::string name;

  explicit NamedType(std::string name)
      : Type(Kind::Named), name(std::move(name)) {
    throw std::runtime_error("Not implemented yet");
  }

  bool operator==(const NamedType &other) const { return false; }
};

struct ArrayKey {
  const Type *element;
  size_t length;

  bool operator==(const ArrayKey &other) const {
    return element == other.element && length == other.length;
  }
};

struct StructKey {
  std::vector<std::pair<std::string, const Type *>> fields;

  bool operator==(const StructKey &other) const {
    return fields == other.fields;
  }
};

struct NamedKey {
  std::string name;

  bool operator==(const NamedKey &other) const { return name == other.name; }
};

namespace std {
template <> struct hash<ArrayKey> {
  size_t operator()(const ArrayKey &key) const {
    return hash<const Type *>()(key.element) ^ hash<size_t>()(key.length);
  }
};

template <> struct hash<StructKey> {
  size_t operator()(const StructKey &key) const {
    size_t h = 0;
    for (const auto &[name, type] : key.fields) {
      h ^= hash<std::string>()(name) ^ hash<const Type *>()(type);
    }
    return h;
  }
};

template <> struct hash<NamedKey> {
  size_t operator()(const NamedKey &key) const {
    return hash<std::string>()(key.name);
  }
};

template <> struct hash<PointerType> {
  size_t operator()(const PointerType &key) const {
    return hash<const Type *>()(key.to);
  }
};
}




class TypeInterner {
  std::unordered_map<const Type *, std::shared_ptr<PointerType>> pointerTypes;
  std::unordered_map<ArrayKey, std::shared_ptr<ArrayType>> arrayTypes;
  std::unordered_map<StructKey, std::shared_ptr<StructType>> structTypes;
  std::unordered_map<NamedKey, std::shared_ptr<NamedType>> namedTypes;

public:
  const ArrayType *getArray(const Type *elem, size_t length) {
    ArrayKey key{elem, length};
    auto it = arrayTypes.find(key);
    if (it != arrayTypes.end())
      return it->second.get();

    auto array = std::make_shared<ArrayType>(elem, length);
    arrayTypes[key] = array;
    return array.get();
  }

  const StructType *
  getStruct(const std::vector<std::pair<std::string, const Type *>> &fields) {
    StructKey key{fields};
    auto it = structTypes.find(key);
    if (it != structTypes.end())
      return it->second.get();

    auto strct = std::make_shared<StructType>(fields);
    structTypes[key] = strct;
    return strct.get();
  }

  const NamedType *getNamed(const std::string &name) {
    NamedKey key{name};
    auto it = namedTypes.find(key);
    if (it != namedTypes.end())
      return it->second.get();

    auto named = std::make_shared<NamedType>(name);
    namedTypes[key] = named;
    return named.get();
  }

  const PointerType *getPointer(const Type *to) {
    auto it = pointerTypes.find(to);
    if (it != pointerTypes.end())
      return it->second.get();

    auto ptr = std::make_shared<PointerType>(to);
    pointerTypes[to] = ptr;
    return ptr.get();
  }
};

#endif // DEFS_TYPE_H
