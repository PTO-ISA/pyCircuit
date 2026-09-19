from __future__ import annotations

import json
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
FIXTURE = (
    ROOT
    / "tests/integration/agentic-circuit/e2e/fixtures/private_queue_payload/architecture.py"
)
ACIR_BIN = Path(os.environ.get("ACIR_BIN", ROOT / ".pycircuit_out/acir/dev-llvm22/bin"))
PYC_TOOLCHAIN = Path(
    os.environ.get("PYC_TOOLCHAIN_ROOT", ROOT / ".pycircuit_out/toolchain/install")
)


def _words(value: int, count: int, width: int) -> tuple[int, ...]:
    mask = (1 << width) - 1
    return tuple((value >> (index * width)) & mask for index in range(count))


class PrivateQueuePayloadRuntimeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.acir_opt = ACIR_BIN / "acir-opt"
        cls.plan = ACIR_BIN / "acir-queue-plan"
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
                    cls.plan,
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
        self.assertEqual(0, completed.returncode, completed.stdout + completed.stderr)
        return completed.stdout

    def test_one_kib_private_payload_is_pruned_and_backend_equivalent(self) -> None:
        payloads = (
            (0,) * 16,
            (0xFFFF_FFFF_FFFF_FFFF,) + (0,) * 14 + (0x1234,),
            (0,) * 7 + (0x8000_0000_0000_0000,) + (0,) * 8,
        )
        transactions = (
            (1, 0, payloads[0]),
            (0xA5, 1, payloads[1]),
            (0xFF, 1, payloads[2]),
        )
        packed_inputs: list[int] = []
        for tag, valid, words in transactions:
            payload = sum(word << (64 * index) for index, word in enumerate(words))
            packed_inputs.append((tag << 1025) | (payload << 1) | valid)
        expected = tuple((tag << 1) | valid for tag, valid, _ in transactions)
        gfsim_inputs = ",\n".join(
            "ac_generated::Packet{"
            f"gfsim::UInt<8>{{{tag}}}, "
            "gfsim::UInt<1024>{gfsim::UInt<1024>::word_array_type{"
            + ", ".join(f"{word}ULL" for word in words)
            + "}}, gfsim::UInt<1>{"
            + str(valid)
            + "}}"
            for tag, valid, words in transactions
        )
        pyc_inputs = ",\n".join(
            "pyc::cpp::Wire<1033>{"
            + ", ".join(f"{word}ULL" for word in _words(value, 17, 64))
            + "}"
            for value in packed_inputs
        )
        verilator_inputs = ",\n".join(
            "Input{" + ", ".join(str(word) for word in _words(value, 33, 32)) + "}"
            for value in packed_inputs
        )

        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            raw = lower_queue_source(
                FIXTURE.read_text(encoding="utf-8"),
                "private_queue_payload",
                source_path=FIXTURE.relative_to(ROOT).as_posix(),
            )
            baseline_raw = raw.replace(
                "ac.system = ",
                "ac.disable_payload_pruning = true, ac.system = ",
                1,
            )
            baseline_text = _lower_queue_acir(
                baseline_raw, optimizer=self.acir_opt
            )
            self.assertNotIn("ac.payload_projections_out", baseline_text)
            frozen_text = _lower_queue_acir(raw, optimizer=self.acir_opt)
            self.assertIn("ac.payload_projections_out", frozen_text)
            self.assertIn("!ac.queue<tuple<i8, i1>>", frozen_text)
            frozen = work / "model.mlir"
            frozen.write_text(frozen_text, encoding="utf-8")
            baseline_frozen = work / "baseline.mlir"
            baseline_frozen.write_text(baseline_text, encoding="utf-8")
            plan_text = self._run((self.plan, frozen), cwd=ROOT)
            plan = json.loads(plan_text)
            private = next(queue for queue in plan["queues"] if queue["name"] == "buffered")
            self.assertEqual(2, private["depth"])
            self.assertEqual(3, private["latency"])
            self.assertEqual("tuple<i8, i1>", private["payload_type"])
            self.assertEqual(
                ["tag", "valid"], private["payload_projection"]["kept_fields"]
            )
            self.assertEqual(
                "!ac.struct<@types::@Packet>",
                private["payload_projection"]["logical_type"],
            )
            self.assertEqual(1033, private["payload_projection"]["logical_bits"])
            self.assertEqual(9, private["payload_projection"]["carrier_bits"])
            self.assertEqual(1024, private["payload_projection"]["removed_bits"])
            projected_block = next(
                block for block in plan["blocks"] if block["name"] == "projected"
            )
            projected_reads = [
                expression
                for expression in projected_block["expressions"]
                if expression["kind"] == "aggregate_get"
            ]
            self.assertEqual(2, len(projected_reads))
            self.assertTrue(
                all(
                    expression["source_provenance"]["origins"][0]["frames"][0][
                        "file"
                    ].endswith("private_queue_payload/architecture.py")
                    for expression in projected_reads
                )
            )
            bundle = work / "bundle"
            self._run(
                (
                    self.cxxgen,
                    frozen,
                    "--output-root",
                    bundle,
                ),
                cwd=ROOT,
            )
            cost_report = json.loads(
                (bundle / "share/generated/cost-report.json").read_text(
                    encoding="utf-8"
                )
            )
            cost_private = next(
                queue
                for queue in cost_report["modules"][0]["queues"]
                if queue["name"] == "buffered"
            )
            self.assertEqual(1033, cost_private["logical_bits"])
            self.assertEqual(9, cost_private["carrier_bits"])
            self.assertEqual(2, cost_private["packed_bytes"])
            self.assertEqual(
                "private_transform_tuple_v1",
                cost_private["projection_profile"],
            )
            gfsim_source = work / "gfsim.cpp"
            generated = self._run((self.cxxgen, frozen), cwd=ROOT)
            self.assertIn("gfsim::SimQueue<gfsim::UInt<9>> buffered_", generated)
            gfsim_source.write_text(generated, encoding="utf-8")
            baseline_gfsim_source = work / "baseline_gfsim.cpp"
            baseline_gfsim_source.write_text(
                self._run((self.cxxgen, baseline_frozen), cwd=ROOT),
                encoding="utf-8",
            )
            pyc = work / "model.pyc"
            pyc.write_text(
                self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            self.assertNotRegex(pyc.read_text(encoding="utf-8"), r"\b(?:scf\.|vector<)")
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
                    static_assert(
                        gfsim::PacketTraits<gfsim::UInt<9>>::serializedSize == 2);
                    static_assert(sizeof(ac_generated::Packet) >= 136);
                    int main() {{
                      ac_generated::PrivateQueuePayload model;
                      const std::array<ac_generated::Packet, 3> inputs{{
                          {gfsim_inputs}}};
                      auto rows = model.dispatch_rows();
                      std::size_t offered = 0;
                      std::size_t received = 0;
                      for (std::uint64_t epoch = 0; epoch < 18; ++epoch) {{
                        if (offered < inputs.size() &&
                            model.packet().proposePush(inputs[offered])) {{
                          std::cout << "A " << epoch << " " << offered << "\\n";
                          ++offered;
                        }}
                        const gfsim::Epoch current{{epoch, 0}};
                        for (auto &row : rows) row.work(row.object, current);
                        for (auto &row : rows)
                          row.xfer(row.object, current, gfsim::XferPhase::Arbitrate);
                        for (auto &row : rows)
                          row.xfer(row.object, current, gfsim::XferPhase::Commit);
                        while (received < model.sink_0_values().size()) {{
                          const auto &value = model.sink_0_values()[received++];
                          std::cout << "O " << epoch << " "
                                    << ((value.tag.value() << 1) |
                                        value.valid.value()) << "\\n";
                        }}
                      }}
                      if (model.sink_0_values().size() != inputs.size()) return 2;
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
            baseline_gfsim_harness = work / "baseline_gfsim_harness.cpp"
            baseline_gfsim_binary = work / "baseline_gfsim_model"
            baseline_gfsim_harness.write_text(
                gfsim_harness.read_text(encoding="utf-8").replace(
                    gfsim_source.name, baseline_gfsim_source.name
                ),
                encoding="utf-8",
            )
            self._run(
                (
                    self.compiler,
                    "-std=c++20",
                    "-I",
                    ROOT / "simulator/gfsim/include",
                    baseline_gfsim_harness,
                    "-o",
                    baseline_gfsim_binary,
                ),
                cwd=work,
            )

            pyc_harness = work / "pyc_harness.cpp"
            pyc_binary = work / "pyc_model"
            pyc_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "private_queue_payload.hpp"
                    #include <array>
                    #include <cstddef>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>
                    int main() {{
                      pyc::gen::private_queue_payload dut;
                      pyc::cpp::Testbench<pyc::gen::private_queue_payload> tb(dut);
                      tb.addClock(dut.clk, 1, 0, false);
                      dut.in_valid = pyc::cpp::Wire<1>(0);
                      dut.out_ready = pyc::cpp::Wire<1>(0);
                      tb.reset(dut.rst, 2, 1);
                      tb.runCycleAutoTrace(0, nullptr);
                      const std::array<pyc::cpp::Wire<1033>, 3> inputs{{
                          {pyc_inputs}}};
                      std::size_t accepted = 0, produced = 0;
                      for (std::uint64_t cycle = 1; cycle < 40; ++cycle) {{
                        const bool offered = accepted < inputs.size();
                        dut.in_valid = pyc::cpp::Wire<1>(offered ? 1 : 0);
                        dut.in_data = offered ? inputs[accepted] : pyc::cpp::Wire<1033>();
                        dut.out_ready = pyc::cpp::Wire<1>(
                            cycle == 4 || cycle == 8 ? 0 : 1);
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
                    cpp_output / "private_queue_payload.cpp",
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
                    #include "Vprivate_queue_payload.h"
                    #include <array>
                    #include <cstddef>
                    #include <cstdint>
                    #include <iostream>
                    struct Input {{ std::array<std::uint32_t, 33> words; }};
                    static void tick(Vprivate_queue_payload &dut) {{
                      dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval();
                      dut.clk = 0; dut.eval();
                    }}
                    int main() {{
                      Vprivate_queue_payload dut;
                      dut.in_valid = 0; dut.out_ready = 0; dut.rst = 1;
                      tick(dut); tick(dut); dut.rst = 0; tick(dut); tick(dut);
                      const std::array<Input, 3> inputs{{{verilator_inputs}}};
                      std::size_t accepted = 0, produced = 0;
                      for (std::uint64_t cycle = 1; cycle < 40; ++cycle) {{
                        const bool offered = accepted < inputs.size();
                        dut.in_valid = offered ? 1 : 0;
                        for (unsigned index = 0; index < 33; ++index)
                          dut.in_data[index] = offered ? inputs[accepted].words[index] : 0;
                        dut.out_ready = cycle == 4 || cycle == 8 ? 0 : 1;
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
                    "private_queue_payload",
                    "--Mdir",
                    verilator_object,
                    verilog_output / "pyc_primitives.v",
                    verilog_output / "private_queue_payload.v",
                    verilator_harness,
                ),
                cwd=work,
            )
            gfsim = self._run((gfsim_binary,), cwd=work)
            baseline_gfsim = self._run((baseline_gfsim_binary,), cwd=work)
            pyc_output = self._run((pyc_binary,), cwd=work)
            verilator = self._run(
                (verilator_object / "Vprivate_queue_payload",), cwd=work
            )

        self.assertEqual(baseline_gfsim, gfsim)
        self.assertEqual(pyc_output, verilator)
        gfsim_values = "".join(
            line.split(maxsplit=2)[2] + "\n"
            for line in gfsim.splitlines()
            if line.startswith("O ")
        )
        expected_text = "".join(f"{value}\n" for value in expected)
        self.assertEqual(expected_text, gfsim_values)
        observed = "".join(
            line.split(maxsplit=2)[2] + "\n"
            for line in pyc_output.splitlines()
            if line.startswith("O ")
        )
        self.assertEqual(expected_text, observed)


if __name__ == "__main__":
    unittest.main()
