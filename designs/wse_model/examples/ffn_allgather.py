#!/usr/bin/env python3
"""Run the FFN two-phase AllGather closure end to end.

This is the model's integration scenario. It compiles the two documented FFN
logical identities (Calendar §2.2.1), installs the timeslot register, and runs
phase B (row AllGather, 32 members) followed by phase C (column AllGather, 32
members) over the same ``opcode`` domain, checking that every member's byte
accounting reaches its ``expVal`` exactly.

Run:

    python examples/ffn_allgather.py
"""

from __future__ import annotations

import json

from wse_model.collective import run_allgather
from wse_model.fixtures import ffn_example
from wse_model.noc import Noc


def main() -> int:
    example = ffn_example()
    table = example.table

    print("== compiled route table ==")
    print(json.dumps(table.describe(), indent=2))
    report = table.validate()
    print(report.format())

    # Install the timeslot register once, in a drained window (Calendar §3.9).
    noc = Noc(table.topology, calreg=example.calreg())

    for label, key_id, geometry, groups in (
        ("phase B: ROW AllGather", 0, example.phase_b_geometry, example.phase_b_groups),
        ("phase C: COL AllGather", 1, example.phase_c_geometry, example.phase_c_groups),
    ):
        outcome = run_allgather(noc, table, key_id, geometry=geometry, groups=groups)
        outcome.check()
        first = outcome.states[0]
        print(f"\n== {label} ==")
        print(f"  members            : {len(outcome.members)}")
        print(f"  epoch              : {outcome.epoch} (opcode {outcome.opcode})")
        print(f"  expVal per member  : {first.exp_val} B (landCount {first.land_count})")
        print(f"  counted per member : {first.counted_bytes} B")
        print(f"  tree depth         : {outcome.max_tree_depth} hops")
        print(f"  peak link load     : {outcome.peak_link_load} flits")
        print(f"  payload delivered  : {outcome.total_payload_bytes} B")
        print(f"  wire bytes         : {outcome.total_wire_bytes} B")
        print(
            f"  useful payload     : {100.0 * outcome.total_payload_bytes / outcome.total_wire_bytes:.1f}%"
        )
        print(f"  complete           : {outcome.ok}")

    print(f"\ndie drained after the launch: {noc.drained}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
