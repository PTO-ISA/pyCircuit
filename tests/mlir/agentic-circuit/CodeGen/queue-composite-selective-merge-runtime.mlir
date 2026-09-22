// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/selective_merge.py %t/raw.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/raw.mlir -o %t/frozen.mlir
// RUN: %acir_queue_cxxgen --output-root=%t/bundle %t/frozen.mlir | %FileCheck %s --check-prefix=EMIT
// RUN: cmake -S %t/bundle -B %t/build -G Ninja -DAC_GFSIM_INCLUDE_DIR=%source_root/simulator/gfsim/include
// RUN: cmake --build %t/build --parallel 2
// RUN: %cxx -std=c++20 -I%t/bundle/include -I%source_root/simulator/gfsim/include %t/harness.cpp %t/build/libac_generated_model.a %binary_root/gfsim/libgfsim.a -o %t/harness
// RUN: %t/harness observe | %FileCheck %s --check-prefix=OBSERVE
// RUN: %t/harness --expect-criterion4 | %FileCheck %s --check-prefix=CRITERION

// Issue #223 acceptance criterion 4 -- runtime proof of selective arbitration.
//
// Criterion 4: "The arbiter publishes only the permitted width per cycle, and
// an unselected child output stays unchanged."
//
// The scenario mixes a local demux rule with two child module instances and a
// selective merge: a lone tid=0 or tid=1 request reaches exactly one bank, and
// `.merge(...)` is the primitive that selects which bank feeds the result. The
// runtime assertions are:
//
//   (a) a request reaching ONLY bank 0 is published (it used to be swallowed
//       because the arbiter required every input);
//   (b) likewise for bank 1 alone;
//   (c) when both banks hold a result, at most ONE is published per cycle and
//       the unselected result is RETAINED, so it is served on a later cycle
//       instead of being consumed and dropped.
//
// The generated arbiter is `gfsim::QueueMerge`, lowered from the frontend's
// `.merge(...)` (`ac.merge`) primitive; its `doWork` picks the first ready
// input under the configured policy and pops only that input. The plain
// multi-input `arbitrate(result0, result1)` rule is unchanged and still lowers
// to the JOINing `QueueAtomicTransform`; `queue-composite-join-arbiter-runtime`
// locks that behaviour so the two primitives stay distinguishable.

// EMIT: emitted model bundle v1

// OBSERVE: selective-merge bank0_only results=1 max_per_tick=1 bank0_accepted=1 bank1_accepted=0
// OBSERVE: selective-merge bank1_only results=1 max_per_tick=1 bank0_accepted=0 bank1_accepted=1
// OBSERVE: selective-merge pair results=2 max_per_tick=1 bank0_accepted=1 bank1_accepted=1

// CRITERION: criterion4 satisfied

//--- lower.py
from pathlib import Path
import sys

from agentic_circuit._queue_frontend import lower_queue_source


source = Path(sys.argv[1]).read_text()
Path(sys.argv[2]).write_text(
    lower_queue_source(source, "composite", source_path="scen/entry.py")
)

//--- selective_merge.py
import agentic_circuit as ac


@ac.struct
class Request:
    value: ac.bits[8]
    tid: ac.bits[1]
    valid: ac.bits[1]


@ac.struct
class Result:
    value: ac.bits[8]
    valid: ac.bits[1]


# Issue #223 demux with per-output presence expressed through None.

@ac.rule
def route(request: Request) -> tuple[Request, Request]:
    thread0 = request if (request.valid & (request.tid == 0)) else None
    thread1 = request if (request.valid & (request.tid == 1)) else None
    return thread0, thread1


@ac.module_decl(source="scen/entry.py")
def state_bank(packet: Request) -> Result:
    ...


state_bank_decl = state_bank


@ac.module(declaration=state_bank_decl)
def state_bank(packet: Request) -> Result:
    return Result(value=packet.value, valid=packet.valid)


@ac.module_decl(source="scen/entry.py")
def top(request: Request) -> Result:
    ...


top_decl = top


@ac.module(declaration=top_decl)
def top(request: Request) -> Result:
    thread0, thread1 = route(request)
    result0 = state_bank(thread0)
    result1 = state_bank(thread1)
    merged = result0.merge(result1, policy="priority", depth=1, latency=1)
    return merged


@ac.system
def composite(request: Request) -> Result:
    return top(request)

//--- harness.cpp
// Criterion 4 probe for issue #223's selective local arbiter.
//
// Usage: harness observe | harness --expect-criterion4
//
// `observe` prints, per stimulus, how many results came out, the largest number
// published in any single cycle, and how often each bank loaded. The default
// `--expect-criterion4` run additionally requires the criterion-4 property: a
// bank that loaded must publish, and a result that is not selected this cycle
// must survive to be served later.

#include "generated/dut.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using ac_generated::Composite;
using ac_generated::Request;
using ac_generated::Result;

Request makeRequest(uint8_t value, bool tid) {
  Request request;
  request.value = gfsim::UInt<8>{value};
  request.tid = gfsim::UInt<1>{tid};
  request.valid = gfsim::UInt<1>{true};
  return request;
}

struct Scenario {
  const char *label;
  std::vector<Request> requests;
  std::vector<std::pair<unsigned, unsigned>> expected;
  uint64_t expected_bank0;
  uint64_t expected_bank1;
};

struct ScenarioResult {
  bool wired = false;
  size_t accepted = 0;
  size_t max_per_tick = 0;
  std::vector<std::pair<unsigned, unsigned>> values;
  uint64_t bank0 = 0;
  uint64_t bank1 = 0;
};

ScenarioResult runScenario(const Scenario &scenario, size_t maxTicks) {
  ScenarioResult run;
  gfsim::SimSystem system{"selective_merge"};
  Composite model;
  std::array<gfsim::TimeDomainRuntime, 1> timeDomains{{{"cycle", 1, 0, 1}}};
  if (!system.root().attachChild(model) || !system.setTimeDomains(timeDomains) ||
      !model.configure_activation_scheduler(system))
    return run;
  run.wired = true;

  size_t next = 0;
  for (size_t tick = 0; tick < maxTicks; ++tick) {
    if (next < scenario.requests.size() &&
        model.offer_request(system, scenario.requests[next])) {
      ++next;
      ++run.accepted;
    }
    const bool advanced = system.step();
    size_t perTick = 0;
    while (std::optional<Result> value = model.try_take_result_0(system)) {
      run.values.emplace_back(static_cast<unsigned>(value->value.value()),
                              static_cast<unsigned>(value->valid.value()));
      ++perTick;
    }
    if (perTick > run.max_per_tick)
      run.max_per_tick = perTick;
    if (!advanced)
      break;
  }

  for (const gfsim::StatSnapshot &snapshot : system.statistics()) {
    if (snapshot.name != "accepted_transactions")
      continue;
    const std::string &path = snapshot.objectPath;
    if (path.ends_with("/child_0_queue"))
      run.bank0 = snapshot.value;
    if (path.ends_with("/child_1_queue"))
      run.bank1 = snapshot.value;
  }
  return run;
}

std::string describe(const std::vector<std::pair<unsigned, unsigned>> &values) {
  std::string text;
  for (const auto &[value, valid] : values) {
    if (!text.empty())
      text += ",";
    text += std::to_string(value) + "/" + std::to_string(valid);
  }
  return text;
}

} // namespace

int main(int argc, char **argv) {
  const bool expectCriterion4 =
      argc == 2 && std::strcmp(argv[1], "--expect-criterion4") == 0;
  if (argc != 2 ||
      (!expectCriterion4 && std::strcmp(argv[1], "observe") != 0)) {
    std::printf("usage: %s observe|--expect-criterion4\n", argv[0]);
    return 2;
  }

  const std::array<Scenario, 3> scenarios{{
      {"bank0_only", {makeRequest(42, /*tid=*/false)}, {{42, 1}}, 1, 0},
      {"bank1_only", {makeRequest(9, /*tid=*/true)}, {{9, 1}}, 0, 1},
      {"pair",
       {makeRequest(7, /*tid=*/false), makeRequest(9, /*tid=*/true)},
       {{7, 1}, {9, 1}},
       1,
       1},
  }};

  for (const Scenario &scenario : scenarios) {
    const ScenarioResult observed = runScenario(scenario, 128);
    if (!observed.wired) {
      std::printf("selective-merge %s wire_failure=1\n", scenario.label);
      return 1;
    }
    std::printf("selective-merge %s results=%zu max_per_tick=%zu "
                "bank0_accepted=%llu bank1_accepted=%llu\n",
                scenario.label, observed.values.size(),
                observed.max_per_tick,
                static_cast<unsigned long long>(observed.bank0),
                static_cast<unsigned long long>(observed.bank1));
    if (!expectCriterion4)
      continue;
    // Criterion 4: only the selected input is consumed per cycle (width one),
    // every bank that loaded publishes, and the unselected result is retained
    // rather than dropped.
    if (observed.max_per_tick > 1) {
      std::printf("criterion4 VIOLATION: %s published %zu results in one cycle\n",
                  scenario.label, observed.max_per_tick);
      return 1;
    }
    if (observed.values != scenario.expected) {
      std::printf("criterion4 VIOLATION: %s published [%s], expected %zu "
                  "result(s) [",
                  scenario.label, describe(observed.values).c_str(),
                  scenario.expected.size());
      return 1;
    }
    if (observed.bank0 != scenario.expected_bank0 ||
        observed.bank1 != scenario.expected_bank1) {
      std::printf("criterion4 VIOLATION: %s loaded the wrong banks\n",
                  scenario.label);
      return 1;
    }
  }
  if (expectCriterion4)
    std::printf("criterion4 satisfied\n");
  return 0;
}
