#ifndef GFSIM_REPLAY_VALUE_H
#define GFSIM_REPLAY_VALUE_H
#include "gfsim/core.h"
#include <array>
#include <bit>
#include <concepts>
#include <map>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <tuple>
#include <variant>
#include <vector>
namespace gfsim {
template <typename T> struct ValueCodec {};
// Replay values are serialized by logical type, never by C++ object layout.
struct ReplayValue {
  struct Integer {
    uint64_t bits;
    uint32_t width;
    bool isSigned = false;
    bool operator==(const Integer &) const = default;
  };
  using Array = std::vector<ReplayValue>;
  using Object = std::map<std::string, ReplayValue>;
  struct FloatBits {
    uint64_t bits;
    bool operator==(const FloatBits &) const = default;
  };
  std::variant<std::monostate, bool, Integer, std::string, Array, Object,
               FloatBits>
      data;
  ReplayValue() = default;
  ReplayValue(bool value) : data(value) {}
  ReplayValue(Integer value) : data(value) {}
  ReplayValue(std::string value) : data(std::move(value)) {}
  ReplayValue(const char *value) : data(std::string(value)) {}
  ReplayValue(Array value) : data(std::move(value)) {}
  ReplayValue(Object value) : data(std::move(value)) {}
  ReplayValue(double value) : data(FloatBits{std::bit_cast<uint64_t>(value)}) {}
  bool operator==(const ReplayValue &) const = default;
};

template <typename T> ReplayValue replayValue(const T &value) {
  if constexpr (std::same_as<T, ReplayValue>)
    return value;
  else if constexpr (std::same_as<T, std::monostate>)
    return {};
  else if constexpr (std::same_as<T, bool>)
    return value;
  else if constexpr (std::integral<T>)
    return ReplayValue::Integer{static_cast<uint64_t>(value), sizeof(T) * 8,
                                std::signed_integral<T>};
  else if constexpr (std::floating_point<T>)
    return static_cast<double>(value);
  else if constexpr (requires {
                       T::width;
                       value.value();
                     })
    return ReplayValue::Integer{value.value(), T::width};
  else if constexpr (std::is_enum_v<T>)
    return replayValue(static_cast<std::underlying_type_t<T>>(value));
  else if constexpr (std::convertible_to<T, std::string_view>)
    return std::string(std::string_view(value));
  else if constexpr (requires { ValueCodec<T>::encode(value); })
    return ValueCodec<T>::encode(value);
  else if constexpr (std::same_as<T, Epoch>)
    return ReplayValue::Object{{"time", replayValue(value.time)},
                               {"delta", replayValue(value.delta)}};
  else if constexpr (requires { value.id(); })
    return ReplayValue::Object{{"ref", replayValue(value.id())}};
  else if constexpr (requires { value->id(); })
    return value ? replayValue(*value) : ReplayValue{};
  else if constexpr (requires {
                       value.has_value();
                       *value;
                     })
    return value ? replayValue(*value) : ReplayValue{};
  else if constexpr (requires {
                       typename T::key_type;
                       requires std::same_as<typename T::key_type, std::string>;
                     }) {
    ReplayValue::Object result;
    for (const auto &[key, item] : value)
      result[key] = replayValue(item);
    return result;
  } else if constexpr (std::ranges::input_range<T>) {
    ReplayValue::Array result;
    for (const auto &item : value)
      result.push_back(replayValue(item));
    return result;
  } else if constexpr (requires { std::tuple_size<T>::value; }) {
    ReplayValue::Array result;
    std::apply(
        [&](const auto &...items) {
          (result.push_back(replayValue(items)), ...);
        },
        value);
    return result;
  } else
    throw std::runtime_error("replay: missing logical value serializer");
}

template <typename T> bool replayFlat() {
  if constexpr (requires { ValueCodec<T>::flat; })
    return ValueCodec<T>::flat;
  return true; // The viewer also validates logical field shapes.
}

template <typename T> ReplayValue::Array replayFields() {
  if constexpr (requires { ValueCodec<T>::fields(); })
    return ValueCodec<T>::fields();
  else {
    const auto value = replayValue(T{});
    ReplayValue::Array fields;
    if (const auto *object = std::get_if<ReplayValue::Object>(&value.data))
      for (const auto &[name, unused] : *object)
        fields.emplace_back(name);
    else
      fields.emplace_back("value");
    return fields;
  }
}

// Built-in feedback payload is adapted here, not in the component definition.
template <typename T> struct FeedbackToken;
template <typename T> struct ValueCodec<FeedbackToken<T>> {
  static ReplayValue encode(const FeedbackToken<T> &value) {
    return ReplayValue::Object{{"value", replayValue(value.value)},
                               {"iteration", replayValue(value.iteration)}};
  }
  static ReplayValue::Array fields() { return {"value", "iteration"}; }
  static constexpr bool flat = false;
};
} // namespace gfsim
#endif
