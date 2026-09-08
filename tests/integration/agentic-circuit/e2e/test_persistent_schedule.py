from __future__ import annotations

import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
EXAMPLE = ROOT / "examples/agentic-circuit/pipelines/persistent_schedule.py"


class ScheduleV2Test(unittest.TestCase):
    def test_frozen_schedule_retains_completion_after_producer_output(self) -> None:
        cxx = shutil.which("c++")
        tools = {
            "opt": ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt",
            "plan": ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-plan",
            "cxxgen": ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-cxxgen",
            "pycgen": ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-pycgen",
        }
        if cxx is None or any(not path.is_file() for path in tools.values()):
            self.skipTest("C++ and native QueueGraph tools are required")

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            acir = root / "schedule-v2.mlir"
            plan = root / "schedule-v2.json"
            model = root / "schedule-v2.cpp"
            generated = subprocess.run(
                (
                    str(ROOT / "compiler/acir/tools/ac-queue-cxxgen.py"),
                    str(EXAMPLE),
                    "--system",
                    "schedule_v2",
                    "--acir-output",
                    str(acir),
                    "--plan-output",
                    str(plan),
                    "--acir-opt",
                    str(tools["opt"]),
                    "--queue-plan-tool",
                    str(tools["plan"]),
                    "--queue-cxxgen-tool",
                    str(tools["cxxgen"]),
                    "--output",
                    str(model),
                ),
                cwd=ROOT,
                env={
                    **os.environ,
                    "PYTHONPATH": os.pathsep.join(
                        (
                            str(ROOT / "python/semantic-core/src"),
                            str(ROOT / "python/agentic-circuit/src"),
                        )
                    ),
                },
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, generated.returncode, generated.stderr)
            raw = acir.read_text(encoding="utf-8")
            plan_value = json.loads(plan.read_bytes())
            dependency = next(
                block for block in plan_value["blocks"] if block["kind"] == "dependency"
            )
            self.assertIn('ac.schedule_provider = "v2"', raw)
            self.assertEqual("v2", dependency["provider"])
            source = model.read_text(encoding="utf-8")
            self.assertIn("gfsim::Schedule<ScheduleToken, 4, 2, 255", source)

            pyc = subprocess.run(
                (str(tools["pycgen"]), str(acir)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertNotEqual(0, pyc.returncode)
            self.assertIn("schedule v2 PYC provider requires issue #21", pyc.stderr)

            harness = root / "harness.cpp"
            executable = root / "schedule-v2"
            harness.write_text(
                f'''#include "{model.name}"

#include <array>

int main() {{
  ac_generated::ScheduleV2 model;
  const ac_generated::ScheduleToken producer{{0, 255, 0, 1, 10}};
  const ac_generated::ScheduleToken blocker{{1, 255, 1, 8, 20}};
  const ac_generated::ScheduleToken dependent{{2, 0, 1, 1, 30}};
  const std::array inputs{{producer, blocker, dependent}};
  std::size_t cursor = 0;
  auto rows = model.dispatch_rows();
  for (unsigned tick = 0; tick < 24; ++tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    if (cursor < inputs.size() && model.incoming().proposePush(inputs[cursor]))
      ++cursor;
    for (auto &row : rows) row.work(row.object, epoch);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Probe);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }}
  const auto &values = model.sink_0_values();
  return values.size() == 3 && values[0].sequence.value() == 0 &&
                 values[1].sequence.value() == 1 &&
                 values[2].sequence.value() == 2
             ? 0
             : 2;
}}
''',
                encoding="utf-8",
            )
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


if __name__ == "__main__":
    unittest.main()
