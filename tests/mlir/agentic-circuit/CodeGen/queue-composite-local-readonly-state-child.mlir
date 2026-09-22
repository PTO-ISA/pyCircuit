// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/readonly_state_child.py %t/raw.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/raw.mlir -o %t/frozen.mlir
// RUN: %not %acir_queue_cxxgen --output-root=%t/bundle %t/frozen.mlir 2>&1 | %FileCheck %s --check-prefix=ERR

// Issue #223 acceptance criterion 9, fail-closed half.
//
// A second local rule that only READS the module-local Table reserves it so the
// observed value is the committed one. The mixed stateless policy binds no
// Table, and the mixed emitter deliberately does not bind a read-only
// reservation, so the shape must be rejected by name instead of emitting a
// half-bound transition.

// ERR: ACLOWER-QUEUE-CXX: mixed nested local firing does not support a read-only state reservation; firing block 'observed' reserves a Table without writing it

//--- lower.py
from pathlib import Path
import sys

from agentic_circuit._queue_frontend import lower_queue_source


source = Path(sys.argv[1]).read_text()
Path(sys.argv[2]).write_text(
    lower_queue_source(source, "composite", source_path="scen/entry.py")
)

//--- readonly_state_child.py
import agentic_circuit as ac


@ac.struct
class Entry:
    value: ac.bits[8]
    tid: ac.bits[1]
    valid: ac.bits[1]


@ac.rule
def accumulate(total, packet):
    total = total + packet.value
    return Entry(value=total, tid=packet.tid, valid=packet.valid)


@ac.module_decl(source="scen/entry.py")
def acc_bank(packet: Entry) -> Entry:
    ...


acc_bank_decl = acc_bank


@ac.module(declaration=acc_bank_decl)
def acc_bank(packet: Entry) -> Entry:
    total: ac.bits[8] = 0
    result = accumulate(total, packet)
    return result


@ac.rule
def poke(entries, incoming):
    updated = Entry(
        value=incoming.value + 1, tid=incoming.tid, valid=incoming.valid
    )
    entries[incoming.tid] = updated
    return updated


@ac.rule
def peek(entries, packet):
    found = entries[packet.tid]
    return Entry(value=found.value, tid=packet.tid, valid=packet.valid)


@ac.module_decl(source="scen/entry.py")
def stage(request: Entry) -> Entry:
    ...


stage_decl = stage


@ac.module(declaration=stage_decl)
def stage(request: Entry) -> Entry:
    entries = ac.table[2, Entry](init=0)
    tagged = poke(entries, request)
    observed = peek(entries, tagged)
    result = acc_bank(observed)
    return result


@ac.system
def composite(request: Entry) -> Entry:
    return stage(request)
