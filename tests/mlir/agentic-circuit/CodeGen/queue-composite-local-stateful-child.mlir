// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/local_state_child.py %t/raw.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/raw.mlir -o %t/frozen.mlir
// RUN: %acir_queue_cxxgen --output-root=%t/bundle %t/frozen.mlir | %FileCheck %s --check-prefix=EMIT
// RUN: %FileCheck %s --check-prefix=STATE < %t/bundle/include/generated/modules/top.hpp
// RUN: for source in %t/bundle/src/generated/*.cpp %t/bundle/src/generated/modules/*.cpp %t/bundle/src/generated/helpers/*.cpp; do %cxx -std=c++20 -fsyntax-only -I%t/bundle/include -I%source_root/simulator/gfsim/include "$source" || exit 1; done

// Issue #223 acceptance criterion 9: a LOCAL `@ac.rule` that owns module-local
// state must coexist with a child `@ac.module` instance in the same composite
// body. The declaration is hoisted to the `ac.module.case` root because the
// segmented body has no `/body` scope, so the frozen body carries a
// case-level `ac.table` next to `ac.scope @seg0` and `ac.instance @child_0`.
//
// The local state becomes a runtime `gfsim::SimTable` owned by the parent
// module, and the local firing is bound to the existing
// `gfsim::QueueTableTransition` runtime the childless stateful case already
// emits. The generated bundle must compile with a plain C++20 compiler.

// EMIT: emitted model bundle v1

// The parent owns the state Table directly (owner "/"), so it is a direct
// member of the generated module next to the segment scope and the child, and
// the local rule binds it through the shared Table transition runtime.
// STATE: gfsim::QueueTableTransition<{{.*}}> block_;
// STATE: gfsim::SimTable<gfsim::UInt<8>> state_total_;
// STATE: std::unique_ptr<StateBank> child_0_;

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
class Request:
    value: ac.bits[8]
    tid: ac.bits[1]
    valid: ac.bits[1]


@ac.struct
class Result:
    value: ac.bits[8]
    valid: ac.bits[1]


@ac.rule
def bump(total, packet):
    total = total + 1
    return packet


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
    total: ac.bits[8] = 0
    tagged = bump(total, request)
    result = state_bank(tagged)
    return result


@ac.system
def composite(request: Request) -> Result:
    return top(request)
