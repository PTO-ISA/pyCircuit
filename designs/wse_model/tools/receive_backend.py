"""Execute externally supplied receive requests using a compiled System driver.

This adapter owns stimulus conversion and backend execution, not expected
results. Both backends consume one linked final artifact. ``EXECUTED`` means a
simulator ran; its nonzero exit and failure terminal remain available to the
caller, which decides whether the failure was expected.
"""

from __future__ import annotations

import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RESULT_FIELDS = ("accepted", "status", "fault", "active", "counted", "complete", "pending")
BOOLEAN_RESULTS = {"accepted", "fault", "active", "complete"}
REQUEST_WIDTHS = {"kind": 2, "capacity": 32, "expected": 32, "compiled_expected": 32}
SEGMENT_WIDTHS = {"opcode": 3, "epoch": 8, "address": 64, "nbytes": 32, "source": 6, "seq": 16}
SEGMENT_BOOLEANS = ("committed", "self_sourced")


def _integer(value, width: int, label: str) -> int:
    if type(value) is not int or not 0 <= value < 1 << width:
        raise ValueError(f"{label} must be an unsigned {width}-bit integer")
    return value


def _validate(scenario: dict) -> None:
    if not isinstance(scenario.get("name"), str) or not scenario["name"]:
        raise ValueError("scenario name must be a nonempty string")
    for name in ("early_slots", "seen_slots"):
        if not 1 <= _integer(scenario[name], 8, name) <= 255:
            raise ValueError(f"{name} must be in 1..255")
    requests = scenario["requests"]
    if not isinstance(requests, list) or not 0 < len(requests) < 2**32:
        raise ValueError("requests must be a nonempty list of fewer than 2**32 cycles")
    for cycle, request in enumerate(requests):
        for name, width in REQUEST_WIDTHS.items():
            _integer(request[name], width, f"requests[{cycle}].{name}")
        for name, width in SEGMENT_WIDTHS.items():
            _integer(request["segment"][name], width, f"requests[{cycle}].segment.{name}")
        if request["segment"]["source"] >= 40:
            raise ValueError(
                f"requests[{cycle}].segment.source must be in the 40-node test profile"
            )
        for name in SEGMENT_BOOLEANS:
            if type(request["segment"][name]) is not bool:
                raise ValueError(f"requests[{cycle}].segment.{name} must be Boolean")


def _select(values: list) -> str:
    if all(value == values[0] for value in values):
        return repr(values[0])
    return (
        "("
        + "".join(f"{value!r} if phase == {cycle} else " for cycle, value in enumerate(values[:-1]))
        + repr(values[-1])
        + ")"
    )


def _stimulus_source(scenario: dict) -> str:
    requests = scenario["requests"]
    segment = [
        f"        {name}={_select([request['segment'][name] for request in requests])},"
        for name in (*SEGMENT_WIDTHS, *SEGMENT_BOOLEANS)
    ]
    request_fields = [
        f"        {name}={_select([request[name] for request in requests])},"
        for name in REQUEST_WIDTHS
    ]
    return "\n".join(
        [
            "import pycircuit as ac",
            "from wse_calendar.receive import ReceiveEngine, Request, Segment",
            "",
            "@ac.system",
            "def ExternalReceive():",
            "    phase: ac.u32 = 0",
            "    segment = Segment(",
            *segment,
            "    )",
            "    request = Request(",
            *request_fields,
            "        segment=segment,",
            "    )",
            "    result = ReceiveEngine(request, "
            f"EARLY_SLOTS={scenario['early_slots']}, SEEN_SLOTS={scenario['seen_slots']})",
            "",
            "    @ac.rule",
            "    def observe() -> None:",
            '        ac.log("info", "receive_result", phase, '
            + ", ".join(f"result.{name}" for name in RESULT_FIELDS)
            + ")",
            "        phase = phase + 1",
            "",
        ]
    )


def _decode(stdout: str) -> tuple[list[dict], dict | None]:
    rows = [json.loads(line) for line in stdout.splitlines() if line.startswith("{")]
    samples = []
    for row in rows:
        if row.get("kind") != "log" or row.get("spec", {}).get("event") != "receive_result":
            continue
        values = [item["value"] for item in row["values"]]
        if len(values) != 1 + len(RESULT_FIELDS):
            raise ValueError("receive_result has an unexpected field count")
        result = {}
        for name, value in zip(RESULT_FIELDS, values[1:], strict=True):
            if name in BOOLEAN_RESULTS:
                if value in (True, "true", "1", 1):
                    result[name] = True
                elif value in (False, "false", "0", 0):
                    result[name] = False
                else:
                    raise ValueError(f"{name} is not a known Boolean: {value!r}")
            else:
                result[name] = int(value)
        samples.append(
            {
                "cycle": int(values[0]),
                "evaluation_epoch": int(row["evaluation_epoch"]),
                "commit_epoch": int(row["commit_epoch"]),
                "result": result,
            }
        )
    terminals = [row for row in rows if row.get("kind") == "result"]
    if len(terminals) > 1:
        raise ValueError("simulator produced multiple terminal records")
    return samples, terminals[0] if terminals else None


def run_scenario(scenario: dict, *, pycircuit_root: Path, toolchain: Path, output: Path) -> dict:
    """Compile requests once, run both targets, and retain all stage evidence.

    Input is ``{name, early_slots, seen_slots, requests}``; each request contains
    ``kind, capacity, expected, compiled_expected, segment`` and every Segment
    field from receive.py. Integer fields are checked for representability only;
    protocol-invalid requests remain valid stimuli. Input rejection raises
    ValueError/KeyError before invoking tools. ``output`` must not already exist.

    The returned/saved receipt has ``status`` EXECUTED or BUILD_FAILED, a final
    path/hash, commands, and ``runs`` keyed by cpp/verilog. Each run records its
    process returncode, raw output paths, full terminal event, and samples with
    cycle/evaluation_epoch/commit_epoch plus all seven typed public Result fields.
    A runtime assertion failure still has status EXECUTED; no prefix samples or
    nonzero returncodes are discarded. Build/decode failures are marked separately.
    """
    _validate(scenario)
    output = output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    toolchain, pycircuit_root = toolchain.resolve(), pycircuit_root.resolve()
    source = output / "source"
    source.mkdir()
    source_file = source / "external_receive.py"
    source_file.write_text(_stimulus_source(scenario), encoding="utf-8")
    (output / "input.json").write_text(json.dumps(scenario, indent=2) + "\n")
    receipt = {
        "schema": "wse-model/receive-backend/1",
        "status": "INCOMPLETE",
        "stimulus_kind": "compiled-system",
        "scenario": {key: scenario[key] for key in ("name", "early_slots", "seen_slots")},
        "source": str(source_file),
        "source_sha256": hashlib.sha256(source_file.read_bytes()).hexdigest(),
        "commands": [],
        "runs": {},
    }
    receipt["scenario"]["cycles"] = len(scenario["requests"])
    receipt["input_sha256"] = hashlib.sha256((output / "input.json").read_bytes()).hexdigest()
    receipt["provider_sha256"] = {
        name: hashlib.sha256((ROOT / "models/calendar" / name).read_bytes()).hexdigest()
        for name in ("receive_checks.py", "receive.py")
    }
    env = dict(os.environ)
    env["PYTHONPATH"] = str(pycircuit_root / "python") + os.pathsep + env.get("PYTHONPATH", "")
    env["PYC_TOOLCHAIN_ROOT"] = str(toolchain)
    for variable, binary in (
        ("PYCIRCUIT_SOURCE_COMPILER", "pycircuit-source-unit"),
        ("PYCIRCUIT_LINKER", "pycircuit-link"),
        ("PYCIRCUIT_EMITTER", "pycircuit-emit"),
    ):
        env[variable] = str(toolchain / "bin" / binary)

    def save() -> None:
        (output / "backend.json").write_text(json.dumps(receipt, indent=2) + "\n")

    def command(name: str, arguments: list[str], timeout: int = 300) -> dict:
        record = {"name": name, "arguments": arguments, "returncode": None}
        for stream in ("stdout", "stderr"):
            record[stream] = str(output / f"{name}.{stream}.log")
        receipt["commands"].append(record)
        save()
        with Path(record["stdout"]).open("w") as stdout, Path(record["stderr"]).open("w") as stderr:
            try:
                completed = subprocess.run(
                    arguments,
                    env=env,
                    stdout=stdout,
                    stderr=stderr,
                    timeout=timeout,
                    check=False,
                    cwd=output,
                )
                record["returncode"] = completed.returncode
            except (OSError, subprocess.TimeoutExpired) as error:
                record["error"] = str(error)
        save()
        return record

    cli = [sys.executable, "-m", "pycircuit.cli"]
    (output / "units").mkdir()
    units = []
    for filename, source_root in (
        ("receive_checks.py", ROOT / "models/calendar"),
        ("receive.py", ROOT / "models/calendar"),
        ("external_receive.py", source),
    ):
        unit = output / "units" / Path(filename).stem
        arguments = [
            *cli,
            "compile",
            "-c",
            str(source_root / filename),
            "--source-root",
            str(source_root),
            "--package-prefix",
            "wse_calendar",
            "-o",
            str(unit),
        ]
        for dependency in units:
            arguments += ["-I", str(dependency)]
        if command("compile-" + Path(filename).stem, arguments)["returncode"] != 0:
            receipt["status"] = "BUILD_FAILED"
            save()
            return receipt
        units.append(unit)
    final = output / "final.ac"
    linked = command(
        "link",
        [
            *cli,
            "link",
            *map(str, units),
            "--top",
            "wse_calendar.external_receive.ExternalReceive",
            "-o",
            str(final),
        ],
    )
    if linked["returncode"] != 0:
        receipt["status"] = "BUILD_FAILED"
        save()
        return receipt
    receipt["final"] = {
        "path": str(final),
        "sha256": hashlib.sha256(final.read_bytes()).hexdigest(),
    }
    for target in ("cpp", "verilog"):
        generated, build = output / f"generated-{target}", output / f"build-{target}"
        run = {"status": "BUILD_FAILED", "samples": [], "terminal": None, "returncode": None}
        receipt["runs"][target] = run
        stages = [
            (
                f"emit-{target}",
                [*cli, "emit", str(final), "--target", target, "-o", str(generated)],
            ),
            (
                f"configure-{target}",
                [
                    "cmake",
                    "-S",
                    str(generated),
                    "-B",
                    str(build),
                    "-G",
                    "Ninja",
                    f"-Dpycircuit_DIR={toolchain / 'share/pycircuit/cmake'}",
                ],
            ),
            (
                f"build-{target}",
                ["cmake", "--build", str(build), "--target", "pycircuit_sim", "--parallel", "2"],
            ),
        ]
        for name, arguments in stages:
            if command(name, arguments)["returncode"] != 0:
                break
        else:
            cycles = str(len(scenario["requests"]))
            options = (
                ["--cycles", cycles, "--workers", "1"] if target == "cpp" else ["+cycles=" + cycles]
            )
            record = command("run-" + target, [str(build / "bin/pycircuit_sim"), *options])
            run.update({key: record[key] for key in ("returncode", "stdout", "stderr")})
            run["status"] = "EXECUTED" if "error" not in record else "EXECUTION_FAILED"
            try:
                run["samples"], run["terminal"] = _decode(Path(record["stdout"]).read_text())
            except (ValueError, KeyError, TypeError) as error:
                run["status"] = "DECODE_FAILED"
                run["error"] = str(error)
        save()
    receipt["final"]["unchanged"] = (
        hashlib.sha256(final.read_bytes()).hexdigest() == receipt["final"]["sha256"]
    )
    receipt["status"] = (
        "EXECUTED"
        if all(run["status"] == "EXECUTED" for run in receipt["runs"].values())
        else "BUILD_FAILED"
    )
    save()
    return receipt
