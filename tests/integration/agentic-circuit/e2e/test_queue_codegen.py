from __future__ import annotations

import os
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[4]
SOURCE = ROOT / "examples/agentic-circuit" / "pipelines" / "davincioo_queue_model.py"
CONDITIONAL_SOURCE = (
    ROOT / "examples/agentic-circuit" / "pipelines" / "pyc_conditional_pipeline.py"
)
REORDER_SOURCE = (
    ROOT / "examples/agentic-circuit" / "pipelines" / "pyc_reorder_pipeline.py"
)
DAVINCIOO_TRACE = (
    ROOT
    / "references/davincioo-gfsim/upstream/tests/fixtures/traces"
    / "examples_intermediate_softmax.pto.trace"
)
DAVINCIOO_PROJECTION = (
    ROOT / "tests/goldens/agentic-circuit/davincioo/softmax-projection.json"
)


class QueueCodegenTest(unittest.TestCase):
    def test_reorder_python_generates_and_runs_typed_cpp(self) -> None:
        compiler = shutil.which("c++")
        if compiler is None:
            self.skipTest("C++ compiler is unavailable")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            model = root / "reorder.cpp"
            acir = root / "reorder.ac.mlir"
            plan = root / "reorder.queue-plan.json"
            generated = subprocess.run(
                (
                    str(ROOT / "compiler/acir/tools/ac-queue-cxxgen.py"),
                    str(REORDER_SOURCE),
                    "--system",
                    "pyc_reorder_pipeline",
                    "--acir-output",
                    str(acir),
                    "--plan-output",
                    str(plan),
                    "--acir-opt",
                    str(ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"),
                    "--queue-plan-tool",
                    str(ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-plan"),
                    "--queue-cxxgen-tool",
                    str(ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-cxxgen"),
                    "--output",
                    str(model),
                ),
                cwd=ROOT,
                env={
                    **os.environ,
                    "PYTHONPATH": str(ROOT / "python/agentic-circuit/src"),
                },
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, generated.returncode, generated.stderr)
            content = model.read_text(encoding="utf-8")
            self.assertIn("gfsim::QueueReorder<Token", content)

            harness = root / "harness.cpp"
            executable = root / "reorder"
            harness.write_text(
                f"""#include "{model.name}"
#include <array>
#include <cstddef>

int main() {{
  ac_generated::PycReorderPipeline model;
  const std::array<ac_generated::Token, 3> input{{
      ac_generated::Token{{2, 20}},
      ac_generated::Token{{0, 0}},
      ac_generated::Token{{1, 10}},
  }};
  for (const auto &item : input)
    if (!model.completed().proposePush(item))
      return 1;
  auto rows = model.dispatch_rows();
  for (std::size_t tick = 0; tick < 20; ++tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }}
  const auto &values = model.sink_0_values();
  if (values.size() != 3)
    return 2;
  for (std::size_t index = 0; index < values.size(); ++index)
    if (values[index].sequence != index)
      return 3;
  return 0;
}}
""",
                encoding="utf-8",
            )
            linked = subprocess.run(
                (
                    compiler,
                    "-std=c++20",
                    "-I",
                    str(ROOT / "simulator/gfsim/include"),
                    str(harness),
                    "-o",
                    str(executable),
                ),
                cwd=root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, linked.returncode, linked.stderr)
            executed = subprocess.run(
                (str(executable),),
                cwd=root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, executed.returncode, executed.stderr)

    def test_serial_runtime_if_generates_and_runs_common_blocks(self) -> None:
        compiler = shutil.which("c++")
        if compiler is None:
            self.skipTest("C++ compiler is unavailable")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            model = root / "conditional.cpp"
            acir = root / "conditional.ac.mlir"
            plan = root / "conditional.queue-plan.json"
            generated = subprocess.run(
                (
                    str(ROOT / "compiler/acir/tools/ac-queue-cxxgen.py"),
                    str(CONDITIONAL_SOURCE),
                    "--system",
                    "pyc_conditional_pipeline",
                    "--acir-output",
                    str(acir),
                    "--plan-output",
                    str(plan),
                    "--acir-opt",
                    str(ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"),
                    "--queue-plan-tool",
                    str(ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-plan"),
                    "--queue-cxxgen-tool",
                    str(ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-cxxgen"),
                    "--output",
                    str(model),
                ),
                cwd=ROOT,
                env={
                    **os.environ,
                    "PYTHONPATH": str(ROOT / "python/agentic-circuit/src"),
                },
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, generated.returncode, generated.stderr)
            content = model.read_text(encoding="utf-8")
            self.assertIn("gfsim::QueueRoute<Item, 2", content)
            self.assertIn("gfsim::QueueTransform<Item, Item", content)
            self.assertIn("gfsim::QueueMerge<Item, 2>", content)
            plan_document = json.loads(plan.read_text(encoding="utf-8"))
            self.assertEqual("0.4", plan_document["contract_epoch"])

            harness = root / "harness.cpp"
            executable = root / "conditional"
            harness.write_text(
                f"""#include "{model.name}"
#include <array>
#include <cstddef>

int main() {{
  ac_generated::PycConditionalPipeline model;
  if (!model.input_queue().proposePush(ac_generated::Item{{1, 0}}) ||
      !model.input_queue().proposePush(ac_generated::Item{{1, 1}}))
    return 1;
  auto rows = model.dispatch_rows();
  for (std::size_t tick = 0; tick < 16; ++tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }}
  const auto values = model.sink_0_values();
  if (values.size() != 2)
    return 2;
  return values[0].value == 11 && values[1].value == 21 ? 0 : 3;
}}
""",
                encoding="utf-8",
            )
            linked = subprocess.run(
                (
                    compiler,
                    "-std=c++20",
                    "-I",
                    str(ROOT / "simulator/gfsim/include"),
                    str(harness),
                    "-o",
                    str(executable),
                ),
                cwd=root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, linked.returncode, linked.stderr)
            executed = subprocess.run(
                (str(executable),),
                cwd=root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, executed.returncode, executed.stderr)

    def test_native_frozen_acir_codegen_covers_common_state_blocks(self) -> None:
        from python_frontend.test_queue_codegen import (
            BROADCAST_SOURCE,
            FEEDBACK_SOURCE,
        )
        from python_frontend.test_queue_frontend import (
            BARRIER_SOURCE,
            CREDIT_SOURCE,
            EXPECT_SOURCE,
            FIRING_SOURCE,
            LOOP_CONTROL_SOURCE,
            MEMORY_SOURCE,
            RECURSION_SOURCE,
            SELECT_SOURCE,
        )

        compiler = shutil.which("c++")
        if compiler is None:
            self.skipTest("C++ compiler is unavailable")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name, source, expected in (
                ("barrier", BARRIER_SOURCE, "gfsim::QueueBarrier"),
                ("broadcast", BROADCAST_SOURCE, "gfsim::QueueBroadcast"),
                ("credit", CREDIT_SOURCE, "gfsim::QueueCredit"),
                ("expect", EXPECT_SOURCE, "gfsim::QueueExpect"),
                ("feedback", FEEDBACK_SOURCE, "gfsim::QueueFeedback"),
                ("firing", FIRING_SOURCE, "gfsim::QueueTransform"),
                ("loop_control", LOOP_CONTROL_SOURCE, "gfsim::QueueFeedback"),
                ("memory", MEMORY_SOURCE, "gfsim::QueueMemory"),
                ("recursion", RECURSION_SOURCE, "gfsim::QueueTransform"),
                ("select", SELECT_SOURCE, "gfsim::QueueSelect"),
            ):
                python = root / f"{name}.py"
                model = root / f"{name}.cpp"
                acir = root / f"{name}.ac.mlir"
                plan = root / f"{name}.queue-plan.json"
                python.write_text(source, encoding="utf-8")
                generated = subprocess.run(
                    (
                        str(ROOT / "compiler/acir/tools" / "ac-queue-cxxgen.py"),
                        str(python),
                        "--system",
                        "pipeline",
                        "--acir-output",
                        str(acir),
                        "--plan-output",
                        str(plan),
                        "--acir-opt",
                        str(ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"),
                        "--queue-plan-tool",
                        str(
                            ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-plan"
                        ),
                        "--queue-cxxgen-tool",
                        str(
                            ROOT
                            / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-cxxgen"
                        ),
                        "-o",
                        str(model),
                    ),
                    cwd=ROOT,
                    env={
                        **os.environ,
                        "PYTHONPATH": str(ROOT / "python/agentic-circuit/src"),
                    },
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(0, generated.returncode, generated.stderr)
                self.assertIn(expected, model.read_text(encoding="utf-8"))
                compiled = subprocess.run(
                    (
                        compiler,
                        "-std=c++20",
                        "-I",
                        str(ROOT / "simulator/gfsim/include"),
                        "-fsyntax-only",
                        str(model),
                    ),
                    cwd=root,
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(0, compiled.returncode, compiled.stderr)

    def test_davincioo_like_python_generates_and_runs_typed_cpp(self) -> None:
        compiler = shutil.which("c++")
        if compiler is None:
            self.skipTest("C++ compiler is unavailable")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            model = root / "model.cpp"
            acir = root / "model.ac.mlir"
            plan = root / "model.queue-plan.json"
            harness = root / "harness.cpp"
            executable = root / "model"
            generated = subprocess.run(
                (
                    str(ROOT / "compiler/acir/tools" / "ac-queue-cxxgen.py"),
                    str(SOURCE),
                    "--system",
                    "davincioo_queue_model",
                    "--acir-output",
                    str(acir),
                    "--plan-output",
                    str(plan),
                    "--acir-opt",
                    str(
                        ROOT
                        / ".pycircuit_out"
                        / "acir"
                        / "dev-llvm22"
                        / "bin"
                        / "acir-opt"
                    ),
                    "--queue-plan-tool",
                    str(ROOT / "build" / "dev-llvm22" / "bin" / "acir-queue-plan"),
                    "--queue-cxxgen-tool",
                    str(ROOT / "build" / "dev-llvm22" / "bin" / "acir-queue-cxxgen"),
                    "-o",
                    str(model),
                ),
                cwd=ROOT,
                env={
                    **os.environ,
                    "PYTHONPATH": str(ROOT / "python/agentic-circuit/src"),
                },
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, generated.returncode, generated.stderr)
            content = model.read_text(encoding="utf-8")
            plan_document = json.loads(plan.read_text(encoding="utf-8"))
            self.assertEqual("davincioo_queue_model", plan_document["system"])
            self.assertEqual(8, len(plan_document["scopes"]))
            self.assertEqual(14, len(plan_document["queues"]))
            self.assertEqual(16, len(plan_document["blocks"]))
            copied_source = root / "copied_model.py"
            copied_model = root / "copied_model.cpp"
            copied_acir = root / "copied_model.ac.mlir"
            copied_plan = root / "copied_model.queue-plan.json"
            shutil.copyfile(SOURCE, copied_source)
            regenerated = subprocess.run(
                (
                    str(ROOT / "compiler/acir/tools" / "ac-queue-cxxgen.py"),
                    str(copied_source),
                    "--system",
                    "davincioo_queue_model",
                    "--acir-output",
                    str(copied_acir),
                    "--plan-output",
                    str(copied_plan),
                    "--acir-opt",
                    str(
                        ROOT
                        / ".pycircuit_out"
                        / "acir"
                        / "dev-llvm22"
                        / "bin"
                        / "acir-opt"
                    ),
                    "--queue-plan-tool",
                    str(ROOT / "build" / "dev-llvm22" / "bin" / "acir-queue-plan"),
                    "--queue-cxxgen-tool",
                    str(ROOT / "build" / "dev-llvm22" / "bin" / "acir-queue-cxxgen"),
                    "-o",
                    str(copied_model),
                ),
                cwd=root,
                env={
                    **os.environ,
                    "PYTHONPATH": str(ROOT / "python/agentic-circuit/src"),
                },
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, regenerated.returncode, regenerated.stderr)
            self.assertEqual(content, copied_model.read_text(encoding="utf-8"))
            for scope in (
                "frontend",
                "dependency",
                "dispatch",
                "scalar_engine",
                "vector_engine",
                "cube_engine",
                "tma_engine",
                "retire",
            ):
                self.assertIn(f'("{scope}", gfsim::kInvalidObjectId', content)
            self.assertIn("gfsim::QueueRoute<WorkItem, 4", content)
            self.assertIn("gfsim::QueueMerge<WorkItem, 4>", content)
            self.assertNotIn("gfsim::QueueFeedback<WorkItem", content)
            self.assertIn("gfsim::QueueDependency<WorkItem", content)
            self.assertIn("gfsim::QueueReorder<WorkItem", content)

            records = [
                json.loads(line)
                for line in DAVINCIOO_TRACE.read_text(encoding="utf-8").splitlines()
                if line.strip()
            ]
            self.assertEqual(list(range(15)), [row["sequence_id"] for row in records])
            projection = json.loads(DAVINCIOO_PROJECTION.read_text(encoding="utf-8"))
            opcode_ids = projection["opcode_ids"]
            model_cost = projection["model_cost"]
            routes = projection["routes"]
            items = [
                (
                    row["sequence_id"],
                    opcode_ids[row["opcode"]],
                    routes[row["opcode"]],
                    projection["waits_for"][row["sequence_id"]],
                    model_cost[row["opcode"]],
                    row["sequence_id"] * 10,
                )
                for row in records
            ]
            input_rows = ",\n      ".join(
                "ac_generated::WorkItem{" + ", ".join(map(str, item)) + "}"
                for item in items
            )
            expected_counts = [0] * len(opcode_ids)
            for opcode, count in projection["opcode_counts"].items():
                expected_counts[opcode_ids[opcode]] = count
            expected_counts_text = ", ".join(map(str, expected_counts))
            completion_order_text = ", ".join(map(str, projection["completion_order"]))
            architectural_values_text = ", ".join(
                map(str, projection["architectural_values"])
            )
            occupancy = projection["occupancy_projection"]
            resource_peaks_text = ", ".join(
                map(str, occupancy["resource_executing_peaks"])
            )

            oracle_summary = root / "oracle-summary.json"
            oracle = subprocess.run(
                (
                    str(
                        ROOT
                        / ".pycircuit_out/acir/dev-llvm22/bin/davincioo-gfsim-reference"
                    ),
                    "simulate",
                    "--trace",
                    str(DAVINCIOO_TRACE),
                    "--summary-out",
                    str(oracle_summary),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, oracle.returncode, oracle.stderr)
            oracle_document = json.loads(oracle_summary.read_text(encoding="utf-8"))
            self.assertEqual(15, oracle_document["record_count"])
            self.assertEqual(
                projection["simulated_cycles"],
                oracle_document["simulated_cycles"],
            )
            self.assertEqual(
                projection["opcode_counts"], oracle_document["opcode_counts"]
            )

            harness.write_text(
                f"""#include "{model.name}"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

int main() {{
  ac_generated::DavinciooQueueModel model;
  const std::array<ac_generated::WorkItem, 15> input{{
      {input_rows},
  }};
  for (const auto &item : input)
    if (!model.trace().proposePush(item))
      return 1;
  auto rows = model.dispatch_rows();
  std::size_t simulatedCycles = 0;
  std::size_t dependencyPeak = 0;
  std::size_t reorderPeak = 0;
  std::array<std::size_t, 4> resourcePeaks{{}};
  for (std::size_t tick = 0; tick < 600; ++tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
    dependencyPeak = std::max(dependencyPeak, model.dependency_0_active());
    reorderPeak = std::max(reorderPeak, model.reorder_0_active());
    for (std::size_t resource = 0; resource < resourcePeaks.size(); ++resource)
      resourcePeaks[resource] = std::max(
          resourcePeaks[resource],
          model.dependency_0_resource_active(resource));
    if (model.sink_0_values().size() == input.size()) {{
      simulatedCycles = tick + 1;
      break;
    }}
  }}
  if (simulatedCycles != {projection["simulated_cycles"]}) {{
    std::cerr << "simulated_cycles=" << simulatedCycles << "\\n";
    return 2;
  }}
  const auto &values = model.sink_0_values();
  if (values.size() != input.size())
    return 3;
  std::array<std::size_t, 8> opcodeCounts{{}};
  const std::array<std::int64_t, 15> architecturalValues{{
      {architectural_values_text}}};
  for (std::size_t index = 0; index < values.size(); ++index) {{
    const auto &value = values[index];
    if (value.sequence_id != index || value.opcode != input[index].opcode ||
        value.waits_for != input[index].waits_for ||
        value.cycles != input[index].cycles)
      return 4;
    if (value.value != architecturalValues[index])
      return 5;
    ++opcodeCounts[value.opcode];
  }}
  const std::array<std::size_t, 8> expectedCounts{{{expected_counts_text}}};
  if (opcodeCounts != expectedCounts)
    return 6;
  const std::array<std::uint8_t, 15> completionOrder{{
      {completion_order_text}}};
  const auto &completed = model.observation_0_values();
  if (completed.size() != completionOrder.size())
    return 7;
  for (std::size_t index = 0; index < completed.size(); ++index)
    if (completed[index].sequence_id != completionOrder[index])
      return 8;
  if (dependencyPeak != {occupancy["dependency_window_peak"]} ||
      reorderPeak != {occupancy["reorder_window_peak"]})
    return 9;
  const std::array<std::size_t, 4> expectedResourcePeaks{{
      {resource_peaks_text}}};
  if (resourcePeaks != expectedResourcePeaks)
    return 10;
  return 0;
}}
""",
                encoding="utf-8",
            )
            linked = subprocess.run(
                (
                    compiler,
                    "-std=c++20",
                    "-I",
                    str(ROOT / "simulator/gfsim/include"),
                    str(harness),
                    "-o",
                    str(executable),
                ),
                cwd=root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, linked.returncode, linked.stderr)
            executed = subprocess.run(
                (str(executable),),
                cwd=root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, executed.returncode, executed.stderr)


if __name__ == "__main__":
    unittest.main()
