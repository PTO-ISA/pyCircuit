#include "pycircuit_system.hpp"
#include <cstdlib>
#include <cstdint>
#include <gfsim/SystemRunner.h>
#include <gfsim/WorkExecutor.h>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "native check oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
auto bit(char value) {
  require(value == '0' || value == '1' || value == 'x' || value == 'z');
  auto planes = gfsim::FourState<1>::fromMasks(
      gfsim::Bits<1>{value == '1' ? 1u : 0u},
      gfsim::Bits<1>{value == '0' || value == '1' ? 1u : 0u},
      gfsim::Bits<1>{value == 'z' ? 1u : 0u});
  return gfsim::wire<gfsim::Bits<1>>::fromPacked(planes);
}
template <unsigned W> std::string planes(gfsim::FourState<W> value) {
  std::string result;
  for (unsigned i = W; i-- > 0;)
    result += value.zMask().bit(i)        ? 'z'
              : !value.knownMask().bit(i) ? 'x'
              : value.value().bit(i)      ? '1'
                                          : '0';
  return result;
}
std::string hex(std::string_view value) {
  if (value.empty())
    return "-";
  constexpr char digits[] = "0123456789abcdef";
  std::string result;
  for (unsigned char byte : value) {
    result += digits[byte >> 4];
    result += digits[byte & 15];
  }
  return result;
}
std::string sample(pyc_dut &dut) {
  auto out = dut.sample();
#if T3_CASE == 0
  return planes(out.value.packed());
#elif T3_CASE == 1
  return planes(gfsim::extract<8>(out.result.packed(), 0));
#elif T3_CASE == 2
  return planes(gfsim::extract<2>(out.result.packed(), 0));
#elif T3_CASE == 5
  return planes(out.left.packed()) + planes(out.right.packed());
#elif T3_CASE == 3
  return planes(out.tail.packed());
#elif T3_CASE == 6
  return planes(out.result.packed());
#else
  return planes(out.qdff.packed()) + planes(out.qdffe.packed()) +
         planes(out.qbyte.packed()) + planes(out.qsync.packed()) +
         planes(out.qdp0.packed()) + planes(out.qdp1.packed()) +
         planes(out.available.packed()) + planes(out.head.packed()) +
         planes(out.delayed_available.packed()) +
         planes(out.delayed_head.packed());
#endif
}
void report(pyc_dut &dut, std::string_view label, gfsim::SimStepResult result) {
  std::string output = "-";
  bool available = false;
  try {
    output = sample(dut);
    available = true;
  } catch (const std::logic_error &) {
  }
  const auto info = dut.system().failureInfo();
  std::cout << "ROW " << label << ' ' << static_cast<unsigned>(result) << ' '
            << dut.system().cycle() << ' ' << static_cast<unsigned>(info.phase)
            << ' ' << hex(info.code) << ' ' << hex(info.message) << ' '
            << hex(info.instance) << ' ' << hex(info.sourceJson) << ' '
            << hex(info.checkIdJson) << ' ' << available << ' ' << output
            << '\n';
}
gfsim::SimStepResult step(pyc_dut &dut) {
  static unsigned attempts = 0;
  require(++attempts <= 512); // Explicit finite host bound, independent of DUT.
  return dut.system().Step();
}
void ready(pyc_dut &dut) {
  dut.system().Build();
  require(dut.system().state() == gfsim::SimSystemState::Built);
  dut.system().Reset();
  require(dut.system().state() == gfsim::SimSystemState::Ready);
  require(dut.system().cycle() == 0);
}
void incomplete(pyc_root &root) {
  gfsim::SimFailureInfo error;
  require(!root.__pyc_validate_checks(error));
  require(error.phase == gfsim::SimFailurePhase::Check);
  require(error.code == "runtime_failure");
  require(error.instance.empty() && error.sourceJson.empty() &&
          error.checkIdJson.empty());
}
#if T3_CASE == 0
struct DrivenChecks {
  pyc_dut &dut;
  unsigned initialized = 0, drives = 0, samples = 0;
  static void initialize(void *context) {
    auto &self = *static_cast<DrivenChecks *>(context);
    ++self.initialized;
    pyc_dut::Inputs inputs;
    inputs.path = known<1>(0);
    inputs.condition = known<1>(0);
    self.dut.drive(inputs);
  }
  static bool drive(void *context, std::uint64_t epoch) {
    auto &self = *static_cast<DrivenChecks *>(context);
    require(epoch == self.dut.system().cycle() && epoch < 2);
    ++self.drives;
    pyc_dut::Inputs inputs;
    inputs.path = known<1>(epoch == 1);
    inputs.condition = known<1>(0);
    self.dut.drive(inputs);
    return true;
  }
  static void sample(void *context, std::uint64_t epoch) {
    auto &self = *static_cast<DrivenChecks *>(context);
    ++self.samples;
    require(epoch == 1 && self.dut.system().cycle() == 1);
    require(planes(self.dut.sample().value.packed()) == "0");
  }
};
#endif
int main(int argc, char **argv) {
  require(argc == 2 || argc == 4);
  unsigned workers = static_cast<unsigned>(std::stoul(argv[1]));
  require(workers == 1 || workers == 2);
#if T3_CASE == 0
  if (argc == 4) {
    std::string binary = "checked-native", workerFlag = "--workers";
    std::string configFlag = "--config", eventsFlag = "--events";
    char *runnerArgs[] = {
        binary.data(), workerFlag.data(), argv[1], configFlag.data(),
        argv[2],       eventsFlag.data(), argv[3]};
    gfsim::SystemRunner runner(7, runnerArgs);
    require(runner.ready() && runner.workers() == workers);
    pyc_dut dut(runner.workers());
    DrivenChecks context{dut};
    gfsim::RunnerCallbacks callbacks{&context, DrivenChecks::initialize,
                                     DrivenChecks::drive, DrivenChecks::sample};
    require(runner.Run(dut.system(), dut.observations(), {}, callbacks) == 1);
    require(context.initialized == 1 && context.drives == 2 &&
            context.samples == 1);
    report(dut, "runner-failure", gfsim::SimStepResult::Failed);
    pyc_dut::Inputs inputs;
    inputs.path = known<1>(0);
    inputs.condition = known<1>(1);
    dut.drive(inputs);
    report(dut, "runner-retry", step(dut));
    std::cout << "RUNNER initialized=1 drives=2 samples=1 bounded=8\n";
    return 0;
  }
  for (char path : std::string_view("01xz"))
    for (char condition : std::string_view("01xz")) {
      pyc_dut dut(workers);
      pyc_dut::Inputs inputs;
      inputs.path = bit(path);
      inputs.condition = bit(condition);
      dut.drive(inputs);
      require(step(dut) == gfsim::SimStepResult::InvalidState);
      ready(dut); // Construction and host Reset do not sample failing checks.
      auto label = std::string("matrix-") + path + condition;
      auto result = step(dut);
      report(dut, label, result);
      if (result == gfsim::SimStepResult::Failed) {
        auto error = dut.system().failureInfo();
        inputs.path = known<1>(0);
        inputs.condition = known<1>(1);
        dut.drive(inputs);
        report(dut, label + "-retry", step(dut));
        require(dut.system().failureInfo().checkIdJson == error.checkIdJson);
        dut.system().Reset();
        require(dut.system().failureInfo().code.empty());
        report(dut, label + "-reset", step(dut));
      }
    }
#elif T3_CASE == 1
  pyc_dut dut(workers);
  pyc_dut::Inputs inputs;
  inputs.first = known<8>(7);
  inputs.second = known<8>(9);
  inputs.allow = known<1>(1);
  inputs.pyc_7079635f636c6b = known<1>(0);
  inputs.pyc_7079635f727374 = known<1>(0);
  dut.drive(inputs);
  ready(dut);
  report(dut, "state-low", step(dut));
  inputs.pyc_7079635f636c6b = known<1>(1);
  dut.drive(inputs);
  report(dut, "state-rise", step(dut));
  inputs.first = known<8>(10);
  inputs.second = known<8>(11);
  dut.drive(inputs);
  report(dut, "state-held", step(dut));
  inputs.pyc_7079635f636c6b = known<1>(0);
  dut.drive(inputs);
  report(dut, "state-fall", step(dut));
  inputs.pyc_7079635f636c6b = known<1>(1);
  inputs.first = known<8>(0);
  dut.drive(inputs);
  report(dut, "state-fail-between", step(dut));
  inputs.first = known<8>(13);
  inputs.second = known<8>(14);
  inputs.pyc_7079635f727374 = known<1>(1);
  dut.drive(inputs);
  report(dut, "state-physical-reset-retry", step(dut));
  dut.system().Reset();
  inputs.pyc_7079635f727374 = known<1>(0);
  dut.drive(inputs);
  report(dut, "state-host-reset", step(dut));
  inputs.first = known<8>(1);
  inputs.second = known<8>(0);
  dut.drive(inputs);
  report(dut, "state-fail-after-held", step(dut));
  dut.system().Reset();
  inputs.pyc_7079635f636c6b = known<1>(0);
  inputs.allow = known<1>(0);
  dut.drive(inputs);
  report(dut, "state-fail-allow-falling", step(dut));
  dut.system().Reset();
  inputs.pyc_7079635f636c6b = bit('x');
  inputs.allow = known<1>(0);
  dut.drive(inputs);
  report(dut, "state-primitive-precedence", step(dut));
  // Explicit discard on the actual generated root, before any successful Xfer.
  gfsim::WorkExecutor executor(workers);
  pyc_root root("root", &executor);
  root.Build();
  incomplete(root);
  root.Reset();
  root.Xfer();
  incomplete(root);
  root.first = known<8>(5);
  root.second = known<8>(6);
  root.allow = known<1>(1);
  root.pyc_7079635f636c6b = known<1>(1);
  root.pyc_7079635f727374 = known<1>(0);
  root.Work();
  gfsim::SimFailureInfo error;
  require(root.__pyc_validate_checks(error));
  root.DiscardNext();
  incomplete(root);
  root.Work();
  require(planes(gfsim::extract<8>(root.result.packed(), 0)) == "00000011");
  require(root.__pyc_validate_checks(error));
  root.Xfer();
  incomplete(root);
  root.Work();
  require(planes(gfsim::extract<8>(root.result.packed(), 0)) == "00000110");
  std::cout << "DIRECT state-discard-clock-rollback\n";
#elif T3_CASE == 2
  for (unsigned scenario = 0; scenario < 4; ++scenario) {
    pyc_dut dut(workers);
    pyc_dut::Inputs inputs;
    inputs.pyc_7079635f636c6b = known<1>(1);
    inputs.pyc_7079635f727374 = known<1>(scenario == 0);
    inputs.data = known<1>(1);
    inputs.allow_a = known<1>(scenario == 1 || scenario == 2);
    inputs.allow_b = known<1>(scenario == 1);
    dut.drive(inputs);
    ready(dut);
    report(dut, "hierarchy-" + std::to_string(scenario), step(dut));
  }
#elif T3_CASE == 3
  for (unsigned scenario = 0; scenario < 3; ++scenario) {
    pyc_dut dut(workers);
    pyc_dut::Inputs inputs;
    for (unsigned lane = 0; lane < 6; ++lane) {
      inputs.conditions.element(lane) = known<1>(1);
      inputs.clocks.element(lane) = known<1>(0);
      inputs.resets.element(lane) = known<1>(0);
    }
    inputs.conditions.element(5) = known<1>(0);
    inputs.resets.element(5) = known<1>(scenario != 0);
    inputs.ordinary = known<1>(scenario == 2);
    dut.drive(inputs);
    ready(dut);
    report(dut, "collection-" + std::to_string(scenario), step(dut));
  }
  for (char condition : std::string_view("01xz"))
    for (char reset : std::string_view("01xz")) {
      pyc_dut dut(workers);
      pyc_dut::Inputs inputs;
      for (unsigned lane = 0; lane < 6; ++lane) {
        inputs.conditions.element(lane) = known<1>(1);
        inputs.clocks.element(lane) = known<1>(0);
        inputs.resets.element(lane) = known<1>(0);
      }
      inputs.conditions.element(5) = bit(condition);
      inputs.resets.element(5) = bit(reset);
      inputs.ordinary = known<1>(1);
      dut.drive(inputs);
      ready(dut);
      report(dut, std::string("collection-matrix-") + condition + reset,
             step(dut));
    }
#elif T3_CASE == 5
  pyc_dut dut(workers);
  pyc_dut::Inputs inputs;
  inputs.clk_a = known<1>(0);
  inputs.clk_b = known<1>(0);
  inputs.rst_a = known<1>(0);
  inputs.rst_b = known<1>(0);
  inputs.data = known<1>(1);
  inputs.allow_a = known<1>(1);
  inputs.allow_b = known<1>(1);
  dut.drive(inputs);
  ready(dut);
  report(dut, "domains-low", step(dut));
  inputs.clk_b = known<1>(1);
  dut.drive(inputs);
  report(dut, "domains-only-b", step(dut));
  inputs.clk_b = known<1>(0);
  dut.drive(inputs);
  report(dut, "domains-observe-b", step(dut));
  inputs.clk_a = known<1>(1);
  inputs.clk_b = known<1>(1);
  dut.drive(inputs);
  report(dut, "domains-rise-a", step(dut));
  inputs.clk_a = known<1>(0);
  inputs.clk_b = known<1>(0);
  dut.drive(inputs);
  report(dut, "domains-observe-both", step(dut));
  inputs.clk_a = known<1>(1);
  inputs.clk_b = known<1>(1);
  inputs.rst_a = known<1>(1);
  inputs.data = known<1>(0);
  inputs.allow_a = known<1>(0);
  inputs.allow_b = known<1>(0);
  dut.drive(inputs);
  report(dut, "domains-reset-candidate-failure", step(dut));
  inputs.rst_b = known<1>(1);
  dut.drive(inputs);
  report(dut, "domains-terminal-reset", step(dut));
  dut.system().Reset();
  inputs.rst_a = known<1>(0);
  inputs.rst_b = known<1>(0);
  inputs.clk_a = known<1>(0);
  inputs.clk_b = known<1>(0);
  inputs.allow_a = known<1>(1);
  inputs.allow_b = known<1>(1);
  dut.drive(inputs);
  report(dut, "domains-host-reset", step(dut));
  // Direct Work/discard preserves Q AND sampled-clock history in both domains.
  gfsim::WorkExecutor executor(workers);
  pyc_root root("root", &executor);
  root.Build();
  root.Reset();
  root.Xfer();
  incomplete(root);
  root.clk_a = known<1>(0);
  root.clk_b = known<1>(1);
  root.rst_a = known<1>(0);
  root.rst_b = known<1>(0);
  root.data = known<1>(1);
  root.allow_a = known<1>(1);
  root.allow_b = known<1>(1);
  auto attempt = [&] {
    root.Work();
    gfsim::SimFailureInfo info;
    require(root.__pyc_validate_checks(info));
    root.Xfer();
    incomplete(root);
  };
  attempt();
  root.clk_b = known<1>(0);
  attempt();
  require(root.left.value() == gfsim::Bits<1>{0} &&
          root.right.value() == gfsim::Bits<1>{1});
  root.clk_a = known<1>(1);
  root.clk_b = known<1>(1);
  attempt();
  root.clk_a = known<1>(0);
  root.clk_b = known<1>(0);
  attempt();
  require(root.left.value() == gfsim::Bits<1>{1} &&
          root.right.value() == gfsim::Bits<1>{1});
  root.clk_a = known<1>(1);
  root.clk_b = known<1>(1);
  root.rst_a = known<1>(1);
  root.data = known<1>(0);
  root.allow_a = known<1>(0);
  root.allow_b = known<1>(0);
  root.Work();
  gfsim::SimFailureInfo error;
  require(!root.__pyc_validate_checks(error) &&
          error.code == "source_check_failed" && error.instance == "root.aaa");
  root.DiscardNext();
  incomplete(root);
  root.rst_a = known<1>(0);
  root.allow_a = known<1>(1);
  root.allow_b = known<1>(1);
  root.Work();
  require(root.left.value() == gfsim::Bits<1>{1} &&
          root.right.value() == gfsim::Bits<1>{1});
  require(root.__pyc_validate_checks(error));
  root.Xfer();
  incomplete(root);
  root.Work();
  require(root.left.value() == gfsim::Bits<1>{0} &&
          root.right.value() == gfsim::Bits<1>{0});
  std::cout << "DIRECT distinct-domains-reset-candidate-and-clock-rollback\n";
#elif T3_CASE == 6
  pyc_dut dut(workers);
  pyc_dut::Inputs inputs;
  inputs.req_a=inputs.req_b=known<1>(1);
  inputs.i=known<1>(0);inputs.j=known<1>(1);
  inputs.write_a=known<8>(17);inputs.write_b=known<8>(34);inputs.allow=known<1>(1);
  inputs.pyc_7079635f636c6b=inputs.pyc_7079635f727374=known<1>(0);
  dut.drive(inputs);ready(dut);
  report(dut,"address-low",step(dut));
  inputs.pyc_7079635f636c6b=known<1>(1);dut.drive(inputs);
  report(dut,"address-rise",step(dut));
  inputs.pyc_7079635f636c6b=known<1>(0);
  inputs.write_a=known<8>(51);inputs.write_b=known<8>(68);dut.drive(inputs);
  report(dut,"address-observe",step(dut));
  inputs.pyc_7079635f636c6b=known<1>(1);inputs.allow=known<1>(0);dut.drive(inputs);
  report(dut,"address-failure",step(dut));
  inputs.allow=known<1>(1);dut.drive(inputs);
  report(dut,"address-terminal",step(dut));
  dut.system().Reset();dut.drive(inputs);
  report(dut,"address-host-reset",step(dut));
  inputs.pyc_7079635f636c6b=known<1>(0);dut.drive(inputs);
  report(dut,"address-observe-reset",step(dut));

  // Exercise the same generated owner/check graph directly. Prepared known
  // discard and a failing source check cancel Table/scalar data and clocks.
  gfsim::WorkExecutor executor(workers);pyc_root root("root",&executor);
  auto drive=[&](unsigned clock,unsigned a,unsigned b,unsigned allow,unsigned i=0,unsigned j=1){
    root.req_a=root.req_b=known<1>(1);root.i=known<1>(i);root.j=known<1>(j);
    root.write_a=known<8>(a);root.write_b=known<8>(b);root.allow=known<1>(allow);
    root.pyc_7079635f636c6b=known<1>(clock);root.pyc_7079635f727374=known<1>(0);
  };
  drive(0,17,34,1);root.Build();incomplete(root);root.Reset();root.Xfer();
  auto equal=[&](std::uint64_t value){auto actual=root.result.packed();
    require(actual.isFullyKnown()&&actual.zMask()==gfsim::Bits<64>{0});
    require(actual.value()==gfsim::Bits<64>{value});};
  gfsim::SimFailureInfo error;
  root.Work();require(root.__pyc_validate_checks(error));equal(0x1122);root.Xfer();
  drive(1,17,34,1);root.Work();require(root.__pyc_validate_checks(error));
  root.DiscardNext();root.Xfer();incomplete(root);
  root.Work();equal(0x1122);require(root.__pyc_validate_checks(error));root.Xfer();
  drive(0,51,68,1);root.Work();equal(0x11ee22dd11223344ULL);
  require(root.__pyc_validate_checks(error));root.Xfer();
  drive(1,51,68,0);root.Work();
  require(!root.__pyc_validate_checks(error)&&error.code=="source_check_failed"&&error.instance=="root");
  auto failedId=std::string(error.checkIdJson),failedSource=std::string(error.sourceJson);
  root.DiscardNext();root.Xfer();incomplete(root);
  drive(1,51,68,1);root.Work();equal(0x11ee22dd11223344ULL);
  require(root.__pyc_validate_checks(error));root.Xfer();
  drive(0,51,68,1);root.Work();equal(0x33cc44bb33443344ULL);
  require(root.__pyc_validate_checks(error));root.Xfer();
  root.Reset();root.DiscardNext();root.Xfer();
  drive(1,85,102,1,1,0);root.Work();equal(0x33cc44bb33445566ULL);
  require(root.__pyc_validate_checks(error));root.Xfer();
  drive(0,85,102,1,1,0);root.Work();equal(0x669955aa55665566ULL);
  require(root.__pyc_validate_checks(error));root.DiscardNext();root.Xfer();
  require(!failedId.empty()&&!failedSource.empty());
  std::cout<<"DIRECT address-domains-owner-check-suffix-discard-clock-rollback\n";

#else
  // All current leaves stage genuine updates. Observations of committed state
  // come from subsequent Work on the same generated hierarchy, never a copy.
  gfsim::WorkExecutor executor(workers);
  pyc_root root("root", &executor);
  root.Build();
  incomplete(root);
  root.Reset();
  root.Xfer();
  incomplete(root);
  root.clk = known<1>(0);
  root.rst = known<1>(0);
  root.en = known<1>(1);
  root.ren = known<1>(1);
  root.write = known<1>(1);
  root.push = known<1>(1);
  root.take = known<1>(0);
  root.condition = known<1>(1);
  root.path = known<1>(1);
  root.data = known<8>(17);
  root.read_addr = known<2>(0);
  root.write_addr = known<2>(0);
  root.strobe = known<1>(1);
  auto work = [&](bool accepted, bool commit) {
    root.Work();
    gfsim::SimFailureInfo info;
    require(root.__pyc_validate_checks(info) == accepted);
    if (commit)
      root.Xfer();
    else
      root.DiscardNext();
    incomplete(root);
  };
  work(true, true);
  root.clk = known<1>(1);
  work(true, true);
  root.clk = known<1>(0);
  root.write = known<1>(0);
  root.push = known<1>(0);
  work(true, true);
  require(root.qdff.value() == gfsim::Bits<8>{17});
  require(root.qdffe.value() == gfsim::Bits<8>{17});
  require(root.qbyte.value() == gfsim::Bits<8>{17});
  require(root.available.value() == gfsim::Bits<1>{1});
  require(root.head.value() == gfsim::Bits<8>{17});
  require(root.delayed_available.value() == gfsim::Bits<1>{0});
  root.clk = known<1>(1);
  root.data = known<8>(34);
  root.write = known<1>(1);
  root.push = known<1>(1);
  root.take = known<1>(1);
  root.condition = known<1>(0);
  for (unsigned rejected = 0; rejected < 5; ++rejected)
    work(false, false);
  root.condition = known<1>(1);
  root.data = known<8>(17);
  root.write = known<1>(0);
  root.push = known<1>(0);
  root.take = known<1>(0);
  root.clk = known<1>(0);
  work(true, true);
  require(root.qdff.value() == gfsim::Bits<8>{17});
  require(root.qdffe.value() == gfsim::Bits<8>{17});
  require(root.qbyte.value() == gfsim::Bits<8>{17});
  require(root.head.value() == gfsim::Bits<8>{17});
  require(root.qsync.value() == gfsim::Bits<8>{0});
  require(root.qdp0.value() == gfsim::Bits<8>{0});
  require(root.qdp1.value() == gfsim::Bits<8>{0});
  require(root.delayed_available.value() == gfsim::Bits<1>{0});
  root.clk = known<1>(1);
  work(true, true);
  root.clk = known<1>(0);
  work(true, true);
  require(root.qsync.value() == gfsim::Bits<8>{17});
  require(root.qdp0.value() == gfsim::Bits<8>{17});
  require(root.qdp1.value() == gfsim::Bits<8>{17});
  require(root.delayed_available.value() == gfsim::Bits<1>{0});
  root.clk = known<1>(1);
  work(true, true);
  root.clk = known<1>(0);
  work(true, true);
  require(root.delayed_available.value() == gfsim::Bits<1>{1});
  require(root.delayed_head.value() == gfsim::Bits<8>{17});
  // Repeated rejected idle edges must not age synchronous read lifetimes.
  root.ren = known<1>(0);
  root.clk = known<1>(1);
  root.condition = known<1>(0);
  for (unsigned rejected = 0; rejected < 5; ++rejected)
    work(false, false);
  root.condition = known<1>(1);
  root.clk = known<1>(0);
  work(true, true);
  require(root.qsync.isFullyKnown() && root.qdp0.isFullyKnown() &&
          root.qdp1.isFullyKnown());
  for (unsigned idle = 0; idle < 2; ++idle) {
    root.clk = known<1>(1);
    work(true, true);
    root.clk = known<1>(0);
    work(true, true);
    require(root.qsync.isFullyKnown() == (idle == 0));
    require(root.qdp0.isFullyKnown() == (idle == 0));
    require(root.qdp1.isFullyKnown() == (idle == 0));
  }
  root.ren = known<1>(1);
  root.clk = known<1>(1);
  work(true, true);
  root.clk = known<1>(0);
  work(true, true);
  require(root.qsync.value() == gfsim::Bits<8>{17});
  require(root.qdp0.value() == gfsim::Bits<8>{17});
  require(root.qdp1.value() == gfsim::Bits<8>{17});
  // A failed ordinary physical-reset candidate must also be discarded.
  root.clk = known<1>(1);
  root.rst = known<1>(1);
  root.condition = known<1>(0);
  work(false, false);
  root.rst = known<1>(0);
  root.condition = known<1>(1);
  root.clk = known<1>(0);
  work(true, true);
  require(root.head.value() == gfsim::Bits<8>{17});
  require(root.qdff.value() == gfsim::Bits<8>{17});
  root.Reset();
  root.Xfer();
  root.clk = known<1>(0);
  work(true, true);
  require(root.qbyte.value() ==
          gfsim::Bits<8>{17}); // Actual host Reset retains RAM.
  std::cout << "DIRECT all-seven-leaves-barrier-reset-and-delayed-age\n";
#endif
}
