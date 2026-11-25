#include "pycircuit_system.hpp"
#include "fifo_ports.hpp"
#include <cstdlib>
#include <fstream>
#include <string>

void require(bool condition) {
  if (!condition)
    std::abort();
}
auto level(const std::string &text) {
  using B = gfsim::Bits<1>;
  using F = gfsim::FourState<1>;
  return gfsim::wire<B>::fromPacked(
      text == "x" ? F::unknown()
      : text == "z" ? F::highImpedance()
                    : F::known(B{std::stoul(text, nullptr, 16)}));
}
template <class T> void expect(const T &value, unsigned expected) {
  require(value.isFullyKnown() && value.value().value() == expected);
}
void same(const gfsim::wire<gfsim::Bits<1>> &value, const std::string &text) {
  const auto &actual = value.packed();
  const auto expected = level(text).packed();
  require(actual.knownMask() == expected.knownMask());
  require(actual.zMask() == expected.zMask());
  require((actual.value() & expected.knownMask()) ==
          (expected.value() & expected.knownMask()));
}
int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream rows(argv[1]);
  require(rows.good());
  pyc_root root("fifo_availability");
  root.pyc_7079635f636c6b = level("0");
  root.pyc_7079635f727374 = level("0");
  root.Reset();
  root.Xfer();
  std::string clock, reset, offer, data, early, late, take, consume;
  std::string viewed, transferred, derived, head, output;
  unsigned viewValid, moveValid, derivedValid, ready, present, outputValid,
      moves, seen, accepted, samples = 0;
  while (rows >> clock >> reset >> offer >> data >> early >> late >> take >>
         consume >> std::hex >> viewValid >> viewed >> moveValid >> transferred >>
         derivedValid >> derived >> ready >> present >> head >> outputValid >>
         output >> moves >> seen >> accepted) {
    root.pyc_7079635f636c6b = level(clock);
    root.pyc_7079635f727374 = level(reset);
    root.offer = level(offer);
    root.data = level(data);
    root.early = level(early);
    root.late = level(late);
    root.take = level(take);
    root.consume = level(consume);
    root.Work();
    expect(root.PYC_VIEW_VALID, viewValid);
    expect(root.PYC_MOVE_VALID, moveValid);
    expect(root.PYC_DERIVED_VALID, derivedValid);
    require(derivedValid == moveValid);
    if (viewValid) same(root.viewed, viewed);
    if (moveValid) same(root.transferred, transferred);
    if (derivedValid) same(root.derived, derived);
    expect(root.in_ready, ready);
    expect(root.in_valid, present);
    same(root.head, head);
    expect(root.out_valid, outputValid);
    same(root.out_data, output);
    expect(root.moves, moves);
    expect(root.seen, seen);
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure) == bool(accepted));
    if (!accepted) {
      require(failure.phase == gfsim::SimFailurePhase::Check);
      root.DiscardNext();
    }
    root.Xfer();
    ++samples;
  }
  require(rows.eof() && samples == 34);
}
