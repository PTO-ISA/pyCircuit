#ifndef GFSIM_POPCOUNT_H
#define GFSIM_POPCOUNT_H

#include "gfsim/bits.h"
#include "gfsim/queue_blocks.h"

#include <bit>

namespace gfsim {

template <unsigned Width>
inline constexpr unsigned PopcountWidth =
    Width <= 1 ? 1 : static_cast<unsigned>(std::bit_width(Width));

template <unsigned Width>
constexpr UInt<PopcountWidth<Width>> populationCount(UInt<Width> input) {
  return UInt<PopcountWidth<Width>>{std::popcount(input.value())};
}

template <unsigned Width> struct PopcountPolicy {
  constexpr UInt<PopcountWidth<Width>> operator()(UInt<Width> input) const {
    return populationCount(input);
  }
};

template <unsigned Width>
class Popcount final
    : public QueueTransform<UInt<Width>, UInt<PopcountWidth<Width>>,
                            PopcountPolicy<Width>> {
  using Base = QueueTransform<UInt<Width>, UInt<PopcountWidth<Width>>,
                              PopcountPolicy<Width>>;

public:
  Popcount(std::string name, ObjectId id, SimObject *parent,
           SimQueue<UInt<Width>> &input,
           SimQueue<UInt<PopcountWidth<Width>>> &output,
           ObservationSink *observations = nullptr)
      : Base(std::move(name), id, parent, input, output, {}, observations) {}
};

} // namespace gfsim

#endif // GFSIM_POPCOUNT_H
