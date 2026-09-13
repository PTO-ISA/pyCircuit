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
EXAMPLE = ROOT / "examples/agentic-circuit/pipelines/record_projection.py"
ACIR_BIN = Path(os.environ.get("ACIR_BIN", ROOT / ".pycircuit_out/acir/dev-llvm22/bin"))
PYC_TOOLCHAIN = Path(
    os.environ.get("PYC_TOOLCHAIN_ROOT", ROOT / ".pycircuit_out/toolchain/install")
)


class RecordProjectionRuntimeTest(unittest.TestCase):
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
        if (
            cls.compiler is None
            or cls.verilator is None
            or not all(
                path.exists()
                for path in (
                    cls.acir_opt,
                    cls.cxxgen,
                    cls.pycgen,
                    cls.pycc,
                    cls.runtime,
                    cls.include,
                )
            )
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

    def test_projection_prunes_fields_and_matches_all_backends(self) -> None:
        transactions = (
            (1, 2, 0x1234, 0),
            (9, 0xFF, 0xABCD, 1),
            (15, 7, 0, 1),
        )
        packed_inputs = tuple(
            (opcode << 25) | (tag << 17) | (payload << 1) | valid
            for opcode, tag, payload, valid in transactions
        )
        expected = tuple(
            (valid << 4) | opcode for opcode, _tag, _payload, valid in transactions
        )
        gfsim_inputs = ",\n".join(
            "ac_generated::Packet{"
            f"ac_generated::Header{{gfsim::UInt<4>{{{opcode}}}}}, "
            f"gfsim::UInt<8>{{{tag}}}, gfsim::UInt<16>{{{payload}}}, "
            f"gfsim::UInt<1>{{{valid}}}}}"
            for opcode, tag, payload, valid in transactions
        )
        packed_cpp = ", ".join(f"{value}ULL" for value in packed_inputs)

        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                EXAMPLE.read_text(encoding="utf-8"),
                "record_projection",
                source_path=EXAMPLE.relative_to(ROOT).as_posix(),
            )
            self.assertIn('field "valid"', raw)
            self.assertIn('field "header"', raw)
            self.assertNotIn('field "tag"', raw)
            self.assertNotIn('field "payload"', raw)
            frozen = work / "model.mlir"
            frozen.write_text(
                _lower_queue_acir(raw, optimizer=self.acir_opt), encoding="utf-8"
            )
            gfsim_source = work / "gfsim.cpp"
            generated = self._run((self.cxxgen, frozen), cwd=ROOT)
            gfsim_source.write_text(generated, encoding="utf-8")
            self.assertIn("struct HeaderView", generated)
            self.assertNotIn("std::uint16_t payload", generated)
            pyc = work / "model.pyc"
            pyc.write_text(
                self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8"
            )
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
                      ac_generated::RecordProjection model;
                      const std::array<ac_generated::Packet, 3> inputs{{
                          {gfsim_inputs}}};
                      auto rows = model.dispatch_rows();
                      std::size_t offered = 0;
                      for (std::uint64_t epoch = 0; epoch < 12; ++epoch) {{
                        if (offered < inputs.size() &&
                            model.packet().proposePush(inputs[offered])) ++offered;
                        const gfsim::Epoch current{{epoch, 0}};
                        for (auto &row : rows) row.work(row.object, current);
                        for (auto &row : rows)
                          row.xfer(row.object, current, gfsim::XferPhase::Arbitrate);
                        for (auto &row : rows)
                          row.xfer(row.object, current, gfsim::XferPhase::Commit);
                      }}
                      if (model.sink_0_values().size() != inputs.size()) return 2;
                      for (const auto &value : model.sink_0_values())
                        std::cout << ((value.valid.value() << 4) |
                                      value.header.opcode.value()) << "\\n";
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
                    #include "record_projection.hpp"
                    #include <array>
                    #include <cstddef>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>
                    int main() {{
                      pyc::gen::record_projection dut;
                      pyc::cpp::Testbench<pyc::gen::record_projection> tb(dut);
                      tb.addClock(dut.clk, 1, 0, false);
                      dut.in_valid = pyc::cpp::Wire<1>(0);
                      dut.out_ready = pyc::cpp::Wire<1>(0);
                      tb.reset(dut.rst, 2, 1);
                      constexpr std::array<std::uint64_t, 3> inputs{{{packed_cpp}}};
                      std::size_t accepted = 0, produced = 0;
                      for (std::uint64_t cycle = 0; cycle < 24; ++cycle) {{
                        const bool offered = accepted < inputs.size();
                        dut.in_valid = pyc::cpp::Wire<1>(offered ? 1 : 0);
                        dut.in_data = pyc::cpp::Wire<29>(offered ? inputs[accepted] : 0);
                        dut.out_ready = pyc::cpp::Wire<1>(
                            cycle == 3 || cycle == 7 ? 0 : 1);
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
                                 ? 0 : 4;
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
                    cpp_output / "record_projection.cpp",
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
                    #include "Vrecord_projection.h"
                    #include <array>
                    #include <cstddef>
                    #include <cstdint>
                    #include <iostream>
                    static void tick(Vrecord_projection &dut) {{
                      dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval();
                      dut.clk = 0; dut.eval();
                    }}
                    int main() {{
                      Vrecord_projection dut;
                      dut.in_valid = 0; dut.out_ready = 0; dut.rst = 1;
                      tick(dut); tick(dut); dut.rst = 0; tick(dut);
                      constexpr std::array<std::uint64_t, 3> inputs{{{packed_cpp}}};
                      std::size_t accepted = 0, produced = 0;
                      for (std::uint64_t cycle = 0; cycle < 24; ++cycle) {{
                        const bool offered = accepted < inputs.size();
                        dut.in_valid = offered ? 1 : 0;
                        dut.in_data = offered ? inputs[accepted] : 0;
                        dut.out_ready = cycle == 3 || cycle == 7 ? 0 : 1;
                        tick(dut);
                        if (dut.in_valid && dut.in_ready) {{
                          std::cout << "A " << cycle << " " << accepted << "\\n";
                          ++accepted;
                        }}
                        if (dut.out_valid && dut.out_ready) {{
                          std::cout << "O " << cycle << " "
                                    << static_cast<std::uint64_t>(dut.out_data)
                                    << "\\n";
                          ++produced;
                        }}
                      }}
                      return accepted == inputs.size() && produced == inputs.size()
                                 ? 0 : 5;
                    }}
                    """
                ),
                encoding="utf-8",
            )
            verilator_object = work / "verilator"
            self._run(
                (
                    self.verilator,
                    "--cc",
                    "--exe",
                    "--build",
                    "-Wno-fatal",
                    "--top-module",
                    "record_projection",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "record_projection.v",
                    verilator_harness,
                ),
                cwd=work,
            )
            gfsim = self._run((gfsim_binary,), cwd=work)
            pyc_output = self._run((pyc_binary,), cwd=work)
            verilator = self._run(
                (verilator_object / "Vrecord_projection",), cwd=work
            )

        expected_text = "".join(f"{value}\n" for value in expected)
        self.assertEqual(expected_text, gfsim)
        self.assertEqual(pyc_output, verilator)
        observed = "".join(
            line.split(maxsplit=2)[2] + "\n"
            for line in pyc_output.splitlines()
            if line.startswith("O ")
        )
        self.assertEqual(expected_text, observed)


if __name__ == "__main__":
    unittest.main()
