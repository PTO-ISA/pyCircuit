#include <cstdlib>
#include <fstream>
#include <gfsim/byte_mem.h>
#include <gfsim/sync_mem.h>
#include <iostream>
#include <string>

constexpr unsigned Width = MEM_WIDTH, Address = MEM_ADDRESS;
#if MEM_KIND == 0
using Kernel = gfsim::byte_mem_kernel<gfsim::Bits<Width>, Address, MEM_DEPTH>;
constexpr unsigned Strobes = Width / 8;
#else
constexpr unsigned Ports = MEM_KIND;
using Kernel =
    gfsim::sync_mem_kernel<gfsim::Bits<Width>, Address, MEM_DEPTH, Ports>;
constexpr unsigned Strobes = (Width + 7) / 8;
#endif
void require(bool value) {
  if (!value)
    std::abort();
}
template <unsigned W> auto wire(const std::string &word) {
  require(word.size() == W);
  gfsim::Bits<W> value{}, known{}, z{};
  for (unsigned index = 0; index < W; ++index) {
    auto bit = W - index - 1;
    if (word[index] == '1')
      value = gfsim::detail::setBit(value, bit);
    if (word[index] == '0' || word[index] == '1')
      known = gfsim::detail::setBit(known, bit);
    if (word[index] == 'z')
      z = gfsim::detail::setBit(z, bit);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(
      gfsim::FourState<W>::fromMasks(value, known, z));
}
template <class T> std::string text(const gfsim::wire<T> &value) {
  std::string result;
  for (unsigned bit = gfsim::wire<T>::width; bit-- > 0;)
    result += value.packed().zMask().bit(bit)        ? 'z'
              : !value.packed().knownMask().bit(bit) ? 'x'
              : value.packed().value().bit(bit)      ? '1'
                                                     : '0';
  return result;
}
std::string trace(const std::string &gold, const std::string &actual) {
  require(gold == actual);
  return gold.find_first_of("xz") == std::string::npos ? actual : "unknown";
}
int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream frames(argv[1]);
  require(frames.good());
  Kernel::Current current;
  Kernel::Pending pending;
  auto clk = wire<1>("0"), rst = clk, write = clk, ren0 = clk, ren1 = clk;
  auto read0 = wire<Address>(std::string(Address, '0')), read1 = read0,
       address = read0;
  auto data = wire<Width>(std::string(Width, '0'));
  auto strobes = wire<Strobes>(std::string(Strobes, '0'));
  gfsim::wire<gfsim::Bits<Width>> q0;
  auto q1 = wire<Width>(std::string(Width, '0'));
#if MEM_KIND == 0
  Kernel::Inputs inputs{clk, rst, read0, write, address, data, strobes};
  Kernel::Outputs outputs{q0};
#elif MEM_KIND == 1
  Kernel::Inputs inputs{clk,   rst,     {&ren0}, {&read0},
                        write, address, data,    strobes};
  Kernel::Outputs outputs{{&q0}};
#else
  q1 = gfsim::wire<gfsim::Bits<Width>>::unknown();
  Kernel::Inputs inputs{clk,   rst,     {&ren0, &ren1}, {&read0, &read1},
                        write, address, data,           strobes};
  Kernel::Outputs outputs{{&q0, &q1}};
#endif
  unsigned command, index = 0, known0, known1;
  std::string c, r, w, en0, en1, a0, a1, wa, d, strobe, permit, gold0, gold1,
      error;
  while (frames >> command >> c >> r >> w >> en0 >> en1 >> a0 >> a1 >> wa >>
         d >> strobe >> permit >> gold0 >> gold1 >> known0 >> known1 >> error) {
    require(index < 10000);
    clk = wire<1>(c);
    rst = wire<1>(r);
    write = wire<1>(w);
    ren0 = wire<1>(en0);
    ren1 = wire<1>(en1);
    read0 = wire<Address>(a0);
    read1 = wire<Address>(a1);
    address = wire<Address>(wa);
    data = wire<Width>(d);
    strobes = wire<Strobes>(strobe);
    if (command == 1) {
      try {
        Kernel::work(current, inputs, pending, outputs);
      } catch (const gfsim::FourStateViolation &) {
        require(!pending.valid);
      }
    } else if (command == 2) {
      if (permit == "1" || permit == "z")
        Kernel::xfer(current, pending, outputs);
      else
        Kernel::discard(current, pending);
    } else if (command == 3)
      Kernel::discard(current, pending);
    else if (command == 4)
      Kernel::reset(current, inputs, pending);
    else
      require(command == 0 || command == 5);
#if MEM_KIND == 0
    Kernel::read(current, read0, outputs);
#endif
    require(error == (pending.valid ? "0" : "1"));
    require(known0 == (gold0.find_first_of("xz") == std::string::npos));
    require(known1 == (gold1.find_first_of("xz") == std::string::npos));
    std::cout << "ROW " << index << ' ' << trace(gold0, text(q0)) << ' '
              << trace(gold1, text(q1)) << ' ' << error << '\n';
    ++index;
  }
  require(frames.eof() && index >= 32);
}
