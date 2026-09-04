#!/usr/bin/env python3
"""Generate and gate two parameterized priority-encoder configurations."""

from __future__ import annotations

from dataclasses import asdict, dataclass
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
ARTIFACTS = Path(
    os.environ.get(
        "PYCIRCUIT_DEMO_OUT",
        str(ROOT / ".pycircuit_out" / "examples" / "priority_encode_runtime"),
    )
) / "variants"
SOURCE = HERE / "priority_encode_variants.py"
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


@dataclass(frozen=True)
class Config:
    name: str
    system: str
    width: int
    lo_to_hi: bool
    input_value: int
    expected_encoded: int

    @property
    def result_width(self) -> int:
        return max(1, (self.width - 1).bit_length()) + 1

    @property
    def payload_width(self) -> int:
        return self.width + self.result_width

    @property
    def packed_input(self) -> int:
        # Queue payloads pack the first field at the low result-width bits.
        return self.input_value << self.result_width

    @property
    def expected_packed(self) -> int:
        return (1 << (self.result_width - 1)) | self.expected_encoded


CONFIGS = (
    Config(
        name="w8_lo_to_hi",
        system="priority_encode_demo_w8_lo",
        width=8,
        lo_to_hi=True,
        input_value=0x28,       # set bits 5 and 3; low-to-high selects 3
        expected_encoded=3,
    ),
    Config(
        name="w16_hi_to_lo",
        system="priority_encode_demo_w16_hi",
        width=16,
        lo_to_hi=False,
        input_value=0x28,       # set bits 5 and 3; high-to-low selects 5
        expected_encoded=5,
    ),
)


def _run(command: list[str], *, env: dict[str, str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command, cwd=ROOT, text=True, capture_output=True, timeout=45,
        check=False, env=env,
    )


def _testbench(config: Config, top: str) -> str:
    width = config.payload_width
    result = config.result_width
    return f'''module priority_encode_variant_tb;
  logic clk = 1'b0, rst = 1'b1;
  logic in_valid = 1'b0;
  logic [{width - 1}:0] in_data = '0;
  logic out_ready = 1'b1;
  wire out_valid, in_ready;
  wire [{width - 1}:0] out_data;
  {top} dut (
    .clk(clk), .rst(rst), .in_valid(in_valid), .in_data(in_data),
    .out_ready(out_ready), .out_valid(out_valid), .out_data(out_data),
    .in_ready(in_ready));
  always #1 clk = ~clk;
  initial begin : test
    integer seen;
    repeat (2) @(posedge clk);
    rst = 1'b0;
    @(negedge clk); in_data = {width}'h{config.packed_input:x}; in_valid = 1'b1;
    while (!in_ready) @(negedge clk);
    @(posedge clk); #0.1; in_valid = 1'b0;
    seen = 0;
    repeat (8) begin
      @(posedge clk); #0.1;
      if (out_valid) begin
        if (out_data[{result - 1}:0] !== {result}'b{config.expected_packed:0{result}b})
          $fatal(1, "encoded result mismatch: %h", out_data);
        seen = 1;
      end
    end
    if (!seen) $fatal(1, "generated priority pipeline produced no output");
    $display("PRIORITY_ENCODE_{config.name.upper()}_PASS"); $finish;
  end
endmodule
'''


def run_one(config: Config) -> dict[str, object]:
    out_dir = ARTIFACTS / config.name
    out_dir.mkdir(parents=True, exist_ok=True)
    acir = out_dir / "priority_encode.ac.mlir"
    verified = out_dir / "priority_encode.verified.ac.mlir"
    plan_path = out_dir / "priority_encode.plan.json"
    cxx = out_dir / "priority_encode.gfsim.cpp"
    pyc = out_dir / "priority_encode.pyc.mlir"
    generated = out_dir / "priority_encode.generated.v"
    report_path = out_dir / "priority_encode.gates.json"
    env = dict(os.environ)
    env["PYTHONPATH"] = str(FRONTEND)

    frontend = _run(
        [
            sys.executable, "-c",
            "from pathlib import Path; from agentic_circuit._queue_frontend import lower_queue_source; "
            "p=Path('" + str(SOURCE).replace("\\", "/") + "'); "
            "Path('" + str(acir).replace("\\", "/") + "').write_text("
            "lower_queue_source(p.read_text(), '" + config.system + "'))",
        ],
        env=env,
    )
    if frontend.returncode:
        raise RuntimeError(frontend.stderr)

    acir_opt = _run_gate(
        _acir_tool("acir-opt"),
        [str(acir)], 45,
    )
    if acir_opt.get("status") != "passed":
        raise RuntimeError(acir_opt.get("stderr", "ACIR verification failed"))
    verified.write_text(str(acir_opt.get("stdout", "")), encoding="utf-8")

    lowered = _run_gate(
        _acir_tool("acir-queue-pycgen"),
        [str(acir)], 45,
    )
    if lowered.get("status") != "passed":
        raise RuntimeError(lowered.get("stderr", "PYC lowering failed"))
    pyc.write_text(str(lowered.get("stdout", "")), encoding="utf-8")

    plan = _run_gate(
        _acir_tool("acir-queue-plan"),
        [str(acir)], 45,
    )
    if plan.get("status") != "passed":
        raise RuntimeError(plan.get("stderr", "Queue plan extraction failed"))
    plan_path.write_text(str(plan.get("stdout", "")), encoding="utf-8")

    cxxgen = _run_gate(
        _acir_tool("acir-queue-cxxgen"),
        [str(acir)], 45,
    )
    if cxxgen.get("status") == "passed":
        cxx.write_text(str(cxxgen.get("stdout", "")), encoding="utf-8")

    generated_run = _run(
        [sys.executable, str(VERILOGGEN), str(pyc), "--pyc-input",
         "--runtime-dir", str(RUNTIME), "-o", str(generated)],
        env=env,
    )
    if generated_run.returncode:
        raise RuntimeError(generated_run.stderr)

    with tempfile.TemporaryDirectory(prefix=f"priority_encode_{config.name}_") as temp:
        temp_path = Path(temp)
        tb = temp_path / "tb.sv"
        tb.write_text(_testbench(config, config.system), encoding="utf-8")
        verilator = _run_gate(
            "wsl:verilator",
            ["--binary", "--timing", "--build-jobs", "1", "-Wno-fatal",
             "--top-module", "priority_encode_variant_tb", "--Mdir",
             str(temp_path / "obj"), str(generated), str(tb), "-MAKEFLAGS",
             "CXX=/usr/bin/clang++-22 LINK=/usr/bin/clang++-22 CXXFLAGS=-std=c++20 LDFLAGS=-no-pie"],
            timeout=45,
        )
        verilator_ok = verilator.get("status") == "passed"
        sim_output = ""
        if verilator_ok:
            binary = temp_path / "obj" / "Vpriority_encode_variant_tb"
            sim = _run_binary(binary, "wsl:verilator", 10)
            sim_output = str(sim.get("stdout", "")) + str(sim.get("stderr", ""))
            verilator_ok = sim.get("status") == "passed" and f"PRIORITY_ENCODE_{config.name.upper()}_PASS" in sim_output
        yosys = _run_gate(
            "wsl:yosys",
            ["-p", f"read_slang --top {config.system} {generated}; "
                    f"hierarchy -top {config.system}; proc; opt; check; stat"],
            timeout=45,
        )
        yosys_ok = yosys.get("status") == "passed"

    report = {
        "demo": "priority_encode_runtime",
        "configuration": asdict(config),
        "derived": {"result_width": config.result_width, "payload_width": config.payload_width,
                    "packed_input": config.packed_input, "expected_packed": config.expected_packed},
        "status": "passed" if verilator_ok and yosys_ok else "failed",
        "artifacts": {"python": str(SOURCE), "acir": str(acir), "acir_verified": str(verified),
                      "plan": str(plan_path), "gfsim_cpp": str(cxx), "pyc": str(pyc),
                      "verilog": str(generated)},
        "primitive_id": "encoding-arbitration.basejump-priority.v1",
        "implementation_id": "github.bespoke-silicon-group.basejump_stl.bsg_priority_encode",
        "stages": {"python_frontend": frontend.returncode == 0,
                   "acir_verify": acir_opt.get("status") == "passed",
                   "queue_plan": plan.get("status") == "passed",
                   "acir_to_gfsim_cpp": cxxgen.get("status") == "passed",
                   "acir_to_pyc": lowered.get("status") == "passed",
                   "pyc_to_verilog": generated_run.returncode == 0},
        "gates": {"verilator": verilator_ok, "yosys": yosys_ok},
        "verilator_output": sim_output[-2000:],
        "yosys_output": str(yosys.get("stdout", ""))[-2000:] + str(yosys.get("stderr", ""))[-2000:],
        "generated_at_unix": time.time(),
    }
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def main() -> int:
    reports = [run_one(config) for config in CONFIGS]
    combined = {"demo": "priority_encode_runtime_variants",
                "status": "passed" if all(r["status"] == "passed" for r in reports) else "failed",
                "configurations": reports}
    combined_path = ARTIFACTS.parent / "priority_encode_variants.gates.json"
    combined_path.write_text(json.dumps(combined, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(combined, indent=2))
    return 0 if combined["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
