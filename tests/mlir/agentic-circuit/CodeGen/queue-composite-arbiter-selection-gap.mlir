// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/arbiter.py %t/raw.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/raw.mlir -o %t/frozen.mlir
// RUN: %acir_queue_cxxgen --output-root=%t/bundle %t/frozen.mlir | %FileCheck %s --check-prefix=EMIT
// RUN: cmake -S %t/bundle -B %t/build -G Ninja -DAC_GFSIM_INCLUDE_DIR=%source_root/simulator/gfsim/include
// RUN: cmake --build %t/build --parallel 2
// RUN: %cxx -std=c++20 -I%t/bundle/include -I%source_root/simulator/gfsim/include %t/harness.cpp %t/build/libac_generated_model.a %binary_root/gfsim/libgfsim.a -o %t/harness
// RUN: %t/harness observe | %FileCheck %s --check-prefix=OBSERVE
// RUN: %not %t/harness --expect-criterion4 | %FileCheck %s --check-prefix=GAP

// Issue #223 acceptance criterion 4 -- CURRENTLY MISSING, pinned as a known gap.
//
// Criterion 4: "The arbiter publishes only the permitted width per cycle, and
// an unselected child output stays unchanged."
//
// The scenario is the issue's minimal repro with per-output presence expressed
// through `None`: a local demux rule feeds two bank children, and the local
// arbiter `result0 if result0.valid else result1` merges their outputs.
//
// The generated arbiter is a `gfsim::QueueAtomicTransform` with two inputs. Its
// runtime requires EVERY input to be ready (`allInputsReady`) and pops EVERY
// input on commit (`publishInputs`), so it does not select: it joins. Observed
// consequences:
//
//   * a lone request on either bank loads that bank but publishes NOTHING,
//     because the other bank's output Queue stays empty;
//   * when both banks hold a result the arbiter pops both and publishes one,
//     so the unselected child output is consumed and dropped instead of staying
//     unchanged.
//
// The `observe` run prints the current behaviour. The `--expect-criterion4` run
// asserts the criterion itself and must keep failing until the arbiter gains
// per-input selection/consumption. When that lands, replace the GAP run with a
// positive assertion.

// EMIT: emitted model bundle v1

// OBSERVE: arbiter bank0_only results=0 bank0_accepted=1 bank1_accepted=0
// OBSERVE: arbiter bank1_only results=0 bank0_accepted=0 bank1_accepted=1
// OBSERVE: arbiter pair results=1 bank0_accepted=1 bank1_accepted=1

// GAP: criterion4 VIOLATION:

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
// Criterion 4 probe for issue #223's local arbiter.
//
// Usage: harness observe | harness --expect-criterion4
//
// `observe` prints the number of published results per stimulus and how many
// times each bank loaded. `--expect-criterion4` additionally requires the
// criterion-4 property (see the test header) and exits non-zero while the arbiter
// cannot select an input.

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
  gfsim::SimSystem system{"arbiter"};
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
  const bool expectCriterion4 =
      argc == 2 && std::strcmp(argv[1], "--expect-criterion4") == 0;
  if (argc != 2 ||
      (!expectCriterion4 && std::strcmp(argv[1], "observe") != 0)) {
    std::printf("usage: %s observe|--expect-criterion4\n", argv[0]);
    return 2;
  }

  const std::array<Scenario, 3> scenarios{{
      {"bank0_only", {makeRequest(42, /*tid=*/false)}, 1, {42}},
      {"bank1_only", {makeRequest(9, /*tid=*/true)}, 1, {9}},
      {"pair",
       {makeRequest(7, /*tid=*/false), makeRequest(9, /*tid=*/true)},
       2,
       {7, 9}},
  }};

  for (const Scenario &scenario : scenarios) {
    const ScenarioResult observed = runScenario(scenario, 128);
    if (!observed.wired) {
      std::printf("arbiter %s wire_failure=1\n", scenario.label);
      return 1;
    }
    std::printf(
        "arbiter %s results=%zu bank0_accepted=%llu bank1_accepted=%llu\n",
        scenario.label, observed.values.size(),
        static_cast<unsigned long long>(observed.bank0),
        static_cast<unsigned long long>(observed.bank1));

    if (!expectCriterion4)
      continue;
    // Criterion 4: every bank that loaded must have its result published, and
    // the unselected child output must survive to be served later.
    if (observed.values.size() != scenario.expected_results) {
      std::printf("criterion4 VIOLATION: %s expected %zu result(s), observed "
                  "%zu\n",
                  scenario.label, scenario.expected_results,
                  observed.values.size());
      return 1;
    }
    if (observed.values != scenario.expected_values) {
      std::printf("criterion4 VIOLATION: %s published the wrong payloads\n",
                  scenario.label);
      return 1;
    }
  }
  if (expectCriterion4)
    std::printf("criterion4 satisfied\n");
  return 0;
}
