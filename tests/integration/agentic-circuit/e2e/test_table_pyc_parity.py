from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
DEFAULT_TOOLCHAIN = ROOT / ".pycircuit_out/toolchain/install"
PYCGEN = Path(
    os.environ.get(
        "ACIR_QUEUE_PYCGEN",
        ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-pycgen",
    )
)
ACIR_OPT = Path(
    os.environ.get(
        "ACIR_OPT", ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt-internal"
    )
)
BUILD = ROOT / "compiler/acir/tools/ac-queue-pyc-build.py"
LOCK = ROOT / "toolchains/agentic-circuit/pyc.lock.json"


class TablePycParityTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.toolchain = Path(os.environ.get("PYC_TOOLCHAIN_ROOT", DEFAULT_TOOLCHAIN))
        cls.pycc = cls.toolchain / "bin/pycc"
        cls.metadata = (
            cls.toolchain / "share/pycircuit/toolchain-metadata.json"
        )
        cls.cxx = shutil.which("c++")
        cls.verilator = shutil.which("verilator")
        if not all(
            (
                PYCGEN.is_file(),
                ACIR_OPT.is_file(),
                cls.pycc.is_file(),
                cls.metadata.is_file(),
                cls.cxx,
                cls.verilator,
            )
        ):
            raise unittest.SkipTest(
                "integrated pyCircuit toolchain, C++, and Verilator are required"
            )

    def build_fixture(
        self, root: Path, fixture: Path, system: str, pipeline: str
    ) -> Path:
        frozen = root / "model.frozen.mlir"
        optimized = subprocess.run(
            (str(ACIR_OPT), f"--pass-pipeline={pipeline}", str(fixture)),
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, optimized.returncode, optimized.stderr)
        frozen.write_text(optimized.stdout, encoding="utf-8")
        output = root / "output"
        completed = subprocess.run(
            (
                str(BUILD),
                str(frozen),
                "--pycgen-tool",
                str(PYCGEN),
                "--pycc",
                str(self.pycc),
                "--toolchain-lock",
                str(LOCK),
                "--toolchain-metadata",
                str(self.metadata),
                "--cxx",
                str(self.cxx),
                "--verilator",
                str(self.verilator),
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
        self.assertTrue((output / "cpp" / f"{system}.cpp").is_file())
        self.assertTrue((output / "verilog" / f"{system}.v").is_file())
        return output

    def run_cpp(self, output: Path, system: str, harness: str) -> str:
        harness_path = output / "cpp" / "harness.cpp"
        executable = output / "cpp" / "harness"
        harness_path.write_text(textwrap.dedent(harness), encoding="utf-8")
        completed = subprocess.run(
            (
                str(self.cxx),
                "-std=c++20",
                "-I",
                str(output / "cpp"),
                "-I",
                str(self.toolchain / "include"),
                str(output / "cpp" / f"{system}.cpp"),
                str(harness_path),
                str(self.toolchain / "lib/libpyc6_runtime.a"),
                "-o",
                str(executable),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, completed.returncode, completed.stderr)
        executed = subprocess.run(
            (str(executable),), text=True, capture_output=True, check=False
        )
        self.assertEqual(0, executed.returncode, executed.stderr)
        return executed.stdout

    def run_verilator(self, output: Path, system: str, harness: str) -> str:
        harness_path = output / "verilog" / "harness.cpp"
        object_dir = output / "verilog" / "obj"
        harness_path.write_text(textwrap.dedent(harness), encoding="utf-8")
        completed = subprocess.run(
            (
                str(self.verilator),
                "--cc",
                "--exe",
                "--build",
                "-Wno-fatal",
                "--top-module",
                system,
                "--Mdir",
                str(object_dir),
                str(output / "verilog" / "pyc_primitives.v"),
                str(output / "verilog" / f"{system}.v"),
                str(harness_path),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, completed.returncode, completed.stderr)
        executed = subprocess.run(
            (str(object_dir / f"V{system}"),),
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, executed.returncode, executed.stderr)
        return executed.stdout

    def test_round_robin_prefix_is_cycle_equivalent(self) -> None:
        fixture = (
            ROOT
            / "tests/mlir/agentic-circuit/CodeGen/table-multi-selection-pyc.mlir"
        )
        cpp_harness = r"""
            #include "table_multi_selection.hpp"
            #include <cstdint>
            #include <iostream>
            #include <cpp/pyc_tb.hpp>
            int main() {
              pyc::gen::table_multi_selection dut;
              pyc::cpp::Testbench<pyc::gen::table_multi_selection> tb(dut);
              tb.addClock(dut.clk, 1, 0, false);
              dut.out0_ready = pyc::cpp::Wire<1>({1});
              dut.out1_ready = pyc::cpp::Wire<1>({1});
              tb.reset(dut.rst, 2, 1);
              dut.out1_ready = pyc::cpp::Wire<1>({0});
              for (unsigned stall = 0; stall < 3; ++stall) {
                tb.runCycleAutoTrace(stall, nullptr);
                if (dut.out0_valid.value() || !dut.out1_valid.value() ||
                    dut.out1_data.value() != 3)
                  return 4;
              }
              dut.out1_ready = pyc::cpp::Wire<1>({1});
              unsigned observed = 0;
              for (unsigned tick = 0; tick < 20 && observed < 3; ++tick) {
                tb.runCycleAutoTrace(tick, nullptr);
                if (dut.out0_valid.value() && dut.out1_valid.value()) {
                  std::cout << dut.out0_data.value() << " "
                            << dut.out1_data.value() << "\n";
                  ++observed;
                }
              }
              return observed == 3 ? 0 : 3;
            }
        """
        verilator_harness = r"""
            #include "Vtable_multi_selection.h"
            #include <iostream>
            static void cycle(Vtable_multi_selection &dut) {
              dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval();
              dut.clk = 0; dut.eval();
            }
            int main() {
              Vtable_multi_selection dut;
              dut.out0_ready = 1; dut.out1_ready = 1; dut.rst = 1;
              cycle(dut); cycle(dut); dut.rst = 0; cycle(dut);
              dut.out1_ready = 0;
              for (unsigned stall = 0; stall < 3; ++stall) {
                cycle(dut);
                if (dut.out0_valid || !dut.out1_valid || dut.out1_data != 3)
                  return 4;
              }
              dut.out1_ready = 1;
              unsigned observed = 0;
              for (unsigned tick = 0; tick < 20 && observed < 3; ++tick) {
                cycle(dut);
                if (dut.out0_valid && dut.out1_valid) {
                  std::cout << unsigned(dut.out0_data) << " "
                            << unsigned(dut.out1_data) << "\n";
                  ++observed;
                }
              }
              return observed == 3 ? 0 : 3;
            }
        """
        with tempfile.TemporaryDirectory() as directory:
            output = self.build_fixture(
                Path(directory),
                fixture,
                "table_multi_selection",
                "builtin.module(ac-freeze-topology)",
            )
            cpp = self.run_cpp(output, "table_multi_selection", cpp_harness)
            rtl = self.run_verilator(
                output, "table_multi_selection", verilator_harness
            )
        self.assertEqual("4 1\n2 3\n4 1\n", cpp)
        self.assertEqual(cpp, rtl)

    def test_priority_writer_wins_before_lower_rank(self) -> None:
        fixture = (
            ROOT
            / "tests/mlir/agentic-circuit/CodeGen/table-writer-arbitration-pyc.mlir"
        )
        cpp_harness = r"""
            #include "writer_arbitration.hpp"
            #include <iostream>
            #include <cpp/pyc_tb.hpp>
            int main() {
              pyc::gen::writer_arbitration dut;
              pyc::cpp::Testbench<pyc::gen::writer_arbitration> tb(dut);
              tb.addClock(dut.clk, 1, 0, false);
              dut.in0_valid = pyc::cpp::Wire<1>({0});
              dut.in1_valid = pyc::cpp::Wire<1>({0});
              dut.in2_valid = pyc::cpp::Wire<1>({0});
              dut.out_ready = pyc::cpp::Wire<1>({1});
              tb.reset(dut.rst, 2, 1);
              bool left = false, right = false;
              unsigned observed = 0;
              for (unsigned tick = 0; tick < 12; ++tick) {
                dut.in0_valid = pyc::cpp::Wire<1>({left ? 0u : 1u});
                dut.in0_data = pyc::cpp::Wire<8>({11});
                dut.in1_valid = pyc::cpp::Wire<1>({right ? 0u : 1u});
                dut.in1_data = pyc::cpp::Wire<8>({22});
                dut.in2_valid = pyc::cpp::Wire<1>({1});
                dut.in2_data = pyc::cpp::Wire<1>({0});
                tb.runCycleAutoTrace(tick, nullptr);
                if (dut.in0_valid.value() && dut.in0_ready.value()) left = true;
                if (dut.in1_valid.value() && dut.in1_ready.value()) right = true;
                if (dut.out_valid.value() && observed++ < 3)
                  std::cout << dut.out_data.value() << "\n";
              }
              return left && right && observed >= 3 ? 0 : 3;
            }
        """
        verilator_harness = r"""
            #include "Vwriter_arbitration.h"
            #include <iostream>
            static void cycle(Vwriter_arbitration &dut) {
              dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval();
              dut.clk = 0; dut.eval();
            }
            int main() {
              Vwriter_arbitration dut;
              dut.in0_valid = 0; dut.in1_valid = 0; dut.in2_valid = 0;
              dut.out_ready = 1; dut.rst = 1;
              cycle(dut); cycle(dut); dut.rst = 0; cycle(dut);
              bool left = false, right = false;
              unsigned observed = 0;
              for (unsigned tick = 0; tick < 12; ++tick) {
                dut.in0_valid = !left; dut.in0_data = 11;
                dut.in1_valid = !right; dut.in1_data = 22;
                dut.in2_valid = 1; dut.in2_data = 0;
                cycle(dut);
                if (dut.in0_valid && dut.in0_ready) left = true;
                if (dut.in1_valid && dut.in1_ready) right = true;
                if (dut.out_valid && observed++ < 3)
                  std::cout << unsigned(dut.out_data) << "\n";
              }
              return left && right && observed >= 3 ? 0 : 3;
            }
        """
        with tempfile.TemporaryDirectory() as directory:
            output = self.build_fixture(
                Path(directory),
                fixture,
                "writer_arbitration",
                "builtin.module(ac-verify-value-constraints,ac-freeze-topology)",
            )
            cpp = self.run_cpp(output, "writer_arbitration", cpp_harness)
            rtl = self.run_verilator(
                output, "writer_arbitration", verilator_harness
            )
        self.assertEqual("0\n22\n11\n", cpp)
        self.assertEqual(cpp, rtl)

    def test_replace_is_applied_after_field_merge(self) -> None:
        fixture = (
            ROOT
            / "tests/mlir/agentic-circuit/CodeGen/table-field-replace-order-pyc.mlir"
        )
        cpp_harness = r"""
            #include "field_replace_order.hpp"
            #include <iostream>
            #include <cpp/pyc_tb.hpp>
            int main() {
              pyc::gen::field_replace_order dut;
              pyc::cpp::Testbench<pyc::gen::field_replace_order> tb(dut);
              tb.addClock(dut.clk, 1, 0, false);
              dut.in0_valid = pyc::cpp::Wire<1>({0});
              dut.in1_valid = pyc::cpp::Wire<1>({0});
              dut.in2_valid = pyc::cpp::Wire<1>({0});
              dut.out_ready = pyc::cpp::Wire<1>({1});
              tb.reset(dut.rst, 2, 1);
              bool field = false, replace = false;
              unsigned observed = 0;
              for (unsigned tick = 0; tick < 12; ++tick) {
                dut.in0_valid = pyc::cpp::Wire<1>({field ? 0u : 1u});
                dut.in0_data = pyc::cpp::Wire<1>({1});
                dut.in1_valid = pyc::cpp::Wire<1>({replace ? 0u : 1u});
                dut.in1_data = pyc::cpp::Wire<8>({18});
                dut.in2_valid = pyc::cpp::Wire<1>({1});
                dut.in2_data = pyc::cpp::Wire<1>({0});
                tb.runCycleAutoTrace(tick, nullptr);
                if (dut.in0_valid.value() && dut.in0_ready.value()) field = true;
                if (dut.in1_valid.value() && dut.in1_ready.value()) replace = true;
                if (dut.out_valid.value() && observed++ < 3)
                  std::cout << dut.out_data.value() << "\n";
              }
              return field && replace && observed >= 3 ? 0 : 3;
            }
        """
        verilator_harness = r"""
            #include "Vfield_replace_order.h"
            #include <iostream>
            static void cycle(Vfield_replace_order &dut) {
              dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval();
              dut.clk = 0; dut.eval();
            }
            int main() {
              Vfield_replace_order dut;
              dut.in0_valid = 0; dut.in1_valid = 0; dut.in2_valid = 0;
              dut.out_ready = 1; dut.rst = 1;
              cycle(dut); cycle(dut); dut.rst = 0; cycle(dut);
              bool field = false, replace = false;
              unsigned observed = 0;
              for (unsigned tick = 0; tick < 12; ++tick) {
                dut.in0_valid = !field; dut.in0_data = 1;
                dut.in1_valid = !replace; dut.in1_data = 18;
                dut.in2_valid = 1; dut.in2_data = 0;
                cycle(dut);
                if (dut.in0_valid && dut.in0_ready) field = true;
                if (dut.in1_valid && dut.in1_ready) replace = true;
                if (dut.out_valid && observed++ < 3)
                  std::cout << unsigned(dut.out_data) << "\n";
              }
              return field && replace && observed >= 3 ? 0 : 3;
            }
        """
        with tempfile.TemporaryDirectory() as directory:
            output = self.build_fixture(
                Path(directory),
                fixture,
                "field_replace_order",
                "builtin.module(ac-verify-value-constraints,ac-freeze-topology)",
            )
            cpp = self.run_cpp(output, "field_replace_order", cpp_harness)
            rtl = self.run_verilator(
                output, "field_replace_order", verilator_harness
            )
        self.assertEqual("2\n18\n18\n", cpp)
        self.assertEqual(cpp, rtl)


if __name__ == "__main__":
    unittest.main()
