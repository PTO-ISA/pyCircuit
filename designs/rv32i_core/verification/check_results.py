#!/usr/bin/env python3
"""Recheck saved raw results against the independent oracle without executing a DUT."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
from verify import compare


# ndf: verifies=RV32-VER-VIEWS-001
def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--programs", type=Path, required=True)
    p.add_argument("--results", type=Path, required=True)
    a = p.parse_args()
    receipt = a.results / "raw-validation.json"
    receipt.write_text(json.dumps({"status": "INCOMPLETE"}) + "\n")
    rows = json.loads((a.results / "summary.json").read_text())
    if not isinstance(rows, list) or not rows:
        raise AssertionError("no execution views in summary")
    required = {(s, t) for s in ("timing", "functional") for t in ("cpp", "verilog")}
    groups = {}
    for row in rows:
        key = row["case"], row["schedule"]
        view = row["strategy"], row["target"]
        selected = groups.setdefault(key, set())
        if view not in required or view in selected:
            raise AssertionError(f"unknown or duplicate execution view: {key} {view}")
        selected.add(view)
    for key, selected in groups.items():
        if selected != required:
            raise AssertionError(f"incomplete execution views: {key}")
    states = {}
    count = 0
    for row in rows:
        if row["status"] != "PASS" or row["returncode"] != 0:
            raise AssertionError("non-passing execution remains in results")
        view = f'{row["strategy"]}-{row["target"]}'
        output = a.results / "runs" / row["case"] / row["schedule"] / view
        build = a.results / "build" / view
        result = compare(
            a.programs / row["case"],
            output,
            build / row["target"] / "ports.json",
            basis=row["basis"],
            epoch=1 if row["schedule"] == "reset-waiting-store" else None,
        )
        assert result["status"] == "PASS"
        state = (output / "state.bits").read_text()
        key = row["case"], row["schedule"]
        if key in states:
            assert states[key] == state, f"complete state mismatch: {key} {view}"
        states[key] = state
        count += 1
    for strategy in ["timing", "functional"]:
        native = a.results / "build" / f"{strategy}-cpp/core.ac"
        rtl = a.results / "build" / f"{strategy}-verilog/core.ac"
        assert (
            native.read_bytes() == rtl.read_bytes()
        ), "backends did not consume identical final IR"
    report = {
        "status": "PASS",
        "views": count,
        "complete_state_groups": len(states),
        "same_final_ir_per_strategy": True,
        "scope": "independent retirement/GPR/PC/RAM, exact stores, complete state equality and C-source signature when supplied",
    }
    (a.results / "raw-validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
