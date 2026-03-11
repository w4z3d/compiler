#ifndef SRC_TYPE_SOURCE_TYPE_REGISTRY_HPP
#define SRC_TYPE_SOURCE_TYPE_REGISTRY_HPP
#include "source_type.hpp"

namespace source_type {
struct TypeRegistry {

  // Dedupe caches
  std::unordered_map<const std::string, const StructType &> struct_types;

  // Builtin types for dedupe
  const BuiltinType int_{BuiltinType::Builtin::Int};
  const BuiltinType float_{BuiltinType::Builtin::Float};
  const BuiltinType double_{BuiltinType::Builtin::Double};
  const BuiltinType char_{BuiltinType::Builtin::Char};
  const BuiltinType bool_{BuiltinType::Builtin::Bool};
  const BuiltinType long_{BuiltinType::Builtin::Long};
  const BuiltinType longlong_{BuiltinType::Builtin::LongLong};
  const BuiltinType unsigned_int_{BuiltinType::Builtin::UInt};
  const BuiltinType unsigned_char_{BuiltinType::Builtin::UChar};
  const BuiltinType unsigned_long_{BuiltinType::Builtin::ULong};

  // Return references to builtin types living in this struct
  const BuiltinType &get_int() { return int_; }
  const BuiltinType &get_float() { return float_; }
  const BuiltinType &get_double() { return double_; }
  const BuiltinType &get_char() { return char_; }
  const BuiltinType &get_bool() { return bool_; }
  const BuiltinType &get_long() { return long_; }
  const BuiltinType &get_longlong() { return longlong_; }
  const BuiltinType &get_unsigned_int() { return unsigned_int_; }
  const BuiltinType &get_unsigned_char() { return unsigned_char_; }
  const BuiltinType &get_unsigned_long() { return unsigned_long_; }
};
} // namespace source_type

#endif
