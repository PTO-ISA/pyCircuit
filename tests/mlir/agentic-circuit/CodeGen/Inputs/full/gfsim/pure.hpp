#ifndef ACIR_TEST_FULL_GFSIM_PURE_HPP
#define ACIR_TEST_FULL_GFSIM_PURE_HPP

#include <concepts>

namespace gfsim {

struct Pure {};

template <typename T>
concept PureModel = std::same_as<T, Pure>;

inline bool is_ready() { return true; }
inline bool is_ready(bool value) { return value; }

} // namespace gfsim

#endif // ACIR_TEST_FULL_GFSIM_PURE_HPP
