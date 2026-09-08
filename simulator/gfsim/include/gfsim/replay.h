#ifndef GFSIM_REPLAY_H
#define GFSIM_REPLAY_H

#include "gfsim/core.h"

#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace gfsim {

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
  else if constexpr (requires { value.replayValue(); })
    return value.replayValue();
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
  if constexpr (requires { T::replayFlat; })
    return T::replayFlat;
  return true; // The viewer also validates logical field shapes.
}

template <typename T> ReplayValue::Array replayFields() {
  if constexpr (requires { T::replayFields(); })
    return T::replayFields();
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

// Header-only so manually driven generated models use the same recorder as
// SimSystem without adding a new linker requirement to existing testbenches.
class ReplayRecorder {
public:
  using Object = ReplayValue::Object;
  struct Source {
    Object descriptor;
    std::function<ReplayValue()> snapshot;
  };

  explicit ReplayRecorder(const std::string &path) {
    out_.open(path, std::ios::binary | std::ios::trunc);
    if (!out_)
      throw std::runtime_error("replay: cannot open output");
    out_.write("PYC6TRC3", 8);
    write32(3);
    write32(1);
  }
  ReplayRecorder(const ReplayRecorder &) = delete;
  ReplayRecorder &operator=(const ReplayRecorder &) = delete;

  void add(ObjectId id, Source source) {
    if (started_ || !sources_.emplace(id, std::move(source)).second)
      throw std::runtime_error("replay: duplicate or late object registration");
  }
  void start(Object metadata = {}) {
    if (started_)
      throw std::runtime_error("replay: already started");
    // Validate all serializers before claiming a complete initial snapshot.
    auto initial = snapshots();
    metadata["format"] = "agentic-circuit-replay";
    metadata["version"] = replayValue(uint32_t{1});
    ReplayValue::Array objects;
    for (const auto &[id, source] : sources_) {
      auto descriptor = source.descriptor;
      descriptor["id"] = replayValue(id);
      objects.emplace_back(std::move(descriptor));
    }
    metadata["objects"] = std::move(objects);
    record("manifest", std::move(metadata));
    state_ = std::move(initial);
    record("initial", {{"state", state_}});
    started_ = true;
  }
  void begin(Epoch epoch) {
    if (!started_ || active_ || finished_)
      throw std::runtime_error("replay: invalid barrier begin");
    epoch_ = epoch;
    active_ = true;
    record("barrier_begin");
  }
  void context(ObjectId owner, std::string_view phase) {
    owner_ = owner;
    phase_ = phase;
  }
  void clearContext() {
    owner_ = kInvalidObjectId;
    phase_ = "external";
  }
  void event(ObjectId resource, std::string_view action, Object detail = {}) {
    if (!started_ || finished_)
      return;
    // Keep only successful data operations. Reads are associated at end with
    // the operation that actually committed, never with failed candidates.
    if (action != "state_read" && action != "state_write" &&
        action != "enqueue" && action != "dequeue" && action != "ready")
      return;
    if (action == "state_read" && phase_ != "Work")
      return;
    detail["object"] = replayValue(resource);
    if (!detail.contains("owner"))
      detail["owner"] = replayValue(owner_);
    detail["action"] = std::string(action);
    pendingEvents_.push_back(std::move(detail));
  }
  ObjectId owner() const { return owner_; }
  uint64_t nextToken() { return nextToken_++; }

  void end() {
    if (!active_)
      throw std::runtime_error("replay: no active barrier");
    auto current = snapshots();
    Object changes;
    for (const auto &[id, value] : current)
      if (state_.at(id) != value)
        changes[id] = Object{{"before", state_.at(id)}, {"after", value}};
    std::map<ObjectId, uint64_t> operations;
    auto ownerOf = [](const Object &event) {
      return static_cast<ObjectId>(
          std::get<ReplayValue::Integer>(event.at("owner").data).bits);
    };
    for (const auto &event : pendingEvents_)
      if (std::get<std::string>(event.at("action").data) != "state_read")
        operations.try_emplace(ownerOf(event), operations.size());
    for (auto &event : pendingEvents_) {
      const auto owner = ownerOf(event);
      if (!operations.contains(owner))
        continue;
      event["operation"] =
          std::to_string(batch_) + ":" + std::to_string(operations.at(owner));
      record("event", std::move(event));
    }
    pendingEvents_.clear();
    record("commit", {{"changes", std::move(changes)}});
    state_ = std::move(current);
    active_ = false;
    ++batch_;
    clearContext();
    out_.flush();
    check();
  }
  void finish(std::string status) {
    if (!started_ || finished_ || active_)
      throw std::runtime_error("replay: cannot finish inside a barrier");
    const auto current = snapshots();
    if (current != state_)
      throw std::runtime_error(
          "replay: unrecorded state mutation after barrier");
    record("end", {{"status", std::move(status)}, {"state", current}});
    out_.flush();
    check();
    finished_ = true;
  }
  bool active() const { return active_; }
  bool recording() const { return started_ && !finished_; }
  void attachment(std::string name, std::string text) {
    record("source", {{"name", std::move(name)}, {"text", std::move(text)}});
  }

private:
  Object snapshots() const {
    Object result;
    for (const auto &[id, source] : sources_)
      result[std::to_string(id)] = source.snapshot();
    return result;
  }
  static void append(std::vector<uint8_t> &bytes, uint64_t value,
                     size_t width) {
    for (size_t i = 0; i < width; ++i)
      bytes.push_back(static_cast<uint8_t>(value >> (8 * i)));
  }
  static void string(std::vector<uint8_t> &bytes, const std::string &value) {
    if (value.size() > UINT32_MAX)
      throw std::runtime_error("replay: string exceeds chunk framing");
    append(bytes, value.size(), 4);
    bytes.insert(bytes.end(), value.begin(), value.end());
  }
  static void encode(std::vector<uint8_t> &bytes, const ReplayValue &value) {
    bytes.push_back(static_cast<uint8_t>(value.data.index()));
    std::visit(
        [&](const auto &item) {
          using T = std::decay_t<decltype(item)>;
          if constexpr (std::same_as<T, bool>)
            bytes.push_back(item);
          else if constexpr (std::same_as<T, ReplayValue::Integer>) {
            append(bytes, item.width, 4);
            bytes.push_back(item.isSigned);
            append(bytes, item.bits, 8);
          } else if constexpr (std::same_as<T, ReplayValue::FloatBits>) {
            append(bytes, item.bits, 8);
          } else if constexpr (std::same_as<T, std::string>)
            string(bytes, item);
          else if constexpr (std::same_as<T, ReplayValue::Array>) {
            append(bytes, item.size(), 4);
            for (const auto &element : item)
              encode(bytes, element);
          } else if constexpr (std::same_as<T, Object>) {
            append(bytes, item.size(), 4);
            for (const auto &[key, element] : item) {
              string(bytes, key);
              encode(bytes, element);
            }
          }
        },
        value.data);
  }
  void record(std::string kind, Object detail = {}) {
    detail["kind"] = std::move(kind);
    detail["sequence"] = replayValue(sequence_++);
    detail["batch"] = replayValue(batch_);
    detail["time"] = replayValue(epoch_.time);
    detail["delta"] = replayValue(epoch_.delta);
    std::vector<uint8_t> bytes;
    encode(bytes, detail);
    if (bytes.size() > UINT32_MAX)
      throw std::runtime_error("replay: record exceeds chunk framing");
    write32(static_cast<uint32_t>(bytes.size()));
    write32(0xAC01);
    out_.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    check();
  }
  void write32(uint32_t value) {
    char bytes[4];
    for (unsigned i = 0; i < 4; ++i)
      bytes[i] = static_cast<char>(value >> (8 * i));
    out_.write(bytes, 4);
  }
  void check() {
    if (!out_)
      throw std::runtime_error("replay: output write failed");
  }
  std::ofstream out_;
  std::map<ObjectId, Source> sources_;
  Object state_;
  std::vector<Object> pendingEvents_;
  uint64_t nextToken_ = 0;
  Epoch epoch_{};
  uint64_t sequence_ = 0;
  uint64_t batch_ = 0;
  ObjectId owner_ = kInvalidObjectId;
  std::string phase_ = "external";
  bool started_ = false;
  bool active_ = false;
  bool finished_ = false;
};

} // namespace gfsim
#endif
