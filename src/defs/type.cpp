#include "type.hpp"

namespace type {
bool PointerType::equals(const Type &other) const {
  if (other.kind != Kind::Pointer)
    return false;
  const auto &otherPtr = dynamic_cast<const PointerType &>(other);
  if (!to || !otherPtr.to)
    return !to && !otherPtr.to; // Both null or both not null
  if (auto *struct_type = dynamic_cast<const StructType *>(otherPtr.to))
    printf("%p: %s\n", otherPtr.to, struct_type->toString().c_str());
  return to->equals(*otherPtr.to);
}
bool StructType::equals(const Type &other) const {

  printf("Struct eq: %s\n", other.toString().c_str());
  return other.kind == Kind::Struct &&
         name == dynamic_cast<const StructType &>(other).name;
}
} // namespace type
