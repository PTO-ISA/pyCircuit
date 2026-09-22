// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/issue223_presence.py %t/presence.raw.mlir
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/issue223_minimal.py %t/masking.raw.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/presence.raw.mlir -o %t/presence.frozen.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/masking.raw.mlir -o %t/masking.frozen.mlir
// RUN: %acir_queue_cxxgen --output-root=%t/presence-bundle %t/presence.frozen.mlir | %FileCheck %s --check-prefix=EMIT
// RUN: %acir_queue_cxxgen --output-root=%t/masking-bundle %t/masking.frozen.mlir | %FileCheck %s --check-prefix=EMIT
// RUN: cmake -S %t/presence-bundle -B %t/presence-build -G Ninja -DAC_GFSIM_INCLUDE_DIR=%source_root/simulator/gfsim/include
// RUN: cmake -S %t/masking-bundle -B %t/masking-build -G Ninja -DAC_GFSIM_INCLUDE_DIR=%source_root/simulator/gfsim/include
// RUN: cmake --build %t/presence-build --parallel 2
// RUN: cmake --build %t/masking-build --parallel 2
// RUN: %cxx -std=c++20 -I%t/presence-bundle/include -I%source_root/simulator/gfsim/include %t/harness.cpp %t/presence-build/libac_generated_model.a %binary_root/gfsim/libgfsim.a -o %t/harness_presence
// RUN: %cxx -std=c++20 -I%t/masking-bundle/include -I%source_root/simulator/gfsim/include %t/harness.cpp %t/masking-build/libac_generated_model.a %binary_root/gfsim/libgfsim.a -o %t/harness_masking
// RUN: %t/harness_presence presence | %FileCheck %s --check-prefix=PRESENCE
// RUN: %not %t/harness_masking masking | %FileCheck %s --check-prefix=MASKING
// RUN: %not %t/harness_presence presence --invert-isolation | %FileCheck %s --check-prefix=INVERTED

// This is the runtime half of issue #223 acceptance criterion 3. Two composite
// systems express the same demux intent; only the second expresses per-output
// presence. Both compile to a model bundle, and one model-agnostic harness runs
// against both. The harness asserts the issue's isolation property without
// knowing which program produced the model:
//
//   A) a tid=0-only request must publish exactly its own bank's result and must
//      never load the tid=1 bank, because the demux must not fabricate a tid=1
//      token;
//   B) one tid=0 plus one tid=1 request must produce both payloads, so a
//      passing A is not merely asserting silence.
//
// Both models use the selective `result0.merge(result1, policy="priority")`
// arbiter, so the tid=0-only result is published (criterion 4) while the
// non-target bank stays untouched. Before selective arbitration this scenario
// asserted `results=0` because the JOINing arbiter had to consume both banks.
//
// Pointed at the presence model the harness passes; pointed at the masking model
// it fails because route publishes both outputs unconditionally, so the tid=1
// bank receives a spurious `valid=0` token. That real failure is the negative
// control. The third run inverts the isolation expectation on purpose and must
// also fail, proving a deliberately broken expectation is detected.

// EMIT: emitted model bundle v1

// PRESENCE: model presence invert_isolation=0
// PRESENCE: scenario tid0_only {{.*}}accepted=1 results=1 {{.*}}bank0_accepted=1 bank1_accepted=0
// PRESENCE: scenario tid0_only result[0] value=42 valid=1
// PRESENCE: scenario pair {{.*}}accepted=2 results=2 {{.*}}bank0_accepted=1 bank1_accepted=1
// PRESENCE: scenario pair result[0] value=7 valid=1
// PRESENCE: scenario pair result[1] value=9 valid=1
// PRESENCE: PASS presence

// MASKING: model masking invert_isolation=0
// MASKING: scenario tid0_only {{.*}}accepted=1 results=2 {{.*}}bank1_accepted=1
// MASKING: FAIL: tid0_only loaded the tid=1 bank

// INVERTED: model presence invert_isolation=1
// INVERTED: FAIL: deliberately inverted expectation

//--- lower.py
from pathlib import Path
import sys

from agentic_circuit._queue_frontend import lower_queue_source


source = Path(sys.argv[1]).read_text()
Path(sys.argv[2]).write_text(
    lower_queue_source(source, "composite", source_path="scen/entry.py")
)

//--- issue223_presence.py
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


@ac.module_decl(source="tests/python/agentic-circuit/python_frontend/test_queue_frontend.py")
def state_bank(packet: Request) -> Result:
    ...


state_bank_decl = state_bank


@ac.module(declaration=state_bank_decl)
def state_bank(packet: Request) -> Result:
    return Result(value=packet.value, valid=packet.valid)


@ac.module_decl(source="tests/python/agentic-circuit/python_frontend/test_queue_frontend.py")
def top(request: Request) -> Result:
    ...


top_decl = top


@ac.module(declaration=top_decl)
def top(request: Request) -> Result:
    thread0, thread1 = route(request)
    result0 = state_bank(thread0)
    result1 = state_bank(thread1)
    result = result0.merge(result1, policy="priority", depth=1, latency=1)
    return result


@ac.system
def composite(request: Request) -> Result:
    return top(request)

//--- issue223_minimal.py
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


# Issue #223 minimal repro: the route rule masks the payload field instead of
# expressing per-output presence, so both outputs publish every cycle.

@ac.rule
def route(request: Request) -> tuple[Request, Request]:
    thread0 = request.with_fields(value=request.value, valid=(request.valid & (request.tid == 0)))
    thread1 = request.with_fields(value=request.value, valid=(request.valid & (request.tid == 1)))
    return thread0, thread1


@ac.rule
def arbitrate(result0: Result, result1: Result) -> Result:
    return result0 if result0.valid else result1


@ac.module_decl(source="tests/python/agentic-circuit/python_frontend/test_queue_frontend.py")
def state_bank(packet: Request) -> Result:
    ...


state_bank_decl = state_bank


@ac.module(declaration=state_bank_decl)
def state_bank(packet: Request) -> Result:
    return Result(value=packet.value, valid=packet.valid)


@ac.module_decl(source="tests/python/agentic-circuit/python_frontend/test_queue_frontend.py")
def top(request: Request) -> Result:
    ...


top_decl = top


@ac.module(declaration=top_decl)
def top(request: Request) -> Result:
    thread0, thread1 = route(request)
    result0 = state_bank(thread0)
    result1 = state_bank(thread1)
    result = result0.merge(result1, policy="priority", depth=1, latency=1)
    return result


@ac.system
def composite(request: Request) -> Result:
    return top(request)

//--- harness.cpp
// Runtime verification harness for the composite-module demux (issue #223).
//
// The harness is deliberately model-agnostic: it asserts the issue's isolation
// property without knowing which demux program produced the model it links
// against. Pointed at the per-output-presence program it must pass; pointed at
// the payload-masking program it must fail, because that program publishes every
// output unconditionally and therefore loads the non-target bank with a spurious
// token. That contrast is what proves the assertion measures the property.
//
// Assertion A (isolation): a tid=0-only request must publish the tid=0 payload
//   (selective arbitration) and must never load the tid=1 bank.
// Assertion B (not silence): one tid=0 plus one tid=1 request must produce both
//   payloads, so a passing A is meaningful.
//
// Usage: harness <label> [--invert-isolation]
//   --invert-isolation deliberately asserts the opposite of A and is used as the
//   negative control that shows a broken expectation fails with a non-zero exit.

#include "generated/dut.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
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

// Object paths are rooted at the SimSystem name, so match on the suffix that
// the generated module hierarchy owns.
uint64_t statForPathSuffix(gfsim::SimSystem &system,
                           std::string_view pathSuffix,
                           std::string_view statName) {
  for (const gfsim::StatSnapshot &snapshot : system.statistics()) {
    if (snapshot.name != statName)
      continue;
    const std::string_view path = snapshot.objectPath;
    if (path.size() >= pathSuffix.size() &&
        path.compare(path.size() - pathSuffix.size(), pathSuffix.size(),
                     pathSuffix) == 0)
      return snapshot.value;
  }
  return 0;
}

struct ScenarioResult {
  size_t epochs = 0;
  size_t accepted = 0;
  size_t results = 0;
  size_t first_result_epoch = 0;
  uint64_t bank0_accepted = 0;
  uint64_t bank1_accepted = 0;
  std::vector<std::pair<unsigned, unsigned>> taken;
};

// Drive one freshly constructed model: offer at most one pending request per
// epoch, advance one epoch, then drain at most one result.
ScenarioResult runScenario(const char *label,
                           const std::vector<Request> &requests,
                           size_t maxEpochs) {
  gfsim::SimSystem system{"issue223"};
  Composite model;
  model.set_sink_retention_limit(0);
  std::array<gfsim::TimeDomainRuntime, 1> timeDomains{{{"cycle", 1, 0, 1}}};

  ScenarioResult run;
  if (!system.root().attachChild(model) ||
      !system.setTimeDomains(timeDomains) ||
      !model.configure_activation_scheduler(system)) {
    std::printf("scenario %s wire_failure=1\n", label);
    return run;
  }

  size_t next = 0;
  while (run.epochs < maxEpochs) {
    if (next < requests.size() && model.offer_request(system, requests[next])) {
      ++run.accepted;
      ++next;
    }
    const bool advanced = system.step();
    ++run.epochs;
    while (std::optional<Result> result = model.try_take_result_0(system)) {
      ++run.results;
      if (run.results == 1)
        run.first_result_epoch = run.epochs;
      run.taken.emplace_back(static_cast<unsigned>(result->value.value()),
                             static_cast<unsigned>(result->valid.value()));
    }
    if (!advanced)
      break;
  }

  run.bank0_accepted =
      statForPathSuffix(system, "/top_0/child_0_queue", "accepted_transactions");
  run.bank1_accepted =
      statForPathSuffix(system, "/top_0/child_1_queue", "accepted_transactions");

  std::printf("scenario %s epochs=%zu accepted=%zu results=%zu"
              " first_result_epoch=%zu bank0_accepted=%llu bank1_accepted=%llu\n",
              label, run.epochs, run.accepted, run.results,
              run.first_result_epoch,
              static_cast<unsigned long long>(run.bank0_accepted),
              static_cast<unsigned long long>(run.bank1_accepted));
  for (size_t index = 0; index < run.taken.size(); ++index)
    std::printf("scenario %s result[%zu] value=%u valid=%u\n", label, index,
                run.taken[index].first, run.taken[index].second);
  return run;
}

int fail(std::string_view message) {
  std::printf("FAIL: %.*s\n", static_cast<int>(message.size()), message.data());
  return 1;
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 2 || argc > 3) {
    std::printf("usage: %s <label> [--invert-isolation]\n", argv[0]);
    return 2;
  }
  const bool invertIsolation =
      argc == 3 && std::strcmp(argv[2], "--invert-isolation") == 0;
  if (argc == 3 && !invertIsolation)
    return fail("unknown option");
  std::printf("model %s invert_isolation=%d\n", argv[1],
              invertIsolation ? 1 : 0);

  // Scenario A: a single tid=0 request, no tid=1 traffic at all. Selective
  // arbitration still publishes that bank's result (criterion 4), but the demux
  // must never load the tid=1 bank.
  const ScenarioResult tid0Only =
      runScenario("tid0_only", {makeRequest(42, /*tid=*/false)}, 64);
  if (tid0Only.accepted != 1)
    return fail("tid0_only request was not accepted by the input queue");
  const bool isolationHeld =
      tid0Only.results == 1 && tid0Only.bank1_accepted == 0 &&
      tid0Only.taken.front() == std::make_pair(42u, 1u);
  if (isolationHeld == invertIsolation) {
    return fail(invertIsolation
                    ? "deliberately inverted expectation: tid0_only stayed "
                      "isolated, so the inverted assertion failed"
                    : "tid0_only loaded the tid=1 bank");
  }

  // Scenario B: one tid=0 request and one tid=1 request must both complete.
  const ScenarioResult pair = runScenario(
      "pair", {makeRequest(7, /*tid=*/false), makeRequest(9, /*tid=*/true)}, 64);
  if (pair.accepted != 2)
    return fail("pair scenario did not accept both requests");
  if (pair.results != 2 || pair.taken.size() != 2)
    return fail("pair scenario did not produce both results");
  if (pair.taken[0] != std::make_pair(7u, 1u) ||
      pair.taken[1] != std::make_pair(9u, 1u))
    return fail("pair scenario did not return both payloads in order");
  if (pair.bank0_accepted != 1 || pair.bank1_accepted != 1)
    return fail("pair scenario did not load both banks once");

  std::printf("PASS %s%s\n", argv[1], invertIsolation ? " (inverted)" : "");
  return 0;
}
