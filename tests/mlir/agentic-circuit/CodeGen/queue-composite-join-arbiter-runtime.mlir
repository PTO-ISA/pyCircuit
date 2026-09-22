// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/arbiter.py %t/raw.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/raw.mlir -o %t/frozen.mlir
// RUN: %acir_queue_cxxgen --output-root=%t/bundle %t/frozen.mlir | %FileCheck %s --check-prefix=EMIT
// RUN: cmake -S %t/bundle -B %t/build -G Ninja -DAC_GFSIM_INCLUDE_DIR=%source_root/simulator/gfsim/include
// RUN: cmake --build %t/build --parallel 2
// RUN: %cxx -std=c++20 -I%t/bundle/include -I%source_root/simulator/gfsim/include %t/harness.cpp %t/build/libac_generated_model.a %binary_root/gfsim/libgfsim.a -o %t/harness
// RUN: %t/harness observe | %FileCheck %s --check-prefix=OBSERVE
// RUN: %t/harness --expect-join | %FileCheck %s --check-prefix=JOIN

// Issue #223 -- the plain multi-input rule stays a JOIN by construction.
//
// The issue's literal arbiter is the rule `result0 if result0.valid else
// result1`. A multi-input rule is a JOIN -- its body is evaluated once per
// transaction and the rule only fires when EVERY input Queue is ready, so the
// lowering to `gfsim::QueueAtomicTransform` is faithful to the rule's
// transaction semantics. This test locks that behaviour:
//
//   * a lone request on either bank is never published, because the other
//     bank's output Queue stays empty;
//   * when both banks hold a result the arbiter publishes one and consumes
//     both, so the unselected result is dropped.
//
// Criterion 4 ("the arbiter publishes only the permitted width per cycle, and
// an unselected child output stays unchanged") is therefore delivered by the
// explicit selective primitive `result0.merge(result1, policy="priority")`,
// which lowers to `gfsim::QueueMerge`. `queue-composite-selective-merge-runtime`
// proves that behaviour; this file keeps the JOIN distinguishable from it so it
// cannot be redefined silently.

// EMIT: emitted model bundle v1

// OBSERVE: join-arbiter bank0_only results=0 bank0_accepted=1 bank1_accepted=0
// OBSERVE: join-arbiter bank1_only results=0 bank0_accepted=0 bank1_accepted=1
// OBSERVE: join-arbiter pair results=1 bank0_accepted=1 bank1_accepted=1
// OBSERVE: join-arbiter pair result[0] value=7 valid=1

// JOIN: join-arbiter satisfied

//--- lower.py
from pathlib import Path
import sys

from agentic_circuit._queue_frontend import lower_queue_source


source = Path(sys.argv[1]).read_text()
Path(sys.argv[2]).write_text(
    lower_queue_source(source, "composite", source_path="scen/entry.py")
)

//--- arbiter.py
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


# Issue #223 demux with per-output PRESENCE expressed through None.

@ac.rule
def route(request: Request) -> tuple[Request, Request]:
    thread0 = request if (request.valid & (request.tid == 0)) else None
    thread1 = request if (request.valid & (request.tid == 1)) else None
    return thread0, thread1


@ac.rule
def arbitrate(result0: Result, result1: Result) -> Result:
    return result0 if result0.valid else result1


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
    result = arbitrate(result0, result1)
    return result


@ac.system
def composite(request: Request) -> Result:
    return top(request)

//--- harness.cpp
// JOIN regression probe for issue #223's plain multi-input arbiter rule.
//
// Usage: harness observe | harness --expect-join
//
// `observe` prints the number of published results per stimulus and how many
// times each bank loaded. `--expect-join` asserts the JOIN contract: a rule
// with two input Queues fires only when both are ready and consumes both.

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
  size_t expected_results;
  std::vector<unsigned> expected_values;
};

struct ScenarioResult {
  bool wired = false;
  size_t accepted = 0;
  std::vector<unsigned> values;
  uint64_t bank0 = 0;
  uint64_t bank1 = 0;
};

ScenarioResult runScenario(const Scenario &scenario, size_t maxTicks) {
  ScenarioResult run;
  gfsim::SimSystem system{"join_arbiter"};
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
    while (std::optional<Result> value = model.try_take_result_0(system))
      run.values.push_back(static_cast<unsigned>(value->value.value()));
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

} // namespace

int main(int argc, char **argv) {
  const bool expectJoin =
      argc == 2 && std::strcmp(argv[1], "--expect-join") == 0;
  if (argc != 2 || (!expectJoin && std::strcmp(argv[1], "observe") != 0)) {
    std::printf("usage: %s observe|--expect-join\n", argv[0]);
    return 2;
  }

  const std::array<Scenario, 3> scenarios{{
      {"bank0_only", {makeRequest(42, /*tid=*/false)}, 0, {}},
      {"bank1_only", {makeRequest(9, /*tid=*/true)}, 0, {}},
      {"pair",
       {makeRequest(7, /*tid=*/false), makeRequest(9, /*tid=*/true)},
       1,
       {7}},
  }};

  for (const Scenario &scenario : scenarios) {
    const ScenarioResult observed = runScenario(scenario, 128);
    if (!observed.wired) {
      std::printf("join-arbiter %s wire_failure=1\n", scenario.label);
      return 1;
    }
    std::printf("join-arbiter %s results=%zu bank0_accepted=%llu "
                "bank1_accepted=%llu\n",
                scenario.label, observed.values.size(),
                static_cast<unsigned long long>(observed.bank0),
                static_cast<unsigned long long>(observed.bank1));
    for (size_t index = 0; index < observed.values.size(); ++index)
      std::printf("join-arbiter %s result[%zu] value=%u valid=1\n",
                  scenario.label, index, observed.values[index]);

    if (!expectJoin)
      continue;
    if (observed.values.size() != scenario.expected_results ||
        observed.values != scenario.expected_values) {
      std::printf("JOIN VIOLATION: %s expected %zu result(s), observed %zu\n",
                  scenario.label, scenario.expected_results,
                  observed.values.size());
      return 1;
    }
    // A lone request loads exactly its own bank and never the other.
    if (scenario.requests.size() == 1) {
      const bool tid0 = scenario.requests.front().tid.value() == 0;
      if (tid0 ? (observed.bank0 != 1 || observed.bank1 != 0)
               : (observed.bank0 != 0 || observed.bank1 != 1)) {
        std::printf("JOIN VIOLATION: %s loaded the wrong bank\n",
                    scenario.label);
        return 1;
      }
    } else if (observed.bank0 != 1 || observed.bank1 != 1) {
      std::printf("JOIN VIOLATION: pair did not load both banks once\n");
      return 1;
    }
  }
  if (expectJoin)
    std::printf("join-arbiter satisfied\n");
  return 0;
}
