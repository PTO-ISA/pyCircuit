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
EXAMPLE = ROOT / "examples/agentic-circuit/blocks/array_combinators.py"
ACIR_BIN = Path(os.environ.get("ACIR_BIN", ROOT / ".pycircuit_out/toolchain/build/bin"))
PYC_TOOLCHAIN = Path(
    os.environ.get("PYC_TOOLCHAIN_ROOT", ROOT / ".pycircuit_out/toolchain/install")
)


def _pack(fields: tuple[tuple[int, int], ...]) -> int:
    result = 0
    for width, value in fields:
        result = (result << width) | value
    return result


def _expected(first: int, second: int, third: int) -> int:
    return _pack(
        (
            (8, (first + 1) & 0xFF),
            (8, (third + 1) & 0xFF),
            (8, second),
            (4, second & 0xF),
            (8, (third + 1) & 0xFF),
            (1, int(second != 0)),
            (8, (first + 1) & 0xFF),
            (8, 1),
            (3, first if first < 5 else 0),
            (3, second if second < 5 else 0),
            (3, third if third < 5 else 0),
        )
    )


class ArrayCombinatorRuntimeTest(unittest.TestCase):
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

    def test_map_zip_nested_and_checked_callbacks_match_all_backends(self) -> None:
        cases = ((0, 1, 4), (3, 5, 7), (255, 2, 254))
        inputs = tuple(
            (first << 16) | (second << 8) | third for first, second, third in cases
        )
        initializers = ", ".join(str(value) for value in inputs)
        expected = "".join(
            f"{_expected(first, second, third)}\n" for first, second, third in cases
        )

        with tempfile.TemporaryDirectory(prefix="array-combinator-") as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                EXAMPLE.read_text(encoding="utf-8"),
                "array_combinators",
                source_path=EXAMPLE.relative_to(ROOT).as_posix(),
            )
            self.assertEqual(3, raw.count("func.call @bump"))
            self.assertEqual(1, raw.count("ac.var.range_checked"))
            frozen = work / "model.mlir"
            frozen.write_text(
                _lower_queue_acir(raw, optimizer=self.acir_opt),
                encoding="utf-8",
            )
            frozen_text = frozen.read_text(encoding="utf-8")
            self.assertEqual(3, frozen_text.count("func.call @bump"))
            self.assertEqual(1, frozen_text.count("ac.var.range_checked"))

            gfsim_source = work / "gfsim.cpp"
            gfsim_source.write_text(
                self._run((self.cxxgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            pyc = work / "model.pyc"
            pyc.write_text(self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8")
            pyc_text = pyc.read_text(encoding="utf-8")
            self.assertNotRegex(pyc_text, r"\bscf\.")
            self.assertNotRegex(pyc_text, r"\bindex\b")

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
                textwrap.dedent(f"""
                    #include "{gfsim_source.name}"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>

                    static std::uint64_t pack(const ac_generated::Result &value) {{
                      std::uint64_t result = 0;
                      auto append = [&](unsigned width, std::uint64_t field) {{
                        result = (result << width) |
                                 (field & ((std::uint64_t{{1}} << width) - 1));
                      }};
                      append(8, static_cast<std::uint64_t>(value.mapped_first));
                      append(8, static_cast<std::uint64_t>(value.mapped_last));
                      append(8, static_cast<std::uint64_t>(value.zipped_left));
                      append(4, static_cast<std::uint64_t>(value.zipped_right));
                      append(8, static_cast<std::uint64_t>(value.tuple_next));
                      append(1, static_cast<std::uint64_t>(value.record_valid));
                      append(8, static_cast<std::uint64_t>(value.nested_value));
                      append(8, static_cast<std::uint64_t>(value.updated_nested_value));
                      append(3, static_cast<std::uint64_t>(value.checked_first));
                      append(3, static_cast<std::uint64_t>(value.checked_second));
                      append(3, static_cast<std::uint64_t>(value.checked_third));
                      return result;
                    }}

                    int main() {{
                      ac_generated::ArrayCombinators model;
                      constexpr std::array<std::uint64_t, {len(inputs)}> inputs{{
                          {initializers}}};
                      auto rows = model.dispatch_rows();
                      std::uint64_t epoch = 0;
                      for (std::uint64_t input : inputs) {{
                        ac_generated::Request request{{
                            gfsim::UInt<8>{{input >> 16}},
                            gfsim::UInt<8>{{(input >> 8) & 0xff}},
                            gfsim::UInt<8>{{input & 0xff}}}};
                        if (!model.request().proposePush(request)) return 1;
                        model.request().doXfer({{epoch++, 0}});
                        const std::size_t before = model.sink_0_values().size();
                        for (unsigned step = 0; step != 12 &&
                             model.sink_0_values().size() == before;
                             ++step, ++epoch) {{
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
                        std::cout << pack(value) << '\\n';
                    }}
                    """),
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
                textwrap.dedent(f"""
                    #include "array_combinators.hpp"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>
                    int main() {{
                      pyc::gen::array_combinators dut;
                      pyc::cpp::Testbench<pyc::gen::array_combinators> tb(dut);
                      tb.addClock(dut.clk, 1, 0, false);
                      dut.in_valid = pyc::cpp::Wire<1>(0);
                      dut.out_ready = pyc::cpp::Wire<1>(1);
                      tb.reset(dut.rst, 2, 1);
                      constexpr std::array<std::uint64_t, {len(inputs)}> inputs{{
                          {initializers}}};
                      std::uint64_t cycle = 0;
                      for (std::uint64_t input : inputs) {{
                        bool accepted = false, observed = false;
                        for (unsigned step = 0; step != 12 && !observed; ++step) {{
                          dut.in_valid = pyc::cpp::Wire<1>(accepted ? 0 : 1);
                          dut.in_data = pyc::cpp::Wire<24>(input);
                          tb.runCycleAutoTrace(cycle++, nullptr);
                          if (dut.in_valid.value() && dut.in_ready.value())
                            accepted = true;
                          if (dut.out_valid.value()) {{
                            std::cout << dut.out_data.value() << '\\n';
                            observed = true;
                          }}
                        }}
                        if (!accepted || !observed) return 3;
                      }}
                    }}
                    """),
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
                    pyc_output / "array_combinators.cpp",
                    pyc_harness,
                    self.runtime,
                    "-o",
                    pyc_binary,
                ),
                cwd=work,
            )

            verilator_harness = work / "verilator_harness.cpp"
            verilator_harness.write_text(
                textwrap.dedent(f"""
                    #include "Varray_combinators.h"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    static void tick(Varray_combinators &dut) {{
                      dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval();
                      dut.clk = 0; dut.eval();
                    }}
                    int main() {{
                      Varray_combinators dut;
                      dut.in_valid = 0; dut.out_ready = 1;
                      dut.rst = 1; tick(dut); tick(dut); dut.rst = 0; tick(dut);
                      constexpr std::array<std::uint64_t, {len(inputs)}> inputs{{
                          {initializers}}};
                      for (std::uint64_t input : inputs) {{
                        bool accepted = false, observed = false;
                        for (unsigned step = 0; step != 12 && !observed; ++step) {{
                          dut.in_valid = accepted ? 0 : 1;
                          dut.in_data = input;
                          tick(dut);
                          if (dut.in_valid && dut.in_ready) accepted = true;
                          if (dut.out_valid) {{
                            std::cout << static_cast<std::uint64_t>(dut.out_data)
                                      << '\\n';
                            observed = true;
                          }}
                        }}
                        if (!accepted || !observed) return 4;
                      }}
                    }}
                    """),
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
                    "array_combinators",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "array_combinators.v",
                    verilator_harness,
                ),
                cwd=work,
            )

            gfsim = self._run((gfsim_binary,), cwd=work)
            pyc_cpp = self._run((pyc_binary,), cwd=work)
            verilator = self._run((verilator_object / "Varray_combinators",), cwd=work)

        self.assertEqual(expected, gfsim)
        self.assertEqual(gfsim, pyc_cpp)
        self.assertEqual(pyc_cpp, verilator)


if __name__ == "__main__":
    unittest.main()
