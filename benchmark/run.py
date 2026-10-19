"""Validate authored pyCircuit modules against unchanged public testbenches.

This measures manually assisted port coverage, not model pass@k or FPGA PPA.
The adapter wires ports only; unsupported lifecycle or port shapes fail closed.
"""

from __future__ import annotations

import argparse
import ast
import hashlib
import json
import os
import re
import signal
import subprocess
import sys
import time
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BENCH = ROOT / "benchmark"


class AdapterUnsupported(ValueError):  # noqa: N818
    pass


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def integer(expression: str) -> int:
    """Read only finite integer arithmetic in emitted port ranges."""
    node = ast.parse(expression, mode="eval").body

    def visit(node):
        if isinstance(node, ast.Constant) and type(node.value) is int:
            return node.value
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub):
            return -visit(node.operand)
        if isinstance(node, ast.BinOp):
            left, right = visit(node.left), visit(node.right)
            if isinstance(node.op, ast.Add):
                return left + right
            if isinstance(node.op, ast.Sub):
                return left - right
            if isinstance(node.op, ast.Mult):
                return left * right
        raise AdapterUnsupported(f"Nonliteral port range: {expression}")

    return visit(node)


def width(declaration: str) -> int:
    ranges = re.findall(r"\[([^:\]]+):([^\]]+)\]", declaration)
    if not ranges:
        return 1
    if len(ranges) != 1:
        raise AdapterUnsupported("Unpacked or multidimensional port")
    left, right = ranges[0]
    return abs(integer(left) - integer(right)) + 1


def header(text: str) -> tuple[str, str, list[dict]]:
    """Discard any supplied implementation after the module declaration."""
    clean = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
    match = re.search(r"\bmodule\s+(\w+)\s*\((.*?)\)\s*;", clean, re.S)
    if not match and ")" not in clean and ";" not in clean:
        # Some upstream code-completion prompts stop after the final port.
        # Supply only the missing terminator, never an implementation body.
        clean = clean.rstrip() + "\n);"
        match = re.search(r"\bmodule\s+(\w+)\s*\((.*?)\)\s*;", clean, re.S)
    if not match:
        raise AdapterUnsupported("Expected a concrete ANSI module declaration")
    ports, direction, declaration = [], None, ""
    for part in match.group(2).split(","):
        part = part.strip()
        kind = re.match(r"(input|output|inout)\b(.*)", part, re.S)
        if kind:
            direction, part = kind.groups()
            declaration = re.sub(r"\b\w+\s*$", "", part).strip()
        name = re.search(r"([A-Za-z_]\w*)\s*$", part)
        if not name or direction not in {"input", "output"}:
            raise AdapterUnsupported(f"Unsupported port: {part}")
        ports.append(
            {"name": name[1], "direction": direction, "width": width(declaration)}
        )
    return match[1], re.sub(r"\breg\b", "wire", match[0]), ports


def adapter(
    prompt: str, ports: dict, solution: dict, corrupt=False, *, root_name="pyc_root"
) -> str:
    """Wire a verified compiler interface to the original benchmark header."""
    _, declaration, original = header(prompt)
    if ports.get("schema_version") != 1:
        raise AdapterUnsupported("Unsupported compiler port-map schema")
    if ports["controls"]:
        raise AdapterUnsupported(
            "Managed PREPARE/check/commit or validity interface cannot be tied off"
        )
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_$]*", root_name):
        raise AdapterUnsupported("Invalid exported root name")
    generated = ports["inputs"] + ports["outputs"]
    if len({p["wire"] for p in generated}) != len(generated):
        raise AdapterUnsupported("Duplicate exported wire")
    for port in generated:
        if (
            not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_$]*", port["wire"])
            or type(port["width"]) is not int
            or port["width"] < 1
        ):
            raise AdapterUnsupported("Invalid exported port")
    inputs = {p["name"]: p for p in original if p["direction"] == "input"}
    outputs = [p for p in original if p["direction"] == "output"]
    declared = [{"name": p["name"], "width": p["width"]} for p in outputs]
    if solution["outputs"] != declared:
        raise AdapterUnsupported(
            "Solution output order/width differs from original header"
        )
    result_width = sum(p["width"] for p in outputs)
    if not ports["outputs"] or any(
        p["availability"] is not None for p in ports["outputs"]
    ):
        raise AdapterUnsupported("Expected total scalar or packed results")
    if sum(p["width"] for p in ports["outputs"]) != result_width:
        raise AdapterUnsupported("Generated result width differs from original outputs")
    bindings, consumed = [], set()
    for port in ports["inputs"]:
        name = port["name"]
        roles = [
            role
            for role, ordinal in ports["domains"].items()
            if ordinal is not None and ordinal == port["ordinal"]
        ]
        if len(roles) > 1:
            raise AdapterUnsupported("Clock and reset ordinals overlap")
        if roles:
            role = roles[0]
            domain = solution.get("domain", {})
            if domain.get("edge") != "posedge" or domain.get("reset_active") != "high":
                raise AdapterUnsupported("Unsupported compiler-domain timing contract")
            external = domain.get(role)
            if role == "reset" and external is None:
                bindings.append(f".{port['wire']}(1'b0)")
                continue
            if (
                external not in inputs
                or inputs[external]["width"] != 1
                or port["width"] != 1
            ):
                raise AdapterUnsupported(f"Unmatched external {role}")
        else:
            external = solution.get("inputs", {}).get(name, name)
            if external not in inputs and external.endswith("_"):
                external = external[:-1]
            if external not in inputs or port["width"] != inputs[external]["width"]:
                raise AdapterUnsupported(f"Unmatched input/width: {name}")
        if external in consumed:
            raise AdapterUnsupported("Original inputs must be connected exactly once")
        consumed.add(external)
        bindings.append(f".{port['wire']}({external})")
    if consumed != set(inputs):
        raise AdapterUnsupported("Not every original input is connected")
    remaining = result_width
    for port in ports["outputs"]:
        remaining -= port["width"]
        bindings.append(f".{port['wire']}(pyc_result[{remaining} +: {port['width']}])")
    destinations = ", ".join(p["name"] for p in outputs)
    value = "~pyc_result" if corrupt else "pyc_result"
    return (
        declaration
        + f"\nwire [{result_width - 1}:0] pyc_result;\n"
        + root_name
        + " implementation("
        + ", ".join(bindings)
        + ");\n"
        + "assign {"
        + destinations
        + "} = "
        + value
        + ";\nendmodule\n"
    )


def judge(suite: str, stdout: str) -> tuple[bool, dict]:
    if "PYCIRCUIT_PHYSICAL_FAULT" in stdout:
        return False, {"reason": "compiler_physical_fault"}
    if suite == "verilogeval":
        matches = re.findall(r"Mismatches: (\d+) in (\d+) samples", stdout)
        if len(matches) != 1:
            return False, {"reason": "missing_or_ambiguous_upstream_result"}
        errors, samples = map(int, matches[0])
        return errors == 0 and samples > 0, {"mismatches": errors, "samples": samples}
    # The upstream ResBench functional driver checks this exact success marker.
    success = any(
        re.fullmatch(r"All tests passed[.!]?", line.strip())
        for line in stdout.splitlines()
    )
    failed = any(
        re.fullmatch(r"Some tests failed[.!]?", line.strip())
        for line in stdout.splitlines()
    )
    return success and not failed, {"criterion": "standalone upstream success marker"}


def problems(suite: str) -> dict:
    if suite == "verilogeval":
        path = BENCH / "upstream/verilogeval-2023/data/VerilogEval_Human.jsonl"
        return {
            row["task_id"]: {"prompt": row["prompt"], "test": row["test"], "top": "tb"}
            for row in map(json.loads, path.read_text().splitlines())
        }
    data = json.loads((BENCH / "upstream/resbench/problems.json").read_text())
    return {
        row["module"]: {
            "prompt": row.get("Module header", row.get("Module Header")),
            "test": row["Testbench"],
            "top": re.search(r"\bmodule\s+(\w+)", row["Testbench"])[1],
        }
        for rows in data.values()
        for row in rows
    }


def command(args: list, env: dict, directory: Path, label: str, timeout: float) -> dict:
    args = list(map(str, args))
    started = time.monotonic()
    with (
        (directory / (label + ".stdout")).open("w") as out,
        (directory / (label + ".stderr")).open("w") as err,
    ):
        process = subprocess.Popen(
            args, cwd=directory, env=env, stdout=out, stderr=err, start_new_session=True
        )
        try:
            code = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
            code = None
    return {
        "command": args,
        "exit_code": code,
        "seconds": time.monotonic() - started,
        "stdout": label + ".stdout",
        "stderr": label + ".stderr",
    }


def evaluate(
    solution: dict,
    problem: dict,
    suite: str,
    output: Path,
    build: Path,
    timeout: float,
    negative_control: bool,
) -> dict:
    task = solution["task_id"]
    row = {"task_id": task, "status": "not_attempted", "commands": []}
    if solution.get("status") not in {"supported", "implemented"}:
        return row | {
            "status": "unsupported",
            "reason": solution.get("reason", "authoring unsupported"),
        }
    folder = output / task
    folder.mkdir()
    source = ROOT / solution["source"]
    env = dict(os.environ, PYTHONPATH=str(ROOT / "python"), PYTHONDONTWRITEBYTECODE="1")
    for key, name in (
        ("PYCIRCUIT_SOURCE_COMPILER", "pycircuit-source-unit"),
        ("PYCIRCUIT_LINKER", "pycircuit-link"),
        ("PYCIRCUIT_EMITTER", "pycircuit-emit"),
    ):
        env[key] = str(build / "bin" / name)
    row["source_sha256"] = sha256(source)
    reset_semantics = solution.get("domain", {}).get("reset_semantics", "strict")
    if reset_semantics not in {"strict", "if_one"}:
        return row | {"status": "unsupported", "reason": "Invalid reset semantics"}
    row["reset_semantics"] = reset_semantics
    final = folder / "final.ac"
    cli = [sys.executable, "-m", "pycircuit.cli"]
    stages = [
        (
            "compile",
            [
                *cli,
                "compile",
                "-c",
                source,
                "--source-root",
                source.parent,
                "--package-prefix",
                suite,
                "-o",
                folder / "unit",
            ],
        ),
        (
            "link",
            [
                *cli,
                "link",
                folder / "unit",
                "--top",
                solution["top"],
                "--reset-semantics",
                reset_semantics,
                "-o",
                final,
            ],
        ),
        ("emit-cpp", [*cli, "emit", final, "--target", "cpp", "-o", folder / "cpp"]),
        (
            "emit-verilog",
            [
                *cli,
                "emit",
                final,
                "--target",
                "verilog",
                *(["--rtl-mode", "physical"] if solution.get("domain") else []),
                "-o",
                folder / "rtl",
            ],
        ),
    ]
    for stage, args in stages:
        outcome = command(args, env, folder, stage, max(timeout, 120))
        row["commands"].append(outcome)
        if outcome["exit_code"] != 0:
            return row | {
                "status": stage + "_failed",
                "diagnostic": (folder / outcome["stderr"]).read_text()[-4000:],
            }
    row["final_ir_sha256"] = sha256(final)
    manifest = json.loads((folder / "rtl/generated.json").read_text())
    if manifest.get("reset_semantics", "strict") != reset_semantics:
        return row | {
            "status": "emit-verilog_failed",
            "reason": "Reset policy mismatch",
        }
    rtl = [
        folder / "rtl" / item["path"]
        for item in manifest["files"]
        if item["path"].endswith((".v", ".sv"))
    ]
    ports = json.loads((folder / "rtl/ports.json").read_text())
    try:
        adapter(problem["prompt"], ports, solution, root_name=manifest["root_rtl_name"])
    except AdapterUnsupported as error:
        return row | {"status": "adapter_unsupported", "reason": str(error)}
    tb = folder / "upstream_tb.sv"
    tb.write_text(problem["test"])
    row["testbench_sha256"] = sha256(tb)
    row["rtl_sha256"] = {str(path.relative_to(folder)): sha256(path) for path in rtl}
    for corrupt in [False, True] if negative_control else [False]:
        prefix = "negative-" if corrupt else ""
        wrapper_path = folder / (prefix + "adapter.sv")
        wrapper_path.write_text(
            adapter(
                problem["prompt"],
                ports,
                solution,
                corrupt,
                root_name=manifest["root_rtl_name"],
            )
        )
        binary = folder / (prefix + "test.vvp")
        args = [
            "iverilog",
            "-Wall",
            "-Winfloop",
            "-Wno-timescale",
            "-g2012",
            "-s",
            problem["top"],
            "-o",
            binary,
            *rtl,
            *(
                [ROOT / "include/verilog/dff.v", ROOT / "include/verilog/dffe.v"]
                if solution.get("domain")
                else []
            ),
            wrapper_path,
            tb,
        ]
        compile_result = command(args, env, folder, prefix + "iverilog", timeout)
        row["commands"].append(compile_result)
        if compile_result["exit_code"] != 0:
            return row | {
                "status": prefix + "rtl_compile_failed",
                "diagnostic": (folder / compile_result["stderr"]).read_text()[-4000:],
            }
        result = command(
            ["vvp", "-n", binary], env, folder, prefix + "simulation", timeout
        )
        row["commands"].append(result)
        transcript = (folder / result["stdout"]).read_text()
        diagnostics = (folder / result["stderr"]).read_text()
        passed, details = judge(suite, transcript)
        if "PYCIRCUIT_PHYSICAL_FAULT" in diagnostics:
            passed, details = False, {"reason": "compiler_physical_fault"}
        if corrupt:
            row["negative_control"] = {
                "detected": result["exit_code"] == 0 and not passed,
                **details,
            }
            if not row["negative_control"]["detected"]:
                return row | {"status": "negative_control_failed"}
        else:
            row.update(
                status=(
                    "passed"
                    if result["exit_code"] == 0 and passed
                    else "behavior_failed"
                ),
                **details,
            )
            if row["status"] != "passed":
                return row
    row["adapter_sha256"] = sha256(folder / "adapter.sv")
    assert sha256(source) == row["source_sha256"], "source changed during evaluation"
    return row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite", choices=["verilogeval", "resbench"], required=True)
    parser.add_argument("--tasks", nargs="*")
    parser.add_argument(
        "--build", type=Path, default=ROOT / ".pycircuit_out/toolchain/build"
    )
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--negative-control", action="store_true")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    pool = problems(args.suite)
    mapping = json.loads((BENCH / "solutions" / (args.suite + ".json")).read_text())
    selected = [
        row for row in mapping if args.tasks is None or row["task_id"] in args.tasks
    ]
    if not selected or (
        args.tasks is not None
        and set(args.tasks) != {row["task_id"] for row in selected}
    ):
        parser.error("selection is empty or contains unmapped task IDs")
    results = []
    with ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = [
            executor.submit(
                evaluate,
                row,
                pool[row["task_id"]],
                args.suite,
                output,
                args.build.resolve(),
                args.timeout,
                args.negative_control,
            )
            for row in selected
        ]
        for future in futures:
            row = future.result()
            results.append(row)
            print(row["task_id"], row["status"], flush=True)
            (output / "results.jsonl").write_text(
                "".join(json.dumps(item) + "\n" for item in results)
            )
    counts = Counter(row["status"] for row in results)
    attempted = len(results) - counts["unsupported"] - counts["not_attempted"]
    report = {
        "suite": args.suite,
        "metric": "assisted_pycircuit_port_validation_not_model_pass_at_k",
        "full_task_count": len(pool),
        "selected": len(results),
        "attempted": attempted,
        "counts": dict(counts),
        "reset_policy_counts": {
            policy: dict(
                Counter(
                    row["status"]
                    for row in results
                    if row.get("reset_semantics") == policy
                )
            )
            for policy in ("strict", "if_one")
        },
        "passed_over_full_suite": counts["passed"] / len(pool),
        "passed_over_attempted": counts["passed"] / attempted if attempted else None,
        "unselected": len(pool) - len(results),
        "cpp": "emitted, not simulated",
        "resource_score": "not_run",
        "runner_sha256": sha256(Path(__file__)),
        "solutions_manifest_sha256": sha256(
            BENCH / "solutions" / (args.suite + ".json")
        ),
        "sources_lock_sha256": sha256(BENCH / "sources.lock.json"),
        "tool_sha256": {
            name: sha256(args.build / "bin" / name)
            for name in ("pycircuit-source-unit", "pycircuit-link", "pycircuit-emit")
        },
    }
    (output / "summary.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
