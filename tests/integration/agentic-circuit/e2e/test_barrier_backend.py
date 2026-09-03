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
EXAMPLE = ROOT / "examples/agentic-circuit" / "pipelines" / "pyc_barrier_pipeline.py"
PYC_REPOSITORY = ROOT
DEFAULT_TOOLCHAIN = PYC_REPOSITORY / ".pycircuit_out/toolchain/install"


class BarrierBackendTest(unittest.TestCase):
    def test_barrier_is_atomic_and_cycle_equivalent_in_cpp_and_verilog(self) -> None:
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
            raw = root / "barrier.raw.ac.mlir"
            frozen = root / "barrier.frozen.ac.mlir"
            output = root / "output"
            raw.write_text(
                lower_queue_source(
                    EXAMPLE.read_text(encoding="utf-8"), "pyc_barrier_pipeline"
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
                ["ac.barrier", "ac.sink", "ac.source"],
                manifest["opcode_lowering_inventory"],
            )

            cpp_harness = root / "cpp_harness.cpp"
            cpp_executable = root / "cpp_model"
            cpp_harness.write_text(
                """#include "pyc_barrier_pipeline.hpp"
#include <cstdint>
#include <iostream>

int main() {
  pyc::gen::pyc_barrier_pipeline dut;
  bool left_sent = false;
  bool right_sent = false;
  for (std::uint64_t cycle = 0; cycle < 16; ++cycle) {
    const bool left_offering = cycle >= 1 && !left_sent;
    const bool right_offering = cycle >= 3 && !right_sent;
    dut.rst = pyc::cpp::Wire<1>(cycle == 0 ? 1 : 0);
    dut.in0_valid = pyc::cpp::Wire<1>(left_offering ? 1 : 0);
    dut.in0_data = pyc::cpp::Wire<16>(left_offering ? 11 : 0);
    dut.in1_valid = pyc::cpp::Wire<1>(right_offering ? 1 : 0);
    dut.in1_data = pyc::cpp::Wire<32>(right_offering ? 22 : 0);
    dut.out0_ready = pyc::cpp::Wire<1>(1);
    dut.out1_ready = pyc::cpp::Wire<1>(1);
    dut.clk = pyc::cpp::Wire<1>(0);
    dut.step();
    dut.clk = pyc::cpp::Wire<1>(1);
    dut.step();
    std::cout << cycle << " " << dut.out0_valid.value() << " "
              << dut.out0_data.value() << " " << dut.out1_valid.value() << " "
              << dut.out1_data.value() << " " << dut.in0_ready.value() << " "
              << dut.in1_ready.value() << "\\n";
    if (left_offering && dut.in0_ready.value() != 0)
      left_sent = true;
    if (right_offering && dut.in1_ready.value() != 0)
      right_sent = true;
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
                    str(output / "cpp/pyc_barrier_pipeline.cpp"),
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
                """#include "Vpyc_barrier_pipeline.h"
#include <cstdint>
#include <iostream>

int main() {
  Vpyc_barrier_pipeline dut;
  bool left_sent = false;
  bool right_sent = false;
  for (std::uint64_t cycle = 0; cycle < 16; ++cycle) {
    const bool left_offering = cycle >= 1 && !left_sent;
    const bool right_offering = cycle >= 3 && !right_sent;
    dut.rst = cycle == 0 ? 1 : 0;
    dut.in0_valid = left_offering ? 1 : 0;
    dut.in0_data = left_offering ? 11 : 0;
    dut.in1_valid = right_offering ? 1 : 0;
    dut.in1_data = right_offering ? 22 : 0;
    dut.out0_ready = 1;
    dut.out1_ready = 1;
    dut.clk = 0;
    dut.eval();
    dut.clk = 1;
    dut.eval();
    std::cout << cycle << " " << unsigned(dut.out0_valid) << " "
              << dut.out0_data << " " << unsigned(dut.out1_valid) << " "
              << dut.out1_data << " " << unsigned(dut.in0_ready) << " "
              << unsigned(dut.in1_ready) << "\\n";
    if (left_offering && dut.in0_ready != 0)
      left_sent = true;
    if (right_offering && dut.in1_ready != 0)
      right_sent = true;
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
                    "pyc_barrier_pipeline",
                    "--Mdir",
                    str(object_dir),
                    str(output / "verilog/pyc_primitives.v"),
                    str(output / "verilog/pyc_barrier_pipeline.v"),
                    str(verilator_harness),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, verilator_build.returncode, verilator_build.stderr)
            verilator_run = subprocess.run(
                (str(object_dir / "Vpyc_barrier_pipeline"),),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, verilator_run.returncode, verilator_run.stderr)
            self.assertEqual(cpp_run.stdout, verilator_run.stdout)

            rows = [line.split() for line in cpp_run.stdout.splitlines()]
            left = [int(row[2]) for row in rows if row[1] == "1"]
            right = [int(row[4]) for row in rows if row[3] == "1"]
            self.assertEqual([11], left)
            self.assertEqual([22], right)
            left_cycle = next(int(row[0]) for row in rows if row[1] == "1")
            right_cycle = next(int(row[0]) for row in rows if row[3] == "1")
            self.assertEqual(left_cycle, right_cycle)
            self.assertGreaterEqual(left_cycle, 3)


if __name__ == "__main__":
    unittest.main()
