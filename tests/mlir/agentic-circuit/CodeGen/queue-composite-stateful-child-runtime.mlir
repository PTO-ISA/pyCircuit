// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/stateful_child.py %t/raw.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/raw.mlir -o %t/frozen.mlir
// RUN: %acir_queue_cxxgen --output-root=%t/bundle %t/frozen.mlir | %FileCheck %s --check-prefix=EMIT
// RUN: %FileCheck %s --check-prefix=SHARED < %t/bundle/include/generated/modules/acc_bank.hpp
// RUN: %FileCheck %s --check-prefix=INSTANCE < %t/bundle/src/generated/modules/top.cpp
// RUN: cmake -S %t/bundle -B %t/build -G Ninja -DAC_GFSIM_INCLUDE_DIR=%source_root/simulator/gfsim/include
// RUN: cmake --build %t/build --parallel 2
// RUN: %cxx -std=c++20 -I%t/bundle/include -I%source_root/simulator/gfsim/include %t/harness.cpp %t/build/libac_generated_model.a %binary_root/gfsim/libgfsim.a -o %t/harness
// RUN: %t/harness | %FileCheck %s --check-prefix=RUNTIME

// Issue #223 acceptance criteria 6 and 7 for the mixed composite shape.
//
// The scenario is one parent that mixes a local demux rule with TWO placements
// of a single STATEFUL child specialization. The child owns module-local state,
// so the two placements prove criterion 7's "shared code but independent Queue
// and state" rather than merely asserting it:
//
//   * exactly one `class AccBank` definition is emitted, with a per-instance
//     `gfsim::SimTable` member, and the parent constructs two instances with
//     distinct ObjectIds and distinct Queues;
//   * the runtime harness drives the same stimulus twice, once through the
//     generated incremental activation plan and once as a full scan that
//     schedules every dispatch row every tick, and requires identical result
//     streams (criterion 6);
//   * bank 0 accumulates 1,2,3 then 4 into 10 while the lone tid=1 request only
//     reaches bank 1, so the two placements cannot be sharing state (criterion
//     7).
//
// The local demux commits into the children's input Queues and the children's
// output Queues are the parent outputs, so the incremental run exercises wakeup
// across the local/child boundary in both directions.

// EMIT: emitted model bundle v1

// SHARED: class AccBank final : public gfsim::Module {
// SHARED: gfsim::SimTable<gfsim::UInt<8>> state_total_;

// INSTANCE: child_0_ = std::make_unique<AccBank>("child_0", object_
// INSTANCE: child_1_ = std::make_unique<AccBank>("child_1", object_

// RUNTIME: incremental result0=1/1,3/1,6/1,10/1 result1=10/1
// RUNTIME: scan        result0=1/1,3/1,6/1,10/1 result1=10/1
// RUNTIME: PASS criteria 6+7

//--- lower.py
from pathlib import Path
import sys

from agentic_circuit._queue_frontend import lower_queue_source


source = Path(sys.argv[1]).read_text()
Path(sys.argv[2]).write_text(
    lower_queue_source(source, "composite", source_path="scen/entry.py")
)

//--- stateful_child.py
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


@ac.rule
def route(request: Request) -> tuple[Request, Request]:
    thread0 = request if (request.valid & (request.tid == 0)) else None
    thread1 = request if (request.valid & (request.tid == 1)) else None
    return thread0, thread1


@ac.rule
def accumulate(total, packet):
    total = total + packet.value
    return Result(value=total, valid=packet.valid)


@ac.module_decl(source="scen/entry.py")
def acc_bank(packet: Request) -> Result:
    ...


acc_bank_decl = acc_bank


@ac.module(declaration=acc_bank_decl)
def acc_bank(packet: Request) -> Result:
    total: ac.bits[8] = 0
    result = accumulate(total, packet)
    return result


@ac.module_decl(source="scen/entry.py")
def top(request: Request) -> tuple[Result, Result]:
    ...


top_decl = top


@ac.module(declaration=top_decl)
def top(request: Request) -> tuple[Result, Result]:
    thread0, thread1 = route(request)
    result0 = acc_bank(thread0)
    result1 = acc_bank(thread1)
    return result0, result1


@ac.system
def composite(request: Request) -> tuple[Result, Result]:
    return top(request)

//--- harness.cpp
// Runtime harness for issue #223 criteria 6 and 7.
//
// Runs the generated model twice with the same stimulus:
//   * incremental: install the generated dispatch table, arbitration order,
//     activation plan and work-closure plan;
//   * scan: install the same dispatch table, arbitration order and work-closure
//     plan but no activation plan, and explicitly schedule every dispatch row
//     on the current and next tick.
// The two runs must produce identical per-output result streams.

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
using ac_generated::Request;
using ac_generated::Result;

Request makeRequest(uint8_t value, bool tid) {
  Request request;
  request.value = gfsim::UInt<8>{value};
  request.tid = gfsim::UInt<1>{tid};
  request.valid = gfsim::UInt<1>{true};
  return request;
}

struct RunResult {
  bool wired = false;
  std::vector<std::pair<unsigned, unsigned>> result0;
  std::vector<std::pair<unsigned, unsigned>> result1;
};

RunResult run(const std::vector<Request> &requests, size_t maxTicks,
              bool scanMode, const char *label) {
  RunResult run;
  gfsim::SimSystem system{label};
  Composite model;
  std::array<gfsim::TimeDomainRuntime, 1> timeDomains{{{"cycle", 1, 0, 1}}};
  if (!system.root().attachChild(model) || !system.setTimeDomains(timeDomains))
    return run;

  if (scanMode) {
    const auto rows = model.dispatch_rows();
    if (!system.setDispatchTable(rows) ||
        !system.setArbitrationOrder(model.arbitration_order()) ||
        !system.setWorkClosurePlan(model.work_closure_offsets(),
                                   model.work_closure_targets()))
      return run;
  } else if (!model.configure_activation_scheduler(system)) {
    return run;
  }
  run.wired = true;

  size_t next = 0;
  for (size_t tick = 0; tick < maxTicks; ++tick) {
    if (next < requests.size() && model.offer_request(system, requests[next]))
      ++next;
    if (scanMode) {
      const auto rows = model.dispatch_rows();
      const gfsim::Epoch now = system.currentEpoch();
      for (size_t id = 0; id < rows.size(); ++id) {
        if (!system.scheduleWork(static_cast<gfsim::ObjectId>(id), now))
          return run;
        // Keep the next tick populated so step() does not conclude that the
        // design quiesced after a single epoch.
        if (!system.scheduleWork(static_cast<gfsim::ObjectId>(id),
                                 gfsim::Epoch{now.time + 1, 0}))
          return run;
      }
    }
    const bool advanced = system.step();
    while (std::optional<Result> value = model.try_take_result_0(system))
      run.result0.emplace_back(static_cast<unsigned>(value->value.value()),
                               static_cast<unsigned>(value->valid.value()));
    while (std::optional<Result> value = model.try_take_result_1(system))
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
  const std::vector<Request> stimulus{
      makeRequest(1, /*tid=*/false), makeRequest(2, /*tid=*/false),
      makeRequest(3, /*tid=*/false), makeRequest(10, /*tid=*/true),
      makeRequest(4, /*tid=*/false)};

  const RunResult incremental =
      run(stimulus, 64, /*scanMode=*/false, "incremental");
  const RunResult scan = run(stimulus, 64, /*scanMode=*/true, "scan");
  if (!incremental.wired || !scan.wired) {
    std::printf("FAIL: model did not wire\n");
    return 1;
  }

  std::printf("incremental result0=%s result1=%s\n",
              describe(incremental.result0).c_str(),
              describe(incremental.result1).c_str());
  std::printf("scan        result0=%s result1=%s\n",
              describe(scan.result0).c_str(), describe(scan.result1).c_str());

  if (incremental.result0 != scan.result0 ||
      incremental.result1 != scan.result1) {
    std::printf("FAIL: scan and incremental modes disagree\n");
    return 1;
  }

  // Independent per-placement state: bank 0 accumulates 1,2,3 then 4 -> 10; the
  // lone tid=1 request must not disturb bank 0 and bank 1 must see only 10.
  const std::vector<std::pair<unsigned, unsigned>> expected0{{1, 1}, {3, 1},
                                                             {6, 1}, {10, 1}};
  const std::vector<std::pair<unsigned, unsigned>> expected1{{10, 1}};
  if (incremental.result0 != expected0) {
    std::printf("FAIL: bank0 state stream unexpected\n");
    return 1;
  }
  if (incremental.result1 != expected1) {
    std::printf("FAIL: bank1 state stream unexpected\n");
    return 1;
  }

  std::printf("PASS criteria 6+7\n");
  return 0;
}
