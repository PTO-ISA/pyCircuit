from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    cas,
    compile_cycle_aware,
    mux,
    u,
    wire_of,
)

RULES = [
    {"mask": 0xF0, "match": 0x10, "op": 1, "len": 4},
    {"mask": 0xF0, "match": 0x20, "op": 2, "len": 4},
    {"mask": 0xF0, "match": 0x30, "op": 3, "len": 4},
]


def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
    insn = cas(domain, m.input("insn", width=8), cycle=0)
    op = cas(domain, u(4, 0), cycle=0)
    ln = cas(domain, u(3, 0), cycle=0)

    for r in RULES:
        hit = (insn & u(8, r["mask"])) == u(8, r["match"])
        op = mux(hit, u(4, r["op"]), op)
        ln = mux(hit, u(3, r["len"]), ln)

    m.output("op", wire_of(op))
    m.output("len", wire_of(ln))


build.__pycircuit_name__ = "decode_rules"

if __name__ == "__main__":
    print(compile_cycle_aware(build, name="decode_rules").emit_mlir())
