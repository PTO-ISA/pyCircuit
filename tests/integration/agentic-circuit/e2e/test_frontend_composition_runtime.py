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
EXAMPLE = ROOT / "examples/agentic-circuit/pipelines/frontend_composition_pipeline.py"
ACIR_BIN = Path(os.environ.get("ACIR_BIN", ROOT / ".pycircuit_out/acir/dev-llvm22/bin"))
PYC_TOOLCHAIN = Path(
    os.environ.get("PYC_TOOLCHAIN_ROOT", ROOT / ".pycircuit_out/toolchain/install")
)


def _pack(fields: tuple[tuple[int, int], ...]) -> int:
    value = 0
    for field, width in fields:
        value = (value << width) | field
    return value


class FrontendCompositionRuntimeTest(unittest.TestCase):
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

    def test_record_spread_sparse_enum_and_onehot_match_pyc_cpp(self) -> None:
        transactions = (
            (5, 2, 9, 1, 3, 0b1010, 1, 1, 1),
            (1, 7, 4, 0, 0, 0b0000, 0, 0, 0),
            (15, 0, 2, 1, 9, 0b0100, 2, 1, 0),
        )
        input_values = tuple(
            _pack(
                (
                    ((header_opcode << 4) | header_tag, 8),
                    ((patch_tag << 1) | patch_valid, 5),
                    (opcode, 4),
                    (flags, 4),
                    (0, 4),
                    (0, 1),
                    (0, 4),
                    (0, 2),
                    (0, 1),
                    (0, 1),
                )
            )
            for (
                header_opcode,
                header_tag,
                patch_tag,
                patch_valid,
                opcode,
                flags,
                _index,
                _valid,
                _conflict,
            ) in transactions
        )
        expected_values = tuple(
            _pack(
                (
                    ((header_opcode << 4) | header_tag, 8),
                    ((patch_tag << 1) | patch_valid, 5),
                    (opcode, 4),
                    (flags, 4),
                    (patch_tag, 4),
                    (patch_valid, 1),
                    (9, 4),
                    (index, 2),
                    (valid, 1),
                    (conflict, 1),
                )
            )
            for (
                header_opcode,
                header_tag,
                patch_tag,
                patch_valid,
                opcode,
                flags,
                index,
                valid,
                conflict,
            ) in transactions
        )
        gfsim_initializers = ",\n".join(
            "ac_generated::Item{"
            f"ac_generated::Header{{{header_opcode}, {header_tag}}}, "
            f"ac_generated::Patch{{{patch_tag}, {patch_valid}}}, "
            f"static_cast<ac_generated::Opcode>({opcode}), "
            f"gfsim::UInt<4>{{{flags}}}, gfsim::UInt<4>{{0}}, "
            "gfsim::UInt<1>{0}, ac_generated::Opcode::NONE, "
            "gfsim::UInt<2>{0}, gfsim::UInt<1>{0}, gfsim::UInt<1>{0}}"
            for (
                header_opcode,
                header_tag,
                patch_tag,
                patch_valid,
                opcode,
                flags,
                _index,
                _valid,
                _conflict,
            ) in transactions
        )
        packed_inputs = ", ".join(f"{value}ULL" for value in input_values)

        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                EXAMPLE.read_text(encoding="utf-8"),
                "frontend_composition_pipeline",
            )
            frozen = work / "model.mlir"
            frozen.write_text(
                _lower_queue_acir(raw, optimizer=self.acir_opt),
                encoding="utf-8",
            )
            gfsim_source = work / "gfsim.cpp"
            gfsim_source.write_text(
                self._run((self.cxxgen, frozen), cwd=ROOT),
                encoding="utf-8",
            )
            pyc = work / "model.pyc"
            pyc.write_text(
                self._run((self.pycgen, frozen), cwd=ROOT),
                encoding="utf-8",
            )
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
                      ac_generated::FrontendCompositionPipeline model;
                      const std::array<ac_generated::Item, 3> inputs{{
                          {gfsim_initializers}}};
                      auto rows = model.dispatch_rows();
                      std::size_t offered = 0;
                      for (std::uint64_t epoch = 0; epoch < 12; ++epoch) {{
                        if (offered < inputs.size() &&
                            model.incoming().proposePush(inputs[offered]))
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
                        append((value.header.opcode_bits.value() << 4) |
                               value.header.tag.value(), 8);
                        append((value.patch.tag.value() << 1) |
                               value.patch.valid.value(), 5);
                        append(static_cast<std::uint64_t>(value.opcode), 4);
                        append(value.flags.value(), 4);
                        append(value.result_tag.value(), 4);
                        append(value.result_valid.value(), 1);
                        append(static_cast<std::uint64_t>(value.result_opcode), 4);
                        append(value.onehot_index.value(), 2);
                        append(value.onehot_valid.value(), 1);
                        append(value.onehot_conflict.value(), 1);
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
                    #include "frontend_composition_pipeline.hpp"
                    #include <array>
                    #include <cstddef>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>

                    int main() {{
                      pyc::gen::frontend_composition_pipeline dut;
                      pyc::cpp::Testbench<pyc::gen::frontend_composition_pipeline> tb(dut);
                      tb.addClock(dut.clk, 1, 0, false);
                      dut.in_valid = pyc::cpp::Wire<1>(0);
                      dut.out_ready = pyc::cpp::Wire<1>(0);
                      tb.reset(dut.rst, 2, 1);
                      constexpr std::array<std::uint64_t, 3> inputs{{
                          {packed_inputs}}};
                      std::size_t accepted = 0;
                      std::size_t produced = 0;
                      for (std::uint64_t cycle = 0; cycle < 24; ++cycle) {{
                        const bool offered = accepted < inputs.size();
                        dut.in_valid = pyc::cpp::Wire<1>(offered ? 1 : 0);
                        dut.in_data = pyc::cpp::Wire<34>(
                            offered ? inputs[accepted] : 0);
                        const bool ready = cycle != 2 && cycle != 3 && cycle != 7;
                        dut.out_ready = pyc::cpp::Wire<1>(ready ? 1 : 0);
                        tb.runCycleAutoTrace(cycle, nullptr);
                        if (dut.in_valid.value() && dut.in_ready.value()) {{
                          std::cout << "A " << cycle << " " << accepted
                                    << "\\n";
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
                    pyc_output,
                    "-I",
                    self.include,
                    pyc_output / "frontend_composition_pipeline.cpp",
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
                    #include "Vfrontend_composition_pipeline.h"
                    #include <array>
                    #include <cstddef>
                    #include <cstdint>
                    #include <iostream>

                    static void tick(Vfrontend_composition_pipeline &dut) {{
                      dut.clk = 0;
                      dut.eval();
                      dut.clk = 1;
                      dut.eval();
                      dut.clk = 0;
                      dut.eval();
                    }}

                    int main() {{
                      Vfrontend_composition_pipeline dut;
                      dut.in_valid = 0;
                      dut.out_ready = 0;
                      dut.rst = 1;
                      tick(dut);
                      tick(dut);
                      dut.rst = 0;
                      tick(dut);
                      constexpr std::array<std::uint64_t, 3> inputs{{
                          {packed_inputs}}};
                      std::size_t accepted = 0;
                      std::size_t produced = 0;
                      for (std::uint64_t cycle = 0; cycle < 24; ++cycle) {{
                        const bool offered = accepted < inputs.size();
                        dut.in_valid = offered ? 1 : 0;
                        dut.in_data = offered ? inputs[accepted] : 0;
                        dut.out_ready = cycle != 2 && cycle != 3 && cycle != 7;
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
                    "frontend_composition_pipeline",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "frontend_composition_pipeline.v",
                    verilator_harness,
                ),
                cwd=work,
            )

            gfsim = self._run((gfsim_binary,), cwd=work)
            pyc = self._run((pyc_binary,), cwd=work)
            verilator = self._run(
                (verilator_object / "Vfrontend_composition_pipeline",), cwd=work
            )

        expected = "".join(f"{value}\n" for value in expected_values)
        self.assertEqual(expected, gfsim)
        self.assertEqual(pyc, verilator)
        observed = "".join(
            line.split(maxsplit=2)[2] + "\n"
            for line in pyc.splitlines()
            if line.startswith("O ")
        )
        self.assertEqual(gfsim, observed)
        self.assertEqual(3, sum(line.startswith("A ") for line in pyc.splitlines()))


if __name__ == "__main__":
    unittest.main()
