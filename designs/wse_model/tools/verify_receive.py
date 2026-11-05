"""Compare external receive histories against core, C++ and Verilog execution.

Inputs are compiled into a stimulus-only System. Expected values remain host
side. Each scenario starts from reset and both backends use one linked artifact.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

from receive_backend import _validate, run_scenario
from receive_scenarios import boundary_scenarios, expected_trace

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from models.calendar.host_checks import check_run  # noqa: E402


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_digests() -> dict:
    paths = list((ROOT / "src/wse_model").rglob("*.py"))
    paths += [
        ROOT / "models/calendar" / name
        for name in ("receive.py", "receive_checks.py", "host_checks.py")
    ]
    paths += [
        ROOT / "tools" / name
        for name in ("receive_scenarios.py", "receive_backend.py", "verify_receive.py")
    ]
    paths += [ROOT / "models/calendar/pycircuit.lock.json"]
    return {str(p.relative_to(ROOT)): digest(p) for p in sorted(paths)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pycircuit-root", type=Path, required=True)
    parser.add_argument("--toolchain", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument("--case", action="append", help="Select named cases; default runs all")
    selection.add_argument(
        "--input",
        type=Path,
        help="One external scenario JSON with explicit capacities and requests",
    )
    args = parser.parse_args()
    cases = boundary_scenarios()
    if args.input:
        cases = [json.loads(args.input.read_text())]
    if args.case:
        unknown = set(args.case) - {c["name"] for c in cases}
        if unknown:
            parser.error(f"unknown cases: {sorted(unknown)}")
        cases = [c for c in cases if c["name"] in args.case]
    for case in cases:
        _validate(case)
        if not re.fullmatch(r"[a-zA-Z0-9][a-zA-Z0-9_-]*", case["name"]):
            parser.error(
                "scenario names must be plain file names using letters, digits, '-' or '_'"
            )
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    receipt = {
        "status": "INCOMPLETE",
        "sources": source_digests(),
        "cases": [],
        "scope": "40-node carrier, 8-bit epochs, one context; explicit finite capacities; compiled stimuli; C++ one worker and Icarus RTL",
    }

    def save():
        (output / "verification.json").write_text(json.dumps(receipt, indent=2) + "\n")

    save()
    try:
        pin = json.loads((ROOT / "models/calendar/pycircuit.lock.json").read_text())
        revision = subprocess.check_output(
            ["git", "-C", str(args.pycircuit_root), "rev-parse", "HEAD"], text=True
        ).strip()
        dirty = subprocess.check_output(
            ["git", "-C", str(args.pycircuit_root), "status", "--porcelain", "-uno"], text=True
        ).strip()
        metadata = json.loads(
            (args.toolchain / "share/pycircuit/toolchain-metadata.json").read_text()
        )
        if revision != pin["revision"] or dirty:
            raise RuntimeError("pyCircuit must match the clean pinned revision")
        if metadata["git_sha"] != revision or metadata["llvm_version"] != pin["llvm_mlir"]:
            raise RuntimeError("installed toolchain does not match pin")
        receipt.update(pycircuit_revision=revision, toolchain=metadata)
        for case in cases:
            expected = expected_trace(case)
            item = {
                "name": case["name"],
                "status": "INCOMPLETE",
                "expected": expected,
                "early_slots": case["early_slots"],
                "seen_slots": case["seen_slots"],
            }
            receipt["cases"].append(item)
            save()
            actual = run_scenario(
                case,
                pycircuit_root=args.pycircuit_root.resolve(),
                toolchain=args.toolchain.resolve(),
                output=output / case["name"],
            )
            item["backend_receipt"] = str(output / case["name"] / "backend.json")
            try:
                assert actual["status"] == "EXECUTED", actual.get("error", actual["status"])
                assert actual["final"]["unchanged"], "linked artifact changed during execution"
                for target in ("cpp", "verilog"):
                    check_run(actual["runs"][target], expected, target)
                item["status"] = "PASS"
                item["observations_per_backend"] = 2 * len(expected["results"])
                item["final_sha256"] = actual["final"]["sha256"]
            except (AssertionError, KeyError, TypeError) as error:
                item.update(status="FAIL", error=str(error))
            save()
            print(f"{case['name']}: {item['status']}", flush=True)
        assert source_digests() == receipt["sources"], "sources changed during verification"
        receipt["status"] = (
            "PASS" if all(c["status"] == "PASS" for c in receipt["cases"]) else "FAIL"
        )
        save()
        return 0 if receipt["status"] == "PASS" else 1
    except Exception as error:
        receipt["error"] = str(error)
        save()
        raise


if __name__ == "__main__":
    raise SystemExit(main())
