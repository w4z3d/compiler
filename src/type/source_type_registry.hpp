#ifndef SRC_TYPE_SOURCE_TYPE_REGISTRY_HPP
#define SRC_TYPE_SOURCE_TYPE_REGISTRY_HPP
#include "../defs/ast.hpp"
#include "source_type.hpp"
#include <cassert>
#include <unordered_map>
#include <utility>

namespace source_type {
struct TypeRegistry {
  arena::Arena &arena;

  explicit TypeRegistry(arena::Arena &arena) : arena(arena) {}
  struct ArrayKey {
    const Type *element;
    size_t length;
    bool operator==(const ArrayKey &o) const {
      return element == o.element && length == o.length;
    }
  };
  struct ArrayKeyHash {
    size_t operator()(const ArrayKey &k) const {
      return std::hash<const void *>()(k.element) ^
             (std::hash<size_t>()(k.length) << 1);
    }
  };
  //
  // Dedupe caches
  std::unordered_map<ArrayKey, ArrayType *, ArrayKeyHash> array_cache;
  std::unordered_map<std::string, StructType *> struct_cache;
  std::unordered_map<std::string, EnumType *> enum_cache;
  std::unordered_map<std::string, const Type *> typedef_map;
  std::unordered_map<const Type *, PointerType *> pointer_cache;
  std::vector<std::unique_ptr<Type>> owned;

  // Builtin types for dedupe
  BuiltinType void_type{BuiltinType::Builtin::Void};
  BuiltinType bool_type{BuiltinType::Builtin::Bool};
  BuiltinType char_type{BuiltinType::Builtin::Char};
  BuiltinType short_type{BuiltinType::Builtin::Short};
  BuiltinType int_type{BuiltinType::Builtin::Int};
  BuiltinType long_type{BuiltinType::Builtin::Long};
  BuiltinType uchar_type{BuiltinType::Builtin::UChar};
  BuiltinType ushort_type{BuiltinType::Builtin::UShort};
  BuiltinType uint_type{BuiltinType::Builtin::UInt};
  BuiltinType ulong_type{BuiltinType::Builtin::ULong};
  BuiltinType float_type{BuiltinType::Builtin::Float};
  BuiltinType double_type{BuiltinType::Builtin::Double};
  BuiltinType string_type{BuiltinType::Builtin::String};

  // Return references to builtin types living in this struct
  [[nodiscard]] const BuiltinType *get_void() { return &void_type; }
  [[nodiscard]] const BuiltinType *get_bool() { return &bool_type; }
  [[nodiscard]] const BuiltinType *get_char() { return &char_type; }
  [[nodiscard]] const BuiltinType *get_short() { return &short_type; }
  [[nodiscard]] const BuiltinType *get_int() { return &int_type; }
  [[nodiscard]] const BuiltinType *get_long() { return &long_type; }
  [[nodiscard]] const BuiltinType *get_uchar() { return &uchar_type; }
  [[nodiscard]] const BuiltinType *get_ushort() { return &ushort_type; }
  [[nodiscard]] const BuiltinType *get_uint() { return &uint_type; }
  [[nodiscard]] const BuiltinType *get_ulong() { return &ulong_type; }
  [[nodiscard]] const BuiltinType *get_float() { return &float_type; }
  [[nodiscard]] const BuiltinType *get_double() { return &double_type; }
  [[nodiscard]] const BuiltinType *get_string() { return &string_type; }

  template <typename T, typename... Args> T *make(Args &&...args) {
    auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
    auto *raw = ptr.get();
    owned.push_back(std::move(ptr));
    return raw;
  }

  const PointerType *get_pointer(const Type *pointee) {
    auto it = pointer_cache.find(pointee);
    if (it != pointer_cache.end())
      return it->second;

    auto *pt = arena.create<PointerType>(pointee);
    pointer_cache[pointee] = pt;
    return pt;
  }

  [[nodiscard]] const ArrayType *get_array(const Type *element,
                                           size_t length = 0) {
    ArrayKey key{element, length};
    auto it = array_cache.find(key);
    if (it != array_cache.end())
      return it->second;

    auto *at = arena.create<ArrayType>(element, length);
    array_cache[key] = at;
    return at;
  }
  StructType *get_struct(const std::string &name) {
    auto it = struct_cache.find(name);
    if (it != struct_cache.end())
      return it->second;

    auto *st = arena.create<StructType>(name);
    struct_cache[name] = st;
    return st;
  }

  const StructType *
  add_struct(const std::string &name,
             std::vector<std::pair<std::string, QualType>> fields) {
    auto it = struct_cache.find(name);
    if (it != struct_cache.end())
      return it->second;

    auto *st = arena.create<StructType>(name, fields);
    struct_cache[name] = st;
    return st;
  }

  void complete_struct(const std::string &name,
                       std::vector<std::pair<std::string, QualType>> fields) {

    auto *st = get_struct(name);
    assert(!st->is_complete && "struct already completed");
    st->fields = std::move(fields);
    st->is_complete = true;
  }

  [[nodiscard]] bool is_struct_complete(const std::string &name) const {
    auto it = struct_cache.find(name);
    return it != struct_cache.end() && it->second->is_complete;
  }

  EnumType *get_enum(const std::string &name) {
    auto it = enum_cache.find(name);
    if (it != enum_cache.end())
      return it->second;

    auto *et = arena.create<EnumType>(name);
    enum_cache[name] = et;
    return et;
  }

  const FunctionType *get_function(QualType return_type,
                                   std::vector<QualType> param_types,
                                   bool is_variadic = false) {

    return arena.create<FunctionType>(return_type, std::move(param_types),
                                      is_variadic);
  }
  void add_typedef(const std::string &name, const Type *type) {
    typedef_map[name] = type;
  }

  [[nodiscard]] const Type *resolve_typedef(const std::string &name) const {
    auto it = typedef_map.find(name);
    return it != typedef_map.end() ? it->second : nullptr;
  }

  const Type *resolve_through_typedefs(const Type *t) const {
    while (t && t->kind == Type::Kind::Named) {
      auto *named = static_cast<const NamedType *>(t);
      auto it = typedef_map.find(named->name);
      if (it == typedef_map.end())
        break;
      t = it->second;
    }
    return t;
  }

  // NOLINTBEGIN
  const Type *resolve(const TypeAnnotation *annotation) {
    if (!annotation)
      return nullptr;

    switch (annotation->kind) {
    case TypeAnnotation::Kind::Builtin: {
      auto *bt = static_cast<const BuiltinTypeAnnotation *>(annotation);
      return resolve_builtin(bt);
    }
    case TypeAnnotation::Kind::Named: {
      auto *nt = static_cast<const NamedTypeAnnotation *>(annotation);
      return resolve_name(std::string(nt->get_name()));
    }
    case TypeAnnotation::Kind::Pointer: {
      auto *pt = static_cast<const PointerTypeAnnotation *>(annotation);
      auto *inner = resolve(pt->get_type());
      return inner ? get_pointer(inner) : nullptr;
    }
    case TypeAnnotation::Kind::Array: {
      auto *at = static_cast<const ArrayTypeAnnotation *>(annotation);
      auto *inner = resolve(at->get_type());
      return inner ? get_array(inner, 0) : nullptr;
    }
    case TypeAnnotation::Kind::Struct: {
      auto *st = static_cast<const StructTypeAnnotation *>(annotation);
      return get_struct(std::string(st->get_name()));
    }
    }
    return nullptr;
  }
  // NOLINTEND

  QualType resolve_qual(const TypeAnnotation *annotation, unsigned quals = 0) {
    return {resolve(annotation), quals};
  }

  // Fast path: pointer equality (works because of interning)
  // Slow path: structural equality (fallback)
  bool types_equal(const Type *a, const Type *b) const {
    if (a == b)
      return true;
    if (!a || !b)
      return false;

    a = resolve_through_typedefs(a);
    b = resolve_through_typedefs(b);

    if (a == b)
      return true;

    return a->equals(*b);
  }

  [[nodiscard]] bool quals_compatible(QualType from, QualType to) const {
    if (!types_equal(from.unqualified(), to.unqualified()))
      return false;

    // Can always add const, but cannot remove it
    if (to.is_const() && !from.is_const())
      return true; // adding const is fine
    if (from.is_const() && !to.is_const())
      return false; // removing const is not

    return from.quals() == to.quals();
  }

private:
  const Type *resolve_builtin(const BuiltinTypeAnnotation *bt) {
    auto builtin_type = bt->get_type();
    switch (builtin_type) {
    case Builtin::Int:
      return get_int();
    case Builtin::Bool:
      return get_bool();
    case Builtin::Char:
      return get_char();
    case Builtin::String:
      return get_string();
    case Builtin::Void:
      return get_void();
    case Builtin::Unknown:
      std::unreachable();
    }
    return nullptr;
  }

  const Type *resolve_name(const std::string &name) {
    if (auto *td = resolve_typedef(name))
      return td;
    if (struct_cache.contains(name))
      return struct_cache[name];
    if (enum_cache.contains(name))
      return enum_cache[name];
    return nullptr;
  }
};
} // namespace source_type

#endif
