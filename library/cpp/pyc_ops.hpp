#pragma once

#include "pyc_bits.hpp"

namespace pyc::cpp {

template <unsigned Width>
struct Add {
  static constexpr Wire<Width> eval(Wire<Width> a, Wire<Width> b) { return a + b; }
};

template <unsigned Width>
struct Mux {
  static constexpr Wire<Width> eval(Wire<1> sel, Wire<Width> a, Wire<Width> b) { return sel.toBool() ? a : b; }
};

template <unsigned Width>
struct And {
  static constexpr Wire<Width> eval(Wire<Width> a, Wire<Width> b) { return a & b; }
};

template <unsigned Width>
struct Or {
  static constexpr Wire<Width> eval(Wire<Width> a, Wire<Width> b) { return a | b; }
};

template <unsigned Width>
struct Xor {
  static constexpr Wire<Width> eval(Wire<Width> a, Wire<Width> b) { return a ^ b; }
};

template <unsigned Width>
struct Not {
  static constexpr Wire<Width> eval(Wire<Width> a) { return ~a; }
};

} // namespace pyc::cpp
