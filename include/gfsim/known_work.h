// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "gfsim/wire.h"

namespace gfsim::detail::known_work {

// The generated Work entry checks the borrowed inputs before entering this
// evaluator. The admitted operations are closed over fully known values.
template <unsigned Width> class Word {
public:
  static Word known(Bits<Width> value) noexcept { return {value}; }
  const Bits<Width> &value() const noexcept { return bits_; }
  constexpr bool isFullyKnown() const noexcept { return true; }

  template <unsigned N>
  void assignSlice(unsigned low, const Word<N> &source) noexcept {
    assignPackedSlice<Width, N>(
        low, [&](const auto &update) { update(bits_, source.value()); });
  }

private:
  Word(Bits<Width> bits) noexcept : bits_(bits) {}
  Bits<Width> bits_;
};

template <class T> class Wire {
public:
  static constexpr unsigned width = hardware_traits<T>::width;
  using Packed = Word<width>;
  static Wire fromPacked(Packed value) noexcept { return Wire(value); }
  static Wire known(const T &value) noexcept {
    return fromPacked(Packed::known(hardware_traits<T>::pack(value)));
  }

  Wire() noexcept = default;
  const Packed &packed() const noexcept { return value_; }
  operator gfsim::wire<T>() const noexcept {
    return gfsim::wire<T>::fromPacked(FourState<width>::known(value_.value()));
  }

  template <unsigned N>
  void assignSlice(unsigned low, const Word<N> &source) noexcept {
    value_.assignSlice(low, source);
  }

private:
  explicit Wire(Packed value) noexcept : value_(value) {}
  Packed value_ = Packed::known({});
};

template <class T, std::size_t... Extents>
class Wire<gfsim::table<T, Extents...>> {
public:
  using Table = gfsim::table<T, Extents...>;
  using element_type = typename Table::element_type;
  static constexpr std::size_t size = Table::size;

  Wire<element_type> &element(std::size_t row) noexcept {
    return elements_[row];
  }
  const Wire<element_type> &element(std::size_t row) const noexcept {
    return elements_[row];
  }

private:
  std::array<Wire<element_type>, size> elements_{};
};

template <class Plane> struct BorrowedPlane {
  const Plane &source;
  auto element(std::size_t row) const noexcept {
    using Cell = Wire<typename Plane::element_type>;
    return Cell::fromPacked(
        Cell::Packed::known(source.element(row).packed().value()));
  }
};

template <unsigned Width>
constexpr bool coversIndex(std::uint64_t size) noexcept {
  if constexpr (Width < 64)
    return (size >> Width) != 0;
  return false;
}

template <unsigned W> Word<W> bit_not(const Word<W> &x) noexcept {
  return Word<W>::known(~x.value());
}

template <unsigned W>
Word<W> bit_and(const Word<W> &x, const Word<W> &y) noexcept {
  return Word<W>::known(x.value() & y.value());
}

template <unsigned W>
Word<W> bit_or(const Word<W> &x, const Word<W> &y) noexcept {
  return Word<W>::known(x.value() | y.value());
}

template <unsigned W>
Word<W> bit_xor(const Word<W> &x, const Word<W> &y) noexcept {
  return Word<W>::known(x.value() ^ y.value());
}

template <unsigned W> Word<W> add(const Word<W> &x, const Word<W> &y) noexcept {
  return Word<W>::known(x.value() + y.value());
}

template <unsigned W> Word<W> sub(const Word<W> &x, const Word<W> &y) noexcept {
  return Word<W>::known(x.value() - y.value());
}

template <unsigned W> Word<W> mul(const Word<W> &x, const Word<W> &y) noexcept {
  return Word<W>::known(x.value() * y.value());
}

template <ComparePredicate P, unsigned W>
Word<1> compare(const Word<W> &x, const Word<W> &y) noexcept {
  return Word<1>::known(gfsim::compare<P>(FourState<W>::known(x.value()),
                                          FourState<W>::known(y.value()))
                            .value());
}

template <unsigned W>
Word<W> select(const Word<1> &condition, const Word<W> &a,
               const Word<W> &b) noexcept {
  return condition.value().toBool() ? a : b;
}

template <unsigned Out, unsigned In>
Word<Out> extract(const Word<In> &x, unsigned low) noexcept {
  return Word<Out>::known(gfsim::extract<Out>(x.value(), low));
}

template <unsigned Out, unsigned In>
Word<Out> truncate(const Word<In> &x) noexcept {
  return Word<Out>::known(gfsim::trunc<Out>(x.value()));
}

template <unsigned Out, unsigned In>
Word<Out> zero_extend(const Word<In> &x) noexcept {
  return Word<Out>::known(gfsim::zext<Out>(x.value()));
}

template <unsigned Out, unsigned In>
Word<Out> sign_extend(const Word<In> &x) noexcept {
  return Word<Out>::known(gfsim::sext<Out>(x.value()));
}

template <unsigned A, unsigned B>
Word<A + B> concat(const Word<A> &a, const Word<B> &b) noexcept {
  return Word<A + B>::known(gfsim::concat(a.value(), b.value()));
}

template <unsigned W, unsigned A>
Word<W> shl(const Word<W> &x, const Word<A> &amount) noexcept {
  auto shift = gfsim::boundedShift<W>(FourState<A>::known(amount.value()));
  return Word<W>::known(gfsim::shl(x.value(), shift));
}

template <unsigned W, unsigned A>
Word<W> lshr(const Word<W> &x, const Word<A> &amount) noexcept {
  auto shift = gfsim::boundedShift<W>(FourState<A>::known(amount.value()));
  return Word<W>::known(gfsim::lshr(x.value(), shift));
}

template <unsigned W, unsigned A>
Word<W> ashr(const Word<W> &x, const Word<A> &amount) noexcept {
  auto shift = gfsim::boundedShift<W>(FourState<A>::known(amount.value()));
  return Word<W>::known(gfsim::ashr(x.value(), shift));
}

} // namespace gfsim::detail::known_work
