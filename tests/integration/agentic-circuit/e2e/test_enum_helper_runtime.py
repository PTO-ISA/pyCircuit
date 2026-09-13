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
EXAMPLE = ROOT / "examples/agentic-circuit/pipelines/enum_helpers.py"
STRESS = (
    ROOT
    / "tests/integration/agentic-circuit/e2e/fixtures/enum_stress/architecture.py"
)
ACIR_BIN = Path(os.environ.get("ACIR_BIN", ROOT / ".pycircuit_out/acir/dev-llvm22/bin"))
PYC_TOOLCHAIN = Path(
    os.environ.get("PYC_TOOLCHAIN_ROOT", ROOT / ".pycircuit_out/toolchain/install")
)


def _pack(fields: tuple[tuple[int, int], ...]) -> int:
    value = 0
    for field, width in fields:
        value = (value << width) | field
    return value


class EnumHelperRuntimeTest(unittest.TestCase):
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

    def test_enum_helpers_match_all_backends_with_invalid_ingress(self) -> None:
        transactions = (
            (1, 0b00, 1, 1, 1, 1, 0, 0, 0, 10),
            (3, 0b01, 3, 3, 1, 3, 1, 0, 1, 11),
            (9, 0b10, 9, 9, 1, 9, 1, 0, 1, 12),
            (2, 0b11, 2, 1, 0, 15, 1, 1, 0, 255),
            (15, 0b00, 15, 15, 1, 1, 0, 0, 0, 13),
        )
        inputs = tuple(
            _pack(((raw, 4), (mask, 2), (selector, 4)))
            for raw, mask, selector, *_ in transactions
        )
        expected = tuple(
            _pack(
                (
                    (decoded, 4),
                    (decoded_valid, 1),
                    (onehot, 4),
                    (present, 1),
                    (conflict, 1),
                    (selected, 1),
                    (classification, 8),
                )
            )
            for (
                _raw,
                _mask,
                _selector,
                decoded,
                decoded_valid,
                onehot,
                present,
                conflict,
                selected,
                classification,
            ) in transactions
        )
        gfsim_inputs = ",\n".join(
            "ac_generated::Command{"
            f"gfsim::UInt<4>{{{raw}}}, gfsim::UInt<2>{{{mask}}}, "
            f"static_cast<ac_generated::Opcode>({selector})}}"
            for raw, mask, selector, *_ in transactions
        )
        packed_inputs = ", ".join(f"{value}ULL" for value in inputs)

        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                EXAMPLE.read_text(encoding="utf-8"),
                "enum_helpers",
                source_path=EXAMPLE.relative_to(ROOT).as_posix(),
            )
            self.assertIn("ac.var.enum_match", raw)
            frozen_text = _lower_queue_acir(raw, optimizer=self.acir_opt)
            self.assertNotIn("ac.var.enum_match", frozen_text)
            frozen = work / "model.mlir"
            frozen.write_text(frozen_text, encoding="utf-8")
            gfsim_source = work / "gfsim.cpp"
            gfsim_source.write_text(
                self._run((self.cxxgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            pyc = work / "model.pyc"
            pyc.write_text(
                self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            self.assertNotRegex(pyc.read_text(encoding="utf-8"), r"\bscf\.")
            cpp_output = work / "pyc"
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
                      ac_generated::EnumHelpers model;
                      const std::array<ac_generated::Command, 5> inputs{{
                          {gfsim_inputs}}};
                      auto rows = model.dispatch_rows();
                      std::size_t offered = 0;
                      for (std::uint64_t epoch = 0; epoch < 18; ++epoch) {{
                        if (offered < inputs.size() &&
                            model.command().proposePush(inputs[offered]))
                          ++offered;
                        const gfsim::Epoch current{{epoch, 0}};
                        for (auto &row : rows) row.work(row.object, current);
                        for (auto &row : rows)
                          row.xfer(row.object, current, gfsim::XferPhase::Arbitrate);
                        for (auto &row : rows)
                          row.xfer(row.object, current, gfsim::XferPhase::Commit);
                      }}
                      if (offered != inputs.size() ||
                          model.sink_0_values().size() != inputs.size()) return 2;
                      for (const auto &value : model.sink_0_values()) {{
                        std::uint64_t packed = 0;
                        auto append = [&](std::uint64_t field, unsigned width) {{
                          packed = (packed << width) | field;
                        }};
                        append(static_cast<std::uint64_t>(value.decoded), 4);
                        append(value.decoded_valid, 1);
                        append(static_cast<std::uint64_t>(value.onehot), 4);
                        append(value.onehot_present, 1);
                        append(value.onehot_conflict, 1);
                        append(value.selected, 1);
                        append(value.classification.value(), 8);
                        std::cout << packed << "\\n";
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
                    #include "enum_helpers.hpp"
                    #include <array>
                    #include <cstddef>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>

                    int main() {{
                      pyc::gen::enum_helpers dut;
                      pyc::cpp::Testbench<pyc::gen::enum_helpers> tb(dut);
                      tb.addClock(dut.clk, 1, 0, false);
                      dut.in_valid = pyc::cpp::Wire<1>(0);
                      dut.out_ready = pyc::cpp::Wire<1>(0);
                      tb.reset(dut.rst, 2, 1);
                      constexpr std::array<std::uint64_t, 5> inputs{{
                          {packed_inputs}}};
                      std::size_t accepted = 0;
                      std::size_t produced = 0;
                      for (std::uint64_t cycle = 0; cycle < 32; ++cycle) {{
                        const bool offered = accepted < inputs.size();
                        dut.in_valid = pyc::cpp::Wire<1>(offered ? 1 : 0);
                        dut.in_data = pyc::cpp::Wire<10>(
                            offered ? inputs[accepted] : 0);
                        const bool ready = cycle != 3 && cycle != 4 && cycle != 9;
                        dut.out_ready = pyc::cpp::Wire<1>(ready ? 1 : 0);
                        tb.runCycleAutoTrace(cycle, nullptr);
                        if (dut.in_valid.value() && dut.in_ready.value()) {{
                          std::cout << "A " << cycle << " " << accepted << "\\n";
                          ++accepted;
                        }}
                        if (dut.out_valid.value() && dut.out_ready.value()) {{
                          std::cout << "O " << cycle << " "
                                    << dut.out_data.value() << "\\n";
                          ++produced;
                        }}
                      }}
                      return accepted == inputs.size() && produced == inputs.size()
                                 ? 0
                                 : 4;
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
                    cpp_output,
                    "-I",
                    self.include,
                    cpp_output / "enum_helpers.cpp",
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
                    #include "Venum_helpers.h"
                    #include <array>
                    #include <cstddef>
                    #include <cstdint>
                    #include <iostream>

                    static void tick(Venum_helpers &dut) {{
                      dut.clk = 0; dut.eval();
                      dut.clk = 1; dut.eval();
                      dut.clk = 0; dut.eval();
                    }}

                    int main() {{
                      Venum_helpers dut;
                      dut.in_valid = 0;
                      dut.out_ready = 0;
                      dut.rst = 1;
                      tick(dut); tick(dut);
                      dut.rst = 0;
                      tick(dut);
                      constexpr std::array<std::uint64_t, 5> inputs{{
                          {packed_inputs}}};
                      std::size_t accepted = 0;
                      std::size_t produced = 0;
                      for (std::uint64_t cycle = 0; cycle < 32; ++cycle) {{
                        const bool offered = accepted < inputs.size();
                        dut.in_valid = offered ? 1 : 0;
                        dut.in_data = offered ? inputs[accepted] : 0;
                        dut.out_ready = cycle != 3 && cycle != 4 && cycle != 9;
                        tick(dut);
                        if (dut.in_valid && dut.in_ready) {{
                          std::cout << "A " << cycle << " " << accepted
                                    << std::endl;
                          ++accepted;
                        }}
                        if (dut.out_valid && dut.out_ready) {{
                          std::cout << "O " << cycle << " "
                                    << static_cast<std::uint64_t>(dut.out_data)
                                    << std::endl;
                          ++produced;
                        }}
                      }}
                      return accepted == inputs.size() && produced == inputs.size()
                                 ? 0
                                 : 5;
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
                    "enum_helpers",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "enum_helpers.v",
                    verilator_harness,
                ),
                cwd=work,
            )

            gfsim = self._run((gfsim_binary,), cwd=work)
            pyc_output_text = self._run((pyc_binary,), cwd=work)
            verilator = self._run(
                (verilator_object / "Venum_helpers",), cwd=work
            )

        expected_text = "".join(f"{value}\n" for value in expected)
        self.assertEqual(expected_text, gfsim)
        self.assertEqual(pyc_output_text, verilator)
        observed = "".join(
            line.split(maxsplit=2)[2] + "\n"
            for line in pyc_output_text.splitlines()
            if line.startswith("O ")
        )
        self.assertEqual(expected_text, observed)
        self.assertEqual(
            len(transactions),
            sum(line.startswith("A ") for line in pyc_output_text.splitlines()),
        )

    def test_wide_checked_and_sixty_five_lane_onehot_match_all_backends(
        self,
    ) -> None:
        low = 0x7FFF_FFFF_FFFF_FFFF
        high = 0x8000_0000_0000_0000
        maximum = 0xFFFF_FFFF_FFFF_FFFF
        transactions = (
            (0, 0, low, 65, 0, 0, low, 1),
            (0, 1, high, 0, 1, 0, high, 1),
            (1, 0, maximum, 64, 1, 0, maximum, 1),
            (1, 1, 0, 66, 1, 1, low, 0),
        )
        gfsim_inputs = ",\n".join(
            "ac_generated::StressInput{"
            f"gfsim::UInt<65>{{gfsim::UInt<65>::word_array_type{{{mask_low}ULL, "
            f"{mask_high}ULL}}}}, gfsim::UInt<64>{{{raw}ULL}}}}"
            for mask_low, mask_high, raw, *_ in transactions
        )
        expected = "".join(
            f"{lane} {present} {conflict} {decoded} {valid}\n"
            for (
                _mask_low,
                _mask_high,
                _raw,
                lane,
                present,
                conflict,
                decoded,
                valid,
            ) in transactions
        )
        cpp_rows = ",\n".join(
            f"Input{{{mask_low}ULL, {mask_high}ULL, {raw}ULL}}"
            for mask_low, mask_high, raw, *_ in transactions
        )

        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                STRESS.read_text(encoding="utf-8"),
                "enum_stress",
                source_path=STRESS.relative_to(ROOT).as_posix(),
            )
            frozen_text = _lower_queue_acir(raw, optimizer=self.acir_opt)
            self.assertNotIn("ac.var.enum_match", frozen_text)
            frozen = work / "stress.mlir"
            frozen.write_text(frozen_text, encoding="utf-8")
            gfsim_source = work / "stress_gfsim.cpp"
            gfsim_source.write_text(
                self._run((self.cxxgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            pyc = work / "stress.pyc"
            pyc.write_text(
                self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            pyc_text = pyc.read_text(encoding="utf-8")
            self.assertNotRegex(pyc_text, r"\b(?:scf\.|vector<)")
            cpp_output = work / "stress_cpp"
            verilog_output = work / "stress_verilog"
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

            gfsim_harness = work / "stress_gfsim_harness.cpp"
            gfsim_binary = work / "stress_gfsim"
            gfsim_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "{gfsim_source.name}"
                    #include <array>
                    #include <cstddef>
                    #include <cstdint>
                    #include <iostream>

                    int main() {{
                      ac_generated::EnumStress model;
                      const std::array<ac_generated::StressInput, 4> inputs{{
                          {gfsim_inputs}}};
                      auto rows = model.dispatch_rows();
                      std::size_t offered = 0;
                      for (std::uint64_t epoch = 0; epoch < 16; ++epoch) {{
                        if (offered < inputs.size() &&
                            model.value().proposePush(inputs[offered]))
                          ++offered;
                        const gfsim::Epoch current{{epoch, 0}};
                        for (auto &row : rows) row.work(row.object, current);
                        for (auto &row : rows)
                          row.xfer(row.object, current, gfsim::XferPhase::Arbitrate);
                        for (auto &row : rows)
                          row.xfer(row.object, current, gfsim::XferPhase::Commit);
                      }}
                      if (offered != inputs.size() ||
                          model.sink_0_values().size() != inputs.size()) return 2;
                      for (const auto &value : model.sink_0_values())
                        std::cout << static_cast<unsigned>(value.lane) << " "
                                  << value.present.value() << " "
                                  << value.conflict.value() << " "
                                  << static_cast<std::uint64_t>(value.decoded) << " "
                                  << value.valid.value() << "\\n";
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

            pyc_harness = work / "stress_pyc_harness.cpp"
            pyc_binary = work / "stress_pyc"
            pyc_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "enum_stress.hpp"
                    #include <array>
                    #include <cstddef>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>

                    struct Input {{
                      std::uint64_t maskLow;
                      std::uint64_t maskHigh;
                      std::uint64_t raw;
                    }};

                    int main() {{
                      pyc::gen::enum_stress dut;
                      pyc::cpp::Testbench<pyc::gen::enum_stress> tb(dut);
                      tb.addClock(dut.clk, 1, 0, false);
                      dut.in_valid = pyc::cpp::Wire<1>(0);
                      dut.out_ready = pyc::cpp::Wire<1>(0);
                      tb.reset(dut.rst, 2, 1);
                      constexpr std::array<Input, 4> inputs{{{cpp_rows}}};
                      std::size_t accepted = 0;
                      std::size_t produced = 0;
                      for (std::uint64_t cycle = 0; cycle < 32; ++cycle) {{
                        const bool offered = accepted < inputs.size();
                        dut.in_valid = pyc::cpp::Wire<1>(offered ? 1 : 0);
                        if (offered) {{
                          const auto &input = inputs[accepted];
                          dut.in_data = pyc::cpp::Wire<129>(
                              {{input.raw, input.maskLow, input.maskHigh}});
                        }} else {{
                          dut.in_data = pyc::cpp::Wire<129>();
                        }}
                        const bool ready = cycle != 3 && cycle != 4 && cycle != 9;
                        dut.out_ready = pyc::cpp::Wire<1>(ready ? 1 : 0);
                        tb.runCycleAutoTrace(cycle, nullptr);
                        if (dut.in_valid.value() && dut.in_ready.value()) {{
                          std::cout << "A " << cycle << " " << accepted << "\\n";
                          ++accepted;
                        }}
                        if (dut.out_valid.value() && dut.out_ready.value()) {{
                          const auto lowWord = dut.out_data.word(0);
                          const auto highWord = dut.out_data.word(1);
                          const auto valid = lowWord & 1ULL;
                          const auto decoded = (lowWord >> 1) |
                              ((highWord & 1ULL) << 63);
                          const auto conflict = (highWord >> 1) & 1ULL;
                          const auto present = (highWord >> 2) & 1ULL;
                          const auto lane = (highWord >> 3) & 0x7fULL;
                          std::cout << "O " << cycle << " " << lane << " "
                                    << present << " " << conflict << " "
                                    << decoded << " " << valid << "\\n";
                          ++produced;
                        }}
                      }}
                      return accepted == inputs.size() && produced == inputs.size()
                                 ? 0
                                 : 4;
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
                    cpp_output,
                    "-I",
                    self.include,
                    cpp_output / "enum_stress.cpp",
                    pyc_harness,
                    self.runtime,
                    "-o",
                    pyc_binary,
                ),
                cwd=work,
            )

            verilator_harness = work / "stress_verilator_harness.cpp"
            verilator_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "Venum_stress.h"
                    #include <array>
                    #include <cstddef>
                    #include <cstdint>
                    #include <iostream>

                    struct Input {{
                      std::uint64_t maskLow;
                      std::uint64_t maskHigh;
                      std::uint64_t raw;
                    }};

                    static void tick(Venum_stress &dut) {{
                      dut.clk = 0; dut.eval();
                      dut.clk = 1; dut.eval();
                      dut.clk = 0; dut.eval();
                    }}

                    int main() {{
                      Venum_stress dut;
                      dut.in_valid = 0;
                      dut.out_ready = 0;
                      dut.rst = 1;
                      tick(dut); tick(dut);
                      dut.rst = 0;
                      tick(dut);
                      constexpr std::array<Input, 4> inputs{{{cpp_rows}}};
                      std::size_t accepted = 0;
                      std::size_t produced = 0;
                      for (std::uint64_t cycle = 0; cycle < 32; ++cycle) {{
                        const bool offered = accepted < inputs.size();
                        dut.in_valid = offered ? 1 : 0;
                        for (unsigned index = 0; index < 5; ++index)
                          dut.in_data[index] = 0;
                        if (offered) {{
                          const auto &input = inputs[accepted];
                          dut.in_data[0] = static_cast<std::uint32_t>(input.raw);
                          dut.in_data[1] = static_cast<std::uint32_t>(input.raw >> 32);
                          dut.in_data[2] = static_cast<std::uint32_t>(input.maskLow);
                          dut.in_data[3] = static_cast<std::uint32_t>(input.maskLow >> 32);
                          dut.in_data[4] = static_cast<std::uint32_t>(input.maskHigh);
                        }}
                        dut.out_ready = cycle != 3 && cycle != 4 && cycle != 9;
                        tick(dut);
                        if (dut.in_valid && dut.in_ready) {{
                          std::cout << "A " << cycle << " " << accepted << "\\n";
                          ++accepted;
                        }}
                        if (dut.out_valid && dut.out_ready) {{
                          const std::uint64_t word0 = dut.out_data[0];
                          const std::uint64_t word1 = dut.out_data[1];
                          const std::uint64_t word2 = dut.out_data[2];
                          const auto valid = word0 & 1ULL;
                          const auto decoded = (word0 >> 1) | (word1 << 31) |
                              ((word2 & 1ULL) << 63);
                          const auto conflict = (word2 >> 1) & 1ULL;
                          const auto present = (word2 >> 2) & 1ULL;
                          const auto lane = (word2 >> 3) & 0x7fULL;
                          std::cout << "O " << cycle << " " << lane << " "
                                    << present << " " << conflict << " "
                                    << decoded << " " << valid << "\\n";
                          ++produced;
                        }}
                      }}
                      return accepted == inputs.size() && produced == inputs.size()
                                 ? 0
                                 : 5;
                    }}
                    """
                ),
                encoding="utf-8",
            )
            verilator_object = work / "stress_verilator"
            self._run(
                (
                    self.verilator,
                    "--cc",
                    "--exe",
                    "--build",
                    "-Wno-fatal",
                    "--top-module",
                    "enum_stress",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "enum_stress.v",
                    verilator_harness,
                ),
                cwd=work,
            )

            gfsim = self._run((gfsim_binary,), cwd=work)
            pyc_output_text = self._run((pyc_binary,), cwd=work)
            verilator = self._run(
                (verilator_object / "Venum_stress",), cwd=work
            )

        self.assertEqual(expected, gfsim)
        self.assertEqual(pyc_output_text, verilator)
        observed = "".join(
            " ".join(line.split()[2:]) + "\n"
            for line in pyc_output_text.splitlines()
            if line.startswith("O ")
        )
        self.assertEqual(expected, observed)


if __name__ == "__main__":
    unittest.main()
