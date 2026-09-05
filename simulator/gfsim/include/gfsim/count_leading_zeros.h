#ifndef GFSIM_COUNT_LEADING_ZEROS_H
#define GFSIM_COUNT_LEADING_ZEROS_H

#include "gfsim/bits.h"
#include "gfsim/queue_blocks.h"

#include <bit>

namespace gfsim {

template <unsigned Width>
inline constexpr unsigned CountLeadingZerosWidth =
    Width <= 1 ? 1 : static_cast<unsigned>(std::bit_width(Width));

template <unsigned Width>
constexpr UInt<CountLeadingZerosWidth<Width>>
countLeadingZeros(UInt<Width> input) {
  unsigned count = 0;
  for (unsigned offset = 0; offset < Width; ++offset) {
    const unsigned bit = Width - 1u - offset;
    if (((input.value() >> bit) & 1u) != 0)
      break;
    ++count;
  }
  return UInt<CountLeadingZerosWidth<Width>>{count};
}

template <unsigned Width> struct CountLeadingZerosPolicy {
  constexpr UInt<CountLeadingZerosWidth<Width>>
  operator()(UInt<Width> input) const {
    return countLeadingZeros(input);
  }
};

template <unsigned Width>
class CountLeadingZeros final
    : public QueueTransform<UInt<Width>, UInt<CountLeadingZerosWidth<Width>>,
                            CountLeadingZerosPolicy<Width>> {
  using Base =
      QueueTransform<UInt<Width>, UInt<CountLeadingZerosWidth<Width>>,
                     CountLeadingZerosPolicy<Width>>;

public:
  CountLeadingZeros(std::string name, ObjectId id, SimObject *parent,
                    SimQueue<UInt<Width>> &input,
                    SimQueue<UInt<CountLeadingZerosWidth<Width>>> &output,
                    ObservationSink *observations = nullptr)
      : Base(std::move(name), id, parent, input, output, {}, observations) {}
};

} // namespace gfsim

#endif // GFSIM_COUNT_LEADING_ZEROS_H
