from __future__ import annotations

import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
RULE_FIXTURE = (
    ROOT / "tests/mlir/agentic-circuit/Transforms/rule-multi-output-lowering.mlir"
)
STATEFUL_FIXTURE_ROOT = (
    ROOT / "tests/integration/agentic-circuit/e2e/fixtures/multi_output_atomic"
)
STATEFUL_FIXTURE = STATEFUL_FIXTURE_ROOT / "architecture.py"
DEFAULT_TOOLCHAIN = ROOT / ".pycircuit_out/toolchain/install"


class MultiOutputAtomicTest(unittest.TestCase):
    def test_public_python_state_and_outputs_commit_as_one_gfsim_transaction(
        self,
    ) -> None:
        toolchain = Path(os.environ.get("PYC_TOOLCHAIN_ROOT", DEFAULT_TOOLCHAIN))
        opt = Path(os.environ.get("ACIR_OPT", toolchain / "bin/acir-opt"))
        plan_tool = Path(
            os.environ.get("ACIR_QUEUE_PLAN", toolchain / "bin/acir-queue-plan")
        )
        cxxgen = Path(
            os.environ.get("ACIR_QUEUE_CXXGEN", toolchain / "bin/acir-queue-cxxgen")
        )
        cxx = shutil.which("c++")
        if cxx is None or any(not path.is_file() for path in (opt, plan_tool, cxxgen)):
            self.skipTest("integrated QueueGraph/gfsim toolchain is unavailable")

        import agentic_circuit as ac
        from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

        spec = importlib.util.spec_from_file_location(
            "multi_output_fixture", STATEFUL_FIXTURE
        )
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load multi-output fixture")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)
        specialization = ac.jit(
            module.multi_output_atomic, workspace=STATEFUL_FIXTURE_ROOT
        )

        with tempfile.TemporaryDirectory(prefix="multi-output-gfsim-") as directory:
            root = Path(directory)
            raw = root / "raw.mlir"
            frozen = root / "frozen.mlir"
            generated = root / "model.cpp"
            raw.write_text(specialization.lower_acir(), encoding="utf-8")
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
            planned = subprocess.run(
                (str(plan_tool), str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, planned.returncode, planned.stderr)
            firing = next(
                block
                for block in json.loads(planned.stdout)["blocks"]
                if block["kind"] == "firing"
            )
            self.assertEqual("dispatch", firing["display_rule_name"])
            self.assertEqual(
                "architecture.py", firing["source_file"]
            )
            expression_names = {item["result"] for item in firing["expressions"]}
            self.assertTrue({"ack", "left", "right"} <= expression_names)
            self.assertEqual(
                [0, 1, 2], [item["ordinal"] for item in firing["output_presence"]]
            )
            emitted = subprocess.run(
                (str(cxxgen), str(frozen)), text=True, capture_output=True, check=False
            )
            self.assertEqual(0, emitted.returncode, emitted.stderr)
            repeated = subprocess.run(
                (str(cxxgen), str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, repeated.returncode, repeated.stderr)
            self.assertEqual(emitted.stdout, repeated.stdout)
            self.assertIn("struct rule_dispatch_policy", emitted.stdout)
            self.assertIn("state_entries_next", emitted.stdout)
            self.assertIn("output_ack_present", emitted.stdout)
            self.assertIn("rule_condition", emitted.stdout)
            self.assertIn("state_entries_", emitted.stdout)
            self.assertNotIn(str(ROOT), emitted.stdout)
            generated.write_text(emitted.stdout, encoding="utf-8")
            harness = root / "harness.cpp"
            harness.write_text(
                """#include "model.cpp"
#include <string_view>
using Model = ac_generated::MultiOutputAtomic;
using Command = ac_generated::Command;
int main() {
  Model model;
  auto rows = model.dispatch_rows();
  unsigned tick = 0;
  auto cycle = [&](bool left, bool right, bool ack) {
    const gfsim::Epoch epoch{++tick, 0};
    for (auto &row : rows) {
      if ((row.id == model.sink_0_id() && !left) ||
          (row.id == model.sink_1_id() && !right) ||
          (row.id == model.sink_2_id() && !ack)) continue;
      row.work(row.object, epoch);
    }
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Probe);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  };
  auto offer = [&](unsigned value, bool left, bool right, bool cancel) {
    if (!model.command().proposePush(Command{gfsim::UInt<8>{value},
          gfsim::UInt<1>{left}, gfsim::UInt<1>{right}, gfsim::UInt<1>{cancel}})) return false;
    model.command().doXfer({tick, 0});
    return true;
  };
  if (!offer(4, true, false, false)) return 1;
  cycle(false, true, true); cycle(false, true, true);
  if (static_cast<unsigned long long>(model.table_count().at(0)) != 1) return 2;
  if (!offer(6, false, true, false)) return 3;
  cycle(false, true, true); cycle(false, true, true);
  if (static_cast<unsigned long long>(model.table_count().at(0)) != 2 ||
      model.sink_1_values().size() != 1 || model.sink_2_values().size() != 2) return 4;
  if (!offer(8, true, false, false)) return 5;
  cycle(false, true, true); cycle(false, true, true);
  if (static_cast<unsigned long long>(model.table_count().at(0)) != 2) return 60;
  if (static_cast<unsigned long long>(model.table_entries().at(0)) != 6) return 61;
  unsigned changes = 0;
  unsigned previous = static_cast<unsigned long long>(model.table_count().at(0));
  unsigned entry_changes = 0;
  unsigned previous_entry =
      static_cast<unsigned long long>(model.table_entries().at(0));
  for (unsigned i = 0; i != 4; ++i) {
    cycle(true, true, true);
    unsigned current = static_cast<unsigned long long>(model.table_count().at(0));
    unsigned current_entry =
        static_cast<unsigned long long>(model.table_entries().at(0));
    changes += current != previous;
    entry_changes += current_entry != previous_entry;
    previous = current;
    previous_entry = current_entry;
  }
  if (changes != 1 || entry_changes != 1 || previous != 3 ||
      previous_entry != 8 || !model.command().isEmpty()) return 7;
  if (model.sink_0_values().size() != 2 || model.sink_1_values().size() != 1 ||
      model.sink_2_values().size() != 3) return 8;
  if (static_cast<unsigned long long>(model.sink_0_values()[0].value) != 4 ||
      static_cast<unsigned long long>(model.sink_0_values()[1].value) != 8) return 9;

  // NO_OPTIONAL and BOTH complete without manufacturing absent tokens.
  if (!offer(10, false, false, false)) return 10;
  cycle(true, true, true); cycle(true, true, true);
  if (!offer(11, true, true, false)) return 11;
  cycle(true, true, true); cycle(true, true, true);
  if (static_cast<unsigned long long>(model.table_count().at(0)) != 5 ||
      model.sink_0_values().size() != 3 || model.sink_1_values().size() != 2 ||
      model.sink_2_values().size() != 5) return 12;

  // RIGHT_FULL retains input and both state owners, then commits once.
  if (!offer(12, false, true, false)) return 13;
  cycle(true, false, true); cycle(true, false, true);
  if (!offer(14, false, true, false)) return 14;
  cycle(true, false, true); cycle(true, false, true);
  if (static_cast<unsigned long long>(model.table_count().at(0)) != 6) return 15;
  unsigned before = static_cast<unsigned long long>(model.table_count().at(0));
  unsigned release_changes = 0;
  for (unsigned i = 0; i != 4; ++i) {
    cycle(true, true, true);
    unsigned current = static_cast<unsigned long long>(model.table_count().at(0));
    release_changes += current != before;
    before = current;
  }
  if (release_changes != 1 || before != 7) return 16;

  // ACK_FULL blocks even with no optional outputs selected.
  if (!offer(16, false, false, false)) return 17;
  cycle(true, true, false); cycle(true, true, false);
  if (!offer(18, false, false, false)) return 18;
  cycle(true, true, false); cycle(true, true, false);
  if (static_cast<unsigned long long>(model.table_count().at(0)) != 8) return 19;
  before = 8;
  release_changes = 0;
  for (unsigned i = 0; i != 4; ++i) {
    cycle(true, true, true);
    unsigned current = static_cast<unsigned long long>(model.table_count().at(0));
    release_changes += current != before;
    before = current;
  }
  if (release_changes != 1 || before != 9) return 20;

  // LEFT+RIGHT+ACK full freezes a BOTH transaction until every selected sink
  // can participate in the same commit boundary.
  if (!offer(20, true, true, false)) return 21;
  cycle(false, false, false); cycle(false, false, false);
  if (!offer(22, true, true, false)) return 22;
  cycle(false, false, false); cycle(false, false, false);
  if (static_cast<unsigned long long>(model.table_count().at(0)) != 10) return 23;
  before = 10;
  release_changes = 0;
  for (unsigned i = 0; i != 5; ++i) {
    cycle(true, true, true);
    unsigned current = static_cast<unsigned long long>(model.table_count().at(0));
    release_changes += current != before;
    before = current;
  }
  if (release_changes != 1 || before != 11 || !model.command().isEmpty()) return 24;
  if (model.sink_0_values().size() != 5 || model.sink_1_values().size() != 6 ||
      model.sink_2_values().size() != 11) return 25;
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

    def test_stateless_packed_outputs_match_pyc_cpp_and_verilog(self) -> None:
        toolchain = Path(os.environ.get("PYC_TOOLCHAIN_ROOT", DEFAULT_TOOLCHAIN))
        opt = Path(os.environ.get("ACIR_OPT", toolchain / "bin/acir-opt"))
        pycgen = Path(
            os.environ.get("ACIR_QUEUE_PYCGEN", toolchain / "bin/acir-queue-pycgen")
        )
        pycc = Path(os.environ.get("PYCC", toolchain / "bin/pycc"))
        metadata = toolchain / "share/pycircuit/toolchain-metadata.json"
        runtime = Path(
            os.environ.get("PYC_RUNTIME_LIB", toolchain / "lib/libpyc6_runtime.a")
        )
        runtime_include = Path(
            os.environ.get("PYC_RUNTIME_INCLUDE", toolchain / "include")
        )
        cxx = shutil.which("c++")
        verilator = shutil.which("verilator")
        required = (opt, pycgen, pycc, metadata, runtime)
        if (
            cxx is None
            or verilator is None
            or any(not path.is_file() for path in required)
        ):
            self.skipTest("integrated PYC C++/Verilog toolchain is unavailable")

        with tempfile.TemporaryDirectory(prefix="multi-output-pyc-") as directory:
            root = Path(directory)
            raw = root / "raw.mlir"
            frozen = root / "frozen.mlir"
            output = root / "output"
            raw.write_text(
                RULE_FIXTURE.read_text(encoding="utf-8").replace(
                    "depths [2, 3] latencies [1, 2]",
                    "depths [1, 1] latencies [1, 1]",
                ),
                encoding="utf-8",
            )
            lowered = subprocess.run(
                (
                    str(opt),
                    "--verify-each=false",
                    "--pass-pipeline=builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)",
                    str(raw),
                    "-o",
                    str(frozen),
                ),
                cwd=ROOT,
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

            driver = r"""
  const unsigned values[] = {0, 5, 9};
  unsigned offered = 0;
  for (unsigned cycle = 0; cycle != 32; ++cycle) {
    const bool valid = cycle != 0 && offered != 3;
    const bool release = cycle >= 12;
    dut.rst = cycle == 0;
    dut.in_valid = valid;
    dut.in_data = valid ? values[offered] : 0;
    dut.out0_ready = release;
    dut.out1_ready = true;
    dut.clk = 0;
    STEP;
    const bool accepted = valid && READY;
    if (VALID0 && release) std::cout << "L " << cycle << " " << DATA0 << "\n";
    if (VALID1) std::cout << "A " << cycle << " " << DATA1 << "\n";
    dut.clk = 1;
    STEP;
    if (accepted) ++offered;
    dut.clk = 0;
    STEP;
  }
"""
            cpp_driver = (
                driver.replace("STEP", "dut.step()")
                .replace("READY", "dut.in_ready.value()")
                .replace("VALID0", "dut.out0_valid.value()")
                .replace("DATA0", "dut.out0_data.value()")
                .replace("VALID1", "dut.out1_valid.value()")
                .replace("DATA1", "dut.out1_data.value()")
                .replace(
                    "dut.rst = cycle == 0;",
                    "dut.rst = pyc::cpp::Wire<1>(cycle == 0 ? 1 : 0);",
                )
                .replace(
                    "dut.in_valid = valid;",
                    "dut.in_valid = pyc::cpp::Wire<1>(valid ? 1 : 0);",
                )
                .replace(
                    "dut.in_data = valid ? values[offered] : 0;",
                    "dut.in_data = pyc::cpp::Wire<8>(valid ? values[offered] : 0);",
                )
                .replace(
                    "dut.out0_ready = release;",
                    "dut.out0_ready = pyc::cpp::Wire<1>(release ? 1 : 0);",
                )
                .replace(
                    "dut.out1_ready = true;", "dut.out1_ready = pyc::cpp::Wire<1>(1);"
                )
                .replace("dut.clk = 0;", "dut.clk = pyc::cpp::Wire<1>(0);")
                .replace("dut.clk = 1;", "dut.clk = pyc::cpp::Wire<1>(1);")
            )
            cpp_harness = root / "cpp_harness.cpp"
            cpp_harness.write_text(
                '#include "multi_output.hpp"\n#include <iostream>\nint main() {\n'
                "  pyc::gen::multi_output dut;\n" + cpp_driver + "}\n",
                encoding="utf-8",
            )
            cpp_executable = root / "cpp_model"
            cpp_sources = sorted((output / "cpp").glob("multi_output*.cpp"))
            compiled = subprocess.run(
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
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, compiled.returncode, compiled.stderr)
            cpp_run = subprocess.run(
                (str(cpp_executable),), text=True, capture_output=True, check=False
            )
            self.assertEqual(0, cpp_run.returncode, cpp_run.stderr)

            rtl_driver = (
                driver.replace("STEP", "dut.eval()")
                .replace("READY", "dut.in_ready")
                .replace("VALID0", "dut.out0_valid")
                .replace("DATA0", "unsigned(dut.out0_data)")
                .replace("VALID1", "dut.out1_valid")
                .replace("DATA1", "unsigned(dut.out1_data)")
            )
            rtl_harness = root / "rtl_harness.cpp"
            rtl_harness.write_text(
                '#include "Vmulti_output.h"\n#include <iostream>\nint main() {\n'
                "  Vmulti_output dut;\n" + rtl_driver + "}\n",
                encoding="utf-8",
            )
            object_dir = root / "verilator_obj"
            rtl_build = subprocess.run(
                (
                    verilator,
                    "--cc",
                    "--exe",
                    "--build",
                    "-Wno-fatal",
                    "--top-module",
                    "multi_output",
                    "--Mdir",
                    str(object_dir),
                    str(output / "verilog/pyc_primitives.v"),
                    str(output / "verilog/multi_output.v"),
                    str(rtl_harness),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, rtl_build.returncode, rtl_build.stderr)
            rtl_run = subprocess.run(
                (str(object_dir / "Vmulti_output"),),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, rtl_run.returncode, rtl_run.stderr)
            self.assertEqual(cpp_run.stdout, rtl_run.stdout)
            lines = cpp_run.stdout.splitlines()
            self.assertEqual(
                [5, 9],
                [int(line.split()[2]) for line in lines if line.startswith("L ")],
            )
            ack_lines = [line.split() for line in lines if line.startswith("A ")]
            self.assertEqual([7, 7, 7], [int(line[2]) for line in ack_lines])
            self.assertLess(int(ack_lines[0][1]), 12)


if __name__ == "__main__":
    unittest.main()
