#!/usr/bin/env python3
"""Check the NDF design contract, then independently execute selected programs."""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path, required=True)
    parser.add_argument("--ndf", type=Path, required=True)
    parser.add_argument("--clang", type=Path, required=True)
    parser.add_argument("--spike", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    source = Path(__file__).resolve().parents[1]
    scripts = source / "verification"
    cli = args.prefix.resolve() / "bin/pycircuit"
    environment = dict(os.environ)
    environment.pop("PYTHONPATH", None)
    environment.pop("PYCIRCUIT_NATIVE_COMPILER", None)
    environment.pop("PYCIRCUIT_NATIVE_OPT", None)
    record = {"status": "INCOMPLETE", "approval": "NOT_RECORDED", "stages": []}
    summary = args.output / "workflow.json"
    summary.write_text(json.dumps(record, indent=2) + "\n")

    def run(stage, command):
        with (args.output / f"{stage}.log").open("w") as log:
            result = subprocess.run(
                command, env=environment, stdout=log, stderr=subprocess.STDOUT
            )
        record["stages"].append(
            {"stage": stage, "command": command, "rc": result.returncode}
        )
        summary.write_text(json.dumps(record, indent=2) + "\n")
        if result.returncode:
            raise SystemExit(f"{stage} failed ({result.returncode}); see {log.name}")

    run(
        "spec",
        [
            str(cli),
            "spec-check",
            str(source),
            "--ndf",
            str(args.ndf.resolve()),
            "--output",
            str(args.output / "traceability"),
        ],
    )
    programs = args.output / "programs"
    selected = [
        "families",
        "throughput",
        "smoke",
        "c-array",
        "ecall",
        "illegal_fence_i",
    ]
    cases = [value for case in selected for value in ("--case", case)]
    run(
        "prepare",
        [
            sys.executable,
            str(scripts / "prepare.py"),
            "--clang",
            str(args.clang.resolve()),
            "--spike",
            str(args.spike.resolve()),
            "--output",
            str(programs),
            *cases,
        ],
    )
    results = args.output / "validation"
    run(
        "execute",
        [
            sys.executable,
            str(scripts / "suite.py"),
            "--prefix",
            str(args.prefix.resolve()),
            "--programs",
            str(programs),
            "--output",
            str(results),
            "--schedule",
            "fast",
            "--schedule",
            "reset-waiting-store",
            *cases,
        ],
    )
    run(
        "independent-check",
        [
            sys.executable,
            str(scripts / "check_results.py"),
            "--programs",
            str(programs),
            "--results",
            str(results),
        ],
    )
    verified = json.loads((results / "raw-validation.json").read_text())
    record.update(
        status="PASS",
        scope="Selected programs and scenarios; not full ISA certification or Spec approval",
        checks=verified,
    )
    summary.write_text(json.dumps(record, indent=2) + "\n")
    revision = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=source, text=True
    ).strip()
    # Standard NDF evidence, scoped by its exact command. It does not change
    # the lifecycle state of the candidate requirements.
    evidence = {
        "format_version": "0.1",
        "id": "RV32-EVID-WORKFLOW-001",
        "command": " ".join(sys.argv),
        "result": "pass",
        "tool_version": "pyCircuit shared execution + NDF 0.2",
        "source_revision": revision,
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "links": [
            {"clause": identifier, "type": "evidenced-by"}
            for identifier in [
                "RV32-VER-ISA-001",
                "RV32-VER-PIPE-001",
                "RV32-VER-EFFECT-001",
                "RV32-VER-VIEWS-001",
            ]
        ],
    }
    (args.output / "evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")
    print(json.dumps(record, indent=2))


if __name__ == "__main__":
    main()
