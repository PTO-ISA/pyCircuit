#ifndef GFSIM_PRIMITIVE_WIDTHS_H
#define GFSIM_PRIMITIVE_WIDTHS_H

namespace gfsim {

inline constexpr unsigned PrimitiveMinimumInputWidth = 1;
inline constexpr unsigned PrimitiveMaximumInputWidth = 64;

template <unsigned Width>
inline constexpr bool IsPrimitiveInputWidth =
    Width >= PrimitiveMinimumInputWidth && Width <= PrimitiveMaximumInputWidth;

template <unsigned Width>
inline constexpr unsigned PriorityIndexWidth = [] {
  static_assert(IsPrimitiveInputWidth<Width>,
                "semantic primitive width must be in [1, 64]");
  unsigned result = 1;
  for (unsigned extent = 2; extent < Width; extent <<= 1)
    ++result;
  return result;
}();

template <unsigned Width>
inline constexpr unsigned CountWidth = [] {
  static_assert(IsPrimitiveInputWidth<Width>,
                "semantic primitive width must be in [1, 64]");
  unsigned result = 1;
  for (unsigned representable = 1; representable < Width;
       representable = (representable << 1) | 1)
    ++result;
  return result;
}();

} // namespace gfsim

#endif // GFSIM_PRIMITIVE_WIDTHS_H
