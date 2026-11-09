#include "gfsim/SimExecutor.h"
#include "design.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string_view>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "defaults oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
struct Row {
  unsigned clock, reset, left, right, index, value;
};
constexpr unsigned sampleCount = 72;
Row row(unsigned n) {
  if (n < 64)
    return {n % 2,
            unsigned(n == 31),
            unsigned(n % 3 != 0),
            unsigned(n % 5 < 2),
            (n / 2) % 2,
            (n * 7 + 13) % 32};
  constexpr std::array<Row, 8> tail{{{1, 0, 1, 0, 0, 17},
                                     {1, 0, 0, 1, 1, 29},
                                     {0, 0, 1, 1, 0, 11},
                                     {0, 0, 0, 1, 1, 19},
                                     {1, 0, 1, 1, 0, 31},
                                     {1, 0, 1, 1, 1, 23},
                                     {0, 0, 0, 0, 0, 0},
                                     {1, 0, 0, 0, 1, 0}}};
  return tail[n - 64];
}
pyc_dut::Inputs inputs(Row row) {
  pyc_dut::Inputs in;
  in.pyc_7079635f636c6b = known<1>(row.clock);
  in.pyc_7079635f727374 = known<1>(row.reset);
  in.left_enable = known<1>(row.left);
  in.right_enable = known<1>(row.right);
  in.index = known<1>(row.index);
  in.value = known<5>(row.value);
  return in;
}
struct Golden {
  unsigned owner[2]{};
  unsigned table[2][2]{{7, 7}, {7, 7}};
  unsigned ready[2][2]{{1, 1}, {1, 1}};
  unsigned boxWords[2]{0x192a3c, 0x192a3c};
  unsigned boxFlags[2]{1, 1};
  unsigned clock = 0;
  std::array<unsigned, 25> evaluate(Row r) const {
    std::array<unsigned, 25> out{};
    for (unsigned side = 0; side < 2; ++side) {
      const unsigned enable = side ? r.right : r.left;
      const unsigned next = enable ? r.value : owner[side];
      const std::array child{owner[side],
                             next,
                             0u,
                             enable ? 0u : ready[side][r.index],
                             0u,
                             table[side][r.index],
                             table[side][r.index ^ 1],
                             0u,
                             r.value,
                             9u,
                             owner[side]};
      for (unsigned field = 0; field < child.size(); ++field)
        out[side * 11 + field] = child[field];
    }
    out[22] = (out[1] + 3) % 32;
    out[23] = (out[1] + 6) % 32;
    out[24] = (out[12] + 6) % 32;
    return out;
  }
  void commit(Row r) {
    if (r.clock && !clock) {
      if (r.reset) {
        owner[0] = owner[1] = 0;
        for (auto &side : table)
          for (auto &v : side)
            v = 7;
        for (auto &side : ready)
          for (auto &v : side)
            v = 1;
        boxWords[0] = boxWords[1] = 0x192a3c;
        boxFlags[0] = boxFlags[1] = 1;
      } else {
        for (unsigned side = 0; side < 2; ++side)
          if (side ? r.right : r.left) {
            owner[side] = r.value;
            table[side][r.index] = r.value;
            ready[side][r.index] = 0;
          }
        if (r.left) {
          boxWords[r.index] = ((r.value * 8 + 6) << 16) | (73u << 8) | 171u;
          boxFlags[r.index] = 0;
        }
      }
    }
    clock = r.clock;
  }
};
template <unsigned W, unsigned Total>
auto slice(const gfsim::wire<gfsim::Bits<Total>> &value, unsigned low) {
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(
      gfsim::extract<W, Total>(value.packed(), low));
}
template <unsigned Low>
auto childFields(const gfsim::wire<gfsim::Bits<332>> &result) {
  return std::array{unsigned(slice<5>(result, Low + 41).value().value()),
                    unsigned(slice<5>(result, Low + 36).value().value()),
                    unsigned(slice<1>(result, Low + 35).value().value()),
                    unsigned(slice<1>(result, Low + 34).value().value()),
                    unsigned(slice<5>(result, Low + 29).value().value()),
                    unsigned(slice<5>(result, Low + 24).value().value()),
                    unsigned(slice<5>(result, Low + 19).value().value()),
                    unsigned(slice<5>(result, Low + 14).value().value()),
                    unsigned(slice<5>(result, Low + 9).value().value()),
                    unsigned(slice<4>(result, Low + 5).value().value()),
                    unsigned(slice<5>(result, Low).value().value())};
}
template <class Output>
void check(const Output &output, const Golden &golden, Row r,
           bool print = false) {
  require(output.result.isFullyKnown());
  std::array<unsigned, 25> actual{};
  const auto left = childFields<286>(output.result);
  const auto right = childFields<240>(output.result);
  for (unsigned field = 0; field < left.size(); ++field) {
    actual[field] = left[field];
    actual[field + 11] = right[field];
  }
  actual[22] = slice<5>(output.result, 235).value().value();
  actual[23] = slice<5>(output.result, 230).value().value();
  actual[24] = slice<5>(output.result, 225).value().value();
  require(actual == golden.evaluate(r));
  // Independent MSB-first packing: each Word is value:u5 then mark:u3,
  // Table element zero is highest, and Box.flag is the least significant bit.
  auto box = [&](unsigned low) {
    return static_cast<unsigned>(slice<25>(output.result, low).value().value());
  };
  constexpr unsigned defaultWords = (25u << 16) | (42u << 8) | 60u;
  const unsigned replacedWords = ((r.value * 8 + 6) << 16) | (73u << 8) | 171u;
  require(box(200) == (defaultWords << 1 | 1u));
  require(box(175) == (defaultWords << 1 | r.left));
  require(box(150) == (replacedWords << 1 | r.left));
  require(box(125) ==
          (r.left ? (replacedWords << 1 | 1u) : (defaultWords << 1 | 1u)));
  require(box(100) == 0);
  require(box(75) == (defaultWords << 1 | 1u));
  require(box(50) ==
          (golden.boxWords[r.index] << 1 | golden.boxFlags[r.index]));
  require(box(25) ==
          (r.left ? replacedWords << 1 :
                    (golden.boxWords[r.index] << 1 | golden.boxFlags[r.index])));
  require(box(0) ==
          (golden.boxWords[r.index ^ 1] << 1 | golden.boxFlags[r.index ^ 1]));
  if (print) {
    std::cout << "WORK";
    for (unsigned field : actual)
      std::cout << ' ' << field;
    std::cout << '\n';
  }
}
void configure(gfsim::SimExecutor &executor) {
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
}
void failure(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  configure(executor);
  dut.drive(inputs({}));
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto in = inputs({1, 0, 1, 1, 0, 11});
  in.right_enable = gfsim::wire<gfsim::Bits<1>>::unknown();
  dut.drive(in);
  PycircuitModelStepResultV1 result{sizeof(result)};
  require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
  require(result.state == PYCIRCUIT_MODEL_STEP_V1_FAILED &&
          executor.cycles() == 0 && dut.system().cycle() == 0);
  bool rejected = false;
  try {
    (void)dut.sample();
  } catch (const std::logic_error &) {
    rejected = true;
  }
  require(rejected);
}
void discardRetry(unsigned workers) {
  pyc_dut dut(workers);
  auto &root = dut.root();
  Golden golden;
  auto drive = [&](const pyc_dut::Inputs &in) {
    root.pyc_7079635f636c6b = in.pyc_7079635f636c6b;
    root.pyc_7079635f727374 = in.pyc_7079635f727374;
    root.left_enable = in.left_enable;
    root.right_enable = in.right_enable;
    root.index = in.index;
    root.value = in.value;
  };
  drive(inputs({}));
  root.Reset();
  root.Xfer();
  const Row transaction{1, 0, 1, 1, 0, 11};
  drive(inputs(transaction));
  root.Work();
  check(root, golden, transaction);
  root.DiscardNext();
  root.Xfer();
  auto bad = inputs(transaction);
  bad.right_enable = gfsim::wire<gfsim::Bits<1>>::unknown();
  drive(bad);
  bool rejected = false;
  try {
    root.Work();
  } catch (const gfsim::FourStateViolation &) {
    rejected = true;
  }
  require(rejected);
  root.DiscardNext();
  root.Xfer();
  drive(inputs(transaction));
  root.Work();
  check(root, golden, transaction);
  root.Xfer();
  golden.commit(transaction);
  for (Row r : std::array<Row, 4>{{{0, 0, 0, 0, 0, 0},
                                   {1, 0, 1, 0, 1, 29},
                                   {0, 0, 0, 0, 1, 0},
                                   {1, 0, 0, 0, 0, 0}}}) {
    drive(inputs(r));
    root.Work();
    check(root, golden, r);
    root.Xfer();
    golden.commit(r);
  }
  require(golden.owner[0] == 29 && golden.owner[1] == 11);
}

template <unsigned W>
void samePlanes(const gfsim::wire<gfsim::Bits<W>> &actual,
                const gfsim::wire<gfsim::Bits<W>> &expected) {
  require(actual.packed().value() == expected.packed().value());
  require(actual.packed().knownMask() == expected.packed().knownMask());
  require(actual.packed().zMask() == expected.packed().zMask());
}
void dataPlanes(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  configure(executor);
  dut.drive(inputs({}));
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  const auto mixed =
      gfsim::wire<gfsim::Bits<5>>::fromPacked(gfsim::FourState<5>::fromMasks(
          gfsim::Bits<5>{21}, gfsim::Bits<5>{25}, gfsim::Bits<5>{2}));
  auto in = inputs({1, 0, 1, 1, 0, 0});
  in.value = mixed;
  dut.drive(in);
  PycircuitModelStepResultV1 result{sizeof(result)};
  require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  const auto resultWire = dut.sample().result;
  samePlanes(slice<5>(resultWire, 327), known<5>(0));
  samePlanes(slice<5>(resultWire, 322), mixed);
  samePlanes(slice<5>(resultWire, 300), known<5>(0));
  samePlanes(slice<5>(resultWire, 295), mixed);
  samePlanes(slice<5>(resultWire, 286), known<5>(0));
  const auto replaced = slice<25>(resultWire, 150);
  const auto expected = gfsim::FourState<25>::fromMasks(
      gfsim::Bits<25>{((174u << 16 | 73u << 8 | 171u) << 1) | 1u},
      gfsim::Bits<25>{((207u << 16 | 0xffffu) << 1) | 1u},
      gfsim::Bits<25>{16u << 17});
  require(replaced.packed().value() == expected.value());
  require(replaced.packed().knownMask() == expected.knownMask());
  require(replaced.packed().zMask() == expected.zMask());
  const auto proposed = slice<25>(resultWire, 25);
  require(proposed.packed().value() == (expected.value() & gfsim::Bits<25>{0x1fffffe}));
  require(proposed.packed().knownMask() == expected.knownMask());
  require(proposed.packed().zMask() == expected.zMask());
  dut.drive(inputs({0, 0, 0, 0, 0, 0}));
  require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  const auto held = dut.sample().result;
  samePlanes(slice<5>(held, 327), mixed);
  samePlanes(slice<5>(held, 322), mixed);
  samePlanes(slice<5>(held, 310), mixed);
  samePlanes(slice<5>(held, 305), known<5>(7));
  samePlanes(slice<5>(held, 300), known<5>(0));
  samePlanes(slice<5>(held, 295), known<5>(0));
  samePlanes(slice<5>(held, 286), mixed);
  const auto oldBox = slice<25>(held, 50);
  require(oldBox.packed().value() == (expected.value() & gfsim::Bits<25>{0x1fffffe}));
  require(oldBox.packed().knownMask() == expected.knownMask());
  require(oldBox.packed().zMask() == expected.zMask());
}
int main(int argc, char **argv) {
  require(argc == 2);
  unsigned workers = std::strtoul(argv[1], nullptr, 10);
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  configure(executor);
  dut.drive(inputs({}));
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  Golden golden;
  for (unsigned n = 0; n < sampleCount; ++n) {
    Row r = row(n);
    dut.drive(inputs(r));
    PycircuitModelStepResultV1 result{sizeof(result)};
    require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    check(dut.sample(), golden, r, true);
    golden.commit(r);
  }
  failure(workers);
  discardRetry(workers);
  dataPlanes(workers);
}
