// SPDX-License-Identifier: BSD-3-Clause

#include "gfsim/dff.h"
#include "gfsim/sampled_clock.h"
#include "gtest/gtest.h"

#include <array>
#include <string_view>

namespace {

using Bit = gfsim::Bits<1>;
using Control = gfsim::wire<Bit>;
using Kernel = gfsim::dffe_kernel<gfsim::Bits<9>>;

Control control(char symbol, bool hiddenValue = true) {
  if (symbol == '0' || symbol == '1')
    return Control::known(Bit{symbol == '1' ? 1u : 0u});
  const Bit value{hiddenValue ? 1u : 0u};
  if (symbol == 'x')
    return Control::fromPacked(gfsim::FourState<1>::unknown(value));
  return Control::fromPacked(gfsim::FourState<1>::highImpedance(value));
}

struct ExpectedSample {
  bool nextClock;
  bool rising;
  bool resetActive;
  gfsim::SampledClockFault fault;
};

ExpectedSample expectedSample(bool previous, char clock, char reset) {
  if (clock == 'x' || clock == 'z')
    return {previous, false, false, gfsim::SampledClockFault::UnknownClock};
  if (clock == '0')
    return {false, false, false, gfsim::SampledClockFault::None};
  if (previous)
    return {true, false, false, gfsim::SampledClockFault::None};
  if (reset == 'x' || reset == 'z')
    return {false, true, false, gfsim::SampledClockFault::UnknownReset};
  return {true, true, reset == '1', gfsim::SampledClockFault::None};
}

void expectSample(const gfsim::SampledClockEvaluation &actual,
                  const ExpectedSample &expected) {
  EXPECT_EQ(actual.nextClock, expected.nextClock);
  EXPECT_EQ(actual.rising, expected.rising);
  EXPECT_EQ(actual.resetActive, expected.resetActive);
  EXPECT_EQ(actual.fault, expected.fault);
}

template <class Callback>
void expectViolation(Callback &&callback, std::string_view message) {
  bool caught = false;
  try {
    callback();
  } catch (const gfsim::FourStateViolation &error) {
    caught = true;
    EXPECT_EQ(error.what(), message);
  } catch (...) {
    FAIL() << "unexpected exception type";
  }
  EXPECT_TRUE(caught);
}

struct RegisterHarness {
  Kernel::Current current;
  Kernel::Pending pending;
  Control clk = control('0');
  Control rst = control('0');
  Control en = control('1');
  gfsim::wire<gfsim::Bits<9>> data = value(0);
  gfsim::wire<gfsim::Bits<9>> init = value(3);
  gfsim::wire<gfsim::Bits<9>> output;

  static gfsim::wire<gfsim::Bits<9>> value(unsigned raw) {
    return gfsim::wire<gfsim::Bits<9>>::known(gfsim::Bits<9>{raw});
  }

  Kernel::Inputs inputs() const noexcept { return {clk, rst, en, data, init}; }

  void initialize() {
    Kernel::reset(current, inputs(), pending);
    Kernel::xfer(current, pending, {output});
  }

  void work() { Kernel::work(current, inputs(), pending, {output}); }
  void xfer() noexcept { Kernel::xfer(current, pending, {output}); }
  void discard() noexcept { Kernel::discard(current, pending); }
};

void expectRolledBack(const RegisterHarness &reg, unsigned q, bool clock) {
  EXPECT_EQ(reg.current.q.value(), gfsim::Bits<9>{q});
  EXPECT_EQ(reg.current.clock, clock);
  EXPECT_FALSE(reg.pending.valid);
  EXPECT_EQ(reg.pending.q.value(), gfsim::Bits<9>{q});
  EXPECT_EQ(reg.pending.clock, clock);
  EXPECT_EQ(reg.output.value(), gfsim::Bits<9>{q});
}

TEST(SampledClockTest, EvaluatorCoversEveryClockResetSymbolAndPriorLevel) {
  constexpr std::array symbols{'0', '1', 'x', 'z'};
  for (bool previous : {false, true}) {
    for (char clock : symbols) {
      for (char reset : symbols) {
        SCOPED_TRACE(::testing::Message()
                     << "previous=" << previous << " clock=" << clock
                     << " reset=" << reset);
        const auto expected = expectedSample(previous, clock, reset);
        expectSample(gfsim::evaluateSampledClock(previous, control(clock),
                                                 control(reset)),
                     expected);
        // X and Z carry an arbitrary binary value plane. Sampling decisions
        // must depend only on their four-state classification.
        expectSample(gfsim::evaluateSampledClock(previous,
                                                 control(clock, false),
                                                 control(reset, false)),
                     expected);
      }
    }
  }
}

TEST(SampledClockTest,
     ResetInterpretationIgnoresHiddenUnknownValueAndPreservesStrictRawState) {
  for (char symbol : {'x', 'z'}) {
    for (bool hidden : {false, true}) {
      SCOPED_TRACE(::testing::Message()
                   << "symbol=" << symbol << " hidden=" << hidden);
      const auto raw = control(symbol, hidden);
      const auto strict =
          gfsim::interpretReset(raw, gfsim::ResetSemantics::Strict);
      EXPECT_FALSE(strict.isFullyKnown());
      EXPECT_EQ(strict.packed().value().bit(0), hidden);
      EXPECT_EQ(strict.packed().zMask().bit(0), symbol == 'z');

      const auto ifOne =
          gfsim::interpretReset(raw, gfsim::ResetSemantics::IfOne);
      EXPECT_TRUE(ifOne.isFullyKnown());
      EXPECT_FALSE(ifOne.value().toBool());

      expectSample(
          gfsim::evaluateSampledClock(false, control('1'), raw,
                                      gfsim::ResetSemantics::Strict),
          {false, true, false, gfsim::SampledClockFault::UnknownReset});
      expectSample(gfsim::evaluateSampledClock(false, control('1'), raw,
                                               gfsim::ResetSemantics::IfOne),
                   {true, true, false, gfsim::SampledClockFault::None});
      expectSample(
          gfsim::evaluateSampledClock(false, control('x', hidden), raw,
                                      gfsim::ResetSemantics::IfOne),
          {false, false, false, gfsim::SampledClockFault::UnknownClock});
    }
  }
}

TEST(SampledClockTest,
     DffeFaultsReportExactMessagesAndRollbackAllPendingState) {
  RegisterHarness reg;
  reg.initialize();
  ASSERT_EQ(reg.current.q.value(), gfsim::Bits<9>{3});

  reg.clk = control('x');
  expectViolation([&] { reg.work(); }, "dffe clock must be known");
  expectRolledBack(reg, 3, false);
  reg.xfer();
  expectRolledBack(reg, 3, false);

  reg.clk = control('1');
  reg.rst = control('z');
  expectViolation([&] { reg.work(); }, "dffe reset must be known at posedge");
  expectRolledBack(reg, 3, false);
  reg.xfer();
  expectRolledBack(reg, 3, false);

  reg.rst = control('0');
  reg.en = control('x');
  expectViolation([&] { reg.work(); }, "dffe enable must be known at posedge");
  expectRolledBack(reg, 3, false);
  reg.xfer();
  expectRolledBack(reg, 3, false);
}

TEST(SampledClockTest, NonRisingSamplesIgnoreResetAndEnable) {
  RegisterHarness reg;
  reg.initialize();

  reg.clk = control('1');
  reg.rst = control('0');
  reg.en = control('0');
  reg.data = RegisterHarness::value(17);
  reg.work();
  ASSERT_TRUE(reg.pending.valid);
  EXPECT_TRUE(reg.pending.clock);
  reg.xfer();
  EXPECT_TRUE(reg.current.clock);
  EXPECT_EQ(reg.current.q.value(), gfsim::Bits<9>{3});

  reg.rst = control('x');
  reg.en = control('z');
  reg.data = RegisterHarness::value(91);
  EXPECT_NO_THROW(reg.work());
  EXPECT_TRUE(reg.pending.clock);
  reg.xfer();
  EXPECT_EQ(reg.current.q.value(), gfsim::Bits<9>{3});

  reg.clk = control('0');
  EXPECT_NO_THROW(reg.work());
  EXPECT_FALSE(reg.pending.clock);
  reg.xfer();
  EXPECT_FALSE(reg.current.clock);
  EXPECT_EQ(reg.current.q.value(), gfsim::Bits<9>{3});

  reg.clk = control('1');
  reg.rst = control('0');
  reg.en = control('1');
  reg.work();
  reg.xfer();
  EXPECT_TRUE(reg.current.clock);
  EXPECT_EQ(reg.current.q.value(), gfsim::Bits<9>{91});
}

TEST(SampledClockTest, ExplicitResetStagesInitialValueWithoutSamplingPins) {
  RegisterHarness reg;
  reg.current.q = RegisterHarness::value(41);
  reg.current.clock = true;
  reg.clk = control('x');
  reg.rst = control('z');
  reg.en = control('x');
  reg.init = RegisterHarness::value(7);

  EXPECT_NO_THROW(Kernel::reset(reg.current, reg.inputs(), reg.pending));
  EXPECT_TRUE(reg.pending.valid);
  EXPECT_FALSE(reg.pending.clock);
  EXPECT_EQ(reg.pending.q.value(), gfsim::Bits<9>{7});
  Kernel::xfer(reg.current, reg.pending, {reg.output});
  EXPECT_FALSE(reg.current.clock);
  EXPECT_EQ(reg.current.q.value(), gfsim::Bits<9>{7});
}

TEST(SampledClockTest, DiscardedRisingEdgeCanRetryAtTheSameHighLevel) {
  RegisterHarness reg;
  reg.initialize();
  reg.clk = control('1');
  reg.rst = control('0');
  reg.en = control('1');
  reg.data = RegisterHarness::value(29);

  reg.work();
  ASSERT_TRUE(reg.pending.valid);
  ASSERT_TRUE(reg.pending.clock);
  ASSERT_EQ(reg.pending.q.value(), gfsim::Bits<9>{29});
  reg.discard();
  expectRolledBack(reg, 3, false);
  reg.xfer();
  expectRolledBack(reg, 3, false);

  reg.data = RegisterHarness::value(37);
  reg.work();
  reg.xfer();
  EXPECT_TRUE(reg.current.clock);
  EXPECT_EQ(reg.current.q.value(), gfsim::Bits<9>{37});
}

} // namespace
