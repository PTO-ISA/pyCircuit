"""Compile the pyCircuit timing source and verify generated C++ observations."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parent
SOURCE = ROOT / "designs/sparsefa/src/fa2_shared_kv_timing.py"
BENCH = HERE / "bench.py"
TOOLCHAIN = Path(
    os.environ.get("PYC_TOOLCHAIN_ROOT", ROOT / ".pycircuit_out/toolchain/install")
).resolve()
DRIVER = TOOLCHAIN / "bin/pycircuit"
CXX = (
    os.environ.get("CXX")
    or shutil.which("x86_64-conda-linux-gnu-g++")
    or shutil.which("g++")
)
CC = (
    os.environ.get("CC")
    or shutil.which("x86_64-conda-linux-gnu-gcc")
    or shutil.which("gcc")
)
if not CXX or not CC:
    raise RuntimeError("a C and C++ compiler are required; set CC and CXX")
CASES = {
    "128-1slot": ("SharedKV128One", 128, 1, 5000),
    "128-2slot": ("SharedKV128Two", 128, 2, 5000),
    "2048-1slot": ("SharedKV2048One", 2048, 1, 60000),
    "2048-2slot": ("SharedKV2048Two", 2048, 2, 60000),
}
FIELDS = (
    "count",
    "kv_slots",
    "cycle",
    "index_allocated",
    "index_released",
    "index_loaded",
    "dispatched",
    "filled",
    "retired",
    "requests",
    "outstanding",
    "stage",
)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_trace(
    path: Path, count: int, slots: int, cycles: int
) -> tuple[list[dict], int]:
    records = [
        json.loads(line)
        for line in path.read_text().splitlines()
        if line.startswith("{")
    ]
    results = [record for record in records if record.get("kind") == "result"]
    if (
        len(results) != 1
        or results[0].get("status") != "TERMINATED"
        or results[0].get("error")
    ):
        raise ValueError(f"generated C++ did not terminate successfully: {path}")
    if int(results[0]["epoch_time"]) != cycles * 2:
        raise ValueError("generated C++ stopped before requested cycle limit")
    events = []
    seen = set()
    for record in records:
        if record.get("kind") != "log":
            continue
        event = record["spec"]["event"]
        if event not in ("skv_generated_progress", "skv_generated_done"):
            continue
        values = [int(item["value"]) for item in record["values"]]
        if event == "skv_generated_progress":
            item = dict(zip(FIELDS, values, strict=True))
        else:
            item = dict(zip(FIELDS[:10], values, strict=True))
        if item["count"] != count or item["kv_slots"] != slots:
            raise ValueError("generated event belongs to another scenario")
        item["event"] = event
        identity = (event, tuple(item.items()))
        if identity not in seen:
            seen.add(identity)
            events.append(item)
    progress = [item for item in events if item["event"] == "skv_generated_progress"]
    done = [item for item in events if item["event"] == "skv_generated_done"]
    if not progress or len(done) != 1:
        raise ValueError("missing progress or unique completion event")
    previous = None
    batches = count // 64
    for item in progress:
        if previous and item["cycle"] < previous["cycle"]:
            raise ValueError("generated cycles decreased")
        for field in (
            "index_allocated",
            "index_released",
            "index_loaded",
            "dispatched",
            "filled",
            "retired",
            "requests",
        ):
            if previous and item[field] < previous[field]:
                raise ValueError(f"generated {field} decreased")
        if not (
            0
            <= item["index_released"]
            <= item["index_loaded"]
            <= item["index_allocated"]
            <= batches
            and item["index_allocated"] - item["index_released"] <= 2
            and item["index_released"] * 64 <= item["requests"] <= count
            and 0
            <= item["retired"]
            <= item["filled"]
            <= item["dispatched"]
            <= item["index_loaded"]
            and item["dispatched"] - item["retired"] <= slots
            and 0 <= item["outstanding"] <= 8
            and 0 <= item["stage"] <= 6
        ):
            raise ValueError(f"resource bound violation at cycle {item['cycle']}")
        previous = item
    last = done[0]
    if not (
        0 < last["cycle"] < cycles
        and last["index_allocated"] == batches
        and last["index_released"] == batches
        and last["index_loaded"] == batches
        and last["dispatched"] == batches
        and last["filled"] == batches
        and last["retired"] == batches
        and last["requests"] == count
    ):
        raise ValueError(
            "generated completion did not account for every batch and KV request"
        )
    if batches > 1 and not any(
        item["dispatched"] > 0 and item["index_loaded"] < batches for item in progress
    ):
        raise ValueError("KV dispatch waited for entire Index list")
    return events, last["cycle"]


def write_csv(path: Path, events: list[dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(
            stream, fieldnames=("event", *FIELDS), lineterminator="\n"
        )
        writer.writeheader()
        writer.writerows(events)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--case", action="append", choices=CASES)
    parser.add_argument(
        "--workers", type=int, nargs="+", choices=(1, 2), default=[1, 2]
    )
    parser.add_argument(
        "--snapshot", action="store_true", help="write validated CSV assets"
    )
    options = parser.parse_args()
    selected = options.case or list(CASES)
    run_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    out = ROOT / ".pycircuit_out" / f"sparsefa-pyc-timing-{run_id}"
    evidence = ROOT / "docs/gates/logs" / f"sparsefa-pyc-timing-{run_id}"
    out.mkdir(parents=True)
    evidence.mkdir(parents=True)
    env = os.environ.copy()
    native_lib = Path(CXX).resolve().parent.parent / "lib"
    env["LD_LIBRARY_PATH"] = os.pathsep.join(
        str(path)
        for path in (native_lib, TOOLCHAIN / "lib", env.get("LD_LIBRARY_PATH", ""))
        if path
    )
    env["PYTHONPATH"] = str(TOOLCHAIN / "share/pycircuit/python")
    env["PYC_TOOLCHAIN_ROOT"] = str(TOOLCHAIN)
    report = {
        "head": subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True
        ).strip(),
        "source_sha256": sha(SOURCE),
        "bench_sha256": sha(BENCH),
        "runner_sha256": sha(Path(__file__)),
        "toolchain_sha256": sha(DRIVER),
        "cc": CC,
        "cxx": CXX,
        "commands": [],
        "cases": {},
        "status": "RUNNING",
        "scope": "pyCircuit-generated abstract resource timing; no numerical Shared-KV datapath",
        "rtl_simulation": "NOT RUN: user excluded Verilog simulation",
    }

    def run(name: str, argv: list[str | Path]) -> Path:
        argv = [str(arg) for arg in argv]
        log = evidence / f"{name}.log"
        with log.open("w") as stream:
            result = subprocess.run(
                argv, cwd=ROOT, env=env, stdout=stream, stderr=subprocess.STDOUT
            )
        report["commands"].append(
            {
                "name": name,
                "argv": argv,
                "exit": result.returncode,
                "log": str(log.relative_to(ROOT)),
            }
        )
        print(name, "exit", result.returncode, flush=True)
        if result.returncode:
            raise RuntimeError(f"{name} failed: {log}")
        return log

    snapshot_events = {}
    try:
        model = out / "model"
        bench = out / "bench"
        run(
            "compile-model",
            [
                DRIVER,
                "compile",
                "-c",
                SOURCE,
                "--source-root",
                SOURCE.parent,
                "--package-prefix",
                "sparsefa",
                "-o",
                model,
            ],
        )
        run(
            "compile-bench",
            [
                DRIVER,
                "compile",
                "-c",
                BENCH,
                "--source-root",
                HERE,
                "--package-prefix",
                "generated_timing",
                "-I",
                model,
                "-o",
                bench,
            ],
        )
        for case in selected:
            top, count, slots, cycles = CASES[case]
            final = out / f"{case}.ac"
            run(
                f"{case}-link",
                [
                    DRIVER,
                    "link",
                    model,
                    bench,
                    "--top",
                    f"generated_timing.bench.{top}",
                    "-o",
                    final,
                ],
            )
            final_hash = sha(final)
            for target in ("cpp", "verilog"):
                run(
                    f"{case}-emit-{target}",
                    [
                        DRIVER,
                        "emit",
                        final,
                        "--target",
                        target,
                        "-o",
                        out / f"{case}-{target}",
                    ],
                )
                if sha(final) != final_hash:
                    raise RuntimeError("verified final changed between emit targets")
            build = out / f"{case}-build"
            run(
                f"{case}-configure",
                [
                    "cmake",
                    "-S",
                    out / f"{case}-cpp",
                    "-B",
                    build,
                    f"-DCMAKE_PREFIX_PATH={TOOLCHAIN}",
                    f"-DCMAKE_CXX_COMPILER={CXX}",
                    f"-DCMAKE_C_COMPILER={CC}",
                ],
            )
            run(f"{case}-build", ["cmake", "--build", build, "-j2"])
            binary = build / "bin/pycircuit_sim"
            traces = []
            for worker in options.workers:
                log = run(
                    f"{case}-workers{worker}",
                    [binary, "--cycles", str(cycles), "--workers", str(worker)],
                )
                traces.append(parse_trace(log, count, slots, cycles))
            if any(trace != traces[0] for trace in traces[1:]):
                raise RuntimeError(f"{case}: workers disagree")
            events, completion = traces[0]
            report["cases"][case] = {
                "count": count,
                "kv_slots": slots,
                "completion_cycle": completion,
                "events": len(events),
                "final_sha256": final_hash,
                "executable_sha256": sha(binary),
                "workers": options.workers,
            }
            snapshot_events[case] = events
        for count in (128, 2048):
            one = report["cases"].get(f"{count}-1slot")
            two = report["cases"].get(f"{count}-2slot")
            if one and two and two["completion_cycle"] >= one["completion_cycle"]:
                raise RuntimeError(
                    f"{count}: second KV slot did not improve generated schedule"
                )
        if not (
            sha(SOURCE) == report["source_sha256"]
            and sha(BENCH) == report["bench_sha256"]
            and sha(Path(__file__)) == report["runner_sha256"]
            and sha(DRIVER) == report["toolchain_sha256"]
        ):
            raise RuntimeError(
                "source, bench, runner or toolchain changed during execution"
            )
        if options.snapshot:
            for case, events in snapshot_events.items():
                write_csv(HERE / "assets" / f"sparsefa-pyc-{case}.csv", events)
        report["status"] = "PASS"
        print(json.dumps(report["cases"], indent=2))
    except Exception as error:
        report["status"] = "FAIL"
        report["error"] = str(error)
        raise
    finally:
        report["sources_unchanged"] = (
            sha(SOURCE) == report["source_sha256"]
            and sha(BENCH) == report["bench_sha256"]
            and sha(Path(__file__)) == report["runner_sha256"]
            and sha(DRIVER) == report["toolchain_sha256"]
        )
        if not report["sources_unchanged"]:
            report["status"] = "FAIL"
        (evidence / "manifest.json").write_text(json.dumps(report, indent=2) + "\n")
        print("evidence", evidence, flush=True)


if __name__ == "__main__":
    main()
