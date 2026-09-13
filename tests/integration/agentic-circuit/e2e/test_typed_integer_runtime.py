from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path

from agentic_circuit._jit import _lower_queue_acir
from agentic_circuit._queue_frontend import lower_queue_source

ROOT = Path(__file__).resolve().parents[4]
EXAMPLE = ROOT / "examples/agentic-circuit/blocks/typed_integer_operations.py"
ACIR_BIN = Path(os.environ.get("ACIR_BIN", ROOT / ".pycircuit_out/acir/dev-llvm22/bin"))
PYC_TOOLCHAIN = Path(
    os.environ.get("PYC_TOOLCHAIN_ROOT", ROOT / ".pycircuit_out/toolchain/install")
)


def _expected(value: int) -> int:
    mask = (1 << 64) - 1
    divisor = value & 0xFF
    quotient = 0 if divisor == 0 else value // divisor
    remainder = 0 if divisor == 0 else value % divisor
    narrow = value & 0x7
    sign_extended = narrow if narrow < 4 else narrow | (mask ^ 0x7)
    return (
        (quotient & 0xFFFF)
        | ((remainder & 0xFF) << 16)
        | (narrow << 24)
        | ((sign_extended & 0xFF) << 32)
        | (17 << 40)
    )


class TypedIntegerRuntimeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.acir_opt = ACIR_BIN / "acir-opt"
        cls.cxxgen = ACIR_BIN / "acir-queue-cxxgen"
        cls.pycgen = ACIR_BIN / "acir-queue-pycgen"
        cls.pycc = Path(
            os.environ.get("PYCC", ROOT / ".pycircuit_out/toolchain/build/bin/pycc")
        )
        cls.compiler = shutil.which("c++")
        cls.verilator = shutil.which("verilator")
        cls.runtime = PYC_TOOLCHAIN / "lib/libpyc6_runtime.a"
        cls.include = PYC_TOOLCHAIN / "include"
        required = (
            cls.acir_opt,
            cls.cxxgen,
            cls.pycgen,
            cls.pycc,
            cls.runtime,
            cls.include,
        )
        if (
            cls.compiler is None
            or cls.verilator is None
            or not all(path.exists() for path in required)
        ):
            raise unittest.SkipTest("integrated ACIR/PYC C++ toolchain is required")

    def _run(self, command: tuple[object, ...], *, cwd: Path) -> str:
        completed = subprocess.run(
            tuple(str(item) for item in command),
            cwd=cwd,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, completed.returncode, completed.stderr)
        return completed.stdout

    def test_u64_casts_and_dynamic_div_rem_match_all_backends(self) -> None:
        inputs = (
            0,
            1,
            3,
            8,
            0x8000000000000000,
            0x8000000000000003,
            0x8000000000000007,
            0xFFFFFFFFFFFFFFFF,
        )
        initializers = ", ".join(f"0x{value:016x}ULL" for value in inputs)
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                EXAMPLE.read_text(encoding="utf-8"),
                "typed_integer_operations",
                static_arguments={"lanes": 1},
            )
            self.assertNotIn("static_assert", raw)
            frozen = work / "model.mlir"
            frozen.write_text(
                _lower_queue_acir(raw, optimizer=self.acir_opt),
                encoding="utf-8",
            )
            frozen_text = frozen.read_text(encoding="utf-8")
            self.assertIn("ac.var.udiv", frozen_text)
            self.assertIn("ac.var.urem", frozen_text)

            gfsim_source = work / "gfsim.cpp"
            gfsim_source.write_text(
                self._run((self.cxxgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            pyc = work / "model.pyc"
            pyc.write_text(self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8")
            pyc_text = pyc.read_text(encoding="utf-8")
            self.assertIn("pyc.udiv", pyc_text)
            self.assertIn("pyc.urem", pyc_text)

            pyc_output = work / "pyc"
            verilog_output = work / "verilog"
            self._run(
                (
                    self.pycc,
                    pyc,
                    "--emit=cpp",
                    "--out-dir",
                    pyc_output,
                    "--hierarchy-policy=strict",
                    "--inline-policy=off",
                ),
                cwd=ROOT,
            )
            self._run(
                (
                    self.pycc,
                    pyc,
                    "--emit=verilog",
                    "--out-dir",
                    verilog_output,
                    "--hierarchy-policy=strict",
                    "--inline-policy=off",
                    "--include-primitives",
                ),
                cwd=ROOT,
            )

            gfsim_harness = work / "gfsim_harness.cpp"
            gfsim_binary = work / "gfsim_model"
            gfsim_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "{gfsim_source.name}"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>

                    int main() {{
                      ac_generated::TypedIntegerOperations model;
                      constexpr std::array<std::uint64_t, {len(inputs)}> inputs{{
                          {initializers}}};
                      auto rows = model.dispatch_rows();
                      std::uint64_t epoch = 0;
                      for (std::uint64_t input : inputs) {{
                        if (!model.incoming().proposePush(gfsim::UInt<64>{{input}}))
                          return 1;
                        model.incoming().doXfer({{epoch++, 0}});
                        for (unsigned step = 0; step < 4; ++step, ++epoch) {{
                          const gfsim::Epoch current{{epoch, 0}};
                          for (auto &row : rows) row.work(row.object, current);
                          for (auto &row : rows)
                            row.xfer(row.object, current, gfsim::XferPhase::Arbitrate);
                          for (auto &row : rows)
                            row.xfer(row.object, current, gfsim::XferPhase::Commit);
                        }}
                      }}
                      if (model.sink_0_values().size() != inputs.size()) return 2;
                      for (const auto &value : model.sink_0_values())
                        std::cout << value.value() << "\\n";
                    }}
                    """
                ),
                encoding="utf-8",
            )
            self._run(
                (
                    self.compiler,
                    "-std=c++20",
                    "-I",
                    ROOT / "simulator/gfsim/include",
                    gfsim_harness,
                    "-o",
                    gfsim_binary,
                ),
                cwd=work,
            )

            pyc_harness = work / "pyc_harness.cpp"
            pyc_binary = work / "pyc_model"
            pyc_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "typed_integer_operations.hpp"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>

                    int main() {{
                      pyc::gen::typed_integer_operations dut;
                      pyc::cpp::Testbench<pyc::gen::typed_integer_operations> tb(dut);
                      tb.addClock(dut.clk, 1, 0, false);
                      dut.in_valid = pyc::cpp::Wire<1>(0);
                      dut.out_ready = pyc::cpp::Wire<1>(1);
                      tb.reset(dut.rst, 2, 1);
                      constexpr std::array<std::uint64_t, {len(inputs)}> inputs{{
                          {initializers}}};
                      std::uint64_t cycle = 0;
                      for (std::uint64_t input : inputs) {{
                        bool accepted = false;
                        bool observed = false;
                        for (unsigned step = 0; step < 12 && !observed; ++step) {{
                          dut.in_valid = pyc::cpp::Wire<1>(accepted ? 0 : 1);
                          dut.in_data = pyc::cpp::Wire<64>(input);
                          tb.runCycleAutoTrace(cycle++, nullptr);
                          if (dut.in_valid.value() && dut.in_ready.value())
                            accepted = true;
                          if (dut.out_valid.value()) {{
                            std::cout << dut.out_data.value() << "\\n";
                            observed = true;
                          }}
                        }}
                        if (!accepted || !observed) return 3;
                      }}
                    }}
                    """
                ),
                encoding="utf-8",
            )
            self._run(
                (
                    self.compiler,
                    "-std=c++20",
                    "-I",
                    pyc_output,
                    "-I",
                    self.include,
                    pyc_output / "typed_integer_operations.cpp",
                    pyc_harness,
                    self.runtime,
                    "-o",
                    pyc_binary,
                ),
                cwd=work,
            )

            verilator_harness = work / "verilator_harness.cpp"
            verilator_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "Vtyped_integer_operations.h"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>

                    static void tick(Vtyped_integer_operations &dut) {{
                      dut.clk = 0; dut.eval();
                      dut.clk = 1; dut.eval();
                      dut.clk = 0; dut.eval();
                    }}

                    int main() {{
                      Vtyped_integer_operations dut;
                      dut.in_valid = 0; dut.out_ready = 1; dut.rst = 1;
                      tick(dut); tick(dut); dut.rst = 0; tick(dut);
                      constexpr std::array<std::uint64_t, {len(inputs)}> inputs{{
                          {initializers}}};
                      for (std::uint64_t input : inputs) {{
                        bool accepted = false;
                        bool observed = false;
                        for (unsigned step = 0; step < 12 && !observed; ++step) {{
                          dut.in_valid = accepted ? 0 : 1;
                          dut.in_data = input;
                          tick(dut);
                          if (dut.in_valid && dut.in_ready) accepted = true;
                          if (dut.out_valid) {{
                            std::cout << static_cast<std::uint64_t>(dut.out_data)
                                      << std::endl;
                            observed = true;
                          }}
                        }}
                        if (!accepted || !observed) return 4;
                      }}
                    }}
                    """
                ),
                encoding="utf-8",
            )
            verilator_object = work / "verilator_obj"
            self._run(
                (
                    self.verilator,
                    "--cc",
                    "--exe",
                    "--build",
                    "-Wno-fatal",
                    "--top-module",
                    "typed_integer_operations",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "typed_integer_operations.v",
                    verilator_harness,
                ),
                cwd=work,
            )

            gfsim = self._run((gfsim_binary,), cwd=work)
            pyc_cpp = self._run((pyc_binary,), cwd=work)
            verilator = self._run(
                (verilator_object / "Vtyped_integer_operations",), cwd=work
            )

        expected = "".join(f"{_expected(value)}\n" for value in inputs)
        self.assertEqual(expected, gfsim)
        self.assertEqual(gfsim, pyc_cpp)
        self.assertEqual(pyc_cpp, verilator)


if __name__ == "__main__":
    unittest.main()
