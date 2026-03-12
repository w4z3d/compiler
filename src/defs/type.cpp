#include "type.hpp"

namespace type {
bool PointerType::equals(const Type &other) const {
  if (other.kind != Kind::Pointer)
    return false;
  const auto &otherPtr = dynamic_cast<const PointerType &>(other);
  if (!to || !otherPtr.to)
    return !to && !otherPtr.to; // Both null or both not null
  return to->equals(*otherPtr.to);
}
bool StructType::equals(const Type &other) const {

  return other.kind == Kind::Struct &&
         name == dynamic_cast<const StructType &>(other).name;
}
} // namespace type
