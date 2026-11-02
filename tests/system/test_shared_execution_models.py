"""Independent transaction/state and bounded queue-time expectations.

The oracle implements the declared FIFO latencies/capacities, single worker
II and consumer readiness. It never learns values or event times from DUT
output. C++ and RTL consume the same source units and selected final IR.
"""

from __future__ import annotations

import ast
import json
import os
import subprocess
from collections import deque
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
EXAMPLE = ROOT / "examples/timed_word_pipeline"
pytestmark = pytest.mark.system


def word_requests():
    return [
        {
            "active": 1,
            "request": dict(
                zip(("tag", "enable", "lhs", "rhs", "operation"), row, strict=True)
            ),
        }
        for row in [
            (0, 1, 1, 2, 0),
            (1, 1, 3, 4, 2),
            (2, 0, 99, 101, 0),
            (3, 1, 7, 8, 15),
            (4, 1, 5, 6, 0),
            (5, 1, 7, 8, 0),
            (6, 1, 2, 8, 2),
            (7, 1, 9, 1, 1),
        ]
    ]


def digest_requests():
    return [
        {
            "active": 1,
            "request": dict(zip(("ticket", "live", "value", "mask"), row, strict=True)),
        }
        for row in [
            (0, 1, 4, 3),
            (1, 0, 99, 7),
            (2, 1, 5, 6),
            (3, 1, 7, 0),
            (4, 1, 9, 1),
        ]
    ]


PROFILES = [
    (
        "word_fast",
        "timed_word_pipeline.timed_word_pipeline.TimedWordPipeline",
        2,
        1,
        2,
        3,
        1,
        False,
    ),
    (
        "word_alt",
        "timed_word_pipeline.timed_word_pipeline.TimedWordPipelineAlt",
        1,
        1,
        1,
        1,
        2,
        False,
    ),
    (
        "digest",
        "timed_word_pipeline.alternate_model.TimedDigestPipeline",
        1,
        2,
        2,
        1,
        1,
        True,
    ),
]


def enabled(request, digest):
    return bool(request["live" if digest else "enable"])


def compute(request, total, digest):
    if digest:
        valid = request["mask"] != 0
        value = ((request["value"] ^ request["mask"]) + total) & 0xFFFF if valid else 0
        return {
            "ticket": request["ticket"],
            "ok": int(valid),
            "fault": int(not valid),
            "payload": value,
        }
    operation = request["operation"]
    valid = operation in (0, 1, 2)
    a, b = request["lhs"], request["rhs"]
    value = (
        (a + b if operation == 0 else a - b if operation == 1 else a * b)
        & ((1 << 64) - 1)
        if valid
        else 0
    )
    return {
        "tag": request["tag"],
        "valid": int(valid),
        "invalid": int(not valid),
        "value": value,
    }


def update(state, response, digest):
    state[2] += 1
    if response["ok" if digest else "valid"]:
        state[1] = (
            response["payload"]
            if digest
            else (state[1] + response["value"]) & ((1 << 64) - 1)
        )
    else:
        state[3] += 1


def functional_expectations(rows, digest):
    state = [0, 0, 0, 0]
    events = []
    for row in rows:
        packet = None
        if row["active"] and enabled(row["request"], digest):
            state[0] += 1
            packet = compute(row["request"], state[1], digest)
            update(state, packet, digest)
        events.append((bool(row["active"]), packet, state.copy()))
    return events


def timing_expectations(rows, profile, cycles=24, consumer=8):
    _, _, depth_in, latency_in, depth_out, latency_out, ii, digest = profile
    incoming, outgoing = deque(), deque()
    state, cursor, next_issue = [0, 0, 0, 0], 0, 0
    events, occupancy, issued = [], [], []
    for cycle in range(cycles):
        take = bool(cycle >= consumer and outgoing and outgoing[0][0] <= cycle)
        process = False
        response = None
        if incoming and incoming[0][0] <= cycle and cycle >= next_issue:
            response = compute(incoming[0][1], state[1], digest)
            valid = response["ok" if digest else "valid"]
            process = not valid or len(outgoing) < depth_out or take
        offer = cursor < len(rows)
        request = rows[cursor]["request"] if offer else None
        acknowledge = bool(
            offer
            and (not enabled(request, digest) or len(incoming) < depth_in or process)
        )
        packet = outgoing.popleft()[1] if take else None
        if process:
            incoming.popleft()
            update(state, response, digest)
            issued.append(cycle)
            next_issue = cycle + ii
            if response["ok" if digest else "valid"]:
                outgoing.append((cycle + latency_out, response))
        if acknowledge:
            cursor += 1
            if enabled(request, digest):
                incoming.append((cycle + latency_in, request.copy()))
                state[0] += 1
        assert 0 <= len(incoming) <= depth_in
        assert 0 <= len(outgoing) <= depth_out
        occupancy.append((len(incoming), len(outgoing)))
        events.append((acknowledge, packet, state.copy()))
    assert cursor == len(rows) and not incoming and not outgoing
    assert all(b - a >= ii for a, b in zip(issued, issued[1:], strict=False))
    assert max(n for n, _ in occupancy) == depth_in
    assert max(n for _, n in occupancy) == depth_out
    # Queue-full admission is followed by successful recovery, not just no loss.
    assert any(not item[0] for item in events[:12])
    assert any(item[0] for item in events[consumer:]) or issued[-1] >= consumer
    return events


def packet(bits, port):
    assert bits is not None and set(bits) <= {"0", "1"}, (port["name"], bits)
    raw = int(bits, 2)
    return {
        field["path"][0]: raw >> field["low"] & ((1 << field["width"]) - 1)
        for field in port["fields"]
    }


def compare(directory, expected):
    ports = json.loads(
        (
            directory / "cpp/ports.json"
            if (directory / "cpp").exists()
            else directory / "verilog/ports.json"
        ).read_text()
    )
    completion = next(p for p in ports["outputs"] if p["name"] == "completion")
    trace = [
        json.loads(line)
        for line in (directory / "model-trace.jsonl").read_text().splitlines()
        if line.startswith('{"cycle":')
    ]
    assert len(trace) == len(expected)
    published = []
    for cycle, (record, (ack, response, state)) in enumerate(
        zip(trace, expected, strict=True)
    ):
        assert record["cycle"] == cycle
        outputs = record["outputs"]
        assert outputs["ack"]["available"] == ack, (cycle, outputs, expected[cycle])
        if ack:
            assert outputs["ack"]["bits"] == "1"
        assert outputs["completion"]["available"] == (response is not None), (
            cycle,
            outputs,
            expected[cycle],
        )
        if response is None:
            assert outputs["completion"]["bits"] is None
        else:
            actual = packet(outputs["completion"]["bits"], completion)
            assert actual == response, (cycle, actual, response)
            if response.get("valid", response.get("ok")):
                published.append(response)
        for n, value in enumerate(state):
            item = outputs[f"snapshot_{n}"]
            assert item["available"] and set(item["bits"]) <= {"0", "1"}
            assert int(item["bits"], 2) == value, (cycle, n, item, value)
    return trace, published


@pytest.fixture(scope="module")
def toolchain():
    prefix = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if not prefix or not (Path(prefix) / "bin/pycircuit").is_file():
        pytest.skip("current installed CompilerDev/Runtime is required")
    return Path(prefix)


def run(toolchain, directory, profile, rows, strategy, target, workers=1, success=True):
    directory.parent.mkdir(parents=True, exist_ok=True)
    vectors = directory.parent / (profile[0] + "-requests.json")
    vectors.write_text(json.dumps(rows) + "\n")
    command = [
        str(toolchain / "bin/pycircuit"),
        "run",
        str(EXAMPLE),
        "--execution",
        strategy,
        "--model",
        profile[1],
        "--inputs",
        str(vectors),
        "--target",
        target,
        "--workers",
        str(workers),
        "--toolchain",
        str(toolchain),
        "--build-dir",
        str(directory),
    ]
    if strategy == "timing":
        command += ["--cycles", "24", "--consumer-start", "10" if profile[-1] else "8"]
    result = subprocess.run(command, capture_output=True, text=True, timeout=180)
    (directory.parent / (directory.name + ".log")).write_text(
        result.stdout + result.stderr
    )
    assert (result.returncode == 0) == success, (
        result.stdout[-3000:] + result.stderr[-3000:]
    )
    receipt = json.loads((directory / "run-execution.json").read_text())
    assert (
        receipt["status"] == ("success" if success else "failed")
        and receipt["execution_model"]["strategy"] == strategy
    )


@pytest.mark.parametrize("profile", PROFILES, ids=lambda value: value[0])
def test_one_source_generates_both_execution_strategies(tmp_path, toolchain, profile):
    digest = profile[-1]
    rows = digest_requests() if digest else word_requests()
    expected = {
        "functional": functional_expectations(rows, digest),
        "timing": timing_expectations(rows, profile, consumer=10 if digest else 8),
    }
    results = {}
    for strategy in ("functional", "timing"):
        for target in ("cpp", "verilog"):
            directory = tmp_path / f"{strategy}-{target}"
            run(toolchain, directory, profile, rows, strategy, target)
            results[strategy, target] = compare(directory, expected[strategy])
        assert (tmp_path / f"{strategy}-cpp/bench.ac").read_bytes() == (
            tmp_path / f"{strategy}-verilog/bench.ac"
        ).read_bytes()
        # Reuse the installed/generated native product: only scheduler worker
        # count changes, never model source or a second numerical DUT.
        run(
            toolchain,
            tmp_path / f"{strategy}-cpp",
            profile,
            rows,
            strategy,
            "cpp",
            workers=2,
        )
        assert (
            compare(tmp_path / f"{strategy}-cpp", expected[strategy])
            == results[strategy, "cpp"]
        )
        assert results[strategy, "cpp"] == results[strategy, "verilog"]
    assert results["functional", "cpp"][1] == results["timing", "cpp"][1]
    assert expected["functional"][-1][2] == expected["timing"][-1][2]
    assert expected["functional"][-1][2] == ([4, 18, 4, 1] if digest else [7, 65, 7, 1])
    assert len(results["functional", "cpp"][1]) == (3 if digest else 6)
    if profile[0] == "word_fast":
        for strategy in ("functional", "timing"):
            check_inactive_and_unknown(
                tmp_path / f"{strategy}-cpp", toolchain, strategy
            )
            check_rtl_admission(tmp_path, toolchain, strategy)


def check_rtl_admission(directory, toolchain, strategy):
    ports = json.loads(
        (directory / f"{strategy}-verilog/verilog/ports.json").read_text()
    )
    request = next(p for p in ports["inputs"] if p["name"] == "request")
    disabled = {field["path"][0]: "x" * field["width"] for field in request["fields"]}
    disabled["enable"] = 0
    unknown = dict(word_requests()[0]["request"], lhs="x" * 64)
    rows = [
        {"active": 0, "request": "x" * request["width"]},
        {"active": 1, "request": disabled},
        {"active": 1, "request": unknown},
    ]
    target = directory / f"{strategy}-rtl-admission"
    profile = ("word_admission", *PROFILES[0][1:])
    run(toolchain, target, profile, rows, strategy, "verilog", success=False)
    # The first two logical offers have no effects; the reached unknown third
    # offer fails managed precheck, publishing no successful sample or payload.
    compare(target, [(False, None, [0, 0, 0, 0]), (True, None, [0, 0, 0, 0])])


def check_inactive_and_unknown(directory, toolchain, strategy):
    """Inspect committed generated state after failure, without resetting it."""
    generated = directory / "cpp"
    ports = json.loads((generated / "ports.json").read_text())
    request = next(p for p in ports["inputs"] if p["name"] == "request")
    fields = {field["path"][0]: field for field in request["fields"]}
    executable = directory / "admission_probe"
    command = [
        "c++",
        "-std=c++20",
        "-O1",
        "-pthread",
        "-I" + str(generated),
        "-I" + str(toolchain / "include"),
        str(EXAMPLE / "functional_driver/functional_projection.cpp"),
        str(toolchain / "lib/libpyc6_runtime.a"),
        "-o",
        str(executable),
    ]
    if strategy == "timing":
        command.append("-DPYC_TIMING")
    subprocess.run(command, check=True, capture_output=True, text=True, timeout=120)
    baseline = (directory / "model-inputs/requests.mem").read_text().splitlines()[0][1:]
    result = subprocess.run(
        [
            str(executable),
            baseline,
            str(fields["enable"]["low"]),
            str(fields["lhs"]["low"]),
        ],
        capture_output=True,
        text=True,
        timeout=30,
    )
    (directory / "admission-probe.log").write_text(result.stdout + result.stderr)
    assert result.returncode == 0, result.stdout + result.stderr
    assert result.stdout.count("held_state=1,3,1,0") == 4


def test_unsafe_projection_does_not_replace_published_program(tmp_path, toolchain):
    """A timer, a second effect and a trapping worker are not pure waiting."""
    source = tmp_path / "model.py"
    original = (EXAMPLE / "alternate_model.py").read_text()
    unit, final = tmp_path / "unit", tmp_path / "model.ac"
    cli = str(toolchain / "bin/pycircuit")

    def invoke(arguments):
        result = subprocess.run(
            [cli, *arguments], capture_output=True, text=True, timeout=60
        )
        with (tmp_path / "rejection.log").open("a") as log:
            log.write(result.stdout + result.stderr)
        return result.returncode

    compile_args = [
        "compile",
        "-c",
        str(source),
        "--source-root",
        str(tmp_path),
        "--package-prefix",
        "projection",
        "-o",
        str(unit),
        "--replace",
    ]
    link_args = [
        "link",
        str(unit),
        "--top",
        "projection.model.TimedDigestPipeline",
        "--execution",
        "functional",
        "-o",
        str(final),
        "--replace",
    ]
    source.write_text(original)
    assert invoke(compile_args) == 0
    assert invoke(link_args) == 0
    good = final.read_bytes()

    for mutation in (
        "timer",
        "duplicate_effect",
        "trap",
        "unproved_ii",
        "invalid_interval",
    ):
        tree = ast.parse(original)
        model = next(
            node
            for node in tree.body
            if isinstance(node, ast.FunctionDef) and node.name == "TimedDigestPipeline"
        )
        if mutation == "timer":
            model.body[0:0] = ast.parse("elapsed: ac.u8 = 0").body
            model.body += ast.parse(
                "@ac.rule\ndef tick() -> None:\n    elapsed = elapsed + 1\n"
            ).body
        elif mutation == "duplicate_effect":
            advance = next(
                node
                for node in model.body
                if isinstance(node, ast.FunctionDef) and node.name == "advance"
            )
            advance.body.insert(
                2, ast.parse("duplicate = engine.apply(request)").body[0]
            )
        elif mutation == "trap":
            worker = next(
                node
                for node in tree.body
                if isinstance(node, ast.FunctionDef) and node.name == "DigestWorker"
            )
            process = next(
                node
                for node in worker.body
                if isinstance(node, ast.FunctionDef) and node.name == "apply"
            )
            process.body.insert(0, ast.parse("assert request.value != 0").body[0])
        elif mutation == "unproved_ii":
            declaration = model.decorator_list[0].keywords[0].value
            for n, key in enumerate(declaration.keys):
                if key.value == "interval":
                    declaration.values[n] = ast.Constant(value=2)
                elif key.value == "wait_state":
                    declaration.values[n] = ast.Constant(value="cooldown")
            model.body[0:0] = ast.parse("cooldown: ac.u1 = 0").body
            advance = next(
                node
                for node in model.body
                if isinstance(node, ast.FunctionDef) and node.name == "advance"
            )
            # Setting a private gap twice without clearing it would stop the
            # timed worker after its first request. It is not a legal II2 loop.
            body = advance.body + ast.parse("cooldown = 1\ncooldown = 1").body
            advance.body = [
                ast.If(
                    test=ast.Name(id="cooldown", ctx=ast.Load()),
                    body=[ast.Pass()],
                    orelse=body,
                )
            ]
        else:
            declaration = model.decorator_list[0].keywords[0].value
            index = next(
                n for n, key in enumerate(declaration.keys) if key.value == "interval"
            )
            declaration.values[index] = ast.Constant(value=0)
        source.write_text(ast.unparse(ast.fix_missing_locations(tree)) + "\n")
        compiled = invoke(compile_args)
        if mutation == "invalid_interval":
            assert compiled != 0
        else:
            assert compiled == 0
            assert invoke(link_args) != 0
        assert final.read_bytes() == good
