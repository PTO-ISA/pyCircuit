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
EXAMPLE = ROOT / "examples/agentic-circuit/blocks/bounded_integer_operations.py"
STATE_FIXTURE = (
    ROOT
    / "tests/integration/agentic-circuit/e2e/fixtures/array_update_state/architecture.py"
)
ACIR_BIN = Path(os.environ.get("ACIR_BIN", ROOT / ".pycircuit_out/toolchain/build/bin"))
PYC_TOOLCHAIN = Path(
    os.environ.get("PYC_TOOLCHAIN_ROOT", ROOT / ".pycircuit_out/toolchain/install")
)


def _expected(raw: int, values: tuple[int, ...]) -> tuple[int, ...]:
    wrapped = raw % 5
    saturated = min(max(raw, 4), 8)
    valid = int(raw < 5)
    checked = raw if valid else 0
    updated = list(values)
    updated[checked] = raw
    chained = list(updated)
    chained[0] = 99
    return (
        wrapped,
        saturated,
        checked,
        valid,
        wrapped + 1,
        values[checked],
        raw % 2,
        raw % 8,
        raw,
        values[raw & 0x3],
        values[0],
        raw if 4 <= raw <= 8 else 4,
        int(4 <= raw <= 8),
        1,
        int(min(max(raw, 4), 8) > wrapped),
        wrapped,
        raw,
        updated[0],
        updated[4],
        values[checked],
        chained[0],
        chained[checked],
        updated[checked],
    )


def _pack_fields(fields: tuple[tuple[int, int], ...]) -> int:
    packed = 0
    for width, value in fields:
        packed = (packed << width) | value
    return packed


def _recursive_expected(raw: int, replacement: int) -> int:
    wide_index = raw % 65
    source_wide = raw & 0x7
    replacement_wide = replacement & 0x7
    return _pack_fields(
        (
            (8, raw),
            (8, replacement),
            (1, 0),
            (1, 1),
            (1, 0),
            (1, 1),
            (2, raw % 3),
            (2, replacement % 3),
            (3, source_wide),
            (3, replacement_wide),
            (3, replacement_wide if wide_index == 0 else source_wide),
            (3, replacement_wide if wide_index == 64 else source_wide),
            (3, 7 if wide_index == 0 else replacement_wide),
            (3, replacement_wide),
        )
    )


class BoundedRangeRuntimeTest(unittest.TestCase):
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

    def test_conversions_and_dynamic_array_match_all_backends(self) -> None:
        values = (11, 22, 33, 44, 55)
        raw_values = (0, 3, 4, 5, 8, 9, 255)
        packed_values = 0
        for value in values:
            packed_values = (packed_values << 8) | value
        packed_inputs = tuple((packed_values << 8) | raw for raw in raw_values)
        expected = "".join(
            " ".join(str(value) for value in _expected(raw, values)) + "\n"
            for raw in raw_values
        )
        raw_initializers = ", ".join(str(value) for value in raw_values)
        packed_initializers = ", ".join(str(value) + "ULL" for value in packed_inputs)
        output_count = 23
        gfsim_size_checks = " ||\n                            ".join(
            f"model.sink_{index}_values().size() != before + 1"
            for index in range(output_count)
        )
        gfsim_output_stream = " << ' ' << ".join(
            f"model.sink_{index}_values()[index].value()"
            for index in range(output_count)
        )
        pyc_ready_assignments = " ".join(
            f"dut.out{index}_ready = pyc::cpp::Wire<1>(ready);"
            for index in range(output_count)
        )
        verilator_ready_assignments = " ".join(
            f"dut.out{index}_ready = ready;" for index in range(output_count)
        )
        pyc_output_valid = " && ".join(
            f"dut.out{index}_valid.value()" for index in range(output_count)
        )
        verilator_output_valid = " && ".join(
            f"dut.out{index}_valid" for index in range(output_count)
        )
        pyc_output_values = " << ' ' << ".join(
            f"dut.out{index}_data.value()" for index in range(output_count)
        )
        verilator_output_values = " << ' ' << ".join(
            f"static_cast<std::uint64_t>(dut.out{index}_data)"
            for index in range(output_count)
        )

        with tempfile.TemporaryDirectory(prefix="bounded-range-runtime-") as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                EXAMPLE.read_text(encoding="utf-8"),
                "bounded_integer_operations",
                source_path=EXAMPLE.relative_to(ROOT).as_posix(),
            )
            frozen = work / "model.mlir"
            frozen.write_text(
                _lower_queue_acir(raw, optimizer=self.acir_opt),
                encoding="utf-8",
            )
            frozen_text = frozen.read_text(encoding="utf-8")
            self.assertIn("ac.var.range_wrap", frozen_text)
            self.assertIn("ac.var.range_saturate", frozen_text)
            self.assertIn("ac.var.range_checked", frozen_text)
            self.assertIn("ac.var.dynamic_element", frozen_text)

            gfsim_source = work / "gfsim.cpp"
            gfsim_source.write_text(
                self._run((self.cxxgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            pyc = work / "model.pyc"
            pyc.write_text(
                self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8"
            )
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
                textwrap.dedent(
                    f"""
                    #include "{gfsim_source.name}"
                    #include <array>
                    #include <cstddef>
                    #include <cstdint>
                    #include <iostream>

                    int main() {{
                      ac_generated::BoundedIntegerOperations model;
                      constexpr std::array<std::uint64_t, {len(raw_values)}> raw{{
                          {raw_initializers}}};
                      auto rows = model.dispatch_rows();
                      std::uint64_t epoch = 0;
                      for (std::uint64_t input : raw) {{
                        ac_generated::Request request{{
                            gfsim::UInt<40>{{{packed_values}ULL}},
                            gfsim::UInt<8>{{input}}}};
                        if (!model.request().proposePush(request)) return 1;
                        model.request().doXfer({{epoch++, 0}});
                        const std::size_t before = model.sink_0_values().size();
                        for (unsigned step = 0; step != 12 &&
                             model.sink_0_values().size() == before; ++step, ++epoch) {{
                          const gfsim::Epoch current{{epoch, 0}};
                          for (auto &row : rows) row.work(row.object, current);
                          for (auto &row : rows)
                            row.xfer(row.object, current, gfsim::XferPhase::Arbitrate);
                          for (auto &row : rows)
                            row.xfer(row.object, current, gfsim::XferPhase::Commit);
                        }}
                        if ({gfsim_size_checks}) return 2;
                      }}
                      for (std::size_t index = 0; index != raw.size(); ++index)
                        std::cout << {gfsim_output_stream} << '\\n';
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

            driver = f"""
              constexpr std::array<std::uint64_t, {len(raw_values)}> inputs{{
                  {packed_initializers}}};
              std::size_t offered = 0;
              std::size_t observed = 0;
              for (std::uint64_t cycle = 0; cycle != 96; ++cycle) {{
                const bool valid = offered < inputs.size();
                SET_INPUT_VALID(valid);
                SET_INPUT_DATA(valid ? inputs[offered] : 0);
                SET_OUTPUT_READY(cycle % 7 != 2 && cycle % 7 != 3);
                STEP;
                if (INPUT_VALID && INPUT_READY) ++offered;
                if (OUTPUT_VALID && OUTPUT_READY) {{
                  std::cout << cycle << ' ' << OUTPUT_VALUES << '\\n';
                  ++observed;
                }}
              }}
              return offered == inputs.size() && observed == inputs.size() ? 0 : 3;
            """

            pyc_driver = (
                driver.replace(
                    "SET_INPUT_VALID(valid)",
                    "dut.in_valid = pyc::cpp::Wire<1>(valid ? 1 : 0)",
                )
                .replace(
                    "SET_INPUT_DATA(valid ? inputs[offered] : 0)",
                    "dut.in_data = pyc::cpp::Wire<48>(valid ? inputs[offered] : 0)",
                )
                .replace(
                    "SET_OUTPUT_READY(cycle % 7 != 2 && cycle % 7 != 3)",
                    "{ const bool ready = cycle % 7 != 2 && cycle % 7 != 3; "
                    + pyc_ready_assignments
                    + " }",
                )
                .replace("STEP", "tb.runCycleAutoTrace(cycle, nullptr)")
                .replace("INPUT_VALID", "dut.in_valid.value()")
                .replace("INPUT_READY", "dut.in_ready.value()")
                .replace("OUTPUT_VALID", f"({pyc_output_valid})")
                .replace("OUTPUT_READY", "dut.out0_ready.value()")
                .replace(
                    "OUTPUT_VALUES",
                    pyc_output_values,
                )
            )
            pyc_harness = work / "pyc_harness.cpp"
            pyc_binary = work / "pyc_model"
            pyc_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "bounded_integer_operations.hpp"
                    #include <array>
                    #include <cstddef>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>
                    int main() {{
                      pyc::gen::bounded_integer_operations dut;
                      pyc::cpp::Testbench<pyc::gen::bounded_integer_operations> tb(dut);
                      tb.addClock(dut.clk, 1, 0, false);
                      dut.in_valid = pyc::cpp::Wire<1>(0);
                      {" ".join(f"dut.out{index}_ready = pyc::cpp::Wire<1>(0);" for index in range(output_count))}
                      tb.reset(dut.rst, 2, 1);
                    {pyc_driver}
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
                    pyc_output / "bounded_integer_operations.cpp",
                    pyc_harness,
                    self.runtime,
                    "-o",
                    pyc_binary,
                ),
                cwd=work,
            )

            verilator_driver = (
                driver.replace("SET_INPUT_VALID(valid)", "dut.in_valid = valid")
                .replace(
                    "SET_INPUT_DATA(valid ? inputs[offered] : 0)",
                    "dut.in_data = valid ? inputs[offered] : 0",
                )
                .replace(
                    "SET_OUTPUT_READY(cycle % 7 != 2 && cycle % 7 != 3)",
                    "{ const bool ready = cycle % 7 != 2 && cycle % 7 != 3; "
                    + verilator_ready_assignments
                    + " }",
                )
                .replace("STEP", "tick(dut)")
                .replace("INPUT_VALID", "dut.in_valid")
                .replace("INPUT_READY", "dut.in_ready")
                .replace("OUTPUT_VALID", f"({verilator_output_valid})")
                .replace("OUTPUT_READY", "dut.out0_ready")
                .replace(
                    "OUTPUT_VALUES",
                    verilator_output_values,
                )
            )
            verilator_harness = work / "verilator_harness.cpp"
            verilator_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "Vbounded_integer_operations.h"
                    #include <array>
                    #include <cstddef>
                    #include <cstdint>
                    #include <iostream>
                    static void tick(Vbounded_integer_operations &dut) {{
                      dut.clk = 0; dut.eval();
                      dut.clk = 1; dut.eval();
                      dut.clk = 0; dut.eval();
                    }}
                    int main() {{
                      Vbounded_integer_operations dut;
                      dut.in_valid = 0;
                      {" ".join(f"dut.out{index}_ready = 0;" for index in range(output_count))}
                      dut.rst = 1; tick(dut); tick(dut); dut.rst = 0; tick(dut);
                    {verilator_driver}
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
                    "bounded_integer_operations",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "bounded_integer_operations.v",
                    verilator_harness,
                ),
                cwd=work,
            )

            gfsim = self._run((gfsim_binary,), cwd=work)
            pyc_cpp = self._run((pyc_binary,), cwd=work)
            verilator = self._run(
                (verilator_object / "Vbounded_integer_operations",), cwd=work
            )

        self.assertEqual(expected, gfsim)
        self.assertEqual(pyc_cpp, verilator)
        pyc_values = "".join(
            line.split(" ", 1)[1] + "\n" for line in pyc_cpp.splitlines()
        )
        self.assertEqual(expected, pyc_values)

    def test_recursive_and_wide_array_updates_match_all_backends(self) -> None:
        cases = ((0, 9), (2, 14), (64, 5), (255, 131))
        inputs = tuple((raw << 8) | replacement for raw, replacement in cases)
        initializers = ", ".join(str(value) for value in inputs)
        expected = "".join(
            f"{_recursive_expected(raw, replacement)}\n" for raw, replacement in cases
        )

        with tempfile.TemporaryDirectory(
            prefix="recursive-array-runtime-"
        ) as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                EXAMPLE.read_text(encoding="utf-8"),
                "recursive_array_updates",
                source_path=EXAMPLE.relative_to(ROOT).as_posix(),
            )
            frozen = work / "model.mlir"
            frozen.write_text(
                _lower_queue_acir(raw, optimizer=self.acir_opt),
                encoding="utf-8",
            )
            frozen_text = frozen.read_text(encoding="utf-8")
            self.assertEqual(5, frozen_text.count("ac.var.with_element"))
            self.assertIn("!ac.value_array<65 x i3>", frozen_text)

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

            gfsim_harness = work / "gfsim_recursive.cpp"
            gfsim_binary = work / "gfsim_recursive"
            gfsim_harness.write_text(
                textwrap.dedent(f"""
                    #include "{gfsim_source.name}"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>

                    static std::uint64_t pack(
                        const ac_generated::RecursiveArrayResult &value) {{
                      std::uint64_t result = 0;
                      auto append = [&](unsigned width, std::uint64_t field) {{
                        result = (result << width) |
                                 (field & ((std::uint64_t{{1}} << width) - 1));
                      }};
                      append(8, static_cast<std::uint64_t>(value.source_struct_tag));
                      append(8, static_cast<std::uint64_t>(value.updated_struct_tag));
                      append(1, static_cast<std::uint64_t>(value.source_struct_mode));
                      append(1, static_cast<std::uint64_t>(value.updated_struct_mode));
                      append(1, static_cast<std::uint64_t>(value.source_enum));
                      append(1, static_cast<std::uint64_t>(value.updated_enum));
                      append(2, static_cast<std::uint64_t>(value.source_range));
                      append(2, static_cast<std::uint64_t>(value.updated_range));
                      append(3, static_cast<std::uint64_t>(value.source_wide));
                      append(3, static_cast<std::uint64_t>(value.updated_wide));
                      append(3, static_cast<std::uint64_t>(value.wide_first));
                      append(3, static_cast<std::uint64_t>(value.wide_last));
                      append(3, static_cast<std::uint64_t>(value.chained_selected));
                      append(3, static_cast<std::uint64_t>(value.updated_after_chain));
                      return result;
                    }}

                    int main() {{
                      ac_generated::RecursiveArrayUpdates model;
                      constexpr std::array<std::uint64_t, {len(inputs)}> inputs{{
                          {initializers}}};
                      auto rows = model.dispatch_rows();
                      std::uint64_t epoch = 0;
                      for (std::uint64_t input : inputs) {{
                        ac_generated::RecursiveArrayRequest request{{
                            gfsim::UInt<8>{{input >> 8}},
                            gfsim::UInt<8>{{input & 0xff}}}};
                        if (!model.request().proposePush(request)) return 1;
                        model.request().doXfer({{epoch++, 0}});
                        const std::size_t before = model.sink_0_values().size();
                        for (unsigned step = 0; step != 8 &&
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

            pyc_harness = work / "pyc_recursive.cpp"
            pyc_binary = work / "pyc_recursive"
            pyc_harness.write_text(
                textwrap.dedent(f"""
                    #include "recursive_array_updates.hpp"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>
                    int main() {{
                      pyc::gen::recursive_array_updates dut;
                      pyc::cpp::Testbench<pyc::gen::recursive_array_updates> tb(dut);
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
                          dut.in_data = pyc::cpp::Wire<16>(input);
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
                    pyc_output / "recursive_array_updates.cpp",
                    pyc_harness,
                    self.runtime,
                    "-o",
                    pyc_binary,
                ),
                cwd=work,
            )

            verilator_harness = work / "verilator_recursive.cpp"
            verilator_harness.write_text(
                textwrap.dedent(f"""
                    #include "Vrecursive_array_updates.h"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    static void tick(Vrecursive_array_updates &dut) {{
                      dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval();
                      dut.clk = 0; dut.eval();
                    }}
                    int main() {{
                      Vrecursive_array_updates dut;
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
            verilator_object = work / "verilator_recursive_obj"
            self._run(
                (
                    self.verilator,
                    "--cc",
                    "--exe",
                    "--build",
                    "-Wno-fatal",
                    "--top-module",
                    "recursive_array_updates",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "recursive_array_updates.v",
                    verilator_harness,
                ),
                cwd=work,
            )

            gfsim = self._run((gfsim_binary,), cwd=work)
            pyc_cpp = self._run((pyc_binary,), cwd=work)
            verilator = self._run(
                (verilator_object / "Vrecursive_array_updates",), cwd=work
            )

        self.assertEqual(expected, gfsim)
        self.assertEqual(gfsim, pyc_cpp)
        self.assertEqual(pyc_cpp, verilator)

    def test_array_update_state_commits_once_on_all_backends(self) -> None:
        cases = (
            (0, 2, 11, 0),
            (0, 2, 22, 11),
            (1, 2, 33, 0),
            (0, 2, 44, 22),
        )
        inputs = tuple(
            (slot << 24) | (index << 16) | (replacement << 8)
            for slot, index, replacement, _ in cases
        )
        initializers = ", ".join(str(value) for value in inputs)
        expected = "".join(
            f"{(slot << 24) | (index << 16) | (replacement << 8) | previous}\n"
            for slot, index, replacement, previous in cases
        )

        with tempfile.TemporaryDirectory(prefix="array-update-state-") as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                STATE_FIXTURE.read_text(encoding="utf-8"),
                "array_update_state",
                source_path=STATE_FIXTURE.relative_to(ROOT).as_posix(),
            )
            self.assertEqual(1, raw.count("ac.var.assign_element"))
            frozen = work / "model.mlir"
            frozen.write_text(
                _lower_queue_acir(raw, optimizer=self.acir_opt),
                encoding="utf-8",
            )
            frozen_text = frozen.read_text(encoding="utf-8")
            self.assertNotIn("ac.var.assign_element", frozen_text)
            self.assertEqual(1, frozen_text.count("ac.var.with_element"))

            gfsim_source = work / "gfsim.cpp"
            gfsim_source.write_text(
                self._run((self.cxxgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            pyc = work / "model.pyc"
            pyc.write_text(self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8")
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

            gfsim_harness = work / "gfsim_state.cpp"
            gfsim_binary = work / "gfsim_state"
            gfsim_harness.write_text(
                textwrap.dedent(f"""
                    #include "{gfsim_source.name}"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>

                    static std::uint64_t pack(const ac_generated::Request &value) {{
                      return (static_cast<std::uint64_t>(value.slot) << 24) |
                             (static_cast<std::uint64_t>(value.index) << 16) |
                             (static_cast<std::uint64_t>(value.replacement) << 8) |
                             static_cast<std::uint64_t>(value.previous);
                    }}

                    int main() {{
                      ac_generated::ArrayUpdateState model;
                      constexpr std::array<std::uint64_t, {len(inputs)}> inputs{{
                          {initializers}}};
                      auto rows = model.dispatch_rows();
                      std::uint64_t epoch = 0;
                      for (std::uint64_t input : inputs) {{
                        ac_generated::Request request{{
                            gfsim::UInt<1>{{input >> 24}},
                            gfsim::UInt<8>{{(input >> 16) & 0xff}},
                            gfsim::UInt<8>{{(input >> 8) & 0xff}},
                            gfsim::UInt<8>{{0}}}};
                        if (!model.request().proposePush(request)) return 1;
                        model.request().doXfer({{epoch++, 0}});
                        const std::size_t before = model.sink_0_values().size();
                        for (unsigned step = 0; step != 16 &&
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

            pyc_harness = work / "pyc_state.cpp"
            pyc_binary = work / "pyc_state"
            pyc_harness.write_text(
                textwrap.dedent(f"""
                    #include "array_update_state.hpp"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>
                    int main() {{
                      pyc::gen::array_update_state dut;
                      pyc::cpp::Testbench<pyc::gen::array_update_state> tb(dut);
                      tb.addClock(dut.clk, 1, 0, false);
                      dut.in_valid = pyc::cpp::Wire<1>(0);
                      dut.out_ready = pyc::cpp::Wire<1>(1);
                      tb.reset(dut.rst, 2, 1);
                      constexpr std::array<std::uint64_t, {len(inputs)}> inputs{{
                          {initializers}}};
                      std::uint64_t cycle = 0;
                      for (std::uint64_t input : inputs) {{
                        bool accepted = false, observed = false;
                        for (unsigned step = 0; step != 20 && !observed; ++step) {{
                          dut.in_valid = pyc::cpp::Wire<1>(accepted ? 0 : 1);
                          dut.in_data = pyc::cpp::Wire<25>(input);
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
                    pyc_output / "array_update_state.cpp",
                    pyc_harness,
                    self.runtime,
                    "-o",
                    pyc_binary,
                ),
                cwd=work,
            )

            verilator_harness = work / "verilator_state.cpp"
            verilator_harness.write_text(
                textwrap.dedent(f"""
                    #include "Varray_update_state.h"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    static void tick(Varray_update_state &dut) {{
                      dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval();
                      dut.clk = 0; dut.eval();
                    }}
                    int main() {{
                      Varray_update_state dut;
                      dut.in_valid = 0; dut.out_ready = 1;
                      dut.rst = 1; tick(dut); tick(dut); dut.rst = 0; tick(dut);
                      constexpr std::array<std::uint64_t, {len(inputs)}> inputs{{
                          {initializers}}};
                      for (std::uint64_t input : inputs) {{
                        bool accepted = false, observed = false;
                        for (unsigned step = 0; step != 20 && !observed; ++step) {{
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
            verilator_object = work / "verilator_state_obj"
            self._run(
                (
                    self.verilator,
                    "--cc",
                    "--exe",
                    "--build",
                    "-Wno-fatal",
                    "--top-module",
                    "array_update_state",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "array_update_state.v",
                    verilator_harness,
                ),
                cwd=work,
            )

            gfsim = self._run((gfsim_binary,), cwd=work)
            pyc_cpp = self._run((pyc_binary,), cwd=work)
            verilator = self._run((verilator_object / "Varray_update_state",), cwd=work)

        self.assertEqual(expected, gfsim)
        self.assertEqual(gfsim, pyc_cpp)
        self.assertEqual(pyc_cpp, verilator)

    def test_full_u64_range_preserves_maximum_value_on_all_backends(self) -> None:
        inputs = (0, 1, 0x8000000000000000, 0xFFFFFFFFFFFFFFFF)
        initializers = ", ".join(f"0x{value:016x}ULL" for value in inputs)
        expected = "".join(f"{value} {value} {value} 1\n" for value in inputs)

        with tempfile.TemporaryDirectory(prefix="bounded-u64-runtime-") as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                EXAMPLE.read_text(encoding="utf-8"),
                "bounded_full_u64",
                source_path=EXAMPLE.relative_to(ROOT).as_posix(),
            )
            frozen = work / "model.mlir"
            frozen.write_text(
                _lower_queue_acir(raw, optimizer=self.acir_opt),
                encoding="utf-8",
            )
            gfsim_source = work / "gfsim.cpp"
            gfsim_source.write_text(
                self._run((self.cxxgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            pyc = work / "model.pyc"
            pyc.write_text(
                self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            pyc_text = pyc.read_text(encoding="utf-8")
            self.assertNotIn("18446744073709551616", pyc_text)
            self.assertNotRegex(pyc_text, r"\bpyc\.urem\b")

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

            gfsim_harness = work / "gfsim_u64.cpp"
            gfsim_binary = work / "gfsim_u64"
            gfsim_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "{gfsim_source.name}"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    int main() {{
                      ac_generated::BoundedFullU64 model;
                      constexpr std::array<std::uint64_t, {len(inputs)}> inputs{{
                          {initializers}}};
                      auto rows = model.dispatch_rows();
                      std::uint64_t epoch = 0;
                      for (std::uint64_t input : inputs) {{
                        if (!model.raw().proposePush(gfsim::UInt<64>{{input}})) return 1;
                        model.raw().doXfer({{epoch++, 0}});
                        const std::size_t before = model.sink_0_values().size();
                        for (unsigned step = 0; step != 8 &&
                             model.sink_0_values().size() == before; ++step, ++epoch) {{
                          const gfsim::Epoch current{{epoch, 0}};
                          for (auto &row : rows) row.work(row.object, current);
                          for (auto &row : rows)
                            row.xfer(row.object, current, gfsim::XferPhase::Arbitrate);
                          for (auto &row : rows)
                            row.xfer(row.object, current, gfsim::XferPhase::Commit);
                        }}
                      }}
                      if (model.sink_0_values().size() != inputs.size() ||
                          model.sink_1_values().size() != inputs.size() ||
                          model.sink_2_values().size() != inputs.size() ||
                          model.sink_3_values().size() != inputs.size()) return 2;
                      for (std::size_t index = 0; index != inputs.size(); ++index)
                        std::cout << model.sink_0_values()[index].value() << ' '
                                  << model.sink_1_values()[index].value() << ' '
                                  << model.sink_2_values()[index].value() << ' '
                                  << model.sink_3_values()[index].value() << '\\n';
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

            pyc_harness = work / "pyc_u64.cpp"
            pyc_binary = work / "pyc_u64"
            pyc_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "bounded_full_u64.hpp"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>
                    int main() {{
                      pyc::gen::bounded_full_u64 dut;
                      pyc::cpp::Testbench<pyc::gen::bounded_full_u64> tb(dut);
                      tb.addClock(dut.clk, 1, 0, false);
                      dut.in_valid = pyc::cpp::Wire<1>(0);
                      dut.out0_ready = pyc::cpp::Wire<1>(1);
                      dut.out1_ready = pyc::cpp::Wire<1>(1);
                      dut.out2_ready = pyc::cpp::Wire<1>(1);
                      dut.out3_ready = pyc::cpp::Wire<1>(1);
                      tb.reset(dut.rst, 2, 1);
                      constexpr std::array<std::uint64_t, {len(inputs)}> inputs{{
                          {initializers}}};
                      std::size_t offered = 0, observed = 0;
                      for (std::uint64_t cycle = 0; cycle != 48; ++cycle) {{
                        const bool valid = offered < inputs.size();
                        dut.in_valid = pyc::cpp::Wire<1>(valid);
                        dut.in_data = pyc::cpp::Wire<64>(valid ? inputs[offered] : 0);
                        tb.runCycleAutoTrace(cycle, nullptr);
                        if (dut.in_valid.value() && dut.in_ready.value()) ++offered;
                        if (dut.out0_valid.value() && dut.out1_valid.value() &&
                            dut.out2_valid.value() && dut.out3_valid.value()) {{
                          std::cout << dut.out0_data.value() << ' '
                                    << dut.out1_data.value() << ' '
                                    << dut.out2_data.value() << ' '
                                    << dut.out3_data.value() << '\\n';
                          ++observed;
                        }}
                      }}
                      return offered == inputs.size() && observed == inputs.size() ? 0 : 3;
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
                    pyc_output / "bounded_full_u64.cpp",
                    pyc_harness,
                    self.runtime,
                    "-o",
                    pyc_binary,
                ),
                cwd=work,
            )

            verilator_harness = work / "verilator_u64.cpp"
            verilator_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "Vbounded_full_u64.h"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    static void tick(Vbounded_full_u64 &dut) {{
                      dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval();
                      dut.clk = 0; dut.eval();
                    }}
                    int main() {{
                      Vbounded_full_u64 dut;
                      dut.in_valid = 0; dut.out0_ready = 1; dut.out1_ready = 1;
                      dut.out2_ready = 1; dut.out3_ready = 1;
                      dut.rst = 1; tick(dut); tick(dut); dut.rst = 0; tick(dut);
                      constexpr std::array<std::uint64_t, {len(inputs)}> inputs{{
                          {initializers}}};
                      std::size_t offered = 0, observed = 0;
                      for (std::uint64_t cycle = 0; cycle != 48; ++cycle) {{
                        const bool valid = offered < inputs.size();
                        dut.in_valid = valid;
                        dut.in_data = valid ? inputs[offered] : 0;
                        tick(dut);
                        if (dut.in_valid && dut.in_ready) ++offered;
                        if (dut.out0_valid && dut.out1_valid &&
                            dut.out2_valid && dut.out3_valid) {{
                          std::cout << static_cast<std::uint64_t>(dut.out0_data)
                                    << ' ' << static_cast<std::uint64_t>(dut.out1_data)
                                    << ' ' << static_cast<std::uint64_t>(dut.out2_data)
                                    << ' ' << static_cast<std::uint64_t>(dut.out3_data)
                                    << '\\n';
                          ++observed;
                        }}
                      }}
                      return offered == inputs.size() && observed == inputs.size() ? 0 : 4;
                    }}
                    """
                ),
                encoding="utf-8",
            )
            verilator_object = work / "verilator_u64_obj"
            self._run(
                (
                    self.verilator,
                    "--cc",
                    "--exe",
                    "--build",
                    "-Wno-fatal",
                    "--top-module",
                    "bounded_full_u64",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "bounded_full_u64.v",
                    verilator_harness,
                ),
                cwd=work,
            )

            gfsim = self._run((gfsim_binary,), cwd=work)
            pyc_cpp = self._run((pyc_binary,), cwd=work)
            verilator = self._run(
                (verilator_object / "Vbounded_full_u64",), cwd=work
            )

        self.assertEqual(expected, gfsim)
        self.assertEqual(expected, pyc_cpp)
        self.assertEqual(pyc_cpp, verilator)


if __name__ == "__main__":
    unittest.main()
