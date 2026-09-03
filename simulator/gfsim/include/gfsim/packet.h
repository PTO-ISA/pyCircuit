#ifndef GFSIM_PACKET_H
#define GFSIM_PACKET_H

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace gfsim {

enum class PacketEndianness : uint8_t { Little, Big };

struct PacketField {
  std::string_view name;
  size_t offset = 0;
  size_t size = 0;

  auto operator<=>(const PacketField &) const = default;
};

/// Static layout information for runtime containers. Public packets specialize
/// this template and set isPacket=true only after providing the complete
/// serialization and reflection contract below.
template <typename T> struct PacketTraits {
  static constexpr bool isPacket = false;
  static constexpr std::string_view schema = {};
  static constexpr size_t serializedSize = sizeof(T);
  static constexpr size_t maximumSerializedSize = serializedSize;
  static constexpr size_t alignment = alignof(T);
  static constexpr PacketEndianness endianness = PacketEndianness::Little;
  static constexpr std::array<PacketField, 0> fields{};
  static constexpr std::optional<std::string_view> routingField = std::nullopt;
  static constexpr std::optional<std::string_view> correlationField =
      std::nullopt;
};

template <typename T> consteval bool hasValidPacketLayout() {
  if constexpr (!requires {
                  PacketTraits<T>::isPacket;
                  PacketTraits<T>::schema;
                  PacketTraits<T>::serializedSize;
                  PacketTraits<T>::maximumSerializedSize;
                  PacketTraits<T>::alignment;
                  PacketTraits<T>::endianness;
                  PacketTraits<T>::fields;
                  PacketTraits<T>::routingField;
                  PacketTraits<T>::correlationField;
                }) {
    return false;
  } else {
    if (!PacketTraits<T>::isPacket ||
        std::string_view(PacketTraits<T>::schema).empty() ||
        PacketTraits<T>::serializedSize == 0 ||
        PacketTraits<T>::maximumSerializedSize <
            PacketTraits<T>::serializedSize ||
        PacketTraits<T>::alignment == 0)
      return false;

    size_t previousEnd = 0;
    for (const PacketField &field : PacketTraits<T>::fields) {
      if (field.name.empty() || field.size == 0 || field.offset < previousEnd ||
          field.offset > PacketTraits<T>::serializedSize ||
          field.size > PacketTraits<T>::serializedSize - field.offset)
        return false;
      previousEnd = field.offset + field.size;
    }
    auto hasField = [](std::string_view name) {
      for (const PacketField &field : PacketTraits<T>::fields)
        if (field.name == name)
          return true;
      return false;
    };
    if ((PacketTraits<T>::routingField &&
         !hasField(*PacketTraits<T>::routingField)) ||
        (PacketTraits<T>::correlationField &&
         !hasField(*PacketTraits<T>::correlationField)))
      return false;
    return true;
  }
}

template <typename T>
concept Packet = hasValidPacketLayout<T>() && requires(
                                                  const T &packet,
                                                  std::span<const std::byte>
                                                      bytes) {
  typename PacketTraits<T>::Serialized;
  requires std::same_as<typename PacketTraits<T>::Serialized,
                        std::array<std::byte, PacketTraits<T>::serializedSize>>;
  { PacketTraits<T>::schema } -> std::convertible_to<std::string_view>;
  { PacketTraits<T>::maximumSerializedSize } -> std::convertible_to<size_t>;
  { PacketTraits<T>::alignment } -> std::convertible_to<size_t>;
  { PacketTraits<T>::endianness } -> std::same_as<const PacketEndianness &>;
  { PacketTraits<T>::fields };
  {
    PacketTraits<T>::routingField
  } -> std::same_as<const std::optional<std::string_view> &>;
  {
    PacketTraits<T>::correlationField
  } -> std::same_as<const std::optional<std::string_view> &>;
  {
    PacketTraits<T>::serialize(packet)
  } -> std::same_as<typename PacketTraits<T>::Serialized>;
  { PacketTraits<T>::deserialize(bytes) } -> std::same_as<std::optional<T>>;
};

template <Packet T>
typename PacketTraits<T>::Serialized serializePacket(const T &packet) {
  return PacketTraits<T>::serialize(packet);
}

template <Packet T>
std::optional<T> deserializePacket(std::span<const std::byte> bytes) {
  if (bytes.size() != PacketTraits<T>::serializedSize)
    return std::nullopt;
  return PacketTraits<T>::deserialize(bytes);
}

} // namespace gfsim

#endif // GFSIM_PACKET_H
