// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/family_lanes.py %t/raw.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/raw.mlir -o %t/frozen.mlir
// RUN: %FileCheck %s --check-prefix=SHAPES < %t/frozen.mlir
// RUN: %acir_queue_cxxgen --output-root=%t/bundle %t/frozen.mlir | %FileCheck %s --check-prefix=EMIT
// RUN: %FileCheck %s --check-prefix=CASE < %t/bundle/include/generated/modules/stage.hpp
// RUN: %FileCheck %s --check-prefix=ROOT < %t/bundle/include/generated/dut.h
// RUN: %cxx -std=c++20 -I%t/bundle/include -I%source_root/simulator/gfsim/include -fsyntax-only %t/bundle/src/generated/modules/stage.cpp
// RUN: %acir_queue_cxxgen %t/frozen.mlir > %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.cpp

// Issue #223 acceptance criterion 8 -- structurally parameterized Thread/bank
// counts in the C++ backend.
//
// ONE `@ac.module_decl(parameters=(lanes,), finite_cases=(2, 4))` declaration
// is typed `ac.Queue[ac.u8, lanes, 2]`, so the frontend emits one
// `ac.module @stage` carrying two concrete `ac.module.case` bodies. The C++
// emitter used to refuse the whole family with
// "unsupported C++ parameter family shape: one readable class name would denote
// incompatible family cases", because every case resolved to the same readable
// class name.
//
// Each concrete case now appends a deterministic suffix derived from its own
// static arguments (`lanes = 2` -> `StageLanes2`), so one declaration emits one
// class per shape. Both interface Queues are multi-lane, so each case binds the
// lane-wise `gfsim::QueueLaneTransform` instead of dropping the lane count.
//
// The selected `core` case is `lanes = 2`, and the emitted root model really
// constructs 2-lane Queues with rate 2: the lane count reaches the model rather
// than only the class name.

// The family declaration stays single: one `ac.module @stage` and two cases
// whose concrete Queue types carry the two lane counts.
// SHAPES: ac.module @stage
// SHAPES: ac.module.case {{.*}}!ac.queue<i8, lanes = 2, rate = 2>
// SHAPES: ac.module.case {{.*}}!ac.queue<i8, lanes = 4, rate = 2>
// SHAPES: ac.module @Top

// EMIT: emitted model bundle v1

// CASE: class StageLanes2 final : public gfsim::Module {
// CASE: gfsim::QueueLaneTransform<gfsim::UInt<8>, gfsim::UInt<8>, StageLanes2_policy> block_;
// CASE: class StageLanes4 final : public gfsim::Module {
// CASE: gfsim::QueueLaneTransform<gfsim::UInt<8>, gfsim::UInt<8>, StageLanes4_policy> block_;

// The root system class keeps its bare readable name (no family suffix), and
// the selected `lanes = 2` case really materializes two lanes at rate two.
// ROOT: class Core final : public gfsim::Module {
// ROOT: input_0_("input_0_queue", 0, this, 2, std::numeric_limits<size_t>::max(), nullptr, 1, 2, 2)
// ROOT: std::make_unique<StageLanes2>("stage_0", 2, this, &input_0_, &stage_0_)

//--- lower.py
from pathlib import Path
import sys

from agentic_circuit._queue_frontend import lower_queue_source


source = Path(sys.argv[1]).read_text()
Path(sys.argv[2]).write_text(
    lower_queue_source(source, "core", source_path="stage.py")
)

//--- family_lanes.py
import agentic_circuit as ac


@ac.module_decl(
    source="stage.py",
    parameters=(ac.static_parameter("lanes", ac.static_int(width=4, signed=False)),),
    finite_cases=(ac.case(("lanes", 2)), ac.case(("lanes", 4))),
)
def stage(value: ac.Queue[ac.u8, lanes, 2]) -> ac.Queue[ac.u8, lanes, 2]:
    ...


stage_decl = stage


@ac.module(declaration=stage_decl)
def stage(value: ac.Queue[ac.u8, lanes, 2]) -> ac.Queue[ac.u8, lanes, 2]:
    return value


@ac.system
def core(value: ac.Queue[ac.u8, 2, 2]) -> ac.Queue[ac.u8, 2, 2]:
    return stage(value, static=ac.case(("lanes", 2)))
