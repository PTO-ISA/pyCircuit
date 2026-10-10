#include "pycircuit_system.hpp"
#include <cstdlib>
#include <string_view>

void require(bool value) { if (!value) std::abort(); }
template <unsigned W> auto known(gfsim::Bits<W> value) {
  return gfsim::wire<gfsim::Bits<W>>::known(value);
}
template <unsigned W> void expect(const gfsim::wire<gfsim::Bits<W>> &wire,
                                  gfsim::Bits<W> value) {
  require(wire.isFullyKnown() && wire.value() == value);
  require(wire.packed().zMask() == gfsim::Bits<W>{0});
}
class TestSystem final : public gfsim::SimSystem {
  bool Precheck() noexcept override { return true; }
};
int main(int argc, char **argv) {
  const unsigned workers = argc == 2 ? std::stoul(argv[1]) : 1;
  gfsim::WorkExecutor pool(workers);
  top dut("constant-field-planes", &pool); dut.Build();
  dut.clk = known<1>(gfsim::Bits<1>{0});
  dut.rst = known<1>(gfsim::Bits<1>{0});
  dut.en = known<1>(gfsim::Bits<1>{1});
  dut.dynamic0 = known<8>(gfsim::Bits<8>{0x5a});
  dut.index0 = known<70>(gfsim::Bits<70>{1});
  dut.dynamic1 = gfsim::wire<gfsim::Bits<8>>::fromPacked(
      gfsim::FourState<8>::fromMasks(gfsim::Bits<8>{0xa5},
                                     gfsim::Bits<8>{0x0f},
                                     gfsim::Bits<8>{0x80}));
  dut.index1 = known<70>(gfsim::Bits<70>{1});
  dut.Reset(); dut.Xfer(); dut.Work();
  const gfsim::Bits<130> wide[] = {
      gfsim::Bits<130>{5ULL, 0ULL, 2ULL},
      gfsim::Bits<130>{9ULL, 2ULL, 0ULL},
      gfsim::Bits<130>{17ULL, 1ULL << 36, 0ULL}};
  for (unsigned i = 0; i < 3; ++i) {
    expect(dut.wide0.element(i), wide[i]);
    expect(dut.wide0.element(i + 3), wide[i]);
    expect(dut.wide1.element(i), wide[i]);
  }
  const gfsim::Bits<3> mode_codes[] = {gfsim::Bits<3>{1},
                                       gfsim::Bits<3>{4},
                                       gfsim::Bits<3>{7}};
  for (unsigned i = 0; i < 3; ++i) {
    const auto mode = dut.modes0.element(i).packed();
    require(mode.isFullyKnown() && mode.value() == mode_codes[i] &&
            mode.zMask() == gfsim::Bits<3>{0});
    expect(dut.mode_bits0.element(i), mode_codes[i]);
  }
  for (unsigned i = 0; i < 6; ++i) {
    const auto record = dut.records0.element(i).packed();
    require(record.isFullyKnown() && record.zMask() == gfsim::Bits<269>{0});
    require((record.value() & gfsim::Bits<269>{0xff}) ==
            gfsim::Bits<269>{0x5a});
  }
  const auto nested_low = gfsim::extract<65>(dut.records0.element(0).packed(), 8);
  const auto nested_high = gfsim::extract<65>(dut.records0.element(0).packed(), 73);
  require(nested_low.isFullyKnown() && nested_low.value() == gfsim::Bits<65>{5});
  require(nested_high.isFullyKnown() &&
          nested_high.value() == gfsim::Bits<65>{3, 1});
  for (unsigned lane = 0; lane < 2; ++lane) {
    expect(dut.generic_observed.element(lane * 3), gfsim::Bits<8>{0});
    expect(dut.generic_observed.element(lane * 3 + 1), gfsim::Bits<8>{1});
    expect(dut.generic_observed.element(lane * 3 + 2), gfsim::Bits<8>{2});
  }
  for (unsigned i = 0; i < 3; ++i) {
    expect(dut.computed_observed.element(i), gfsim::Bits<8>{3});
    expect(dut.forwarded_observed.element(i), gfsim::Bits<8>{1});
  }
  expect(dut.mapped_observed.element(0), gfsim::Bits<8>{1});
  expect(dut.mapped_observed.element(1), gfsim::Bits<8>{2});
  expect(dut.mapped_observed.element(2), gfsim::Bits<8>{1});
  expect(dut.mixed0.element(0), gfsim::Bits<8>{0});
  expect(dut.mixed0.element(1), gfsim::Bits<8>{0x5a});
  expect(dut.mixed0.element(2), gfsim::Bits<8>{1});
  expect(dut.flag0, gfsim::Bits<1>{1});
  expect(dut.selected_wide0, wide[1]);
  expect(dut.selected_dynamic0, gfsim::Bits<8>{0x5a});
  expect(dut.in_range0, gfsim::Bits<1>{1});
  const auto mixed_xz = dut.mixed1.element(1).packed();
  require(mixed_xz.value() == gfsim::Bits<8>{0xa5});
  require(mixed_xz.knownMask() == gfsim::Bits<8>{0x0f});
  require(mixed_xz.zMask() == gfsim::Bits<8>{0x80});
  const auto selected_xz = dut.selected_dynamic1.packed();
  require(selected_xz.value() == mixed_xz.value());
  require(selected_xz.knownMask() == mixed_xz.knownMask());
  require(selected_xz.zMask() == mixed_xz.zMask());
  expect(dut.flag1, gfsim::Bits<1>{1});
  expect(dut.selected_wide1, wide[1]);
  expect(dut.in_range1, gfsim::Bits<1>{1});
  expect(dut.q0, gfsim::Bits<8>{0});
  expect(dut.q1, gfsim::Bits<8>{0});
  dut.clk = known<1>(gfsim::Bits<1>{1});
  dut.Work(); dut.DiscardNext(); dut.Xfer(); dut.Work();
  expect(dut.q0, gfsim::Bits<8>{0});
  expect(dut.q1, gfsim::Bits<8>{0});
  dut.Work(); dut.Xfer(); dut.Work();
  expect(dut.q0, gfsim::Bits<8>{0x5a});
  const auto q1 = dut.q1.packed();
  require(q1.value() == gfsim::Bits<8>{0xa5});
  require(q1.knownMask() == gfsim::Bits<8>{0x0f});
  require(q1.zMask() == gfsim::Bits<8>{0x80});
  dut.dynamic0 = known<8>(gfsim::Bits<8>{0xc3});
  dut.index0 = known<70>(gfsim::Bits<70>{3});
  dut.Work();
  expect(dut.wide0.element(0), wide[0]);
  expect(dut.mixed0.element(1), gfsim::Bits<8>{0xc3});
  expect(dut.in_range0, gfsim::Bits<1>{0});
  require(dut.selected_wide0.packed().knownMask() == gfsim::Bits<130>{0});
  require(dut.selected_wide0.packed().zMask() == gfsim::Bits<130>{0});
  dut.index1 = gfsim::wire<gfsim::Bits<70>>::unknown();
  dut.Work();
  require(dut.selected_dynamic1.packed().knownMask() == gfsim::Bits<8>{0});
  require(dut.selected_dynamic1.packed().zMask() == gfsim::Bits<8>{0});
  require(dut.selected_dynamic1.packed().value() == gfsim::Bits<8>{0});
  gfsim::Bits<70> zbit{1};
  dut.index1 = gfsim::wire<gfsim::Bits<70>>::fromPacked(
      gfsim::FourState<70>::fromMasks(zbit, ~zbit, zbit));
  dut.Work();
  require(dut.in_range1.packed().knownMask() == gfsim::Bits<1>{0});
  require(dut.selected_wide1.packed().knownMask() == gfsim::Bits<130>{0});
  gfsim::Bits<70> high_index{0}; high_index.setWord(1, 1);
  dut.index0 = known<70>(high_index);
  dut.Work();
  expect(dut.in_range0, gfsim::Bits<1>{0});

  gfsim::WorkExecutor failure_pool(workers);
  top failure("failed-system", &failure_pool); TestSystem system;
  require(system.AddModule(failure));
  failure.clk = known<1>(gfsim::Bits<1>{0});
  failure.rst = known<1>(gfsim::Bits<1>{0});
  failure.en = known<1>(gfsim::Bits<1>{1});
  failure.dynamic0 = known<8>(gfsim::Bits<8>{0x31});
  failure.index0 = known<70>(gfsim::Bits<70>{0});
  failure.dynamic1 = known<8>(gfsim::Bits<8>{0x72});
  failure.index1 = known<70>(gfsim::Bits<70>{2});
  system.Build(); system.Reset();
  require(system.Step() == gfsim::SimStepResult::Running);
  expect(failure.q0, gfsim::Bits<8>{0});
  expect(failure.q1, gfsim::Bits<8>{0});
  expect(failure.pair_q.element(0), gfsim::Bits<8>{0});
  expect(failure.pair_q.element(1), gfsim::Bits<8>{0});
  const auto cycle = system.cycle();
  failure.clk = known<1>(gfsim::Bits<1>{1});
  failure.dynamic0 = known<8>(gfsim::Bits<8>{0x44});
  failure.en = gfsim::wire<gfsim::Bits<1>>::unknown();
  require(system.Step() == gfsim::SimStepResult::Failed);
  require(system.cycle() == cycle);
  expect(failure.q0, gfsim::Bits<8>{0});
  expect(failure.q1, gfsim::Bits<8>{0});
  expect(failure.pair_q.element(0), gfsim::Bits<8>{0});
  expect(failure.pair_q.element(1), gfsim::Bits<8>{0});
  failure.en = known<1>(gfsim::Bits<1>{1});
  failure.Work(); failure.Xfer(); failure.Work();
  expect(failure.q0, gfsim::Bits<8>{0x44});
  expect(failure.q1, gfsim::Bits<8>{0x72});
  expect(failure.pair_q.element(0), gfsim::Bits<8>{0x44});
  expect(failure.pair_q.element(1), gfsim::Bits<8>{0x44});
  require(system.state() == gfsim::SimSystemState::Failed);

  pyc_dut wrapped(workers); pyc_dut::Inputs ports;
  ports.clk=known<1>(gfsim::Bits<1>{0}); ports.rst=known<1>(gfsim::Bits<1>{0});
  ports.en=known<1>(gfsim::Bits<1>{1}); ports.dynamic0=known<8>(gfsim::Bits<8>{1});
  ports.index0=known<70>(gfsim::Bits<70>{0}); ports.dynamic1=known<8>(gfsim::Bits<8>{2});
  ports.index1=known<70>(gfsim::Bits<70>{0}); wrapped.drive(ports);
  wrapped.system().Build(); wrapped.system().Reset();
  require(wrapped.system().Step()==gfsim::SimStepResult::Running);
  ports.clk=known<1>(gfsim::Bits<1>{1}); ports.en=gfsim::wire<gfsim::Bits<1>>::unknown(); wrapped.drive(ports);
  require(wrapped.system().Step()==gfsim::SimStepResult::Failed);
  bool rejected=false; try { (void)wrapped.sample(); } catch(const std::logic_error&) { rejected=true; }
  require(rejected); wrapped.system().Reset();
  ports.en=known<1>(gfsim::Bits<1>{1}); wrapped.drive(ports);
  require(wrapped.system().Step()==gfsim::SimStepResult::Running);
  expect(wrapped.sample().wide0.element(0), wide[0]);
  require(wrapped.system().state()==gfsim::SimSystemState::Ready);
}
