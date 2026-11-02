"""Clean installed-project flow and independent FIFO/state expectations."""

import json
import os
import subprocess
from collections import deque
from pathlib import Path

import pytest

pytestmark = pytest.mark.system


def test_installed_model_project(tmp_path):
    prefix = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if not prefix:
        pytest.skip("installed CompilerDev/Runtime required")
    cli = str(Path(prefix) / "bin/pycircuit")
    project, build = tmp_path / "project", tmp_path / "build"

    def invoke(*args, success=True):
        result = subprocess.run(
            [cli, *map(str, args)], text=True, capture_output=True, timeout=180
        )
        with (tmp_path / "commands.log").open("a") as stream:
            stream.write(result.stdout + result.stderr)
        assert (result.returncode == 0) == success, (
            result.stdout[-2500:] + result.stderr[-2500:]
        )
        return result

    invoke("init", project, "--template", "backpressure")
    invoke("verify", project, "--build-dir", build, "--toolchain", prefix, "--rtl")
    result = json.loads((build / "verification.json").read_text())
    assert result["status"] == "passed" and len(result["views"]) == 4
    assert result["same_final_ir_per_strategy"] is True
    assert all(view["final_state"] == [4, 27912, 4, 1] for view in result["views"])
    # Independent discrete reference: input depth2/L1, output depth1/L2,
    # II1, consumer waits until6. No DUT event times or outputs enter this model.
    requests = json.loads((project / "requests.json").read_text())
    incoming, outgoing = deque(), deque()
    state, cursor, stalled = [0, 0, 0, 0], 0, False
    expected = []
    for cycle in range(16):
        take = bool(cycle >= 6 and outgoing and outgoing[0][0] <= cycle)
        process = bool(
            incoming
            and incoming[0][0] <= cycle
            and (incoming[0][1]["mask"] == 0 or len(outgoing) < 1 or take)
        )
        current = requests[cursor] if cursor < len(requests) else None
        live = bool(current and current["active"])
        ack = bool(
            live and (not current["request"]["enable"] or len(incoming) < 2 or process)
        )
        packet = outgoing.popleft()[1] if take else None
        if process:
            _, request = incoming.popleft()
            state[2] += 1
            if request["mask"]:
                state[1] = (state[1] + (request["value"] ^ request["mask"])) & 0xFFFF
                outgoing.append(
                    (
                        cycle + 2,
                        {
                            "tag": request["tag"],
                            "valid": 1,
                            "invalid": 0,
                            "value": state[1],
                        },
                    )
                )
            else:
                state[3] += 1
        if current and (ack or not live):
            cursor += 1
            if live and current["request"]["enable"]:
                incoming.append((cycle + 1, current["request"].copy()))
                state[0] += 1
        stalled |= live and not ack
        expected.append((ack, packet, state.copy()))
    assert cursor == 6 and not incoming and not outgoing and stalled
    for target in ("cpp", "verilog"):
        directory = build / f"timing-{target}"
        trace = [
            json.loads(line)
            for line in (directory / "model-trace.jsonl").read_text().splitlines()
            if line.startswith('{"cycle":')
        ]
        ports = json.loads((directory / target / "ports.json").read_text())
        completion = next(
            item for item in ports["outputs"] if item["name"] == "completion"
        )
        assert len(trace) == 16
        for row, (ack, packet, state) in zip(trace, expected, strict=True):
            outputs = row["outputs"]
            assert outputs["ack"]["available"] == ack
            assert outputs["completion"]["available"] == (packet is not None)
            if packet:
                raw = int(outputs["completion"]["bits"], 2)
                assert {
                    field["path"][0]: raw >> field["low"] & ((1 << field["width"]) - 1)
                    for field in completion["fields"]
                } == packet
            assert [int(outputs[f"snapshot_{n}"]["bits"], 2) for n in range(4)] == state
    # Golden mismatch is a failed verification, never a new source or DUT value.
    golden = project / "expected.json"
    data = json.loads(golden.read_text())
    data["functional_completions"][0]["value"] += 1
    golden.write_text(json.dumps(data))
    error = invoke(
        "verify", project, "--build-dir", build, "--toolchain", prefix, success=False
    )
    assert "completion[0]" in error.stderr
    assert json.loads((build / "verification.json").read_text())["status"] == "failed"

    inputs = project / "requests.json"
    rows = json.loads(inputs.read_text())
    rows[0]["request"]["value"] = 65536
    inputs.write_text(json.dumps(rows))
    error = invoke(
        "verify", project, "--build-dir", build, "--toolchain", prefix, success=False
    )
    assert "inputs[0].request.value" in error.stderr
