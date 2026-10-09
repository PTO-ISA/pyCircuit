"""One ordinary persistent valid-bit owner with an inferred compiler domain."""

# ruff: noqa: F841 -- the rule proposes the persistent owner update.
from pycircuit import module, rule, u1


@rule
def update(q, en, d):
    if en:
        q = d


@module
def valid_cell(en: u1, d: u1) -> {"q": u1}:
    q: u1 = 0
    update(q, en, d)
    return {"q": q}
