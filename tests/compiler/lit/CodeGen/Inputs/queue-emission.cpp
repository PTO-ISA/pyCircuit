#ifdef QUEUE_ATOMIC
#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <array>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <source_location>
#include <string_view>
#include <type_traits>
void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "atomic oracle at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
struct Token {
  unsigned value, known = 8191, z = 0;
  std::int64_t birth = 0;
};
auto wire(Token t) {
  return gfsim::wire<gfsim::Bits<13>>::fromPacked(
      gfsim::FourState<13>::fromMasks(gfsim::Bits<13>{t.value},
                                      gfsim::Bits<13>{t.known},
                                      gfsim::Bits<13>{t.z}));
}
template <class Wire> void same(const Wire &w, Token t) {
  const auto p = w.packed();
  require(p.value() == gfsim::Bits<13>{t.value});
  require(p.knownMask() == gfsim::Bits<13>{t.known});
  require(p.zMask() == gfsim::Bits<13>{t.z});
}
pyc_dut::Inputs initial() {
  pyc_dut::Inputs p;
  p.clk = known<1>(0);
  p.rst = known<1>(0);
  p.valid0 = p.valid1 = p.take0 = p.take1 = p.loop0 = p.loop1 = p.reg_enable =
      known<1>(0);
  p.data0 = p.data1 = p.reg_data = known<13>(0);
  return p;
}
void drive(pyc_root &r, const pyc_dut::Inputs &p) {
  r.clk = p.clk;
  r.rst = p.rst;
  r.valid0 = p.valid0;
  r.valid1 = p.valid1;
  r.take0 = p.take0;
  r.take1 = p.take1;
  r.data0 = p.data0;
  r.data1 = p.data1;
  r.loop0 = p.loop0;
  r.loop1 = p.loop1;
  r.reg_enable = p.reg_enable;
  r.reg_data = p.reg_data;
}
struct Oracle {
  std::array<std::deque<Token>, 2> q;
  bool clock = false;
  std::int64_t edge = -1;
#ifdef QUEUE_DELAYED_ATOMIC
  static constexpr unsigned latency = 2;
#else
  static constexpr unsigned latency = 1;
#endif
  bool available(unsigned i) const {
    return !q[i].empty() && q[i].front().birth + latency - 1 <= edge;
  }
  Token reg{0};
  template <class Out> void check(const Out &o, const pyc_dut::Inputs &p) {
    require(o.ready0.value() == gfsim::Bits<1>{q[0].size() < 3});
    require(o.ready0.isFullyKnown());
    require(o.ready1.value() ==
            gfsim::Bits<1>{
                q[1].size() < 5 ||
                (available(1) && p.take1.value() == gfsim::Bits<1>{1})});
    require(o.ready1.isFullyKnown());
    require(o.available0.isFullyKnown() && o.available1.isFullyKnown());
    require(o.available0.value() == gfsim::Bits<1>{available(0)});
    require(o.available1.value() == gfsim::Bits<1>{available(1)});
    same(o.head0, available(0) ? q[0].front() : Token{0});
    same(o.head1, available(1) ? q[1].front() : Token{0});
    same(o.parent_q, reg);
  }
  void commit(const pyc_dut::Inputs &p) {
    const bool next = p.clk.value() == gfsim::Bits<1>{1};
    if (next && !clock) {
      std::array<bool, 2> old_available{available(0), available(1)};
      ++edge;
      for (unsigned i = 0; i < 2; ++i) {
        const auto &valid = i ? p.valid1 : p.valid0;
        const auto &take = i ? p.take1 : p.take0;
        const auto &loop = i ? p.loop1 : p.loop0;
        const auto &data = i ? p.data1 : p.data0;
        bool pop = old_available[i] && take.value() == gfsim::Bits<1>{1};
        bool push = valid.value() == gfsim::Bits<1>{1} &&
                    (q[i].size() < (i ? 5 : 3) || (i && pop));
        auto packed = data.packed();
        Token token{unsigned(packed.value().word(0)),
                    unsigned(packed.knownMask().word(0)),
                    unsigned(packed.zMask().word(0))};
        if (loop.value() == gfsim::Bits<1>{1})
          token = old_available[i] ? q[i].front() : Token{0};
        token.birth = edge;
        if (pop)
          q[i].pop_front();
        if (push)
          q[i].push_back(token);
      }
      if (p.reg_enable.value() == gfsim::Bits<1>{1})
        reg = {unsigned(p.reg_data.value().word(0))};
    }
    clock = next;
  }
};
void direct(unsigned workers) {
  gfsim::WorkExecutor pool(workers);
  pyc_root root("atomic", &pool);
  root.Build();
  auto p = initial();
  drive(root, p);
  root.Reset();
  root.Xfer();
  Oracle oracle;
  auto step = [&] {
    drive(root, p);
    root.Work();
    oracle.check(root, p);
    root.Xfer();
    oracle.commit(p);
  };
  step();
  p.valid0 = p.valid1 = known<1>(1);
  p.data0 = wire({17});
  p.data1 = wire({39});
  p.clk = known<1>(1);
  step();
  p.clk = known<1>(0);
  step();
  // A parent failure occurs after both wrappers have proposed their new token.
  p.clk = known<1>(1);
  p.data0 = wire({0x1357, 0x11bb, 0x0244});
  p.data1 = wire({0x06a5, 0x1555, 0x0888});
  p.reg_enable = gfsim::wire<gfsim::Bits<1>>::unknown();
  drive(root, p);
  bool failed = false;
  try {
    root.Work();
  } catch (const gfsim::FourStateViolation &) {
    failed = true;
  }
  require(failed);
  root.DiscardNext();
  root.Xfer();
  // Preserve the high edge and replace both failed payloads before direct
  // retry.
  p.data0 = wire({0x1a2b, 0x1555, 0x0888});
  p.data1 = wire({0x0345, 0x0f0f, 0x1020});
  p.reg_enable = known<1>(1);
  p.reg_data = known<13>(71);
  step();
  p.valid0 = p.valid1 = known<1>(0);
  p.reg_enable = known<1>(0);
  p.clk = known<1>(0);
  step();
  // Both feedback inputs select their own independently snapshotted old head.
  p.valid0 = p.valid1 = p.take0 = p.take1 = p.loop0 = p.loop1 = known<1>(1);
  p.clk = known<1>(1);
  step();
  p.clk = known<1>(0);
  step();
  p.valid0 = p.valid1 = p.loop0 = p.loop1 = known<1>(0);
  for (unsigned i = 0; i < 3; ++i) {
    p.clk = known<1>(1);
    step();
    p.clk = known<1>(0);
    step();
  }
  require(oracle.q[0].empty() && oracle.q[1].empty());
  std::cout
      << "DIRECT workers=" << workers
      << " late-parent rollback, same-edge retry, feedback and drain passed\n";
}
void executor(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor exec(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":32,"schema":"pycircuit-model-config","version":"1"})";
  require(
      exec.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                         config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto p = initial();
  dut.drive(p);
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  PycircuitModelStepResultV1 result{sizeof(result)};
  require(exec.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  p.clk = p.valid0 = p.valid1 = known<1>(1);
  p.data0 = wire({12});
  p.data1 = wire({23});
  dut.drive(p);
  require(exec.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  p.clk = known<1>(0);
  dut.drive(p);
  require(exec.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  const auto epoch = exec.cycles();
  p.clk = known<1>(1);
  p.reg_enable = gfsim::wire<gfsim::Bits<1>>::unknown();
  dut.drive(p);
  require(exec.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
  require(result.state == PYCIRCUIT_MODEL_STEP_V1_FAILED &&
          result.epoch_time == epoch);
  require(exec.cycles() == epoch && dut.system().cycle() == epoch);
  bool unavailable = false;
  try {
    (void)dut.sample();
  } catch (const std::logic_error &) {
    unavailable = true;
  }
  require(unavailable);
  require(exec.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
  p = initial();
  dut.drive(p);
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(exec.cycles() == 0 && dut.system().cycle() == 0);
  require(exec.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  Oracle{}.check(dut.sample(), p);
  std::cout << "EXECUTOR workers=" << workers
            << " failed epoch/sample and Reset recovery passed\n";
}
template <class Wire> void coldWire(const Wire &wire) {
  const auto p = wire.packed();
  require(!wire.isFullyKnown());
  require(p.knownMask() ==
          std::remove_cvref_t<decltype(p.knownMask())>{}); // every bit is X
  require(p.zMask() == std::remove_cvref_t<decltype(p.zMask())>{});
}
void coldOutputs(const pyc_root &r) {
  coldWire(r.ready0);
  coldWire(r.available0);
  coldWire(r.head0);
  coldWire(r.ready1);
  coldWire(r.available1);
  coldWire(r.head1);
}
void lifecycle(unsigned workers) {
  gfsim::WorkExecutor pool(workers);
  pyc_root r("lifecycle", &pool);
  r.Build();
  auto p = initial();
  drive(r, p);
  coldOutputs(r);
  r.Work();
  coldOutputs(r);
  r.Xfer();
  coldOutputs(r);
  // Cold no-op can commit clock history without manufacturing initialized Q.
  p.clk = known<1>(1);
  drive(r, p);
  r.Work();
  coldOutputs(r);
  r.Xfer();
  coldOutputs(r);
  p.clk = known<1>(0);
  drive(r, p);
  r.Work();
  r.Xfer();
  p.clk = p.valid0 = p.valid1 = known<1>(1);
  drive(r, p);
  bool failed = false;
  try {
    r.Work();
  } catch (const gfsim::FourStateViolation &) {
    failed = true;
  }
  require(failed);
  r.DiscardNext();
  r.Xfer();
  coldOutputs(r);
  p.valid0 = p.valid1 = known<1>(0);
  drive(r, p);
  r.Work();
  coldOutputs(r);
  r.Xfer();
  // Host Reset is staged; discard preserves cold Current and committed high.
  r.Reset();
  coldOutputs(r);
  r.DiscardNext();
  r.Xfer();
  coldOutputs(r);
  p.rst = gfsim::wire<gfsim::Bits<1>>::unknown();
  drive(r, p);
  r.Work();
  coldOutputs(r);
  r.Xfer();
  p = initial();
  drive(r, p);
  r.Reset();
  coldOutputs(r);
  r.Xfer();
  coldOutputs(r);
  r.Work();
  Oracle{}.check(r, p);
  r.Xfer();
  Token left{0x1357, 0x1555, 0x0888}, right{0x07a5, 0x0f0f, 0x1020};
  p.clk = p.valid0 = p.valid1 = known<1>(1);
  p.data0 = wire(left);
  p.data1 = wire(right);
  drive(r, p);
  r.Work();
  Oracle{}.check(r, p);
  r.Xfer();
  Oracle{}.check(r, p);
  p.valid0 = p.valid1 = known<1>(0);
  drive(r, p);
  r.Work();
  same(r.head0, left);
  same(r.head1, right);
  r.Xfer();
  // Staged/discarded host Reset also retains actual occupied Current.
  r.Reset();
  same(r.head0, left);
  same(r.head1, right);
  r.DiscardNext();
  r.Xfer();
  p.rst = p.valid0 = p.valid1 = p.take0 = p.take1 = p.reg_enable =
      gfsim::wire<gfsim::Bits<1>>::unknown();
  drive(r, p);
  r.Work();
  same(r.head0, left);
  same(r.head1, right);
  r.Xfer();
  // Held clock ignores unknown reset/handshakes; a rising reset masks them.
  p.clk = known<1>(0);
  drive(r, p);
  r.Work();
  same(r.head0, left);
  same(r.head1, right);
  r.Xfer();
  p.clk = p.rst = known<1>(1);
  drive(r, p);
  r.Work();
  same(r.head0, left);
  same(r.head1, right);
  r.Xfer();
  same(r.head0, left);
  same(r.head1, right);
  p.rst = gfsim::wire<gfsim::Bits<1>>::unknown();
  drive(r, p);
  r.Work();
  Oracle{}.check(r, p);
  r.Xfer();
  p = initial();
  drive(r, p);
  r.Work();
  r.Xfer();
  p.clk = p.valid0 = p.valid1 = known<1>(1);
  p.data0 = wire(left);
  p.data1 = wire(right);
  drive(r, p);
  r.Work();
  r.Xfer();
  p.valid0 = p.valid1 = known<1>(0);
  drive(r, p);
  r.Work();
  same(r.head0, left);
  same(r.head1, right);
  r.Xfer();
  r.Reset();
  same(r.head0, left);
  same(r.head1, right);
  r.Xfer();
  same(r.head0, left);
  same(r.head1, right);
  p = initial();
  drive(r, p);
  r.Work();
  Oracle{}.check(r, p);
  r.Xfer();
  std::cout << "LIFECYCLE workers=" << workers
            << " cold/no-op/failure, staged/discarded host Reset, snapshot "
               "hold, held unknown reset and rising reset priority passed\n";
}
int main() {
  for (unsigned workers : {1, 2}) {
    direct(workers);
    executor(workers);
#ifndef QUEUE_DELAYED_ATOMIC
    lifecycle(workers);
#endif
  }
}

#elif defined(QUEUE_OWNERS)
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "owner oracle at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
int main() {
  for (unsigned workers : {1, 2}) {
    gfsim::WorkExecutor pool(workers);
    pyc_root root("owner", &pool);
    root.Build();
    root.clk = root.rst = root.valid = root.take = known<1>(0);
    root.data = known<13>(47);
    root.Reset();
    root.Xfer();
    root.Work();
    require(root.head.isFullyKnown() &&
            root.head.value() == gfsim::Bits<13>{0});
    root.Xfer();
    root.clk = root.valid = known<1>(1);
    root.Work();
    root.Xfer();
    root.clk = known<1>(0);
    root.valid = known<1>(0);
    root.Work();
#ifdef QUEUE_DEAD
    require(root.ready.value() == gfsim::Bits<1>{1} &&
            root.available.value() == gfsim::Bits<1>{0} &&
            root.head.value() == gfsim::Bits<13>{0} &&
            root.echo.value() == gfsim::Bits<13>{0});
    root.Xfer();
#ifdef QUEUE_DELAYED_DEAD
    root.valid = gfsim::wire<gfsim::Bits<1>>::unknown();
    root.take = gfsim::wire<gfsim::Bits<1>>::unknown();
    for (unsigned age = 1; age <= 2; ++age) {
      root.clk = known<1>(1);
      root.Work();
      root.Xfer();
      root.clk = known<1>(0);
      root.Work();
      root.Xfer();
    }
    root.valid = known<1>(0);
#else
    root.valid = gfsim::wire<gfsim::Bits<1>>::unknown();
#endif
    root.clk = known<1>(1);
    bool failed = false;
    try {
      root.Work();
    } catch (const gfsim::FourStateViolation &) {
      failed = true;
    }
    require(failed);
    root.DiscardNext();
    root.Xfer();
    std::cout << "DEAD workers=" << workers
              << " unobserved queue failure preserved\n";
#else
    require(root.ready.isFullyKnown() && root.available.isFullyKnown() &&
            root.echo.isFullyKnown());
    require(root.available.value() == gfsim::Bits<1>{1} &&
            root.head.value() == gfsim::Bits<13>{47} &&
            root.echo.value() == gfsim::Bits<13>{47});
    root.Xfer();
    root.clk = root.take = known<1>(1);
    root.Work();
    root.Xfer();
    root.clk = known<1>(0);
    root.Work();
    require(root.available.value() == gfsim::Bits<1>{0} &&
            root.head.value() == gfsim::Bits<13>{0});
    std::cout << "COLLISION workers=" << workers
              << " fifo plus fifo_state passed\n";
#endif
  }
}

#else
#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string_view>
#include <type_traits>
void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "queue oracle at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto bits(std::string_view text) {
  require(text.size() == W);
  gfsim::Bits<W> result;
  for (unsigned bit = 0; bit < W; ++bit)
    if (text[W - 1 - bit] == '1')
      result.setWord(bit / 64,
                     result.word(bit / 64) | (std::uint64_t{1} << (bit % 64)));
  return result;
}
template <unsigned W>
auto input(std::string_view value, std::string_view known, std::string_view z) {
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(
      bits<W>(value), bits<W>(known), bits<W>(z)));
}
template <unsigned W, class Wire>
void check(const Wire &actual, std::string_view value, std::string_view known,
           std::string_view z) {
  const auto packed = actual.packed();
  require(packed.value() == bits<W>(value));
  require(packed.knownMask() == bits<W>(known));
  require(packed.zMask() == bits<W>(z));
}
#include "queue_vectors.hpp"
struct Context {
  pyc_dut &dut;
  unsigned sampled = 0;
  static void initialize(void *p) { require(drive(p, 0)); }
  static bool drive(void *p, std::uint64_t epoch) {
    auto &c = *static_cast<Context *>(p);
    if (epoch == row_count)
      return false;
    pyc_dut::Inputs ports;
    drivePorts(ports, epoch);
    c.dut.drive(ports);
    return true;
  }
  static void sample(void *p, std::uint64_t epoch) {
    auto &c = *static_cast<Context *>(p);
    require(epoch == c.sampled + 1);
    checkPorts(c.dut.sample(), c.sampled);
    std::cout << "WORK " << c.sampled << ' ' << golden_trace[c.sampled] << '\n';
    ++c.sampled;
  }
};
int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());
  Context context{dut};
  const gfsim::RunnerCallbacks callbacks{&context, &Context::initialize,
                                         &Context::drive, &Context::sample};
  const int status =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0 && context.sampled == row_count);
  return status;
}

#endif
