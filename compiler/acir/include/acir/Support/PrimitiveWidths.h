#ifndef ACIR_SUPPORT_PRIMITIVEWIDTHS_H
#define ACIR_SUPPORT_PRIMITIVEWIDTHS_H

namespace acir {

inline constexpr unsigned kPrimitiveMinimumInputWidth = 1;
inline constexpr unsigned kPrimitiveMaximumInputWidth = 64;

constexpr bool isPrimitiveInputWidth(unsigned width) {
  return width >= kPrimitiveMinimumInputWidth &&
         width <= kPrimitiveMaximumInputWidth;
}

constexpr unsigned primitivePriorityIndexWidth(unsigned inputWidth) {
  unsigned result = 1;
  for (unsigned extent = 2; extent < inputWidth; extent <<= 1)
    ++result;
  return result;
}

constexpr unsigned primitiveCountWidth(unsigned inputWidth) {
  unsigned result = 1;
  for (unsigned representable = 1; representable < inputWidth;
       representable = (representable << 1) | 1)
    ++result;
  return result;
}

} // namespace acir

#endif // ACIR_SUPPORT_PRIMITIVEWIDTHS_H
