from __future__ import annotations

import importlib.util
import json
import re
import sys
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]


@pytest.fixture
def runner():
    spec = importlib.util.spec_from_file_location(
        "benchmark_runner", ROOT / "benchmark/run.py"
    )
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def generated_top(*, inputs="input logic [2:0] value", result_width=3):
    rows, controls = [], []
    domains = {"clock": None, "reset": None}
    for ordinal, declaration in enumerate(inputs.split(",")):
        name = declaration.split()[-1]
        bound = re.search(r"\[(\d+):0\]", declaration)
        bits = int(bound[1]) + 1 if bound else 1
        if name in {"pyc_phase", "pyc_root_commit_ok"}:
            controls.append(
                {"direction": "input", "role": "phase", "wire": name, "width": bits}
            )
            continue
        rows.append({"ordinal": ordinal, "name": name, "wire": name, "width": bits})
        if name == "pyc_7079635f636c6b":
            domains["clock"] = ordinal
        if name == "pyc_7079635f727374":
            domains["reset"] = ordinal
    return {
        "schema_version": 1,
        "inputs": rows,
        "outputs": (
            [
                {
                    "ordinal": 0,
                    "name": "result",
                    "wire": "result",
                    "width": result_width,
                    "availability": None,
                }
            ]
            if result_width
            else []
        ),
        "domains": domains,
        "controls": controls,
    }


def solution() -> dict:
    return {"outputs": [{"name": "left", "width": 2}, {"name": "right", "width": 1}]}


def test_adapter_uses_only_header_and_preserves_output_order(runner) -> None:
    prompt = """module original(input logic [2:0] value, output logic [1:0] left, output logic right);
assign left = 2'b11;
assign right = 1'b1;
endmodule
"""

    wrapper = runner.adapter(prompt, generated_top(), solution())

    assert "assign left" not in wrapper
    assert "2'b11" not in wrapper
    assert ".value(value)" in wrapper
    assert "assign {left, right} = pyc_result;" in wrapper


def test_adapter_accepts_flat_scalar_result(runner) -> None:
    prompt = "module original(input logic [2:0] value, output logic [1:0] left, output logic right); endmodule"
    top = generated_top()

    wrapper = runner.adapter(prompt, top, solution())

    assert "wire [2:0] pyc_result;" in wrapper
    assert ".result(pyc_result[0 +: 3])" in wrapper
    assert "assign {left, right} = pyc_result;" in wrapper


def test_host_domain_metadata_can_bind_clock_and_omit_external_reset(runner) -> None:
    prompt = "module original(input logic clk, input logic [2:0] value, output logic [2:0] out); endmodule"
    top = generated_top(
        inputs="input logic pyc_7079635f636c6b, input logic pyc_7079635f727374, input logic [2:0] value"
    )
    mapping = {
        "domain": {"edge": "posedge", "reset_active": "high", "clock": "clk"},
        "outputs": [{"name": "out", "width": 3}],
    }

    wrapper = runner.adapter(prompt, top, mapping)

    assert ".pyc_7079635f636c6b(clk)" in wrapper
    assert ".pyc_7079635f727374(1'b0)" in wrapper


@pytest.mark.parametrize(
    ("top", "outputs", "diagnostic"),
    [
        (generated_top(result_width=2), solution()["outputs"], "result width"),
        (
            generated_top(result_width=0),
            solution()["outputs"],
            "total scalar or packed results",
        ),
        (
            generated_top(
                inputs="input logic [2:0] value, input logic [2:0] pyc_phase"
            ),
            solution()["outputs"],
            "PREPARE",
        ),
        (generated_top(), list(reversed(solution()["outputs"])), "order/width"),
    ],
)
def test_adapter_rejects_missing_mismatched_private_or_reordered_results(
    runner, top: dict, outputs: list[dict], diagnostic: str
) -> None:
    prompt = "module original(input logic [2:0] value, output logic [1:0] left, output logic right); endmodule"
    with pytest.raises(runner.AdapterUnsupported, match=diagnostic):
        runner.adapter(prompt, top, {"outputs": outputs})


@pytest.mark.parametrize(
    "declaration",
    (
        "module top(input logic [7:0][1:0] value, output done); endmodule",
        "module top(input logic [WIDTH-1:0] value, output done); endmodule",
        "module top(input logic [3:0] value, inout done); endmodule",
    ),
)
def test_header_rejects_unpacked_symbolic_and_inout_ports(
    runner, declaration: str
) -> None:
    with pytest.raises(runner.AdapterUnsupported):
        runner.header(declaration)


def test_adapter_rejects_duplicate_generated_input_fanout(runner) -> None:
    prompt = "module original(input logic a, input logic b, output logic left, output logic right); endmodule"
    top = generated_top(
        inputs="input logic x, input logic y, input logic z", result_width=2
    )
    mapping = {
        "inputs": {"x": "a", "y": "b", "z": "a"},
        "outputs": [{"name": "left", "width": 1}, {"name": "right", "width": 1}],
    }

    with pytest.raises(runner.AdapterUnsupported, match="connected exactly once"):
        runner.adapter(prompt, top, mapping)


def test_adapter_rejects_commit_control_even_if_original_header_names_it(
    runner,
) -> None:
    prompt = "module original(input logic [2:0] value, input logic pyc_root_commit_ok, output logic [1:0] left, output logic right); endmodule"
    top = generated_top(
        inputs="input logic [2:0] value, input logic pyc_root_commit_ok"
    )

    with pytest.raises(runner.AdapterUnsupported, match="commit"):
        runner.adapter(prompt, top, solution())


def test_judges_require_unambiguous_nonempty_success(runner) -> None:
    assert runner.judge("verilogeval", "Mismatches: 0 in 4 samples\n")[0]
    assert not runner.judge("verilogeval", "Mismatches: 0 in 0 samples\n")[0]
    assert not runner.judge("verilogeval", "no summary\n")[0]
    assert runner.judge("resbench", "All tests passed\n")[0]
    assert not runner.judge("resbench", "Not All tests passed\n")[0]
    assert not runner.judge("resbench", "PYCIRCUIT_PHYSICAL_FAULT\nAll tests passed\n")[
        0
    ]
    assert not runner.judge(
        "verilogeval", "Mismatches: 0 in 4 samples\nPYCIRCUIT_PHYSICAL_FAULT\n"
    )[0]


def test_adapter_uses_compiler_names_and_multiple_results(runner) -> None:
    ports = generated_top()
    ports["inputs"][0]["wire"] = "encoded_input"
    ports["outputs"] = [
        {
            "ordinal": 0,
            "name": "result_0",
            "wire": "encoded_first",
            "width": 2,
            "availability": None,
        },
        {
            "ordinal": 1,
            "name": "result_1",
            "wire": "encoded_second",
            "width": 1,
            "availability": None,
        },
    ]
    wrapper = runner.adapter(
        "module original(input [2:0] value, output [1:0] left, output right); endmodule",
        ports,
        solution(),
        root_name="exported_root",
    )
    assert "exported_root implementation" in wrapper
    assert ".encoded_input(value)" in wrapper
    assert ".encoded_first(pyc_result[1 +: 2])" in wrapper
    assert ".encoded_second(pyc_result[0 +: 1])" in wrapper


def test_domain_metadata_not_signal_spelling_selects_clock(runner) -> None:
    ports = generated_top(
        inputs="input clock_data, input compiler_clock, input compiler_reset",
        result_width=1,
    )
    ports["domains"] = {"clock": 1, "reset": 2}
    mapping = {
        "inputs": {"clock_data": "value"},
        "outputs": [{"name": "out", "width": 1}],
        "domain": {
            "clock": "clk",
            "reset": None,
            "edge": "posedge",
            "reset_active": "high",
        },
    }
    wrapper = runner.adapter(
        "module original(input value, input clk, output out); endmodule", ports, mapping
    )
    assert ".clock_data(value)" in wrapper
    assert ".compiler_clock(clk)" in wrapper
    assert ".compiler_reset(1'b0)" in wrapper


def test_empty_selection_exits_before_evaluation(
    runner, tmp_path: Path, monkeypatch
) -> None:
    bench = tmp_path / "benchmark"
    (bench / "solutions").mkdir(parents=True)
    (bench / "solutions/verilogeval.json").write_text("[]\n")
    runner.BENCH = bench
    monkeypatch.setattr(runner, "problems", lambda suite: {"known": {}})
    monkeypatch.setattr(
        sys,
        "argv",
        ["run.py", "--suite", "verilogeval", "--output", str(tmp_path / "out")],
    )

    with pytest.raises(SystemExit) as failure:
        runner.main()

    assert failure.value.code == 2


def test_negative_control_compile_failure_never_counts_as_behavior_detection(
    runner, tmp_path: Path, monkeypatch
) -> None:
    runner.ROOT = tmp_path
    source = tmp_path / "solution.py"
    source.write_text("# fixture\n")
    output = tmp_path / "out"
    output.mkdir()

    def fake_command(args, env, directory, label, timeout):
        del args, env, timeout
        (directory / f"{label}.stdout").write_text(
            "All tests passed\n" if label == "simulation" else ""
        )
        (directory / f"{label}.stderr").write_text("")
        if label == "link":
            (directory / "final.ac").write_text("module {}\n")
        if label == "emit-verilog":
            rtl = directory / "rtl"
            rtl.mkdir()
            (rtl / "dut.v").write_text("module dut; endmodule\n")
            (rtl / "ports.json").write_text(json.dumps(generated_top()))
            (rtl / "generated.json").write_text(
                json.dumps(
                    {
                        "root_rtl_name": "pyc_root",
                        "files": [{"path": "dut.v", "role": "rtl"}],
                    }
                )
            )
        return {
            "command": [],
            "exit_code": 1 if label == "negative-iverilog" else 0,
            "seconds": 0,
            "stdout": f"{label}.stdout",
            "stderr": f"{label}.stderr",
        }

    monkeypatch.setattr(runner, "command", fake_command)
    result = runner.evaluate(
        {
            "task_id": "fixture",
            "status": "implemented",
            "source": "solution.py",
            "top": "fixture.Top",
            "outputs": solution()["outputs"],
        },
        {
            "prompt": "module original(input logic [2:0] value, output logic [1:0] left, output logic right); endmodule",
            "test": "module tb; endmodule",
            "top": "tb",
        },
        "resbench",
        output,
        tmp_path / "build",
        1,
        True,
    )

    assert result["status"] == "negative-rtl_compile_failed"
    assert "negative_control" not in result
