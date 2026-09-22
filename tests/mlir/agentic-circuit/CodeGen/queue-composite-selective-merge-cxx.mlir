// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/selective_merge.py %t/raw.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/raw.mlir -o %t/frozen.mlir
// RUN: %acir_queue_cxxgen --output-root=%t/bundle %t/frozen.mlir | %FileCheck %s --check-prefix=EMIT
// RUN: %FileCheck %s --check-prefix=MEMBER < %t/bundle/include/generated/modules/top.hpp
// RUN: %FileCheck %s --check-prefix=BIND < %t/bundle/src/generated/modules/top.cpp
// RUN: %cxx -std=c++20 -I%t/bundle/include -I%source_root/simulator/gfsim/include -fsyntax-only %t/bundle/src/generated/modules/top.cpp

// Issue #223 acceptance criterion 4 -- end-to-end code generation.
//
// The scenario is a composite `@ac.module` body that mixes a local demux rule
// with TWO child module instances and a selective merge:
//
//   thread0, thread1 = route(request)          # local rule -> two Queues
//   result0 = state_bank(thread0)              # child instance
//   result1 = state_bank(thread1)              # child instance
//   merged  = result0.merge(result1, ...)      # local selective arbiter
//
// The mixed-shape whitelist used to admit only transform, broadcast, and
// stateless firing blocks, so this body failed at `acir-queue-cxxgen` with
// "block 'merged' has kind 'merge'". A selective merge is now admitted with the
// arity `gfsim::QueueMerge` requires (>= 2 inputs, exactly 1 output) and lowered
// by the mixed emitter to the same runtime primitive the generic `ac.merge`
// (`kind == "merge"`) path already binds.
//
// This test compiles the body all the way through Python lowering, the rule
// lowering pipeline, C++ emission, and a C++20 syntax check, and it pins the
// emission shape so a regression back to the JOINing `QueueAtomicTransform`
// cannot pass silently.

// EMIT: emitted model bundle v1

// The local arbiter is the selective runtime primitive, instantiated for the
// two child output Queues and the shared payload type.
// MEMBER: gfsim::QueueMerge<Result, 2>
// The parent binds the merge to both child output Queues, to the module output,
// and to the requested priority policy.
// BIND: "merge_merged"
// BIND: std::array<gfsim::SimQueue<Result> *, 2>{&queue_2_, &queue_3_}
// BIND: gfsim::QueueMergePolicy::Priority
// The merge is a first-class dispatch row alongside the local firing rule and
// the two child instances.
// BIND: makeDispatchRow(&block_1_)

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
