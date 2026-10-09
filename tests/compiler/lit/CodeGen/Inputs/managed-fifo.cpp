// Actual native FIFO kernel on the same managed frames, in addition to the
// independent absolute-age ordered-token golden in managed-fifo.py.
#include <cstdlib>
#include <fstream>
#include <gfsim/fifo.h>
#include <iostream>
#include <string>

constexpr unsigned Width = FIFO_WIDTH;
using Payload = gfsim::Bits<Width>;
using Kernel = gfsim::fifo_kernel<Payload, FIFO_DEPTH,
                                  FIFO_POLICY == 0
                                      ? gfsim::QueueReadyPolicy::LocalOccupancy
                                      : gfsim::QueueReadyPolicy::DownstreamPop,
                                  FIFO_LATENCY>;
void require(bool value) {
  if (!value)
    std::abort();
}
template <unsigned W> auto wire(const std::string &text) {
  require(text.size() == W);
  gfsim::Bits<W> value{}, known{}, z{};
  for (unsigned index = 0; index < W; ++index) {
    unsigned position = W - index - 1;
    if (text[index] == '1')
      value = gfsim::detail::setBit(value, position);
    if (text[index] == '0' || text[index] == '1')
      known = gfsim::detail::setBit(known, position);
    if (text[index] == 'z')
      z = gfsim::detail::setBit(z, position);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(
      gfsim::FourState<W>::fromMasks(value, known, z));
}
template <class T> std::string text(const gfsim::wire<T> &value) {
  std::string result;
  for (unsigned index = gfsim::wire<T>::width; index-- > 0;)
    result += value.packed().zMask().bit(index)        ? 'z'
              : !value.packed().knownMask().bit(index) ? 'x'
              : value.packed().value().bit(index)      ? '1'
                                                       : '0';
  return result;
}
int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream frames(argv[1]);
  require(frames.good());
  Kernel::Current current;
  Kernel::Pending pending;
  auto clk = wire<1>("0"), rst = clk, valid = clk, take = clk;
  auto data = wire<Width>(std::string(Width, '0'));
  gfsim::wire<gfsim::Bits<1>> ready, available;
  gfsim::wire<Payload> head;
  Kernel::Inputs inputs{clk, rst, valid, data, take};
  Kernel::Outputs outputs{ready, available, head};
  unsigned command, initialized, index = 0;
  std::string clockWord, resetWord, validWord, takeWord, permission, payload,
      readyGold, validGold, dataGold, errorGold;
  while (frames >> command >> clockWord >> resetWord >> validWord >> takeWord >>
         permission >> payload >> initialized >> readyGold >> validGold >>
         dataGold >> errorGold) {
    require(index < 10000);
    clk = wire<1>(clockWord);
    rst = wire<1>(resetWord);
    valid = wire<1>(validWord);
    take = wire<1>(takeWord);
    data = wire<Width>(payload);
    if (command == 1) {
      try {
        Kernel::work(current, inputs, pending, outputs);
      } catch (const gfsim::FourStateViolation &) {
        require(!pending.valid);
      }
    } else if (command == 2) {
      if (permission == "1" || permission == "z")
        Kernel::xfer(current, pending, outputs);
      else
        Kernel::discard(current, pending);
    } else if (command == 3)
      Kernel::discard(current, pending);
    else if (command == 4)
      Kernel::reset(current, inputs, pending);
    else
      require(command == 0 || command == 5);
    ready = Kernel::readReady(current, take);
    available = Kernel::readValid(current);
    head = Kernel::readData(current);
    auto error = pending.valid ? "0" : "1";
    require(errorGold == error);
    if (initialized) {
      require(text(ready) == readyGold && text(available) == validGold &&
              text(head) == dataGold);
      std::cout << "ROW " << index << ' ' << text(ready) << ' '
                << text(available) << ' ' << text(head) << ' ' << error << '\n';
    } else
      std::cout << "ROW " << index << " cold " << error << '\n';
    ++index;
  }
  require(frames.eof() && index >= 32);
}
