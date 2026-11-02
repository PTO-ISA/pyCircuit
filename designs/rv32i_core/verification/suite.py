#!/usr/bin/env python3
"""Public installed CLI, same images, independent reference, retained per-run evidence."""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
from verify import compare


# ndf: verifies=RV32-VER-PIPE-001,RV32-VER-EFFECT-001
def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--prefix", type=Path, required=True)
    p.add_argument("--programs", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--case", action="append")
    p.add_argument("--target", action="append", choices=["cpp", "verilog"])
    p.add_argument("--strategy", action="append", choices=["functional", "timing"])
    p.add_argument(
        "--schedule",
        action="append",
        choices=["fast", "waits", "long-waits", "reset-waiting-store", "held-wb"],
        help="Select existing latency/reset scenarios without changing them",
    )
    a = p.parse_args()
    a.prefix = a.prefix.resolve()
    a.programs = a.programs.resolve()
    a.output = a.output.resolve()
    a.output.mkdir(parents=True, exist_ok=True)
    source = Path(__file__).resolve().parents[1]
    cli = a.prefix / "bin/pycircuit"
    manifest = json.loads((a.programs / "reference-summary.json").read_text())
    reports = []
    traces = {}
    for case in manifest:
        name = case["case"]
        basis = case.get("basis", "spike")
        if a.case and name not in a.case:
            continue
        schedules = [("fast", [0], [0], [])]
        if name in [
            "smoke",
            "families",
            "random-17",
            "c-array",
            "load_access",
            "store_access",
        ]:
            schedules += [
                ("waits", [0, 3, 1, 8], [4, 0, 7, 2], []),
                ("long-waits", [12], [8, 15], []),
            ]
        if name.startswith("held_wb_"):
            # Keep fetch fast so EX captures before producer WB, then hold EX
            # long enough for that producer to disappear from the forwarding bus.
            schedules += [("held-wb", [0], [2], [])]
        if name == "smoke":
            schedules += [("reset-waiting-store", [0], [8], [12])]
        for schedule, imem, dmem, resets in schedules:
            if a.schedule and schedule not in a.schedule:
                continue
            cfgdir = a.output / "inputs" / name / schedule
            cfgdir.mkdir(parents=True, exist_ok=True)
            cfg = json.loads((a.programs / name / "simulation.json").read_text())
            cfg.update(
                image=os.path.relpath(a.programs / name / "image.bin", cfgdir),
                instruction_latency=imem,
                data_latency=dmem,
                reset_at=resets,
            )
            config = cfgdir / "simulation.json"
            config.write_text(json.dumps(cfg, indent=2) + "\n")
            for strategy in a.strategy or ["timing", "functional"]:
                for target in a.target or ["cpp", "verilog"]:
                    build = a.output / "build" / f"{strategy}-{target}"
                    out = a.output / "runs" / name / schedule / f"{strategy}-{target}"
                    out.mkdir(parents=True, exist_ok=True)
                    command = [
                        str(cli),
                        "run",
                        str(source),
                        "--model",
                        "rv32i_core.core.Core",
                        "--execution",
                        strategy,
                        "--target",
                        target,
                        "--toolchain",
                        str(a.prefix),
                        "--memory",
                        str(config),
                        "--build-dir",
                        str(build),
                        "--timeout",
                        "180",
                    ]
                    with (out / "command.log").open("w") as log:
                        run = subprocess.run(
                            command, stdout=log, stderr=subprocess.STDOUT, timeout=540
                        )
                    report = {
                        "case": name,
                        "schedule": schedule,
                        "strategy": strategy,
                        "target": target,
                        "returncode": run.returncode,
                        "basis": basis,
                        "command": command,
                        "status": "FAIL",
                    }
                    try:
                        if run.returncode:
                            raise AssertionError("public run failed; see command.log")
                        current = build / "clocked-run"
                        for file in current.iterdir():
                            if file.is_file():
                                shutil.copy2(file, out / file.name)
                        report.update(
                            compare(
                                a.programs / name,
                                out,
                                build / target / "ports.json",
                                basis=basis,
                                epoch=1 if resets else None,
                            )
                        )
                        execution = json.loads((out / "execution.json").read_text())
                        report.update(
                            executed_ticks=execution["executed_ticks"],
                            skipped_idle_cycles=execution["skipped_idle_cycles"],
                        )
                        rows = [
                            json.loads(line)
                            for line in (out / "cycles.jsonl").read_text().splitlines()
                        ]
                        if name == "throughput":
                            retire = [
                                r["cycle"]
                                for r in rows
                                if r["outputs"]["retire_valid"]
                                and not r["outputs"]["retire_trap"]
                            ]
                            assert retire == list(
                                range(4, 68)
                            ), "independent ALU CPI/latency contract failed"
                            assert (
                                report["max_occupied"] == 5
                                and report["load_use_cycles"] == 0
                            )
                        if schedule == "held-wb":
                            assert report["memory_wait_cycles"] >= 2
                        if name == "smoke" and schedule == "fast":
                            assert report["load_use_cycles"] == 1
                        traces[name, schedule, strategy, target] = [
                            (r["cycle"], r["reset"], r["outputs"]) for r in rows
                        ]
                    except (AssertionError, ValueError, KeyError) as error:
                        report.update(status="FAIL", error=str(error))
                    reports.append(report)
                    (out / "verification.json").write_text(
                        json.dumps(report, indent=2) + "\n"
                    )
                    print(
                        name,
                        schedule,
                        strategy,
                        target,
                        report["status"],
                        report.get("error", ""),
                        flush=True,
                    )
                    (a.output / "summary.json").write_text(
                        json.dumps(reports, indent=2) + "\n"
                    )
    cross = []
    for name, schedule, strategy, target in traces:
        if target == "verilog" and (name, schedule, strategy, "cpp") in traces:
            assert (
                traces[name, schedule, strategy, target]
                == traces[name, schedule, strategy, "cpp"]
            ), f"native/RTL cycle divergence: {name} {schedule} {strategy}"
            cross.append(
                {
                    "case": name,
                    "schedule": schedule,
                    "strategy": strategy,
                    "status": "CYCLE_EQUAL",
                }
            )
    (a.output / "cycle-comparison.json").write_text(json.dumps(cross, indent=2) + "\n")
    print(
        "views",
        len(reports),
        "pass",
        sum(r["status"] == "PASS" for r in reports),
        "cycle_equal",
        len(cross),
        flush=True,
    )
    if not reports:
        raise SystemExit("no requested case/schedule was executed")
    if any(r["status"] != "PASS" for r in reports):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
