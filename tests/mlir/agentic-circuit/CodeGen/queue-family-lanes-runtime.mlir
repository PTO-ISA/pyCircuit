// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/family_lane_rate.py %t/raw.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/raw.mlir -o %t/frozen.mlir
// RUN: %acir_queue_cxxgen --output-root=%t/bundle %t/frozen.mlir | %FileCheck %s --check-prefix=EMIT
// RUN: %FileCheck %s --check-prefix=CASE < %t/bundle/include/generated/modules/stage.hpp
// RUN: cmake -S %t/bundle -B %t/build -G Ninja -DAC_GFSIM_INCLUDE_DIR=%source_root/simulator/gfsim/include
// RUN: cmake --build %t/build --parallel 2
// RUN: %cxx -std=c++20 -I%t/bundle/include -I%source_root/simulator/gfsim/include %t/harness.cpp %t/build/libac_generated_model.a %binary_root/gfsim/libgfsim.a -o %t/harness
// RUN: %t/harness | %FileCheck %s --check-prefix=RUNTIME

// Issue #223 acceptance criterion 8 -- runtime lane materialization.
//
// The scenario instantiates ONE `@ac.module_decl(parameters=(lanes,),
// finite_cases=(2, 4))` declaration TWICE, on a 2-lane Queue and on a 4-lane
// Queue, from a single system body. `rate = lanes` makes the lane count
// observable: each case owns a `gfsim::QueueLaneTransform`, so one firing
// retires exactly `lanes` items.
//
// The harness reads the lane count back out of the runtime Queues and requires
// the narrow case to retire two items and the wide case four, so a
// codegen-only change that stopped erroring without materializing lanes would
// fail here.

// EMIT: emitted model bundle v1

// CASE: class StageLanes2 final : public gfsim::Module {
// CASE: class StageLanes4 final : public gfsim::Module {

// RUNTIME: materialized lanes narrow=2 wide=4
// RUNTIME: narrow result=10,11
// RUNTIME: wide result=20,21,22,23
// RUNTIME: PASS criterion 8 lanes

//--- lower.py
from pathlib import Path
import sys

from agentic_circuit._queue_frontend import lower_queue_source


source = Path(sys.argv[1]).read_text()
Path(sys.argv[2]).write_text(
    lower_queue_source(source, "core", source_path="stage.py")
)

//--- family_lane_rate.py
import agentic_circuit as ac


@ac.module_decl(
    source="stage.py",
    parameters=(ac.static_parameter("lanes", ac.static_int(width=4, signed=False)),),
    finite_cases=(ac.case(("lanes", 2)), ac.case(("lanes", 4))),
)
def stage(value: ac.Queue[ac.u8, lanes, lanes]) -> ac.Queue[ac.u8, lanes, lanes]:
    ...


stage_decl = stage


@ac.module(declaration=stage_decl)
def stage(value: ac.Queue[ac.u8, lanes, lanes]) -> ac.Queue[ac.u8, lanes, lanes]:
    return value


@ac.system
def core(
    narrow: ac.Queue[ac.u8, 2, 2],
    wide: ac.Queue[ac.u8, 4, 4],
) -> tuple[ac.Queue[ac.u8, 2, 2], ac.Queue[ac.u8, 4, 4]]:
    narrow_out = stage(narrow, static=ac.case(("lanes", 2)))
    wide_out = stage(wide, static=ac.case(("lanes", 4)))
    return narrow_out, wide_out

//--- harness.cpp
// Runtime harness for issue #223 acceptance criterion 8.
//
// ONE static family declaration backs two generated C++ classes
// (`StageLanes2` and `StageLanes4`). The generated root model instantiates both
// cases side by side, so the harness reads the lane count back out of the
// runtime Queues and requires each lane transform to retire exactly `lanes`
// items per firing.

#include "generated/dut.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

namespace {

using ac_generated::Core;

std::string describe(const std::vector<unsigned> &values) {
  std::string text;
  for (unsigned value : values) {
    if (!text.empty())
      text += ",";
    text += std::to_string(value);
  }
  return text;
}

} // namespace

int main() {
  gfsim::SimSystem system{"lane_family"};
  Core model;
  std::array<gfsim::TimeDomainRuntime, 1> timeDomains{{{"cycle", 1, 0, 1}}};
  model.set_sink_retention_limit(0);
  if (!system.root().attachChild(model) || !system.setTimeDomains(timeDomains) ||
      !model.configure_activation_scheduler(system)) {
    std::printf("FAIL: model did not wire\n");
    return 1;
  }

  std::printf("materialized lanes narrow=%zu wide=%zu\n",
              model.narrow().lanes(), model.wide().lanes());
  if (model.narrow().lanes() != 2 || model.narrow().rate() != 2) {
    std::printf("FAIL: narrow Queue did not materialize 2 lanes\n");
    return 2;
  }
  if (model.wide().lanes() != 4 || model.wide().rate() != 4) {
    std::printf("FAIL: wide Queue did not materialize 4 lanes\n");
    return 3;
  }

  // Saturate both lane bundles in one epoch. Each family case owns a
  // QueueLaneTransform, so a single firing has to retire `lanes` items: two on
  // the narrow case and four on the wide case.
  for (uint8_t index = 0; index < 2; ++index)
    model.offer_narrow(system, gfsim::UInt<8>{static_cast<uint8_t>(10 + index)});
  for (uint8_t index = 0; index < 4; ++index)
    model.offer_wide(system, gfsim::UInt<8>{static_cast<uint8_t>(20 + index)});

  std::vector<unsigned> narrow;
  std::vector<unsigned> wide;
  for (size_t tick = 0; tick < 32; ++tick) {
    const bool advanced = system.step();
    while (std::optional<gfsim::UInt<8>> value = model.try_take_result_0(system))
      narrow.push_back(static_cast<unsigned>(value->value()));
    while (std::optional<gfsim::UInt<8>> value = model.try_take_result_1(system))
      wide.push_back(static_cast<unsigned>(value->value()));
    if (!advanced)
      break;
  }

  std::printf("narrow result=%s\n", describe(narrow).c_str());
  std::printf("wide result=%s\n", describe(wide).c_str());

  const std::vector<unsigned> expectedNarrow{10, 11};
  const std::vector<unsigned> expectedWide{20, 21, 22, 23};
  if (narrow != expectedNarrow) {
    std::printf("FAIL: 2-lane case did not move exactly two lanes\n");
    return 4;
  }
  if (wide != expectedWide) {
    std::printf("FAIL: 4-lane case did not move exactly four lanes\n");
    return 5;
  }

  std::printf("PASS criterion 8 lanes\n");
  return 0;
}
