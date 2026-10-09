// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "gfsim/four_state.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

namespace gfsim {

// A hardware payload has a finite, explicit bit layout. There is deliberately
// no fallback to the host object's size, padding or byte representation.
template <class T> struct hardware_traits;

template <unsigned Width> struct hardware_traits<Bits<Width>> {
  static constexpr unsigned width = Width;

  static Bits<Width> pack(const Bits<Width> &value) noexcept { return value; }
  static Bits<Width> unpack(const Bits<Width> &value) noexcept { return value; }
};

namespace detail {

template <class T> struct hardware_member;

template <class Owner, class Field> struct hardware_member<Field Owner::*> {
  using owner_type = Owner;
  using field_type = Field;
};

template <auto Left, auto Right> constexpr bool sameHardwareMember() noexcept {
  if constexpr (std::is_same_v<decltype(Left), decltype(Right)>)
    return Left == Right;
  return false;
}

template <auto... Members> struct distinct_hardware_members : std::true_type {};

template <auto First, auto... Rest>
struct distinct_hardware_members<First, Rest...>
    : std::bool_constant<(!sameHardwareMember<First, Rest>() && ...) &&
                         distinct_hardware_members<Rest...>::value> {};

} // namespace detail

// Static table payloads use logical row-major order. Nested tables concatenate
// their shapes; the storage always contains the scalar leaf payloads.
template <class T, std::size_t... Extents> struct table;

namespace detail {
template <class T> struct table_payload {
  using leaf_type = T;
  static constexpr std::size_t size = 1;
  static constexpr std::size_t rank = 0;
  static constexpr std::array<std::size_t, 0> shape{};
};

template <class T, std::size_t... Extents>
struct table_payload<table<T, Extents...>> {
  using leaf_type = typename table_payload<T>::leaf_type;
  static constexpr std::size_t rank =
      sizeof...(Extents) + table_payload<T>::rank;
  static constexpr auto shape = [] {
    std::array<std::size_t, rank> result{};
    std::size_t axis = 0;
    ((result[axis++] = Extents), ...);
    for (auto extent : table_payload<T>::shape)
      result[axis++] = extent;
    return result;
  }();
  static constexpr std::size_t size = [] {
    std::size_t count = table_payload<T>::size;
    for (auto extent :
         std::array<std::size_t, sizeof...(Extents)>{Extents...}) {
      if (extent == 0 ||
          count > std::numeric_limits<std::int64_t>::max() / extent)
        return std::size_t{0};
      count *= extent;
    }
    return count;
  }();
};
} // namespace detail

template <class T, std::size_t... Extents> struct table {
  static_assert(sizeof...(Extents) > 0, "table rank must be positive");
  static_assert(((Extents > 0) && ...), "table extents must be positive");
  using descriptor = detail::table_payload<table>;
  using element_type = typename descriptor::leaf_type;
  static constexpr std::size_t size = descriptor::size;
  static constexpr std::size_t rank = descriptor::rank;
  static constexpr auto shape = descriptor::shape;
  static_assert(size > 0,
                "table shape product exceeds positive signed-64 range");
  std::array<element_type, size> elements{};
  element_type &operator[](std::size_t ordinal) noexcept {
    return elements[ordinal];
  }
  const element_type &operator[](std::size_t ordinal) const noexcept {
    return elements[ordinal];
  }
};

template <class T, std::size_t... Extents>
struct hardware_traits<table<T, Extents...>> {
  using value_type = table<T, Extents...>;
  using element_type = typename value_type::element_type;
  using ElementTraits = hardware_traits<element_type>;
  static_assert(value_type::size <=
                    std::numeric_limits<unsigned>::max() / ElementTraits::width,
                "table packed width exceeds runtime bit-width range");
  static constexpr unsigned width = value_type::size * ElementTraits::width;
  static Bits<width> pack(const value_type &value) noexcept {
    Bits<width> result{};
    for (std::size_t ordinal = 0; ordinal < value_type::size; ++ordinal) {
      auto offset = static_cast<unsigned>((value_type::size - 1 - ordinal) *
                                          ElementTraits::width);
      result = result |
               shl(zext<width>(ElementTraits::pack(value[ordinal])), offset);
    }
    return result;
  }
  static value_type unpack(const Bits<width> &value) noexcept {
    value_type result{};
    for (std::size_t ordinal = 0; ordinal < value_type::size; ++ordinal) {
      auto offset = static_cast<unsigned>((value_type::size - 1 - ordinal) *
                                          ElementTraits::width);
      result[ordinal] =
          ElementTraits::unpack(extract<ElementTraits::width>(value, offset));
    }
    return result;
  }
};

// Specialize hardware_traits<Record> by inheriting this helper with the
// ordered data-member pointers. The first field occupies the most significant
// bits, recursively using each field's hardware_traits. Every hardware field
// must occur once in the declared layout; C++ does not reflect omitted fields.
template <class T, auto... Members> struct hardware_struct_traits {
  static_assert((std::is_member_object_pointer_v<decltype(Members)> && ...),
                "hardware layout requires data-member pointers");
  static_assert(detail::distinct_hardware_members<Members...>::value,
                "hardware layout must not repeat a member");
  static_assert(
      (std::is_same_v<
           typename detail::hardware_member<decltype(Members)>::owner_type,
           T> &&
       ...),
      "hardware layout members must belong to the declared struct");
  static_assert(std::is_nothrow_default_constructible_v<T> &&
                    std::is_nothrow_move_constructible_v<T>,
                "hardware unpack requires no-throw value construction");

  static constexpr auto fields = std::tuple{Members...};

private:
  template <auto Member>
  using field_type =
      typename detail::hardware_member<decltype(Member)>::field_type;

  static constexpr std::uintmax_t totalWidth =
      (std::uintmax_t{0} + ... + hardware_traits<field_type<Members>>::width);
  static_assert(totalWidth > 0 &&
                    totalWidth <= std::numeric_limits<unsigned>::max(),
                "hardware layout width must be positive and representable");

public:
  static constexpr unsigned width = static_cast<unsigned>(totalWidth);

  static Bits<width> pack(const T &value) noexcept {
    Bits<width> result;
    unsigned offset = width;
    (packField<Members>(result, offset, value), ...);
    return result;
  }

  static T unpack(const Bits<width> &value) noexcept {
    T result{};
    unsigned offset = width;
    (unpackField<Members>(result, offset, value), ...);
    return result;
  }

private:
  template <auto Member>
  static void packField(Bits<width> &result, unsigned &offset,
                        const T &value) noexcept {
    using Traits = hardware_traits<field_type<Member>>;
    static_assert(noexcept(Traits::pack(value.*Member)),
                  "hardware field pack must not throw");
    offset -= Traits::width;
    result = result | shl(zext<width>(Traits::pack(value.*Member)), offset);
  }

  template <auto Member>
  static void unpackField(T &result, unsigned &offset,
                          const Bits<width> &value) noexcept {
    using Traits = hardware_traits<field_type<Member>>;
    static_assert(
        noexcept(Traits::unpack(std::declval<const Bits<Traits::width> &>())),
        "hardware field unpack must not throw");
    static_assert(
        std::is_nothrow_assignable_v<field_type<Member> &, field_type<Member>>,
        "hardware field assignment must not throw");
    offset -= Traits::width;
    result.*Member = Traits::unpack(extract<Traits::width>(value, offset));
  }
};

// The signal owns all value/known/Z bits. Copying a signal never invokes
// user-defined payload construction or assignment.
template <class T> class wire final {
public:
  using value_type = T;
  static constexpr unsigned width = hardware_traits<T>::width;
  using packed_type = FourState<width>;

  static_assert(
      std::is_same_v<
          decltype(hardware_traits<T>::pack(std::declval<const T &>())),
          Bits<width>> &&
          std::is_same_v<decltype(hardware_traits<T>::unpack(
                             std::declval<const Bits<width> &>())),
                         T>,
      "hardware_traits must map the payload to its declared finite bits");
  static_assert(noexcept(hardware_traits<T>::pack(std::declval<const T &>())) &&
                    noexcept(hardware_traits<T>::unpack(
                        std::declval<const Bits<width> &>())),
                "hardware_traits pack and unpack must not throw");

  wire() noexcept = default;
  wire(const wire &) noexcept = default;
  wire(wire &&) noexcept = default;
  wire &operator=(const wire &) noexcept = default;
  wire &operator=(wire &&) noexcept = default;

  static wire unknown() noexcept { return wire{}; }
  static wire known(const T &value) noexcept {
    return fromPacked(packed_type::known(hardware_traits<T>::pack(value)));
  }
  static wire fromPacked(packed_type value) noexcept {
    return wire(std::move(value));
  }

  const packed_type &packed() const noexcept { return packed_; }
  template <unsigned N>
  void assignSlice(unsigned low, const FourState<N> &value) noexcept {
    packed_.assignSlice(low, value);
  }
  bool isFullyKnown() const noexcept { return packed_.isFullyKnown(); }

  // This is the binary payload view and deliberately discards the X/Z masks.
  // Check isFullyKnown() before using it as a known hardware value.
  T value() const noexcept {
    return hardware_traits<T>::unpack(packed_.value());
  }

private:
  explicit wire(packed_type value) noexcept : packed_(std::move(value)) {}

  packed_type packed_ = packed_type::unknown();
};

// Dense value/known/Z lanes. Hot-path access borrows one scalar wire and
// never copies an owner or allocates a per-element control block. packed() is
// an explicit boundary conversion, rather than the representation of a table.
template <class T, std::size_t... Extents>
class wire<table<T, Extents...>> final {
public:
  using value_type = table<T, Extents...>;
  using element_type = typename value_type::element_type;
  static constexpr std::size_t size = value_type::size;
  static constexpr unsigned width = hardware_traits<value_type>::width;
  using packed_type = FourState<width>;
  static wire unknown() noexcept { return {}; }
  static wire known(const value_type &value) noexcept {
    wire result;
    for (std::size_t i = 0; i < size; ++i)
      result.element(i) = wire<element_type>::known(value[i]);
    return result;
  }
  static wire fromPacked(const packed_type &value) noexcept {
    wire result;
    constexpr unsigned elementWidth = hardware_traits<element_type>::width;
    for (std::size_t i = 0; i < size; ++i)
      result.element(i) = wire<element_type>::fromPacked(extract<elementWidth>(
          value, static_cast<unsigned>((size - 1 - i) * elementWidth)));
    return result;
  }
  wire<element_type> &element(std::size_t ordinal) noexcept {
    return elements_[ordinal];
  }
  const wire<element_type> &element(std::size_t ordinal) const noexcept {
    return elements_[ordinal];
  }
  wire<element_type> *data() noexcept { return elements_.data(); }
  const wire<element_type> *data() const noexcept { return elements_.data(); }
  bool isFullyKnown() const noexcept {
    for (const auto &element : elements_)
      if (!element.isFullyKnown())
        return false;
    return true;
  }
  value_type value() const noexcept {
    value_type result;
    for (std::size_t i = 0; i < size; ++i)
      result[i] = element(i).value();
    return result;
  }
  packed_type packed() const noexcept {
    Bits<width> values{}, known{}, z{};
    constexpr unsigned elementWidth = hardware_traits<element_type>::width;
    for (std::size_t i = 0; i < size; ++i) {
      const auto &lane = element(i).packed();
      auto offset = static_cast<unsigned>((size - 1 - i) * elementWidth);
      values = values | shl(zext<width>(lane.value()), offset);
      known = known | shl(zext<width>(lane.knownMask()), offset);
      z = z | shl(zext<width>(lane.zMask()), offset);
    }
    return packed_type::fromMasks(values, known, z);
  }

private:
  std::array<wire<element_type>, size> elements_{};
};

} // namespace gfsim
