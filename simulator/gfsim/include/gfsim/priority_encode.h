#ifndef GFSIM_PRIORITY_ENCODE_H
#define GFSIM_PRIORITY_ENCODE_H

#include "gfsim/bits.h"
#include "gfsim/queue_blocks.h"

namespace gfsim {

template <unsigned Width>
inline constexpr unsigned PriorityIndexWidth =
    Width <= 1 ? 1 : static_cast<unsigned>(std::bit_width(Width - 1));

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
  if (orderLow) {
    for (unsigned bit = 0; bit < Width; ++bit)
      if ((input.value() & (std::uint64_t{1} << bit)) != 0) {
        result.index = bit;
        result.valid = 1;
        return result;
      }
  } else {
    for (unsigned offset = 0; offset < Width; ++offset) {
      unsigned bit = Width - 1 - offset;
      if ((input.value() & (std::uint64_t{1} << bit)) != 0) {
        result.index = bit;
        result.valid = 1;
        return result;
      }
    }
  }
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
