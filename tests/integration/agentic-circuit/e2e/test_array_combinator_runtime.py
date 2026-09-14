from __future__ import annotations

import json
import os
import re
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


def _reduction_expected(first: int, second: int, third: int) -> int:
    values = (first, second, third)
    flags = tuple(value != 0 for value in values)
    valid_indices = [index for index, valid in enumerate(flags) if valid]
    first_index = valid_indices[0] if valid_indices else 0
    best_index = (
        min(valid_indices, key=lambda index: (values[index], index))
        if valid_indices
        else 0
    )
    bounded = tuple(value % 5 for value in values)
    range_best_index = min(range(3), key=lambda index: (bounded[index], index))
    return _pack(
        (
            (1, int(all(flags))),
            (1, int(any(flags))),
            (2, sum(flags)),
            (2, sum(flags)),
            (8, sum(values) & 0xFF),
            (8, (first * second * third) & 0xFF),
            (8, min(values)),
            (8, max(values)),
            (1, int(flags[0] ^ flags[1] ^ flags[2])),
            (2, first_index),
            (1, int(bool(valid_indices))),
            (2, best_index),
            (1, int(bool(valid_indices))),
            (2, range_best_index),
        )
    )


def _balanced_add_shape(acir: str) -> tuple[int, int]:
    depths: dict[str, int] = {}
    additions = 0
    maximum = 0
    for line in acir.splitlines():
        element = re.search(r"%([^ ]+) = ac\.var\.element ", line)
        if element is not None:
            depths[element.group(1)] = 0
            continue
        addition = re.search(r"%([^ ]+) = ac\.var\.add %([^, ]+), %([^ ]+)", line)
        if addition is None:
            continue
        left = depths.get(addition.group(2))
        right = depths.get(addition.group(3))
        if left is None or right is None:
            continue
        depth = max(left, right) + 1
        depths[addition.group(1)] = depth
        additions += 1
        maximum = max(maximum, depth)
    return additions, maximum


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

    def test_fold_tree_is_balanced_at_boundary_extents(self) -> None:
        for extent, expected_depth in ((1, 0), (3, 2), (5, 3), (16, 4), (65, 7)):
            source = f"""import agentic_circuit as ac
@ac.struct
class Request:
    values: ac.array[{extent}, ac.u8]
@ac.rule
def fold(request: Request) -> ac.u8:
    return request.values.fold(kind="add")
@ac.system
def pipeline(request: Request) -> ac.u8:
    result = fold(request)
    return result
"""
            raw = lower_queue_source(source, "pipeline")
            additions, depth = _balanced_add_shape(raw)
            with self.subTest(extent=extent):
                self.assertEqual(extent - 1, additions)
                self.assertEqual(expected_depth, depth)

    def test_argmin_65_compiles_with_default_logic_depth_budget(self) -> None:
        source = """import agentic_circuit as ac
@ac.struct
class Item:
    age: ac.u8
    valid: bool
@ac.struct
class Request:
    items: ac.array[65, Item]
@ac.rule
def select(request: Request) -> ac.index[65]:
    best = request.items.argmin(
        key=lambda item: item.age,
        where=lambda item: item.valid,
    )
    return best.index
@ac.system
def pipeline(request: Request) -> ac.index[65]:
    result = select(request)
    return result
"""
        with tempfile.TemporaryDirectory(prefix="array-argmin-65-") as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                source, "pipeline", source_path="array_argmin_cost.py"
            )
            frozen = work / "model.mlir"
            frozen.write_text(
                _lower_queue_acir(raw, optimizer=self.acir_opt),
                encoding="utf-8",
            )
            bundle = work / "bundle"
            self._run(
                (
                    self.cxxgen,
                    frozen,
                    "--output-root",
                    bundle,
                    "--sdk-product-version",
                    "6.1.0",
                    "--sdk-source-revision",
                    "a" * 40,
                ),
                cwd=ROOT,
            )
            cost_report = json.loads(
                (bundle / "share/generated/cost-report.json").read_text(
                    encoding="utf-8"
                )
            )
            select_cost = next(
                rule
                for rule in cost_report["modules"][0]["rules"]
                if rule["display_rule_name"] == "select"
            )
            self.assertEqual(647, select_cost["metrics"]["queuegraph_nodes"]["value"])
            self.assertEqual(
                65,
                select_cost["metrics"]["fixed_array_expansion_bound"]["value"],
            )
            self.assertEqual(34, select_cost["metrics"]["logic_depth"]["value"])
            self.assertEqual(
                "array_argmin_cost.py",
                select_cost["source_provenance"]["origins"][0]["frames"][0][
                    "file"
                ],
            )
            pyc = work / "model.pyc"
            pyc.write_text(self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8")
            profile = work / "profile.json"
            self._run(
                (
                    self.pycc,
                    pyc,
                    "--emit=none",
                    "--out-dir",
                    work / "checked",
                    "--hierarchy-policy=strict",
                    "--inline-policy=off",
                    f"--profile-json={profile}",
                ),
                cwd=ROOT,
            )
            report = json.loads(profile.read_text(encoding="utf-8"))
            self.assertEqual(32, report["compile_stats"]["logic_depth_limit"])
            self.assertEqual(31, report["compile_stats"]["max_logic_depth"])

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

    def test_ordered_scan_prefixes_match_all_backends(self) -> None:
        cases = ((0, 0, 0), (1, 2, 3), (255, 1, 2), (7, 9, 11))
        inputs = tuple(
            (first << 16) | (second << 8) | third for first, second, third in cases
        )
        initializers = ", ".join(str(value) for value in inputs)

        def expected(first: int, second: int, third: int) -> int:
            subtract_first = (-first) & 0xFF
            subtract_second = (subtract_first - second) & 0xFF
            subtract_third = (subtract_second - third) & 0xFF
            nonzero_last = (10 + first + second + third) & 0xFF
            reset_first = 0 if first == 0 else (5 + first) & 0xFF
            reset_second = 0 if second == 0 else (reset_first + second) & 0xFF
            reset_third = 0 if third == 0 else (reset_second + third) & 0xFF
            tuple_first = 0 if first == 0 else (first + first) & 0xFF
            tuple_second = 0 if second == 0 else (tuple_first + second) & 0xFF
            tuple_third = 0 if third == 0 else (tuple_second + third) & 0xFF
            return _pack(
                (
                    (8, subtract_first),
                    (8, subtract_second),
                    (8, subtract_third),
                    (8, nonzero_last),
                    (8, reset_second),
                    (8, reset_third),
                    (8, tuple_first),
                    (8, tuple_third),
                )
            )

        expected_output = "".join(
            f"{expected(first, second, third)}\n" for first, second, third in cases
        )

        with tempfile.TemporaryDirectory(prefix="array-scan-") as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                EXAMPLE.read_text(encoding="utf-8"),
                "array_scans",
                source_path=EXAMPLE.relative_to(ROOT).as_posix(),
            )
            self.assertEqual(3, raw.count("func.call @subtract"))
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
            pyc.write_text(self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8")
            pyc_text = pyc.read_text(encoding="utf-8")
            self.assertNotRegex(pyc_text, r"\bscf\.")
            self.assertNotRegex(pyc_text, r"\bindex\b")

            pyc_output = work / "pyc-scan"
            verilog_output = work / "verilog-scan"
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

            gfsim_harness = work / "gfsim_scan.cpp"
            gfsim_binary = work / "gfsim_scan"
            gfsim_harness.write_text(
                textwrap.dedent(f"""
                    #include "{gfsim_source.name}"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>

                    static std::uint64_t pack(
                        const ac_generated::ScanResult &value) {{
                      std::uint64_t result = 0;
                      auto append = [&](std::uint64_t field) {{
                        result = (result << 8) | (field & 0xff);
                      }};
                      append(static_cast<std::uint64_t>(value.subtract_first));
                      append(static_cast<std::uint64_t>(value.subtract_second));
                      append(static_cast<std::uint64_t>(value.subtract_third));
                      append(static_cast<std::uint64_t>(value.nonzero_last));
                      append(static_cast<std::uint64_t>(value.reset_second));
                      append(static_cast<std::uint64_t>(value.reset_third));
                      append(static_cast<std::uint64_t>(value.tuple_first));
                      append(static_cast<std::uint64_t>(value.tuple_third));
                      return result;
                    }}

                    int main() {{
                      ac_generated::ArrayScans model;
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

            pyc_harness = work / "pyc_scan.cpp"
            pyc_binary = work / "pyc_scan"
            pyc_harness.write_text(
                textwrap.dedent(f"""
                    #include "array_scans.hpp"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>
                    int main() {{
                      pyc::gen::array_scans dut;
                      pyc::cpp::Testbench<pyc::gen::array_scans> tb(dut);
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
                    pyc_output / "array_scans.cpp",
                    pyc_harness,
                    self.runtime,
                    "-o",
                    pyc_binary,
                ),
                cwd=work,
            )

            verilator_harness = work / "verilator_scan.cpp"
            verilator_harness.write_text(
                textwrap.dedent(f"""
                    #include "Varray_scans.h"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    static void tick(Varray_scans &dut) {{
                      dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval();
                      dut.clk = 0; dut.eval();
                    }}
                    int main() {{
                      Varray_scans dut;
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
            verilator_object = work / "verilator_scan_obj"
            self._run(
                (
                    self.verilator,
                    "--cc",
                    "--exe",
                    "--build",
                    "-Wno-fatal",
                    "--top-module",
                    "array_scans",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "array_scans.v",
                    verilator_harness,
                ),
                cwd=work,
            )

            gfsim = self._run((gfsim_binary,), cwd=work)
            pyc_cpp = self._run((pyc_binary,), cwd=work)
            verilator = self._run((verilator_object / "Varray_scans",), cwd=work)

        self.assertEqual(expected_output, gfsim)
        self.assertEqual(gfsim, pyc_cpp)
        self.assertEqual(pyc_cpp, verilator)

    def test_count_and_last_selection_at_extent_65_match_all_backends(
        self,
    ) -> None:
        all_flags = ", ".join(["request.flag"] * 65)
        last_flags = ", ".join(["False"] * 64 + ["request.flag"])
        source = f"""import agentic_circuit as ac
@ac.struct
class Request:
    flag: bool
@ac.struct
class Arrays:
    all_flags: ac.array[65, bool]
    last_flags: ac.array[65, bool]
@ac.struct
class Result:
    count: ac.range[0, 66]
    complete: bool
    first_index: ac.index[65]
    first_valid: bool
@ac.rule
def reduce(request: Request) -> Result:
    arrays = Arrays(
        all_flags=({all_flags}),
        last_flags=({last_flags}),
    )
    first = arrays.last_flags.first(where=lambda flag: flag)
    return Result(
        count=arrays.all_flags.count(),
        complete=arrays.all_flags.all(),
        first_index=first.index,
        first_valid=first.valid,
    )
@ac.system
def pipeline(request: Request) -> Result:
    result = reduce(request)
    return result
"""
        expected = "0\n" + f"{_pack(((7, 65), (1, 1), (7, 64), (1, 1)))}\n"

        with tempfile.TemporaryDirectory(prefix="array-extent-65-") as temporary:
            work = Path(temporary)
            raw = lower_queue_source(source, "pipeline")
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
            pyc.write_text(self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8")
            pyc_output = work / "pyc-65"
            verilog_output = work / "verilog-65"
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

            gfsim_harness = work / "gfsim_65.cpp"
            gfsim_binary = work / "gfsim_65"
            gfsim_harness.write_text(
                textwrap.dedent(f"""
                    #include "{gfsim_source.name}"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>

                    static std::uint64_t pack(const ac_generated::Result &value) {{
                      return (static_cast<std::uint64_t>(value.count) << 9) |
                             (static_cast<std::uint64_t>(value.complete) << 8) |
                             (static_cast<std::uint64_t>(value.first_index) << 1) |
                             static_cast<std::uint64_t>(value.first_valid);
                    }}

                    int main() {{
                      ac_generated::Pipeline model;
                      constexpr std::array<unsigned, 2> inputs{{0, 1}};
                      auto rows = model.dispatch_rows();
                      std::uint64_t epoch = 0;
                      for (unsigned input : inputs) {{
                        if (!model.request().proposePush(
                                ac_generated::Request{{gfsim::UInt<1>{{input}}}}))
                          return 1;
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

            pyc_harness = work / "pyc_65.cpp"
            pyc_binary = work / "pyc_65"
            pyc_harness.write_text(
                textwrap.dedent("""
                    #include "pipeline.hpp"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>
                    int main() {
                      pyc::gen::pipeline dut;
                      pyc::cpp::Testbench<pyc::gen::pipeline> tb(dut);
                      tb.addClock(dut.clk, 1, 0, false);
                      dut.in_valid = pyc::cpp::Wire<1>(0);
                      dut.out_ready = pyc::cpp::Wire<1>(1);
                      tb.reset(dut.rst, 2, 1);
                      constexpr std::array<unsigned, 2> inputs{0, 1};
                      std::uint64_t cycle = 0;
                      for (unsigned input : inputs) {
                        bool accepted = false, observed = false;
                        for (unsigned step = 0; step != 12 && !observed; ++step) {
                          dut.in_valid = pyc::cpp::Wire<1>(accepted ? 0 : 1);
                          dut.in_data = pyc::cpp::Wire<1>(input);
                          tb.runCycleAutoTrace(cycle++, nullptr);
                          if (dut.in_valid.value() && dut.in_ready.value())
                            accepted = true;
                          if (dut.out_valid.value()) {
                            std::cout << dut.out_data.value() << '\\n';
                            observed = true;
                          }
                        }
                        if (!accepted || !observed) return 3;
                      }
                    }
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
                    pyc_output / "pipeline.cpp",
                    pyc_harness,
                    self.runtime,
                    "-o",
                    pyc_binary,
                ),
                cwd=work,
            )

            verilator_harness = work / "verilator_65.cpp"
            verilator_harness.write_text(
                textwrap.dedent("""
                    #include "Vpipeline.h"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    static void tick(Vpipeline &dut) {
                      dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval();
                      dut.clk = 0; dut.eval();
                    }
                    int main() {
                      Vpipeline dut;
                      dut.in_valid = 0; dut.out_ready = 1;
                      dut.rst = 1; tick(dut); tick(dut); dut.rst = 0; tick(dut);
                      constexpr std::array<unsigned, 2> inputs{0, 1};
                      for (unsigned input : inputs) {
                        bool accepted = false, observed = false;
                        for (unsigned step = 0; step != 12 && !observed; ++step) {
                          dut.in_valid = accepted ? 0 : 1;
                          dut.in_data = input;
                          tick(dut);
                          if (dut.in_valid && dut.in_ready) accepted = true;
                          if (dut.out_valid) {
                            std::cout << static_cast<std::uint64_t>(dut.out_data)
                                      << '\\n';
                            observed = true;
                          }
                        }
                        if (!accepted || !observed) return 4;
                      }
                    }
                    """),
                encoding="utf-8",
            )
            verilator_object = work / "verilator_65_obj"
            self._run(
                (
                    self.verilator,
                    "--cc",
                    "--exe",
                    "--build",
                    "-Wno-fatal",
                    "--top-module",
                    "pipeline",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "pipeline.v",
                    verilator_harness,
                ),
                cwd=work,
            )

            gfsim = self._run((gfsim_binary,), cwd=work)
            pyc_cpp = self._run((pyc_binary,), cwd=work)
            verilator = self._run((verilator_object / "Vpipeline",), cwd=work)

        self.assertEqual(expected, gfsim)
        self.assertEqual(gfsim, pyc_cpp)
        self.assertEqual(pyc_cpp, verilator)

    def test_balanced_reductions_and_stable_selection_match_all_backends(
        self,
    ) -> None:
        cases = ((0, 0, 0), (0, 0, 5), (7, 7, 9), (3, 1, 2), (255, 2, 3))
        inputs = tuple(
            (first << 16) | (second << 8) | third for first, second, third in cases
        )
        initializers = ", ".join(str(value) for value in inputs)
        expected = "".join(
            f"{_reduction_expected(first, second, third)}\n"
            for first, second, third in cases
        )

        with tempfile.TemporaryDirectory(prefix="array-reduction-") as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                EXAMPLE.read_text(encoding="utf-8"),
                "array_reductions",
                source_path=EXAMPLE.relative_to(ROOT).as_posix(),
            )
            self.assertEqual(0, raw.count("ac.var.range_refine"))
            self.assertEqual(2, raw.count("ac.var.range_cmp"))
            frozen = work / "model.mlir"
            frozen.write_text(
                _lower_queue_acir(raw, optimizer=self.acir_opt),
                encoding="utf-8",
            )
            frozen_text = frozen.read_text(encoding="utf-8")
            self.assertEqual(0, frozen_text.count("ac.var.range_refine"))
            self.assertEqual(2, frozen_text.count("ac.var.range_cmp"))

            gfsim_source = work / "gfsim.cpp"
            gfsim_source.write_text(
                self._run((self.cxxgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            pyc = work / "model.pyc"
            pyc.write_text(self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8")
            pyc_text = pyc.read_text(encoding="utf-8")
            self.assertNotRegex(pyc_text, r"\bscf\.")
            self.assertNotRegex(pyc_text, r"\bindex\b")

            pyc_output = work / "pyc-reduction"
            verilog_output = work / "verilog-reduction"
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

            gfsim_harness = work / "gfsim_reduction.cpp"
            gfsim_binary = work / "gfsim_reduction"
            gfsim_harness.write_text(
                textwrap.dedent(f"""
                    #include "{gfsim_source.name}"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>

                    static std::uint64_t pack(
                        const ac_generated::ReductionResult &value) {{
                      std::uint64_t result = 0;
                      auto append = [&](unsigned width, std::uint64_t field) {{
                        result = (result << width) |
                                 (field & ((std::uint64_t{{1}} << width) - 1));
                      }};
                      append(1, static_cast<std::uint64_t>(value.complete));
                      append(1, static_cast<std::uint64_t>(value.present));
                      append(2, static_cast<std::uint64_t>(value.count));
                      append(2, static_cast<std::uint64_t>(value.helper_count));
                      append(8, static_cast<std::uint64_t>(value.total));
                      append(8, static_cast<std::uint64_t>(value.product));
                      append(8, static_cast<std::uint64_t>(value.minimum));
                      append(8, static_cast<std::uint64_t>(value.maximum));
                      append(1, static_cast<std::uint64_t>(value.parity));
                      append(2, static_cast<std::uint64_t>(value.first_index));
                      append(1, static_cast<std::uint64_t>(value.first_valid));
                      append(2, static_cast<std::uint64_t>(value.best_index));
                      append(1, static_cast<std::uint64_t>(value.best_valid));
                      append(2, static_cast<std::uint64_t>(value.range_best_index));
                      return result;
                    }}

                    int main() {{
                      ac_generated::ArrayReductions model;
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

            pyc_harness = work / "pyc_reduction.cpp"
            pyc_binary = work / "pyc_reduction"
            pyc_harness.write_text(
                textwrap.dedent(f"""
                    #include "array_reductions.hpp"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>
                    int main() {{
                      pyc::gen::array_reductions dut;
                      pyc::cpp::Testbench<pyc::gen::array_reductions> tb(dut);
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
                    pyc_output / "array_reductions.cpp",
                    pyc_harness,
                    self.runtime,
                    "-o",
                    pyc_binary,
                ),
                cwd=work,
            )

            verilator_harness = work / "verilator_reduction.cpp"
            verilator_harness.write_text(
                textwrap.dedent(f"""
                    #include "Varray_reductions.h"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    static void tick(Varray_reductions &dut) {{
                      dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval();
                      dut.clk = 0; dut.eval();
                    }}
                    int main() {{
                      Varray_reductions dut;
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
            verilator_object = work / "verilator_reduction_obj"
            self._run(
                (
                    self.verilator,
                    "--cc",
                    "--exe",
                    "--build",
                    "-Wno-fatal",
                    "--top-module",
                    "array_reductions",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "array_reductions.v",
                    verilator_harness,
                ),
                cwd=work,
            )

            gfsim = self._run((gfsim_binary,), cwd=work)
            pyc_cpp = self._run((pyc_binary,), cwd=work)
            verilator = self._run((verilator_object / "Varray_reductions",), cwd=work)

        self.assertEqual(expected, gfsim)
        self.assertEqual(gfsim, pyc_cpp)
        self.assertEqual(pyc_cpp, verilator)


if __name__ == "__main__":
    unittest.main()
