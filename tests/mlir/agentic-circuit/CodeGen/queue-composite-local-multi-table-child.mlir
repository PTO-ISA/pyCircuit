// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/local_multi_table_child.py %t/raw.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/raw.mlir -o %t/frozen.mlir
// RUN: %acir_queue_cxxgen --output-root=%t/bundle %t/frozen.mlir | %FileCheck %s --check-prefix=EMIT
// RUN: %FileCheck %s --check-prefix=MULTI < %t/bundle/include/generated/modules/stage.hpp
// RUN: for source in %t/bundle/src/generated/*.cpp %t/bundle/src/generated/modules/*.cpp %t/bundle/src/generated/helpers/*.cpp; do %cxx -std=c++20 -fsyntax-only -I%t/bundle/include -I%source_root/simulator/gfsim/include "$source" || exit 1; done

// Issue #223 acceptance criterion 9 for a local rule that owns TWO
// module-local Tables next to a child. The multi-owner shape binds the same
// `gfsim::QueueStateTransition` runtime the direct-interface stateful emitter
// uses, with one write mode and one merge policy per Table, so it must compile
// rather than fall back to a partial transition.

// EMIT: emitted model bundle v1

// MULTI: gfsim::QueueStateTransition<{{.*}}> block_;
// MULTI: gfsim::SimTable<gfsim::UInt<8>> state_total_;
// MULTI: gfsim::SimTable<gfsim::UInt<8>> state_count_;
// MULTI: std::unique_ptr<AccBank> child_0_;

//--- lower.py
from pathlib import Path
import sys

from agentic_circuit._queue_frontend import lower_queue_source


source = Path(sys.argv[1]).read_text()
Path(sys.argv[2]).write_text(
    lower_queue_source(source, "composite", source_path="scen/entry.py")
)

//--- local_multi_table_child.py
import agentic_circuit as ac


@ac.struct
class Count:
    value: ac.bits[8]
    tid: ac.bits[1]
    valid: ac.bits[1]


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
def bump2(total, count, packet):
    total = total + packet.value
    count = count + 1
    return Count(value=total + count, tid=packet.tid, valid=packet.valid)


@ac.module_decl(source="scen/entry.py")
def stage(request: Count) -> Count:
    ...


stage_decl = stage


@ac.module(declaration=stage_decl)
def stage(request: Count) -> Count:
    total: ac.bits[8] = 0
    count: ac.bits[8] = 0
    tagged = bump2(total, count, request)
    result = acc_bank(tagged)
    return result


@ac.system
def composite(request: Count) -> Count:
    return stage(request)
