"""Run standalone array System and independently verify all lanes/events.

Verilog emission consumes the same final artifact; RTL simulation is excluded.
"""

import argparse
import hashlib
import json
import os
import subprocess
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]


def validate(path, cases):
    records = [
        json.loads(line)
        for line in path.read_text().splitlines()
        if line.startswith("{")
    ]
    terminal = [r for r in records if r.get("kind") == "result"]
    if (
        len(terminal) != 1
        or terminal[0].get("status") != "TERMINATED"
        or "error" not in terminal[0]
        or terminal[0]["error"] is not None
    ):
        raise ValueError("unsuccessful/missing generated terminal result")
    if str(terminal[0]["epoch_time"]) != str(2 * cases["cycles"]):
        raise ValueError("incomplete System horizon")
    events = []
    for record in records:
        if record.get("kind") != "log" or int(record["commit_epoch"]) % 2:
            continue
        event = record.get("spec", {}).get("event")
        if event not in ("array_action", "array_row", "array_result", "array_pass"):
            continue
        values = [int(v["value"]) for v in record["values"]]
        events.append((event, int(record["commit_epoch"]), values))
    actions = [v for e, _, v in events if e == "array_action"]
    if actions != cases["accepted_actions"]:
        mismatch = next(
            (
                i
                for i, (a, b) in enumerate(
                    zip(actions, cases["accepted_actions"], strict=False)
                )
                if a != b
            ),
            min(len(actions), len(cases["accepted_actions"])),
        )
        raise ValueError(
            f"accepted action mismatch at {mismatch}; observed {len(actions)} expected {len(cases['accepted_actions'])}"
        )
    # Free-running schedule cycle is sampled before the corresponding rising edge.
    for event, epoch, values in events:
        if event == "array_action" and epoch != 2 * (values[0] + 1):
            raise ValueError("action cycle differs from successful commit epoch")
    expected_rows = [
        [r["tag"], row, *words]
        for r in cases["results"]
        for row, words in enumerate(r["rows"])
    ]
    if [v for e, _, v in events if e == "array_row"] != expected_rows:
        raise ValueError("all-lane row payload/order mismatch")
    expected_results = [
        [r["tag"], r["steps"], r["useful_macs"]] for r in cases["results"]
    ]
    if [v for e, _, v in events if e == "array_result"] != expected_results:
        raise ValueError("result count/tag/step/useful-MAC mismatch")
    passes = [v for e, _, v in events if e == "array_pass"]
    if passes != [[cases["total_results"]]]:
        raise ValueError("missing/duplicate pass event")
    take_epochs = {
        v[2]: epoch for e, epoch, v in events if e == "array_action" and v[1] == 4
    }
    for event, epoch, values in events:
        if event in ("array_row", "array_result") and epoch != take_epochs[values[0]]:
            raise ValueError("result was published outside its accepted take")
    metrics = []
    for result in cases["results"]:
        tag = result["tag"]
        selected = [v for v in actions if v[2] == tag]
        begin = next(v[0] for v in selected if v[1] == 1)
        end = next(v[0] for v in selected if v[1] == 3)
        take = next(v[0] for v in selected if v[1] == 4)
        steps = [v[0] for v in selected if v[1] == 2]
        metrics.append(
            {
                "tag": tag,
                "accepted_steps": len(steps),
                "useful_macs": result["useful_macs"],
                "begin_to_take_cycles": take - begin,
                "occupied_cycles": take - begin + 1,
                "active_stall_cycles": end - begin - len(steps) - 1,
                "result_wait_cycles": take - end - 1,
                "step_intervals": [
                    b - a for a, b in zip(steps, steps[1:], strict=False)
                ],
            }
        )
    return events, metrics


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--toolchain", type=Path, default=ROOT / ".pycircuit_out/toolchain/install"
    )
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--top", default="array_test.bench.ArraySystem")
    args = parser.parse_args()
    cases = json.loads((HERE / "cases.json").read_text())
    run_id = time.strftime("%Y%m%dT%H%M%S")
    out = ROOT / ".pycircuit_out" / ("fa2-array-" + run_id)
    evidence = ROOT / "docs/gates/logs/fa2-p0t-array" / run_id
    out.mkdir(parents=True, exist_ok=False)
    evidence.mkdir(parents=True, exist_ok=False)
    env = dict(os.environ)
    env["PYC_TOOLCHAIN_ROOT"] = str(args.toolchain)
    env["PYTHONPATH"] = str(args.toolchain / "share/pycircuit/python")
    env["LD_LIBRARY_PATH"] = (
        str(args.toolchain / "lib") + ":" + env.get("LD_LIBRARY_PATH", "")
    )
    driver = args.toolchain / "bin/pycircuit"

    def sha(path):
        return hashlib.sha256(path.read_bytes()).hexdigest()

    sources = ROOT / "designs/sparsefa/src"
    stems = ("fa2_numeric", "fa2_array_interfaces", "fa2_array")
    inputs = [sources / (s + ".py") for s in stems] + [
        HERE / "bench.py",
        HERE / "emit_stimulus.py",
        HERE / "cases.json",
        HERE / "generate.py",
        Path(__file__),
    ]
    inputs += [
        args.toolchain / "bin" / name
        for name in (
            "pycircuit",
            "pycircuit-link",
            "pycircuit-source-unit",
            "pycircuit-emit",
        )
    ]
    inputs += [p for p in args.toolchain.rglob("*") if p.is_file()]
    inputs = list(dict.fromkeys(inputs))
    before = {str(p): sha(p) for p in inputs}
    (evidence / "before.json").write_text(json.dumps(before, indent=2) + "\n")
    commands = []

    def run(name, command):
        command = [str(v) for v in command]
        log = evidence / (name + ".log")
        with log.open("w") as stream:
            status = subprocess.run(
                command, env=env, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT
            ).returncode
        commands.append(
            {"stage": name, "command": command, "exit": status, "log_sha256": sha(log)}
        )
        (evidence / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        if status:
            raise RuntimeError(f"{name} exited {status}; {log}")
        return log

    units = []
    for stem in stems:
        unit = out / stem
        includes = [x for prior in units for x in ("-I", prior)]
        run(
            "compile-" + stem,
            [
                driver,
                "compile",
                "-c",
                sources / (stem + ".py"),
                "--source-root",
                sources,
                "--package-prefix",
                "fa2",
                *includes,
                "-o",
                unit,
            ],
        )
        units.append(unit)
    includes = [x for prior in units for x in ("-I", prior)]
    run(
        "compile-bench",
        [
            driver,
            "compile",
            "-c",
            HERE / "bench.py",
            "--source-root",
            HERE,
            "--package-prefix",
            "array_test",
            *includes,
            "-o",
            out / "bench",
        ],
    )
    final = out / "final.ac"
    run("link", [driver, "link", *units, out / "bench", "--top", args.top, "-o", final])
    final_hash = sha(final)
    for target in ("cpp", "verilog"):
        run(
            "emit-" + target,
            [driver, "emit", final, "--target", target, "-o", out / target],
        )
        assert sha(final) == final_hash
    cxx = os.environ.get(
        "CXX", "/home/xiekunpeng/micromamba/envs/acir/bin/x86_64-conda-linux-gnu-g++"
    )
    run(
        "configure",
        [
            "cmake",
            "-S",
            out / "cpp",
            "-B",
            out / "build",
            f"-DCMAKE_PREFIX_PATH={args.toolchain}",
            f"-DCMAKE_CXX_COMPILER={cxx}",
        ],
    )
    run("build", ["cmake", "--build", out / "build", "-j", args.jobs])
    executable = out / "build/bin/pycircuit_sim"
    executable_hash = sha(executable)
    traces = []
    for workers in (1, 2):
        log = run(
            "workers" + str(workers),
            [
                out / "build/bin/pycircuit_sim",
                "--cycles",
                cases["cycles"],
                "--workers",
                workers,
            ],
        )
        assert sha(executable) == executable_hash, "generated binary changed"
        traces.append(validate(log, cases))
    assert traces[0] == traces[1], "worker traces differ"
    after = {str(p): sha(p) for p in inputs}
    assert before == after, "source/tool changed during execution"
    artifacts = {
        str(p.relative_to(out)): sha(p)
        for p in out.rglob("*")
        if p.is_file() and "build" not in p.relative_to(out).parts
    }
    manifest = {
        "status": "PASS",
        "commands": commands,
        "before": before,
        "after": after,
        "final_sha256": final_hash,
        "generated_executable_sha256": executable_hash,
        "artifacts": artifacts,
        "results": cases["total_results"],
        "lane_checks": 256 * cases["total_results"],
        "metrics": traces[0][1],
        "not_run": cases["not_run"],
    }
    (evidence / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(evidence)  # noqa: T201 -- CLI evidence location.


if __name__ == "__main__":
    main()
