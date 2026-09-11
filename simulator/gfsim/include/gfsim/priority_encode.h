#ifndef GFSIM_PRIORITY_ENCODE_H
#define GFSIM_PRIORITY_ENCODE_H

#include "gfsim/bits.h"
#include "gfsim/primitive_widths.h"
#include "gfsim/queue_blocks.h"

namespace gfsim {

template <unsigned Width> struct PriorityEncodeResult {
  UInt<PriorityIndexWidth<Width>> index{};
  UInt<1> valid{};

  friend constexpr bool operator==(const PriorityEncodeResult &,
                                   const PriorityEncodeResult &) = default;
};

template <unsigned Width>
constexpr PriorityEncodeResult<Width> priorityEncode(UInt<Width> input,
                                                     bool orderLow = true) {
  PriorityEncodeResult<Width> result;
  constexpr std::uint64_t widthMask = [] {
    if constexpr (Width == 64)
      return ~std::uint64_t{0};
    return (std::uint64_t{1} << Width) - 1;
  }();
  const std::uint64_t value = input.value() & widthMask;
  if (value == 0)
    return result;
  const unsigned bit =
      orderLow ? static_cast<unsigned>(std::countr_zero(value))
               : 63u - static_cast<unsigned>(std::countl_zero(value));
  result.index = bit;
  result.valid = 1;
  return result;
}

template <unsigned Width, bool OrderLow> struct PriorityEncodePolicy {
  constexpr PriorityEncodeResult<Width> operator()(UInt<Width> input) const {
    return priorityEncode(input, OrderLow);
  }
};

template <unsigned Width, bool OrderLow>
class PriorityEncode final
    : public QueueTransform<UInt<Width>, PriorityEncodeResult<Width>,
                            PriorityEncodePolicy<Width, OrderLow>> {
  using Base = QueueTransform<UInt<Width>, PriorityEncodeResult<Width>,
                              PriorityEncodePolicy<Width, OrderLow>>;

public:
  PriorityEncode(std::string name, ObjectId id, SimObject *parent,
                 SimQueue<UInt<Width>> &input,
                 SimQueue<PriorityEncodeResult<Width>> &output,
                 ObservationSink *observations = nullptr)
      : Base(std::move(name), id, parent, input, output, {}, observations) {}
};

} // namespace gfsim

#endif // GFSIM_PRIORITY_ENCODE_H
