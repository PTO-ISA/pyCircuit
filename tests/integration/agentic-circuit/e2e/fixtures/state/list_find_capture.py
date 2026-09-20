"""Storage-neutral list selection with one read-only persistent owner."""

import agentic_circuit as ac


@ac.struct
class Entry:
    index: ac.u2
    age: ac.u8
    src0_tag: ac.u6
    src1_tag: ac.u6
    valid: bool


@ac.struct
class Wakeup:
    tag: ac.u6


@ac.rule
def wake(ready_tags, wakeup):
    ready_tags[wakeup.tag] = True


@ac.rule
def issue(entries, ready_tags):
    selected = entries.find(
        where=lambda entry: (
            entry.valid and ready_tags[entry.src0_tag] and ready_tags[entry.src1_tag]
        ),
        key=lambda entry: entry.age,
    )
    if selected.valid:
        entries[selected.index] = selected.value.with_fields(valid=False)
        return selected.value


@ac.module_decl(source="tests/integration/agentic-circuit/e2e/fixtures/state/list_find_capture.py")
def find_module(wakeup: Wakeup) -> Entry:
    ...

find_module_decl = find_module

@ac.module(declaration=find_module_decl)
def find_module(wakeup: Wakeup) -> Entry:
    entries = ac.table[4, Entry](init=0)
    ready_tags = ac.table[64, bool](init=0)
    wake(ready_tags, wakeup)
    issued = issue(entries, ready_tags)
    return issued


@ac.system
def list_find_capture(wakeup: Wakeup) -> Entry:
    issued = find_module(wakeup)
    return issued
