"""One ordinary persistent Event owner with an inferred compiler domain."""

# ruff: noqa: F841 -- the rule proposes the persistent owner update.
from history_slot_issue.payload_types import Event
from pycircuit import module, rule, u1


@rule
def update(q, en, d):
    if en:
        q = d


@module
def payload_cell(en: u1, d: Event) -> {"q": Event}:
    q: Event = 0
    update(q, en, d)
    return {"q": q}
