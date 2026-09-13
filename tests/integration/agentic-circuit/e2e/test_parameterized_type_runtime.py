from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path

from agentic_circuit._jit import _lower_queue_acir
from agentic_circuit._queue_frontend import lower_queue_source, parse_queue_program

ROOT = Path(__file__).resolve().parents[4]
EXAMPLE = ROOT / "examples/agentic-circuit/types/parameterized_types.py"
ACIR_BIN = Path(os.environ.get("ACIR_BIN", ROOT / ".pycircuit_out/acir/dev-llvm22/bin"))
PYC_TOOLCHAIN = Path(
    os.environ.get("PYC_TOOLCHAIN_ROOT", ROOT / ".pycircuit_out/toolchain/install")
)


class ParameterizedTypeRuntimeTest(unittest.TestCase):
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
            raise unittest.SkipTest("integrated ACIR/PYC toolchain is required")

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

    def test_dependent_aggregate_executes_in_all_backends_with_stalls(self) -> None:
        source = EXAMPLE.read_text(encoding="utf-8")
        static_arguments = {"rob_entries": 4, "issue_width": 2}
        program = parse_queue_program(
            source, "parameterized_types", static_arguments=static_arguments
        )
        group = next(item for item in program.payloads if item.name == "IssueGroup")
        self.assertEqual(41, group.descriptor.bit_width())
        group_symbol = group.descriptor.symbol
        entries = (3 << 19) | 7
        count = 2
        packed = (entries << 3) | count

        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                source,
                "parameterized_types",
                static_arguments=static_arguments,
                source_path=EXAMPLE.relative_to(ROOT).as_posix(),
            )
            frozen = work / "model.mlir"
            frozen.write_text(
                _lower_queue_acir(raw, optimizer=self.acir_opt), encoding="utf-8"
            )
            gfsim_source = work / "gfsim.cpp"
            gfsim_source.write_text(
                self._run((self.cxxgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            pyc = work / "model.pyc"
            pyc.write_text(self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8")
            cpp_output = work / "cpp"
            verilog_output = work / "verilog"
            self._run(
                (
                    self.pycc,
                    pyc,
                    "--emit=cpp",
                    "--out-dir",
                    cpp_output,
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
            gfsim_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "{gfsim_source.name}"
                    #include <cstdint>
                    #include <iostream>

                    int main() {{
                      ac_generated::ParameterizedTypes model;
                      ac_generated::{group_symbol} input{{
                          gfsim::UInt<38>{{{entries}ULL}}, gfsim::UInt<3>{{{count}}}}};
                      if (!model.value().proposePush(input)) return 1;
                      auto rows = model.dispatch_rows();
                      for (std::uint64_t epoch = 0; epoch < 6; ++epoch) {{
                        const gfsim::Epoch current{{epoch, 0}};
                        for (auto &row : rows) row.work(row.object, current);
                        for (auto &row : rows)
                          row.xfer(row.object, current, gfsim::XferPhase::Arbitrate);
                        for (auto &row : rows)
                          row.xfer(row.object, current, gfsim::XferPhase::Commit);
                      }}
                      if (model.sink_0_values().size() != 1) return 2;
                      const auto &value = model.sink_0_values().front();
                      std::cout << ((value.entries.value() << 3) | value.count.value())
                                << std::endl;
                    }}
                    """
                ),
                encoding="utf-8",
            )
            gfsim_binary = work / "gfsim_model"
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

            cpp_harness = work / "cpp_harness.cpp"
            cpp_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "parameterized_types.hpp"
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>

                    int main() {{
                      pyc::gen::parameterized_types dut;
                      pyc::cpp::Testbench<pyc::gen::parameterized_types> tb(dut);
                      tb.addClock(dut.clk, 1, 0, false);
                      dut.in_valid = pyc::cpp::Wire<1>(0);
                      dut.out_ready = pyc::cpp::Wire<1>(0);
                      tb.reset(dut.rst, 2, 1);
                      bool accepted = false;
                      for (std::uint64_t cycle = 0; cycle < 16; ++cycle) {{
                        dut.in_valid = pyc::cpp::Wire<1>(accepted ? 0 : 1);
                        dut.in_data = pyc::cpp::Wire<41>({packed}ULL);
                        dut.out_ready = pyc::cpp::Wire<1>(
                            cycle == 2 || cycle == 3 ? 0 : 1);
                        tb.runCycleAutoTrace(cycle, nullptr);
                        if (dut.in_valid.value() && dut.in_ready.value())
                          accepted = true;
                        if (dut.out_valid.value() && dut.out_ready.value()) {{
                          std::cout << cycle << " " << dut.out_data.value()
                                    << std::endl;
                          return accepted ? 0 : 3;
                        }}
                      }}
                      return 4;
                    }}
                    """
                ),
                encoding="utf-8",
            )
            cpp_binary = work / "cpp_model"
            self._run(
                (
                    self.compiler,
                    "-std=c++20",
                    "-I",
                    cpp_output,
                    "-I",
                    self.include,
                    cpp_output / "parameterized_types.cpp",
                    cpp_harness,
                    self.runtime,
                    "-o",
                    cpp_binary,
                ),
                cwd=work,
            )

            verilator_harness = work / "verilator_harness.cpp"
            verilator_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "Vparameterized_types.h"
                    #include <cstdint>
                    #include <iostream>

                    static void tick(Vparameterized_types &dut) {{
                      dut.clk = 0; dut.eval();
                      dut.clk = 1; dut.eval();
                      dut.clk = 0; dut.eval();
                    }}

                    int main() {{
                      Vparameterized_types dut;
                      dut.in_valid = 0; dut.out_ready = 0; dut.rst = 1;
                      tick(dut); tick(dut); dut.rst = 0; tick(dut);
                      bool accepted = false;
                      for (std::uint64_t cycle = 0; cycle < 16; ++cycle) {{
                        dut.in_valid = accepted ? 0 : 1;
                        dut.in_data = {packed}ULL;
                        dut.out_ready = cycle == 2 || cycle == 3 ? 0 : 1;
                        tick(dut);
                        if (dut.in_valid && dut.in_ready) accepted = true;
                        if (dut.out_valid && dut.out_ready) {{
                          std::cout << cycle << " "
                                    << static_cast<std::uint64_t>(dut.out_data)
                                    << std::endl;
                          return accepted ? 0 : 5;
                        }}
                      }}
                      return 6;
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
                    "parameterized_types",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "parameterized_types.v",
                    verilator_harness,
                ),
                cwd=work,
            )

            gfsim = self._run((gfsim_binary,), cwd=work)
            cpp = self._run((cpp_binary,), cwd=work)
            verilator = self._run(
                (verilator_object / "Vparameterized_types",), cwd=work
            )

        self.assertEqual(f"{packed}\n", gfsim)
        self.assertEqual(cpp, verilator)
        self.assertEqual(str(packed), cpp.split()[1])


if __name__ == "__main__":
    unittest.main()
