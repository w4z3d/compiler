#include "hir_type.hpp"
#include <sstream>

namespace hir::type {

// ============================================================
// FunctionType::to_string
// ============================================================
std::string FunctionType::to_string() const {
  std::ostringstream oss;
  oss << "(";
  for (size_t i = 0; i < param_types.size(); i++) {
    if (i > 0)
      oss << ", ";
    oss << param_types[i]->to_string();
  }
  if (is_variadic) {
    if (!param_types.empty())
      oss << ", ";
    oss << "...";
  }
  oss << ") -> " << return_type->to_string();
  return oss.str();
}

// ============================================================
// TypeContext
// ============================================================
VoidType *TypeContext::void_t() { return &void_type_; }

PointerType *TypeContext::ptr() { return &pointer_type_; }

IntegerType *TypeContext::i1() { return get_integer(1); }
IntegerType *TypeContext::i8() { return get_integer(8); }
IntegerType *TypeContext::i16() { return get_integer(16); }
IntegerType *TypeContext::i32() { return get_integer(32); }
IntegerType *TypeContext::i64() { return get_integer(64); }

// Private helper — not in header, define inline here
IntegerType *TypeContext::get_integer(uint8_t width) {
  auto it = integer_types_.find(width);
  if (it != integer_types_.end())
    return &it->second;
  integer_types_.emplace(width, IntegerType{width});
  return &integer_types_.at(width);
}

ArrayType *TypeContext::get_array(size_t n, Type *inner_type) {
  // Deduplicate — same count and same inner type pointer
  for (const auto &a : array_types_) {
    if (a->count == n && a->inner_type == inner_type)
      return a.get();
  }
  array_types_.push_back(std::make_unique<ArrayType>(n, inner_type));
  return array_types_.back().get();
}

StructType *TypeContext::get_struct(std::string_view name) {
  // Return existing struct type if already created
  for (const auto &s : struct_types) {
    if (s->name == name)
      return s.get();
  }
  struct_types.push_back(std::make_unique<StructType>(std::string(name)));
  return struct_types.back().get();
}

FunctionType *TypeContext::get_function(Type *return_type,
                                        std::vector<Type *> params,
                                        bool variadic) {
  // Deduplicate by return type, param types and variadic flag
  for (const auto &f : function_types) {
    if (f->return_type != return_type)
      continue;
    if (f->is_variadic != variadic)
      continue;
    if (f->param_types != params)
      continue;
    return f.get();
  }
  auto ft = std::make_unique<FunctionType>(return_type, std::move(params));
  ft->is_variadic = variadic;
  function_types.push_back(std::move(ft));
  return function_types.back().get();
}

} // namespace hir::type
