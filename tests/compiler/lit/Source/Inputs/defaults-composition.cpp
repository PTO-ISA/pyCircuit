#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
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
      } else
        for (unsigned side = 0; side < 2; ++side)
          if (side ? r.right : r.left) {
            owner[side] = r.value;
            table[side][r.index] = r.value;
            ready[side][r.index] = 0;
          }
    }
    clock = r.clock;
  }
};
template <class Child> auto fields(const Child &c) {
  auto value = [](const auto &bits) {
    return static_cast<unsigned>(bits.value());
  };
  return std::array{value(c.prior),
                    value(c.proposed),
                    value(c.raw_ready),
                    value(c.configured_ready),
                    value(c.zero_payload),
                    value(c.default_payload),
                    value(c.untouched_payload),
                    value(c.snapshot_payload),
                    value(c.local_payload),
                    value(c.kept_mark),
                    value(c.owner_snapshot)};
}
template <class Output>
void check(const Output &output, const Golden &golden, Row r,
           bool print = false) {
  require(output.result.isFullyKnown());
  const auto value = output.result.value();
  std::array<unsigned, 25> actual{};
  const auto left = fields(value.left), right = fields(value.right);
  for (unsigned field = 0; field < left.size(); ++field) {
    actual[field] = left[field];
    actual[field + 11] = right[field];
  }
  actual[22] = value.fanout.value();
  actual[23] = value.sequential.value();
  actual[24] = value.nested.value();
  require(actual == golden.evaluate(r));
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
  gfsim::WorkExecutor pool(workers);
  pyc_root root("defaults_discard", &pool);
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

template <class T, std::size_t I>
using FieldType =
    std::remove_cvref_t<decltype(std::declval<T>().*
                                 std::get<I>(
                                     gfsim::hardware_traits<T>::fields))>;
template <std::size_t I, class T> auto field(const gfsim::wire<T> &value) {
  using Traits = gfsim::hardware_traits<T>;
  using Field = FieldType<T, I>;
  constexpr unsigned low =
      []<std::size_t... J>(std::index_sequence<J...>) {
        return (0u + ... +
                gfsim::hardware_traits<FieldType<T, I + 1 + J>>::width);
      }(std::make_index_sequence<std::tuple_size_v<decltype(Traits::fields)> -
                                 I - 1>{});
  return gfsim::wire<Field>::fromPacked(
      gfsim::extract<gfsim::hardware_traits<Field>::width>(value.packed(),
                                                           low));
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
  const auto first = field<0>(dut.sample().result);
  samePlanes(field<0>(first), known<5>(0));
  samePlanes(field<1>(first), mixed);
  samePlanes(field<7>(first), known<5>(0));
  samePlanes(field<8>(first), mixed);
  samePlanes(field<10>(first), known<5>(0));
  dut.drive(inputs({0, 0, 0, 0, 0, 0}));
  require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  const auto held = field<0>(dut.sample().result);
  samePlanes(field<0>(held), mixed);
  samePlanes(field<1>(held), mixed);
  samePlanes(field<5>(held), mixed);
  samePlanes(field<6>(held), known<5>(7));
  samePlanes(field<7>(held), known<5>(0));
  samePlanes(field<8>(held), known<5>(0));
  samePlanes(field<10>(held), mixed);
}
void scalarImages(unsigned workers) {
  gfsim::WorkExecutor pool(workers);
  defaults_probe::pyc_64657369676e::ScalarLiteral literal("literal_image",
                                                          &pool);
  defaults_probe::pyc_64657369676e::ScalarExpression expression(
      "expression_image", &pool);
  auto drive = [&](unsigned clock, unsigned index) {
    literal.pyc_7079635f636c6b = expression.pyc_7079635f636c6b =
        known<1>(clock);
    literal.pyc_7079635f727374 = expression.pyc_7079635f727374 = known<1>(0);
    literal.index = expression.index = known<1>(index);
  };
  drive(0, 0);
  literal.Reset();
  expression.Reset();
  literal.Xfer();
  expression.Xfer();
  constexpr std::array clocks{0u, 1u, 1u, 0u, 1u};
  for (unsigned n = 0; n < clocks.size(); ++n) {
    drive(clocks[n], n % 2);
    literal.Work();
    expression.Work();
    require(literal.result.isFullyKnown() && expression.result.isFullyKnown());
    require(literal.result.value().value.value() == 7);
    require(expression.result.value().value.value() == 7);
    literal.Xfer();
    expression.Xfer();
  }
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
  scalarImages(workers);
}
