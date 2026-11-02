#!/usr/bin/env python3
"""Copy only public project source and execute a C ELF from a clean directory."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from verify import compare


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--prefix", type=Path, required=True)
    p.add_argument("--program", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    a = p.parse_args()
    a.prefix = a.prefix.resolve()
    a.program = a.program.resolve()
    a.output = a.output.resolve()
    a.output.mkdir(parents=True, exist_ok=True)
    clean = Path(tempfile.mkdtemp(prefix="pycircuit-rv32i-consumer-"))
    project = clean / "project"
    shutil.copytree(
        Path(__file__).resolve().parents[1],
        project,
        ignore=shutil.ignore_patterns("__pycache__", "*.pyc", "image.bin"),
    )
    platform = clean / "platform"
    platform.mkdir()
    shutil.copy2(a.program / "image.bin", platform / "image.bin")
    cfg = json.loads((a.program / "simulation.json").read_text())
    cfg.update(instruction_latency=[0], data_latency=[0])
    (platform / "simulation.json").write_text(json.dumps(cfg, indent=2) + "\n")
    reports = []
    for strategy in ["timing", "functional"]:
        for target in ["cpp", "verilog"]:
            view = f"{strategy}-{target}"
            build = a.output / view
            command = [
                str(a.prefix / "bin/pycircuit"),
                "run",
                str(project),
                "--model",
                "rv32i_core.core.Core",
                "--execution",
                strategy,
                "--target",
                target,
                "--memory",
                str(platform / "simulation.json"),
                "--build-dir",
                str(build),
                "--timeout",
                "180",
            ]
            with (a.output / (view + ".log")).open("w") as log:
                run = subprocess.run(
                    command,
                    cwd=clean,
                    stdout=log,
                    stderr=subprocess.STDOUT,
                    timeout=540,
                )
            if run.returncode:
                raise RuntimeError(view + " failed; see log")
            result = compare(
                a.program, build / "clocked-run", build / target / "ports.json"
            )
            report = dict(
                result,
                strategy=strategy,
                target=target,
                returncode=run.returncode,
                basis="spike",
                command=command,
                executed_ticks=json.loads(
                    (build / "clocked-run/execution.json").read_text()
                )["executed_ticks"],
            )
            (build / "clocked-run/verification.json").write_text(
                json.dumps(report, indent=2) + "\n"
            )
            reports.append(report)
            print(view, result["status"], "retired", result["retired"], flush=True)
    for strategy in ["timing", "functional"]:
        assert (a.output / f"{strategy}-cpp/core.ac").read_bytes() == (
            a.output / f"{strategy}-verilog/core.ac"
        ).read_bytes()
    report = {
        "status": "PASS",
        "clean_source": str(project),
        "prefix": str(a.prefix),
        "same_ir_per_strategy": True,
        "views": reports,
    }
    (a.output / "clean-consumer.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
