from __future__ import annotations

import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from agentic_circuit._queue_frontend import lower_queue_source


ROOT = Path(__file__).resolve().parents[4]
EXAMPLE = ROOT / "examples/agentic-circuit" / "pipelines" / "pyc_select_pipeline.py"
PYC_REPOSITORY = ROOT
DEFAULT_TOOLCHAIN = PYC_REPOSITORY / ".pycircuit_out/toolchain/install"


class SelectBackendTest(unittest.TestCase):
    def test_select_is_cycle_equivalent_in_cpp_and_verilog(self) -> None:
        toolchain = Path(os.environ.get("PYC_TOOLCHAIN_ROOT", DEFAULT_TOOLCHAIN))
        pycc = toolchain / "bin" / "pycc"
        metadata = toolchain / "share" / "pycircuit" / "toolchain-metadata.json"
        cxx = shutil.which("c++")
        verilator = shutil.which("verilator")
        if (
            not pycc.is_file()
            or not metadata.is_file()
            or cxx is None
            or verilator is None
        ):
            self.skipTest(
                "pinned pyCircuit toolchain, C++, or Verilator is unavailable"
            )
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            raw = root / "select.raw.ac.mlir"
            frozen = root / "select.frozen.ac.mlir"
            output = root / "output"
            raw.write_text(
                lower_queue_source(
                    EXAMPLE.read_text(encoding="utf-8"), "pyc_select_pipeline"
                ),
                encoding="utf-8",
            )
            optimized = subprocess.run(
                (str(ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"), str(raw)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, optimized.returncode, optimized.stderr)
            frozen.write_text(optimized.stdout, encoding="utf-8")
            completed = subprocess.run(
                (
                    str(ROOT / "compiler/acir/tools/ac-queue-pyc-build.py"),
                    str(frozen),
                    "--pycgen-tool",
                    str(ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-pycgen"),
                    "--pycc",
                    str(pycc),
                    "--toolchain-lock",
                    str(ROOT / "toolchains/agentic-circuit/pyc.lock.json"),
                    "--toolchain-metadata",
                    str(metadata),
                    "--cxx",
                    cxx,
                    "--verilator",
                    verilator,
                    "--pyc-output",
                    str(output / "model.pyc"),
                    "--cpp-output-dir",
                    str(output / "cpp"),
                    "--verilog-output-dir",
                    str(output / "verilog"),
                    "--manifest",
                    str(output / "manifest.json"),
                ),
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, completed.returncode, completed.stderr)
            manifest = json.loads((output / "manifest.json").read_text())
            self.assertEqual(
                ["ac.select", "ac.sink", "ac.source"],
                manifest["opcode_lowering_inventory"],
            )

            cpp_harness = root / "cpp_harness.cpp"
            cpp_executable = root / "cpp_model"
            cpp_harness.write_text(
                """#include "pyc_select_pipeline.hpp"
#include <cstdint>
#include <iostream>

int main() {
  pyc::gen::pyc_select_pipeline dut;
  bool sent = false;
  for (std::uint64_t cycle = 0; cycle < 14; ++cycle) {
    const bool offering = cycle >= 1 && !sent;
    dut.rst = pyc::cpp::Wire<1>(cycle == 0 ? 1 : 0);
    dut.in0_valid = pyc::cpp::Wire<1>(offering ? 1 : 0);
    dut.in0_data = pyc::cpp::Wire<1>(1);
    dut.in1_valid = pyc::cpp::Wire<1>(offering ? 1 : 0);
    dut.in1_data = pyc::cpp::Wire<64>(10);
    dut.in2_valid = pyc::cpp::Wire<1>(offering ? 1 : 0);
    dut.in2_data = pyc::cpp::Wire<64>(20);
    dut.out_ready = pyc::cpp::Wire<1>(1);
    dut.clk = pyc::cpp::Wire<1>(0);
    dut.step();
    dut.clk = pyc::cpp::Wire<1>(1);
    dut.step();
    std::cout << cycle << " " << dut.out_valid.value() << " "
              << dut.out_data.value() << " " << dut.in0_ready.value() << " "
              << dut.in1_ready.value() << " " << dut.in2_ready.value() << "\\n";
    if (offering && dut.in0_ready.value() && dut.in2_ready.value())
      sent = true;
    dut.clk = pyc::cpp::Wire<1>(0);
    dut.step();
  }
}
""",
                encoding="utf-8",
            )
            cpp_build = subprocess.run(
                (
                    cxx,
                    "-std=c++17",
                    "-I",
                    str(output / "cpp"),
                    "-I",
                    str(toolchain / "include"),
                    str(output / "cpp/pyc_select_pipeline.cpp"),
                    str(cpp_harness),
                    str(toolchain / "lib/libpyc6_runtime.a"),
                    "-o",
                    str(cpp_executable),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, cpp_build.returncode, cpp_build.stderr)
            cpp_run = subprocess.run(
                (str(cpp_executable),), text=True, capture_output=True, check=False
            )
            self.assertEqual(0, cpp_run.returncode, cpp_run.stderr)

            verilator_harness = root / "verilator_harness.cpp"
            verilator_harness.write_text(
                """#include "Vpyc_select_pipeline.h"
#include <cstdint>
#include <iostream>

int main() {
  Vpyc_select_pipeline dut;
  bool sent = false;
  for (std::uint64_t cycle = 0; cycle < 14; ++cycle) {
    const bool offering = cycle >= 1 && !sent;
    dut.rst = cycle == 0 ? 1 : 0;
    dut.in0_valid = offering ? 1 : 0;
    dut.in0_data = 1;
    dut.in1_valid = offering ? 1 : 0;
    dut.in1_data = 10;
    dut.in2_valid = offering ? 1 : 0;
    dut.in2_data = 20;
    dut.out_ready = 1;
    dut.clk = 0;
    dut.eval();
    dut.clk = 1;
    dut.eval();
    std::cout << cycle << " " << unsigned(dut.out_valid) << " "
              << dut.out_data << " " << unsigned(dut.in0_ready) << " "
              << unsigned(dut.in1_ready) << " " << unsigned(dut.in2_ready)
              << "\\n";
    if (offering && dut.in0_ready && dut.in2_ready)
      sent = true;
    dut.clk = 0;
    dut.eval();
  }
}
""",
                encoding="utf-8",
            )
            object_dir = root / "verilator_obj"
            verilator_build = subprocess.run(
                (
                    verilator,
                    "--cc",
                    "--exe",
                    "--build",
                    "-Wno-fatal",
                    "--top-module",
                    "pyc_select_pipeline",
                    "--Mdir",
                    str(object_dir),
                    str(output / "verilog/pyc_primitives.v"),
                    str(output / "verilog/pyc_select_pipeline.v"),
                    str(verilator_harness),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, verilator_build.returncode, verilator_build.stderr)
            verilator_run = subprocess.run(
                (str(object_dir / "Vpyc_select_pipeline"),),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, verilator_run.returncode, verilator_run.stderr)
            self.assertEqual(cpp_run.stdout, verilator_run.stdout)
            transactions = [
                int(fields[2])
                for line in cpp_run.stdout.splitlines()
                if len(fields := line.split()) == 6 and fields[1] == "1"
            ]
            self.assertEqual([20], transactions)


if __name__ == "__main__":
    unittest.main()
