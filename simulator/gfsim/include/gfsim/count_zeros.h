#ifndef GFSIM_COUNT_ZEROS_H
#define GFSIM_COUNT_ZEROS_H

#include "gfsim/bits.h"
#include "gfsim/queue_blocks.h"

#include <bit>

namespace gfsim {

enum class ZeroCountDirection { Leading, Trailing };

template <unsigned Width>
inline constexpr unsigned CountZerosWidth =
    Width <= 1 ? 1 : static_cast<unsigned>(std::bit_width(Width));

template <unsigned Width, ZeroCountDirection Direction>
constexpr UInt<CountZerosWidth<Width>> countZeros(UInt<Width> input) {
  unsigned count = 0;
  for (unsigned offset = 0; offset < Width; ++offset) {
    const unsigned bit = Direction == ZeroCountDirection::Trailing
                             ? offset
                             : Width - 1u - offset;
    if (((input.value() >> bit) & 1u) != 0)
      break;
    ++count;
  }
  return UInt<CountZerosWidth<Width>>{count};
}

template <unsigned Width>
constexpr UInt<CountZerosWidth<Width>> countLeadingZeros(UInt<Width> input) {
  return countZeros<Width, ZeroCountDirection::Leading>(input);
}

template <unsigned Width>
constexpr UInt<CountZerosWidth<Width>> countTrailingZeros(UInt<Width> input) {
  return countZeros<Width, ZeroCountDirection::Trailing>(input);
}

template <unsigned Width, ZeroCountDirection Direction> struct CountZerosPolicy {
  constexpr UInt<CountZerosWidth<Width>> operator()(UInt<Width> input) const {
    return countZeros<Width, Direction>(input);
  }
};

template <unsigned Width, ZeroCountDirection Direction>
class CountZeros final
    : public QueueTransform<UInt<Width>, UInt<CountZerosWidth<Width>>,
                            CountZerosPolicy<Width, Direction>> {
  using Base = QueueTransform<UInt<Width>, UInt<CountZerosWidth<Width>>,
                              CountZerosPolicy<Width, Direction>>;

public:
  CountZeros(std::string name, ObjectId id, SimObject *parent,
             SimQueue<UInt<Width>> &input,
             SimQueue<UInt<CountZerosWidth<Width>>> &output,
             ObservationSink *observations = nullptr)
      : Base(std::move(name), id, parent, input, output, {}, observations) {}
};

template <unsigned Width>
using CountLeadingZeros = CountZeros<Width, ZeroCountDirection::Leading>;

template <unsigned Width>
using CountTrailingZeros = CountZeros<Width, ZeroCountDirection::Trailing>;

} // namespace gfsim

#endif // GFSIM_COUNT_ZEROS_H
