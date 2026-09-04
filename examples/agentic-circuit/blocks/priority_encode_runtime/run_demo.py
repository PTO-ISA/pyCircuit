#!/usr/bin/env python3
"""Run the priority encoder Python → ACIR → PYC → Verilog demo and gates."""

from __future__ import annotations

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parent
ARTIFACTS = Path(
    os.environ.get(
        "PYCIRCUIT_DEMO_OUT",
        str(ROOT / ".pycircuit_out" / "examples" / "priority_encode_runtime"),
    )
)
PYTHON_SOURCE = HERE / "priority_encode.py"
VERILOGGEN = ROOT / "compiler" / "acir" / "tools" / "acir-queue-veriloggen.py"
FRONTEND = ROOT / "python" / "agentic-circuit" / "src"
RUNTIME = ROOT / "library" / "verilog"
ACIR_BIN = ROOT / ".pycircuit_out" / "acir" / "dev-llvm22" / "bin"
if not (ACIR_BIN / "acir-opt").is_file():
    ACIR_BIN = ROOT / ".pycircuit_out" / "toolchain" / "build" / "bin"
if not (ACIR_BIN / "acir-opt").is_file():
    ACIR_BIN = ROOT / "build" / "dev-llvm22" / "bin"
sys.path.insert(0, str(ROOT / "tools" / "runtime"))
from acir_runtime_crawler import _run_gate
from acir_runtime_functional import _run_binary


def _acir_tool(name: str) -> str:
    """Run the repo-local Linux ACIR tools through the configured WSL CAD host."""

    path = str(ACIR_BIN / name).replace("\\", "/")
    if len(path) >= 2 and path[1] == ":":
        path = "/mnt/" + path[0].lower() + path[2:]
    return "wsl:" + path


def run(
    command: list[str], *, timeout: float = 45.0, env: dict[str, str] | None = None
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, cwd=ROOT, text=True, capture_output=True,
                          timeout=timeout, check=False, env=env)


def main() -> int:
    ARTIFACTS.mkdir(parents=True, exist_ok=True)
    acir = ARTIFACTS / "priority_encode.ac.mlir"
    acir_verified = ARTIFACTS / "priority_encode.verified.ac.mlir"
    pyc = ARTIFACTS / "priority_encode.pyc.mlir"
    generated = ARTIFACTS / "priority_encode.generated.v"
    cxx = ARTIFACTS / "priority_encode.gfsim.cpp"
    report_path = ARTIFACTS / "priority_encode.gates.json"
    plan_path = ARTIFACTS / "priority_encode.plan.json"
    env = dict(os.environ)
    env["PYTHONPATH"] = str(FRONTEND)
    frontend = run([
        sys.executable, "-c",
        "from pathlib import Path; from agentic_circuit._queue_frontend import lower_queue_source; "
        "p=Path('" + str(PYTHON_SOURCE).replace("\\", "/") + "'); "
        "Path('" + str(acir).replace("\\", "/") + "').write_text(lower_queue_source(p.read_text(), 'priority_encode_demo'))",
    ], env=env)
    if frontend.returncode:
        print(frontend.stderr, file=sys.stderr)
        return frontend.returncode
    acir_opt = _run_gate(
        _acir_tool("acir-opt"),
        [str(acir)],
        45,
    )
    if acir_opt.get("status") != "passed":
        print(acir_opt.get("stderr", "ACIR verification failed"), file=sys.stderr)
        return 1
    acir_verified.write_text(str(acir_opt.get("stdout", "")), encoding="utf-8")

    # acir-queue-pycgen is an ELF binary in the reproducible WSL build.
    lowered = _run_gate(
        _acir_tool("acir-queue-pycgen"),
        [str(acir)],
        45,
    )
    if lowered.get("status") != "passed":
        print(lowered.get("stderr", lowered.get("reason", "PYC lowering failed")), file=sys.stderr)
        return 1
    pyc.write_text(str(lowered.get("stdout", "")), encoding="utf-8")
    plan = _run_gate(
        _acir_tool("acir-queue-plan"),
        [str(acir)],
        45,
    )
    if plan.get("status") != "passed":
        print(plan.get("stderr", "Queue plan extraction failed"), file=sys.stderr)
        return 1
    plan_path.write_text(str(plan.get("stdout", "")), encoding="utf-8")
    cxxgen = _run_gate(
        _acir_tool("acir-queue-cxxgen"),
        [str(acir)],
        45,
    )
    if cxxgen.get("status") == "passed":
        cxx.write_text(str(cxxgen.get("stdout", "")), encoding="utf-8")

    generated_run = run([
        sys.executable, str(VERILOGGEN), str(pyc), "--pyc-input",
        "--runtime-dir", str(RUNTIME), "-o", str(generated),
    ], timeout=45.0, env=env)
    if generated_run.returncode:
        print(generated_run.stderr, file=sys.stderr)
        return generated_run.returncode

    with tempfile.TemporaryDirectory(prefix="priority_encode_gate_") as temp:
        temp_path = Path(temp)
        tb = temp_path / "tb.sv"
        tb.write_text('''module priority_encode_tb;
  logic clk = 1'b0, rst = 1'b1;
  logic in_valid = 1'b0;
  logic [11:0] in_data = '0;
  logic out_ready = 1'b1;
  wire out_valid, in_ready;
  wire [11:0] out_data;
  priority_encode_demo dut (
    .clk(clk), .rst(rst), .in_valid(in_valid), .in_data(in_data),
    .out_ready(out_ready), .out_valid(out_valid), .out_data(out_data),
    .in_ready(in_ready));
  always #1 clk = ~clk;
  initial begin : test
    integer seen;
    repeat (2) @(posedge clk);
    rst = 1'b0;
    @(negedge clk); in_data = 12'h280; in_valid = 1'b1;
    while (!in_ready) @(negedge clk);
    @(posedge clk); #0.1; in_valid = 1'b0;
    seen = 0;
    repeat (8) begin
      @(posedge clk); #0.1;
      if (out_valid) begin
        if (out_data[3:0] !== 4'b1011) $fatal(1, "encoded result mismatch: %h", out_data);
        seen = 1;
      end
    end
    if (!seen) $fatal(1, "generated priority pipeline produced no output");
    $display("PRIORITY_ENCODE_PASS"); $finish;
  end
endmodule
''', encoding="utf-8")
        verilator_run = _run_gate(
            "wsl:verilator",
            [
                "--binary",
                "--timing",
                "--build-jobs",
                "1",
                "-Wno-fatal",
                "--top-module",
                "priority_encode_tb",
                "--Mdir",
                str(temp_path / "obj"),
                str(generated),
                str(tb),
                "-MAKEFLAGS",
                "CXX=/usr/bin/clang++-22 LINK=/usr/bin/clang++-22 CXXFLAGS=-std=c++20 LDFLAGS=-no-pie",
            ],
            timeout=45,
        )
        verilator_ok = verilator_run.get("status") == "passed"
        sim_output = ""
        if verilator_ok:
            binary = temp_path / "obj" / "Vpriority_encode_tb"
            sim = _run_binary(binary, "wsl:verilator", 10)
            verilator_ok = sim.get("status") == "passed" and "PRIORITY_ENCODE_PASS" in str(sim.get("stdout", ""))
            sim_output = str(sim.get("stdout", "")) + str(sim.get("stderr", ""))
        yosys_run = _run_gate(
            "wsl:yosys",
            [
                "-p",
                f"read_slang --top priority_encode_demo {generated}; "
                "hierarchy -top priority_encode_demo; proc; opt; check; stat",
            ],
            45,
        )
        yosys_ok = yosys_run.get("status") == "passed"

    report = {
        "demo": "priority_encode_runtime",
        "status": "passed" if verilator_ok and yosys_ok else "failed",
        "artifacts": {
            "python": str(PYTHON_SOURCE),
            "acir": str(acir),
            "acir_verified": str(acir_verified),
            "plan": str(plan_path),
            "gfsim_cpp": str(cxx),
            "pyc": str(pyc),
            "verilog": str(generated),
            "primitive": str(HERE / "primitive.json"),
        },
        "primitive_id": "encoding-arbitration.basejump-priority.v1",
        "implementation_id": "github.bespoke-silicon-group.basejump_stl.bsg_priority_encode",
        "gates": {"verilator": verilator_ok, "yosys": yosys_ok},
        "stages": {
            "python_frontend": frontend.returncode == 0,
            "acir_verify": acir_opt.get("status") == "passed",
            "queue_plan": plan.get("status") == "passed",
            "acir_to_gfsim_cpp": cxxgen.get("status") == "passed",
            "acir_to_pyc": lowered.get("status") == "passed",
            "pyc_to_verilog": generated_run.returncode == 0,
        },
        "verilator_output": sim_output[-2000:],
        "yosys_output": str(yosys_run.get("stdout", ""))[-2000:] + str(yosys_run.get("stderr", ""))[-2000:],
        "generated_at_unix": time.time(),
    }
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
