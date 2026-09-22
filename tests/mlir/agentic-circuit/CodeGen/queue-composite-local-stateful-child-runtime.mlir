// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/local_state_child.py %t/raw.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/raw.mlir -o %t/frozen.mlir
// RUN: %acir_queue_cxxgen --output-root=%t/bundle %t/frozen.mlir | %FileCheck %s --check-prefix=EMIT
// RUN: cmake -S %t/bundle -B %t/build -G Ninja -DAC_GFSIM_INCLUDE_DIR=%source_root/simulator/gfsim/include
// RUN: cmake --build %t/build --parallel 2
// RUN: %cxx -std=c++20 -I%t/bundle/include -I%source_root/simulator/gfsim/include %t/harness.cpp %t/build/libac_generated_model.a %binary_root/gfsim/libgfsim.a -o %t/harness
// RUN: %t/harness | %FileCheck %s --check-prefix=RUNTIME

// Issue #223 acceptance criterion 9, runtime half.
//
// Two placements of `stage` run side by side. Each `stage` owns a module-local
// `total` Var that its own local `bump` rule updates every tick, and each feeds
// its own stateful `acc_bank` child. The two placements carry different
// stimulus, so sharing either the local Var or the child Table would change the
// observable result stream:
//
//   stage(tid=0): local total 1,2; child bank total 1,3  -> 1,3
//   stage(tid=1): local total 10,20; child bank total 10,30 -> 10,30
//
// The local Var therefore persists across ticks, is updated by the local rule,
// and stays independent per placement.

// EMIT: emitted model bundle v1

// RUNTIME: result0=1/1,3/1 result1=10/1,30/1
// RUNTIME: PASS criterion 9 local state

//--- lower.py
from pathlib import Path
import sys

from agentic_circuit._queue_frontend import lower_queue_source


source = Path(sys.argv[1]).read_text()
Path(sys.argv[2]).write_text(
    lower_queue_source(source, "composite", source_path="scen/entry.py")
)

//--- local_state_child.py
import agentic_circuit as ac


@ac.struct
class Count:
    value: ac.bits[8]
    tid: ac.bits[1]
    valid: ac.bits[1]


@ac.rule
def route(request: Count) -> tuple[Count, Count]:
    thread0 = request if (request.valid & (request.tid == 0)) else None
    thread1 = request if (request.valid & (request.tid == 1)) else None
    return thread0, thread1


@ac.rule
def accumulate(total, packet):
    total = total + packet.value
    return Count(value=total, tid=packet.tid, valid=packet.valid)


@ac.module_decl(source="scen/entry.py")
def acc_bank(packet: Count) -> Count:
    ...


acc_bank_decl = acc_bank


@ac.module(declaration=acc_bank_decl)
def acc_bank(packet: Count) -> Count:
    total: ac.bits[8] = 0
    result = accumulate(total, packet)
    return result


@ac.rule
def bump(total, packet):
    total = total + packet.value
    return Count(value=total, tid=packet.tid, valid=packet.valid)


@ac.module_decl(source="scen/entry.py")
def stage(request: Count) -> Count:
    ...


stage_decl = stage


@ac.module(declaration=stage_decl)
def stage(request: Count) -> Count:
    total: ac.bits[8] = 0
    tagged = bump(total, request)
    result = acc_bank(tagged)
    return result


@ac.module_decl(source="scen/entry.py")
def root(request: Count) -> tuple[Count, Count]:
    ...


root_decl = root


@ac.module(declaration=root_decl)
def root(request: Count) -> tuple[Count, Count]:
    thread0, thread1 = route(request)
    result0 = stage(thread0)
    result1 = stage(thread1)
    return result0, result1


@ac.system
def composite(request: Count) -> tuple[Count, Count]:
    return root(request)

//--- harness.cpp
// Runtime harness for issue #223 acceptance criterion 9.

#include "generated/dut.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using ac_generated::Composite;
using ac_generated::Count;

Count makeCount(uint8_t value, bool tid) {
  Count count;
  count.value = gfsim::UInt<8>{value};
  count.tid = gfsim::UInt<1>{tid};
  count.valid = gfsim::UInt<1>{true};
  return count;
}

struct RunResult {
  bool wired = false;
  std::vector<std::pair<unsigned, unsigned>> result0;
  std::vector<std::pair<unsigned, unsigned>> result1;
};

RunResult run(const std::vector<Count> &requests, size_t maxTicks) {
  RunResult run;
  gfsim::SimSystem system{"criterion9"};
  Composite model;
  model.set_sink_retention_limit(0);
  std::array<gfsim::TimeDomainRuntime, 1> timeDomains{{{"cycle", 1, 0, 1}}};
  if (!system.root().attachChild(model) || !system.setTimeDomains(timeDomains))
    return run;
  if (!model.configure_activation_scheduler(system))
    return run;
  run.wired = true;

  size_t next = 0;
  for (size_t tick = 0; tick < maxTicks; ++tick) {
    if (next < requests.size() && model.offer_request(system, requests[next]))
      ++next;
    const bool advanced = system.step();
    while (std::optional<Count> value = model.try_take_result_0(system))
      run.result0.emplace_back(static_cast<unsigned>(value->value.value()),
                               static_cast<unsigned>(value->valid.value()));
    while (std::optional<Count> value = model.try_take_result_1(system))
      run.result1.emplace_back(static_cast<unsigned>(value->value.value()),
                               static_cast<unsigned>(value->valid.value()));
    if (!advanced)
      break;
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

int main() {
  const std::vector<Count> stimulus{makeCount(1, /*tid=*/false),
                                    makeCount(1, /*tid=*/false),
                                    makeCount(10, /*tid=*/true),
                                    makeCount(10, /*tid=*/true)};

  const RunResult result = run(stimulus, 64);
  if (!result.wired) {
    std::printf("FAIL: model did not wire\n");
    return 1;
  }

  std::printf("result0=%s result1=%s\n",
              describe(result.result0).c_str(),
              describe(result.result1).c_str());

  // Shared local state would have produced 11/12 and 13/16, so these streams
  // prove per-placement independence as well as persistence.
  const std::vector<std::pair<unsigned, unsigned>> expected0{{1, 1}, {3, 1}};
  const std::vector<std::pair<unsigned, unsigned>> expected1{{10, 1}, {30, 1}};
  if (result.result0 != expected0) {
    std::printf("FAIL: local+child state stream for placement 0 unexpected\n");
    return 1;
  }
  if (result.result1 != expected1) {
    std::printf("FAIL: local+child state stream for placement 1 unexpected\n");
    return 1;
  }

  std::printf("PASS criterion 9 local state\n");
  return 0;
}
