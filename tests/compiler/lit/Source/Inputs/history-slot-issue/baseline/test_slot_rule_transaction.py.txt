from __future__ import annotations

import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
FIXTURES = ROOT / "tests/integration/agentic-circuit/e2e/fixtures"
DEFAULT_TOOLCHAIN = ROOT / ".pycircuit_out/toolchain/install"


class SlotRuleTransactionTest(unittest.TestCase):
    def test_public_slot_rule_examples_generate_native_cpp(self) -> None:
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

        examples = ROOT / "examples/agentic-circuit/state"
        for system in ("slot_rule_mailbox",):
            with self.subTest(system=system), tempfile.TemporaryDirectory(
                prefix=f"{system}-"
            ) as directory:
                output = Path(directory)
                generated = output / "model.cpp"
                completed = subprocess.run(
                    (
                        str(ROOT / "compiler/acir/tools/ac-queue-cxxgen.py"),
                        str(examples / f"{system}.py"),
                        "--system",
                        system,
                        "--acir-output",
                        str(output / "model.mlir"),
                        "--plan-output",
                        str(output / "plan.json"),
                        "--acir-opt",
                        str(opt),
                        "--queue-plan-tool",
                        str(plan_tool),
                        "--queue-cxxgen-tool",
                        str(cxxgen),
                        "-o",
                        str(generated),
                    ),
                    text=True,
                    capture_output=True,
                )
                self.assertEqual(0, completed.returncode, completed.stderr)
                compiled = subprocess.run(
                    (
                        cxx,
                        "-std=c++20",
                        "-I",
                        str(ROOT / "simulator/gfsim/include"),
                        "-fsyntax-only",
                        str(generated),
                    ),
                    text=True,
                    capture_output=True,
                )
                self.assertEqual(0, compiled.returncode, compiled.stderr)

    def test_flat_and_nested_slot_rules_generate_and_run_identically(self) -> None:
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

        cases = (
            ("slot_rule_transaction", "SlotRuleTransaction"),
            ("slot_rule_transaction_nested", "SlotRuleTransactionNested"),
        )
        for system, class_name in cases:
            with self.subTest(system=system), tempfile.TemporaryDirectory(
                prefix=f"{system}-"
            ) as directory:
                output = Path(directory)
                source = FIXTURES / system / "architecture.py"
                generated = output / "model.cpp"
                plan = output / "plan.json"
                command = (
                    str(ROOT / "compiler/acir/tools/ac-queue-cxxgen.py"),
                    str(source),
                    "--system",
                    system,
                    "--acir-output",
                    str(output / "model.mlir"),
                    "--plan-output",
                    str(plan),
                    "--acir-opt",
                    str(opt),
                    "--queue-plan-tool",
                    str(plan_tool),
                    "--queue-cxxgen-tool",
                    str(cxxgen),
                    "-o",
                    str(generated),
                )
                first = subprocess.run(command, text=True, capture_output=True)
                self.assertEqual(0, first.returncode, first.stderr)
                first_source = generated.read_text(encoding="utf-8")
                first_plan = plan.read_text(encoding="utf-8")
                repeated = subprocess.run(command, text=True, capture_output=True)
                self.assertEqual(0, repeated.returncode, repeated.stderr)
                self.assertEqual(first_source, generated.read_text(encoding="utf-8"))
                self.assertEqual(first_plan, plan.read_text(encoding="utf-8"))

                document = json.loads(first_plan)
                nested_plans = document.get("module_specializations", ())
                selected = nested_plans[0] if nested_plans else document
                firing = next(
                    block for block in selected["blocks"] if block["kind"] == "firing"
                )
                self.assertEqual(
                    [{"slot": "mailbox", "when": firing["guard"]}],
                    firing["slot_releases"],
                )
                self.assertIn(
                    {"kind": "slot", "ordinal": 0, "resource": "mailbox"},
                    firing["transaction_resources"],
                )

                harness = output / "harness.cpp"
                harness.write_text(
                    f'''#include "model.cpp"
int main() {{
  ac_generated::{class_name} model;
  auto rows = model.dispatch_rows();
  unsigned tick = 0;
  auto cycle = [&]() {{
    const gfsim::Epoch epoch{{++tick, 0}};
    for (auto &row : rows) row.work(row.object, epoch);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Probe);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }};
  if (!model.incoming().proposePush({{gfsim::UInt<8>{{7}}}})) return 1;
  model.incoming().doXfer({{0, 0}});
  cycle();
  if (!model.sink_0_values().empty()) return 2;
  cycle(); cycle();
  if (model.sink_0_values().size() != 1) return 3;
  if (static_cast<unsigned long long>(model.sink_0_values()[0].value) != 7) return 4;
  cycle(); cycle(); cycle();
  return model.sink_0_values().size() == 1 ? 0 : 5;
}}
''',
                    encoding="utf-8",
                )
                executable = output / "model"
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
                )
                self.assertEqual(0, compiled.returncode, compiled.stderr)
                executed = subprocess.run(
                    (str(executable),), text=True, capture_output=True
                )
                self.assertEqual(0, executed.returncode, executed.stderr)

    def test_read_only_and_releasing_rules_share_the_committed_snapshot(self) -> None:
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

        source = FIXTURES / "state" / "shared_slot_snapshot.py"
        with tempfile.TemporaryDirectory(prefix="shared-slot-snapshot-") as directory:
            output = Path(directory)
            generated = output / "model.cpp"
            completed = subprocess.run(
                (
                    str(ROOT / "compiler/acir/tools/ac-queue-cxxgen.py"),
                    str(source),
                    "--system",
                    "shared_slot_snapshot",
                    "--acir-output",
                    str(output / "model.mlir"),
                    "--plan-output",
                    str(output / "plan.json"),
                    "--acir-opt",
                    str(opt),
                    "--queue-plan-tool",
                    str(plan_tool),
                    "--queue-cxxgen-tool",
                    str(cxxgen),
                    "-o",
                    str(generated),
                ),
                text=True,
                capture_output=True,
            )
            self.assertEqual(0, completed.returncode, completed.stderr)

            harness = output / "harness.cpp"
            harness.write_text(
                """#include "model.cpp"
int main() {
  ac_generated::SharedSlotSnapshot model;
  auto rows = model.dispatch_rows();
  unsigned tick = 0;
  auto cycle = [&]() {
    const gfsim::Epoch epoch{++tick, 0};
    for (auto &row : rows) row.work(row.object, epoch);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Probe);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  };
  if (!model.incoming().proposePush({gfsim::UInt<8>{7}})) return 1;
  model.incoming().doXfer({0, 0});
  cycle();
  if (!model.sink_0_values().empty() || !model.sink_1_values().empty()) return 2;
  cycle(); cycle();
  if (model.sink_0_values().size() != 1 || model.sink_1_values().size() != 1)
    return 3;
  if (static_cast<unsigned long long>(model.sink_0_values()[0].value) != 7 ||
      static_cast<unsigned long long>(model.sink_1_values()[0].value) != 7)
    return 4;
  cycle(); cycle();
  return model.sink_0_values().size() == 1 &&
                 model.sink_1_values().size() == 1 ? 0 : 5;
}
""",
                encoding="utf-8",
            )
            executable = output / "model"
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
            )
            self.assertEqual(0, compiled.returncode, compiled.stderr)
            executed = subprocess.run(
                (str(executable),), text=True, capture_output=True
            )
            self.assertEqual(0, executed.returncode, executed.stderr)

    def test_queue_state_output_and_slot_release_commit_together(self) -> None:
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

        source = FIXTURES / "slot_rule_atomic_effects" / "architecture.py"
        with tempfile.TemporaryDirectory(prefix="slot-rule-atomic-") as directory:
            output = Path(directory)
            generated = output / "model.cpp"
            completed = subprocess.run(
                (
                    str(ROOT / "compiler/acir/tools/ac-queue-cxxgen.py"),
                    str(source),
                    "--system",
                    "slot_rule_atomic_effects",
                    "--acir-output",
                    str(output / "model.mlir"),
                    "--plan-output",
                    str(output / "plan.json"),
                    "--acir-opt",
                    str(opt),
                    "--queue-plan-tool",
                    str(plan_tool),
                    "--queue-cxxgen-tool",
                    str(cxxgen),
                    "-o",
                    str(generated),
                ),
                text=True,
                capture_output=True,
            )
            self.assertEqual(0, completed.returncode, completed.stderr)

            plan = json.loads((output / "plan.json").read_text(encoding="utf-8"))
            firing = next(
                block for block in plan["blocks"] if block["kind"] == "firing"
            )
            self.assertEqual(
                {"input_queue", "output_queue", "state", "slot"},
                {resource["kind"] for resource in firing["transaction_resources"]},
            )
            self.assertEqual(
                [{"slot": "mailbox", "when": firing["guard"]}], firing["slot_releases"]
            )

            harness = output / "harness.cpp"
            harness.write_text(
                """#include "model.cpp"
int main() {
  ac_generated::SlotRuleAtomicEffects model;
  auto rows = model.dispatch_rows();
  unsigned tick = 0;
  auto cycle = [&]() {
    const gfsim::Epoch epoch{++tick, 0};
    for (auto &row : rows) row.work(row.object, epoch);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Probe);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  };
  if (!model.request().proposePush({gfsim::UInt<8>{3}})) return 1;
  model.request().doXfer({0, 0});
  cycle(); cycle();
  if (model.request().committedSize() != 1 || !model.sink_0_values().empty())
    return 2;
  if (!model.incoming().proposePush({gfsim::UInt<8>{9}})) return 3;
  model.incoming().doXfer({tick, 0});
  cycle();
  if (model.request().committedSize() != 1 || !model.sink_0_values().empty())
    return 4;
  cycle(); cycle();
  if (!model.request().isEmpty() || model.sink_0_values().size() != 1) return 5;
  if (static_cast<unsigned long long>(model.sink_0_values()[0].value) != 12)
    return 6;
  if (!model.request().proposePush({gfsim::UInt<8>{4}}) ||
      !model.incoming().proposePush({gfsim::UInt<8>{1}})) return 7;
  model.request().doXfer({tick, 0});
  model.incoming().doXfer({tick, 0});
  cycle(); cycle(); cycle();
  if (model.sink_0_values().size() != 2) return 8;
  if (static_cast<unsigned long long>(model.sink_0_values()[1].value) != 8)
    return 9;
  cycle(); cycle();
  return model.sink_0_values().size() == 2 ? 0 : 10;
}
""",
                encoding="utf-8",
            )
            executable = output / "model"
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
            )
            self.assertEqual(0, compiled.returncode, compiled.stderr)
            executed = subprocess.run(
                (str(executable),), text=True, capture_output=True
            )
            self.assertEqual(0, executed.returncode, executed.stderr)


if __name__ == "__main__":
    unittest.main()
