#include "pycircuit_system.hpp"

#include <cstdlib>

template <unsigned Width> auto known(unsigned value) {
  using Bits = gfsim::Bits<Width>;
  return gfsim::wire<Bits>::known(Bits{value});
}

void require(bool ok) {
  if (!ok)
    std::abort();
}

void allocation(const pyc_root &root, unsigned index, unsigned count) {
  require(root.result.isFullyKnown());
  auto result = root.result.value();
  require(result.allocate_accepted.value() == 1);
  require(result.allocated_index.value() == index);
  require(result.count.value() == count);
}

// Build independent expected planes through the public Result layout, rather
// than depending on compiler-private state, native helpers or packed offsets.
void completionOrRetirement(const pyc_root &root, unsigned completed,
                            unsigned retired, unsigned tag, unsigned value,
                            unsigned count, unsigned valueKnown = 0xffff,
                            unsigned valueZ = 0) {
  using Result = decltype(root.result)::value_type;
  Result expected{}, masks{}, z{};
  expected.complete_accepted = gfsim::Bits<1>{completed};
  expected.retire_accepted = gfsim::Bits<1>{retired};
  expected.retired_tag = gfsim::Bits<8>{tag};
  expected.retired_value = gfsim::Bits<16>{value};
  expected.count = gfsim::Bits<3>{count};
  masks.allocate_accepted = gfsim::Bits<1>{1};
  masks.allocated_index = gfsim::Bits<2>{3};
  masks.complete_accepted = gfsim::Bits<1>{1};
  masks.retire_accepted = gfsim::Bits<1>{1};
  masks.retired_tag = gfsim::Bits<8>{255};
  masks.retired_value = gfsim::Bits<16>{valueKnown};
  masks.count = gfsim::Bits<3>{7};
  z.retired_value = gfsim::Bits<16>{valueZ};
  const auto actual = root.result.packed();
  const auto knownMask = gfsim::hardware_traits<Result>::pack(masks);
  require(actual.knownMask() == knownMask);
  require(actual.zMask() == gfsim::hardware_traits<Result>::pack(z));
  require((actual.value() & knownMask) ==
          (gfsim::hardware_traits<Result>::pack(expected) & knownMask));
}

int main() {
  gfsim::WorkExecutor executor{1};
  pyc_root root{"repeated-request", &executor};
  root.Build();
  root.Reset();
  root.Xfer();

  root.pyc_7079635f636c6b = known<1>(0);
  root.pyc_7079635f727374 = known<1>(0);
  root.allocate = known<1>(1);
  root.allocate_tag = known<8>(17);
  root.complete = known<1>(0);
  root.complete_index = known<2>(0);
  root.complete_tag = known<8>(0);
  root.complete_value = known<16>(0);
  root.retire = known<1>(0);
  root.flush = known<1>(0);
  root.Work();
  allocation(root, 0, 1);
  root.Xfer();

  // The same raw data bit now has an X mask. This edge must fail.
  using Bit = gfsim::Bits<1>;
  root.allocate = decltype(root.allocate)::fromPacked(gfsim::FourState<1>::unknown(Bit{1}));
  root.pyc_7079635f636c6b = known<1>(1);
  bool rejected = false;
  try {
    root.Work();
  } catch (const gfsim::FourStateViolation &) {
    rejected = true;
  }
  require(rejected);
  root.DiscardNext();
  root.Xfer();

  root.allocate = known<1>(1);
  root.Work();
  allocation(root, 0, 1);
  root.Xfer();

  // A successful transfer changes the committed state for the same request.
  root.pyc_7079635f636c6b = known<1>(0);
  root.Work();
  allocation(root, 1, 2);
  root.Xfer();

  // Only index 0/tag 17 was committed. The last low-clock allocation was a
  // snapshot, so completing that original entry leaves exactly one live row.
  auto cycle = [&](auto check) {
    for (unsigned clock = 0; clock < 2; ++clock) {
      root.pyc_7079635f636c6b = known<1>(clock);
      root.Work();
      check();
      root.Xfer();
    }
  };
  root.allocate = known<1>(0);
  root.complete = known<1>(1);
  root.complete_tag = known<8>(17);
  using Payload = gfsim::Bits<16>;
  root.complete_value = decltype(root.complete_value)::fromPacked(
      gfsim::FourState<16>::fromMasks(Payload{0xa05a}, Payload{0xf0ff},
                                    Payload{0x0500}));
  cycle([&] { completionOrRetirement(root, 1, 0, 0, 0, 1); });

  // All request inputs are now known. Retirement must still preserve the X/Z
  // payload read from committed state: 1010xzxz01011010, not a binary value.
  root.complete = known<1>(0);
  root.complete_value = known<16>(0);
  root.retire = known<1>(1);
  cycle([&] {
    completionOrRetirement(root, 0, 1, 17, 0xa05a, 0, 0xf0ff, 0x0500);
  });

  root.retire = known<1>(0);
  root.allocate = known<1>(1);
  root.allocate_tag = known<8>(34);
  cycle([&] { allocation(root, 1, 1); });

  // Known completion and retirement share one transaction. Retire observes
  // the completed candidate, including its new payload, before the commit.
  root.allocate = known<1>(0);
  root.complete = known<1>(1);
  root.complete_index = known<2>(1);
  root.complete_tag = known<8>(34);
  root.complete_value = known<16>(0xbeef);
  root.retire = known<1>(1);
  cycle([&] { completionOrRetirement(root, 1, 1, 34, 0xbeef, 0); });
}
