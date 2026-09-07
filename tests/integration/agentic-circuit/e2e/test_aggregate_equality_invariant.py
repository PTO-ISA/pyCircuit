from __future__ import annotations

import importlib.util
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
FIXTURE_ROOT = (
    ROOT / "tests/integration/agentic-circuit/e2e/fixtures/aggregate_equality_invariant"
)
FIXTURE = FIXTURE_ROOT / "architecture.py"
PYC_FIXTURE = FIXTURE_ROOT / "packed_parity.py"
DEFAULT_TOOLCHAIN = ROOT / ".pycircuit_out/acir/dev-llvm22"
DEFAULT_PYC_TOOLCHAIN = ROOT / ".pycircuit_out/toolchain/install"


class AggregateEqualityInvariantTest(unittest.TestCase):
    def test_recursive_equality_and_invariant_execute_through_gfsim(self) -> None:
        cxx = shutil.which("c++")
        toolchain = Path(os.environ.get("PYC_TOOLCHAIN_ROOT", DEFAULT_TOOLCHAIN))
        opt = Path(os.environ.get("ACIR_OPT", toolchain / "bin/acir-opt"))
        plan_tool = Path(
            os.environ.get("ACIR_QUEUE_PLAN", toolchain / "bin/acir-queue-plan")
        )
        cxxgen = Path(
            os.environ.get("ACIR_QUEUE_CXXGEN", toolchain / "bin/acir-queue-cxxgen")
        )
        if cxx is None or any(not path.is_file() for path in (opt, plan_tool, cxxgen)):
            self.skipTest("integrated QueueGraph/gfsim toolchain is unavailable")

        import agentic_circuit as ac
        from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

        spec = importlib.util.spec_from_file_location(
            "aggregate_contract_fixture", FIXTURE
        )
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load aggregate equality fixture")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)
        specialization = ac.jit(
            module.aggregate_equality_invariant, workspace=FIXTURE_ROOT
        )
        raw_text = specialization.lower_acir()
        self.assertIn("ac.var.invariant", raw_text)
        self.assertIn('name "Inner.valid_inner"', raw_text)
        self.assertIn('name "Payload.valid_payload"', raw_text)
        self.assertIn("!ac.var<!ac.struct<@types::@Payload>>", raw_text)

        with tempfile.TemporaryDirectory(prefix="aggregate-contract-") as directory:
            root = Path(directory)
            raw = root / "raw.mlir"
            frozen = root / "frozen.mlir"
            generated = root / "model.cpp"
            raw.write_text(raw_text, encoding="utf-8")
            lowered = subprocess.run(
                (
                    str(opt),
                    f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                    str(raw),
                    "-o",
                    str(frozen),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, lowered.returncode, lowered.stderr)
            frozen_text = frozen.read_text(encoding="utf-8")
            self.assertNotIn("ac.var.invariant", frozen_text)
            self.assertIsNone(
                re.search(
                    r"ac\.var\.cmp[^\n]*!ac\.var<(?:!ac\.(?:struct|value_array)|tuple<)",
                    frozen_text,
                )
            )
            planned = subprocess.run(
                (str(plan_tool), str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, planned.returncode, planned.stderr)
            plan = json.loads(planned.stdout)
            self.assertGreater(
                sum(len(block["expressions"]) for block in plan["blocks"]), 20
            )
            emitted = subprocess.run(
                (str(cxxgen), str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, emitted.returncode, emitted.stderr)
            generated.write_text(emitted.stdout, encoding="utf-8")
            harness = root / "harness.cpp"
            harness.write_text(
                """#include "model.cpp"
#include <array>
using namespace ac_generated;
Payload payload(unsigned tag, Mode mode, unsigned pair0, unsigned pair1,
                unsigned lane0, bool valid) {
  return Payload{Inner{gfsim::UInt<8>{tag}, mode},
                 gfsim::UInt<16>{(pair0 << 8) | pair1},
                 gfsim::UInt<64>{static_cast<unsigned long long>(lane0) << 56},
                 gfsim::UInt<1>{valid}};
}
int main() {
  AggregateEqualityInvariant model;
  auto rows = model.dispatch_rows();
  unsigned tick = 0;
  auto cycle = [&]() {
    gfsim::Epoch epoch{++tick, 0};
    for (auto &row : rows) row.work(row.object, epoch);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Probe);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  };
  Payload base = payload(3, Mode::RUN, 3, 4, 7, false);
  std::array<Payload, 6> right{{base,
      payload(4, Mode::RUN, 3, 4, 7, false),
      payload(3, Mode::IDLE, 3, 4, 7, false),
      payload(3, Mode::RUN, 3, 5, 7, false),
      payload(3, Mode::RUN, 3, 4, 6, false),
      payload(3, Mode::RUN, 3, 4, 7, true)}};
  std::array<Payload, 6> checked{{base, base, base,
      payload(3, Mode::IDLE, 3, 4, 7, false),
      payload(3, Mode::RUN, 2, 4, 7, false),
      payload(3, Mode::RUN, 3, 4, 8, false)}};
  for (size_t index = 0; index != right.size(); ++index) {
    if (!model.request().proposePush(
            ComparisonRequest{base, right[index], gfsim::UInt<1>{0}}) ||
        !model.checked().proposePush(checked[index])) return 1;
    model.request().doXfer({tick, 0});
    model.checked().doXfer({tick, 0});
    cycle(); cycle(); cycle();
  }
  if (model.sink_0_values().size() != 6 || model.sink_1_values().size() != 3)
    return 2;
  for (size_t index = 0; index != 6; ++index) {
    unsigned equal = static_cast<unsigned long long>(model.sink_0_values()[index].equal);
    if (equal != (index == 0)) return 3 + index;
  }
  return 0;
}
""",
                encoding="utf-8",
            )
            executable = root / "model"
            compiled = subprocess.run(
                (
                    cxx,
                    "-std=c++20",
                    "-I",
                    str(ROOT / "simulator/gfsim/include"),
                    str(harness),
                    "-o",
                    str(executable),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, compiled.returncode, compiled.stderr)
            executed = subprocess.run(
                (str(executable),), text=True, capture_output=True, check=False
            )
            self.assertEqual(0, executed.returncode, executed.stderr)

    def test_packed_equality_and_invariant_match_pyc_cpp_and_verilog(self) -> None:
        cxx = shutil.which("c++")
        verilator = shutil.which("verilator")
        native = Path(os.environ.get("ACIR_TOOLCHAIN_ROOT", DEFAULT_TOOLCHAIN))
        pyc_toolchain = Path(
            os.environ.get("PYC_TOOLCHAIN_ROOT", DEFAULT_PYC_TOOLCHAIN)
        )
        opt = Path(os.environ.get("ACIR_OPT", native / "bin/acir-opt"))
        pycgen = Path(
            os.environ.get("ACIR_QUEUE_PYCGEN", native / "bin/acir-queue-pycgen")
        )
        pycc = Path(os.environ.get("PYCC", pyc_toolchain / "bin/pycc"))
        metadata = pyc_toolchain / "share/pycircuit/toolchain-metadata.json"
        runtime = pyc_toolchain / "lib/libpyc6_runtime.a"
        runtime_include = pyc_toolchain / "include"
        required = (opt, pycgen, pycc, metadata, runtime)
        if (
            cxx is None
            or verilator is None
            or any(not path.is_file() for path in required)
        ):
            self.skipTest("current-checkout ACIR and PYC backend tools are unavailable")

        import agentic_circuit as ac
        from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

        spec = importlib.util.spec_from_file_location(
            "aggregate_packed_fixture", PYC_FIXTURE
        )
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load packed aggregate fixture")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)
        specialization = ac.jit(
            module.aggregate_equality_packed, workspace=FIXTURE_ROOT
        )
        raw_text = specialization.lower_acir()
        self.assertIn('name "PackedPayload.valid_packed_shape"', raw_text)
        self.assertIn('name "PackedPayload.valid_packed"', raw_text)

        with tempfile.TemporaryDirectory(prefix="aggregate-contract-pyc-") as directory:
            root = Path(directory)
            raw = root / "raw.mlir"
            frozen = root / "frozen.mlir"
            output = root / "output"
            raw.write_text(raw_text, encoding="utf-8")
            lowered = subprocess.run(
                (
                    str(opt),
                    f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                    str(raw),
                    "-o",
                    str(frozen),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, lowered.returncode, lowered.stderr)
            built = subprocess.run(
                (
                    str(ROOT / "compiler/acir/tools/ac-queue-pyc-build.py"),
                    str(frozen),
                    "--pycgen-tool",
                    str(pycgen),
                    "--pycc",
                    str(pycc),
                    "--toolchain-lock",
                    str(ROOT / "toolchains/agentic-circuit/pyc.lock.json"),
                    "--toolchain-metadata",
                    str(metadata),
                    "--cxx",
                    cxx,
                    "--verilator",
                    verilator,
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
            self.assertEqual(0, built.returncode, built.stderr)
            pyc = (output / "model.pyc").read_text(encoding="utf-8")
            self.assertIn("%in0_data: i29", pyc)
            self.assertIn("%in1_data: i14", pyc)
            self.assertNotIn("vector<", pyc)
            self.assertNotIn("pyc.v_", pyc)
            self.assertNotIn("ac.var.invariant", pyc)

            driver = r"""
  constexpr std::array<unsigned, 2> requests{
      (0x0f90u << 15) | (0x0f90u << 1),
      (0x0f90u << 15) | (0x0d90u << 1)};
  constexpr std::array<unsigned, 2> checked{0x0f90u, 0x0f98u};
  unsigned sent0 = 0;
  unsigned sent1 = 0;
  for (unsigned cycle = 0; cycle != 16; ++cycle) {
    const bool valid0 = cycle != 0 && sent0 != requests.size();
    const bool valid1 = cycle != 0 && sent1 != checked.size();
    dut.rst = RESET;
    dut.in0_valid = WIRE1(valid0);
    dut.in0_data = WIRE29(valid0 ? requests[sent0] : 0);
    dut.in1_valid = WIRE1(valid1);
    dut.in1_data = WIRE14(valid1 ? checked[sent1] : 0);
    dut.out0_ready = WIRE1(true);
    dut.out1_ready = WIRE1(true);
    dut.clk = WIRE1(false);
    STEP;
    const bool accepted0 = valid0 && READY0;
    const bool accepted1 = valid1 && READY1;
    dut.clk = WIRE1(true);
    STEP;
    if (VALID0) std::cout << "E " << cycle << " " << DATA0 << "\n";
    if (VALID1) std::cout << "I " << cycle << " " << DATA1 << "\n";
    if (accepted0) ++sent0;
    if (accepted1) ++sent1;
    dut.clk = WIRE1(false);
    STEP;
  }
"""
            cpp_driver = (
                driver.replace("RESET", "pyc::cpp::Wire<1>(cycle == 0)")
                .replace("WIRE29", "pyc::cpp::Wire<29>")
                .replace("WIRE14", "pyc::cpp::Wire<14>")
                .replace("WIRE1", "pyc::cpp::Wire<1>")
                .replace("STEP", "dut.step()")
                .replace("READY0", "dut.in0_ready.value()")
                .replace("READY1", "dut.in1_ready.value()")
                .replace("VALID0", "dut.out0_valid.value()")
                .replace("VALID1", "dut.out1_valid.value()")
                .replace("DATA0", "dut.out0_data.value()")
                .replace("DATA1", "dut.out1_data.value()")
            )
            cpp_harness = output / "cpp_harness.cpp"
            cpp_harness.write_text(
                '#include "aggregate_equality_packed.hpp"\n'
                "#include <array>\n#include <iostream>\n"
                "int main() { pyc::gen::aggregate_equality_packed dut;"
                + cpp_driver
                + "return 0; }\n",
                encoding="utf-8",
            )
            cpp_executable = output / "cpp_model"
            cpp_sources = sorted(
                output.joinpath("cpp").glob("aggregate_equality_packed*.cpp")
            )
            cpp_build = subprocess.run(
                (
                    cxx,
                    "-std=c++17",
                    "-I",
                    str(output / "cpp"),
                    "-I",
                    str(runtime_include),
                    *(str(path) for path in cpp_sources),
                    str(cpp_harness),
                    str(runtime),
                    "-o",
                    str(cpp_executable),
                ),
                cwd=output,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, cpp_build.returncode, cpp_build.stderr)
            cpp_run = subprocess.run(
                (str(cpp_executable),),
                cwd=output,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, cpp_run.returncode, cpp_run.stderr)

            verilog_driver = (
                driver.replace("RESET", "cycle == 0")
                .replace("WIRE29", "")
                .replace("WIRE14", "")
                .replace("WIRE1", "")
                .replace("STEP", "dut.eval()")
                .replace("READY0", "dut.in0_ready")
                .replace("READY1", "dut.in1_ready")
                .replace("VALID0", "dut.out0_valid")
                .replace("VALID1", "dut.out1_valid")
                .replace("DATA0", "dut.out0_data")
                .replace("DATA1", "dut.out1_data")
            )
            verilator_harness = output / "verilator_harness.cpp"
            verilator_harness.write_text(
                '#include "Vaggregate_equality_packed.h"\n'
                "#include <array>\n#include <iostream>\n"
                "int main() { Vaggregate_equality_packed dut;"
                + verilog_driver
                + "return 0; }\n",
                encoding="utf-8",
            )
            object_dir = output / "verilator_obj"
            verilator_build = subprocess.run(
                (
                    verilator,
                    "--cc",
                    "--exe",
                    "--build",
                    "-Wno-fatal",
                    "--top-module",
                    "aggregate_equality_packed",
                    "--Mdir",
                    str(object_dir),
                    str(output / "verilog/pyc_primitives.v"),
                    str(output / "verilog/aggregate_equality_packed.v"),
                    str(verilator_harness),
                ),
                cwd=output,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, verilator_build.returncode, verilator_build.stderr)
            verilator_run = subprocess.run(
                (str(object_dir / "Vaggregate_equality_packed"),),
                cwd=output,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, verilator_run.returncode, verilator_run.stderr)
            self.assertEqual(cpp_run.stdout, verilator_run.stdout)
            equal_bits = [
                int(line.split()[2]) & 1
                for line in cpp_run.stdout.splitlines()
                if line.startswith("E ")
            ]
            invariant_bits = [
                int(line.split()[2]) & 1
                for line in cpp_run.stdout.splitlines()
                if line.startswith("I ")
            ]
            self.assertEqual([1, 0], equal_bits)
            self.assertEqual([1, 0], invariant_bits)


if __name__ == "__main__":
    unittest.main()
