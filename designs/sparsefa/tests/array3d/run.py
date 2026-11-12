"""Compile the full 3D System, emit both targets, and check independent traces."""

import hashlib
import json
import os
import subprocess
from datetime import datetime, timezone
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
        raise ValueError("unsuccessful/missing terminal result")
    if str(terminal[0]["epoch_time"]) != str(2 * cases["cycles"]):
        raise ValueError("incomplete System horizon")
    events = []
    for record in records:
        if record.get("kind") != "log" or int(record["commit_epoch"]) % 2:
            continue
        event = record.get("spec", {}).get("event")
        if event in ("array3d_action", "array3d_row", "array3d_result", "array3d_pass"):
            events.append(
                (
                    event,
                    int(record["commit_epoch"]),
                    [int(v["value"]) for v in record["values"]],
                )
            )
    actions = [v for e, _, v in events if e == "array3d_action"]
    if actions != cases["expected_actions"]:
        raise ValueError(
            "action admission/advance trace differs from independent schedule"
        )
    for event, epoch, values in events:
        if event == "array3d_action" and epoch != 2 * (values[0] + 1):
            raise ValueError("action cycle is not its rising-edge commit")
    rows = [
        [r["tag"], row, *words]
        for r in cases["results"]
        for row, words in enumerate(r["rows"])
    ]
    if [v for e, _, v in events if e == "array3d_row"] != rows:
        raise ValueError("all-lane payload/order mismatch")
    results = [[r["tag"], r["groups"], r["useful_macs"]] for r in cases["results"]]
    if [v for e, _, v in events if e == "array3d_result"] != results:
        raise ValueError("result tag/group/useful count mismatch")
    passes = [(epoch, v) for e, epoch, v in events if e == "array3d_pass"]
    if passes != [(2 * (len(cases["instructions"]) + 1), [cases["total_results"]])]:
        raise ValueError("pass event missing, duplicated, or mistimed")
    take_epochs = {
        v[3]: epoch for e, epoch, v in events if e == "array3d_action" and v[1] == 3
    }
    for event, epoch, values in events:
        if event in ("array3d_row", "array3d_result") and epoch != take_epochs.get(
            values[0]
        ):
            raise ValueError("result data outside accepted take epoch")
    metrics = []
    for result in cases["results"]:
        tag = result["tag"]
        begin = next(v[0] for v in actions if v[1] == 1 and v[3] == tag)
        take = next(v[0] for v in actions if v[1] == 3 and v[3] == tag)
        groups = [v[0] for v in actions if v[1:4] == [2, 1, tag] and v[5] == 1]
        advances = [
            v for v in actions if v[1] == 2 and begin < v[0] <= result["result_cycle"]
        ]
        metrics.append(
            {
                "tag": tag,
                "groups": len(groups),
                "useful_products": result["useful_macs"],
                "begin_to_take_cycles": take - begin,
                "occupied_edges": take - begin + 1,
                "group_intervals": [
                    b - a for a, b in zip(groups, groups[1:], strict=False)
                ],
                "model_window_advance_edges": len(advances),
                "model_window_invalid_advance_edges": sum(v[5] == 0 for v in advances),
                "model_hold_edges_before_result": result["result_cycle"]
                - begin
                - len(advances),
                "model_result_wait_edges": take - result["result_cycle"] - 1,
            }
        )
    return events, metrics


def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--toolchain", type=Path, default=ROOT / ".pycircuit_out/fa2-resolution-install"
    )
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--top", default="array3d_test.bench.Array3DSystem")
    args = parser.parse_args()
    args.toolchain = args.toolchain.resolve()
    run_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    evidence = ROOT / "docs/gates/logs/fa2-array3d" / run_id
    out = ROOT / ".pycircuit_out" / ("fa2-array3d-" + run_id)
    evidence.mkdir(parents=True)
    out.mkdir(parents=True)

    def sha(p):
        return hashlib.sha256(p.read_bytes()).hexdigest()

    env = dict(os.environ)
    env["PYC_TOOLCHAIN_ROOT"] = str(args.toolchain)
    env["PYTHONPATH"] = str(args.toolchain / "share/pycircuit/python")
    env["LD_LIBRARY_PATH"] = (
        str(args.toolchain / "lib") + ":" + env.get("LD_LIBRARY_PATH", "")
    )
    driver = args.toolchain / "bin/pycircuit"
    sources = ROOT / "designs/sparsefa/src"
    stems = (
        "fa2_numeric",
        "fa2_array_interfaces",
        "fa2_array3d_interfaces",
        "fa2_array3d",
    )
    inputs = [sources / (stem + ".py") for stem in stems]
    inputs += [p for p in HERE.iterdir() if p.suffix in (".py", ".json")]
    inputs += [p for p in args.toolchain.rglob("*") if p.is_file()]
    inputs = list(dict.fromkeys(inputs))
    before = {str(p): sha(p) for p in inputs}
    (evidence / "before.json").write_text(json.dumps(before, indent=2) + "\n")
    commands, traces = [], []
    cases = json.loads((HERE / "cases.json").read_text())
    manifest = {
        "status": "FAIL",
        "scope": "full16x16x16 product/tree pipeline, not full attention",
        "commands": commands,
        "not_run": cases["not_run"],
        "stage": "preflight",
    }

    def run(name, command):
        command = [str(v) for v in command]
        manifest["stage"] = name
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
            raise RuntimeError(f"{name} exited {status}: {log}")
        return log

    try:
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
                "array3d_test",
                *includes,
                "-o",
                out / "bench",
            ],
        )
        final = out / "final.ac"
        run(
            "link",
            [driver, "link", *units, out / "bench", "--top", args.top, "-o", final],
        )
        manifest["final_sha256"] = sha(final)
        for target in ("cpp", "verilog"):
            run(
                "emit-" + target,
                [driver, "emit", final, "--target", target, "-o", out / target],
            )
            assert sha(final) == manifest["final_sha256"]
        cxx = os.environ.get(
            "CXX",
            "/home/xiekunpeng/micromamba/envs/acir/bin/x86_64-conda-linux-gnu-g++",
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
        manifest["executable_sha256"] = sha(executable)
        for workers in (1, 2):
            log = run(
                "workers" + str(workers),
                [executable, "--cycles", cases["cycles"], "--workers", workers],
            )
            manifest["stage"] = "validate-workers" + str(workers)
            traces.append(validate(log, cases))
            assert sha(executable) == manifest["executable_sha256"]
        assert traces[0] == traces[1], "workers differ"
        manifest.update(
            status="PASS",
            stage="complete",
            results=cases["total_results"],
            lane_checks_per_worker=256 * cases["total_results"],
            metrics=traces[0][1],
        )
    except Exception as error:
        manifest["error"] = str(error)
        raise
    finally:
        after = {str(p): sha(p) if p.exists() else None for p in inputs}
        manifest["inputs_unchanged"] = before == after
        manifest["artifacts"] = {
            str(p.relative_to(out)): sha(p)
            for p in out.rglob("*")
            if p.is_file() and "build" not in p.relative_to(out).parts
        }
        if before != after:
            manifest["status"] = "FAIL"
            manifest["input_mutation"] = True
        (evidence / "after.json").write_text(json.dumps(after, indent=2) + "\n")
        (evidence / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        print(evidence)  # noqa: T201 -- CLI evidence path.
        if before != after:
            raise RuntimeError("sources/toolchain changed during run")


if __name__ == "__main__":
    main()
