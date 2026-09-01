from __future__ import annotations

import os
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "examples/state/table_scoreboard.py"


class TableBackendTest(unittest.TestCase):
    def test_scoreboard_generates_compiles_and_runs_old_data_contract(self) -> None:
        compiler = shutil.which("c++")
        if compiler is None:
            self.skipTest("C++ compiler is unavailable")

        from agentic_circuit._queue_codegen import lower_queue_program_to_cpp
        from agentic_circuit._queue_frontend import parse_queue_program

        generated = lower_queue_program_to_cpp(
            parse_queue_program(SOURCE.read_text(encoding="utf-8"), "table_scoreboard")
        )
        self.assertIn("gfsim::SimTable<Entry>", generated)
        self.assertIn("gfsim::QueueTableWrite<Update, Entry", generated)
        self.assertIn("gfsim::QueueTableRead<Request, Entry", generated)
        self.assertIn("gfsim::TableReadSource<Entry", generated)

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            model = root / "table_scoreboard.cpp"
            harness = root / "harness.cpp"
            executable = root / "table_scoreboard"
            model.write_text(generated, encoding="utf-8")
            harness.write_text(
                f'''#include "{model.name}"
#include <cstddef>

int main() {{
  ac_generated::TableScoreboard model;
  if (!model.updates().proposePush(ac_generated::Update{{0, true, true, 0x1234}}))
    return 1;
  if (!model.requests().proposePush(ac_generated::Request{{0, true}}))
    return 2;
  auto rows = model.dispatch_rows();
  for (std::size_t tick = 0; tick < 16; ++tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
    if (tick == 1 &&
        !model.requests().proposePush(ac_generated::Request{{0, true}}))
      return 7;
  }}
  const auto &responses = model.sink_0_values();
  if (responses.size() != 2)
    return 3;
  if (responses[0].valid || responses[0].done || responses[0].result != 0)
    return 4;
  if (!responses[1].valid || !responses[1].done || responses[1].result != 0x1234)
    return 5;
  const auto &snapshots = model.sink_1_values();
  if (snapshots.empty() || !snapshots.front().valid ||
      snapshots.front().result != 0x1234)
    return 6;
  return 0;
}}
''',
                encoding="utf-8",
            )
            compiled = subprocess.run(
                (
                    compiler,
                    "-std=c++20",
                    "-I",
                    str(ROOT / "include"),
                    str(harness),
                    "-o",
                    str(executable),
                ),
                cwd=root,
                env=os.environ,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, compiled.returncode, compiled.stderr)
            executed = subprocess.run(
                (str(executable),),
                cwd=root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, executed.returncode, executed.stderr)

    def test_native_queuegraph_and_pyc_boundary_when_tools_are_available(self) -> None:
        tools = {
            "opt": Path(
                os.environ.get("ACIR_OPT", ROOT / "build/dev-llvm22/bin/acir-opt")
            ),
            "plan": Path(
                os.environ.get(
                    "ACIR_QUEUE_PLAN",
                    ROOT / "build/dev-llvm22/bin/acir-queue-plan",
                )
            ),
            "cxxgen": Path(
                os.environ.get(
                    "ACIR_QUEUE_CXXGEN",
                    ROOT / "build/dev-llvm22/bin/acir-queue-cxxgen",
                )
            ),
            "pycgen": Path(
                os.environ.get(
                    "ACIR_QUEUE_PYCGEN",
                    ROOT / "build/dev-llvm22/bin/acir-queue-pycgen",
                )
            ),
        }
        missing = [name for name, path in tools.items() if not path.is_file()]
        if missing:
            self.skipTest(
                "native QueueGraph tools are unavailable: " + ", ".join(missing)
            )

        from agentic_circuit._queue_frontend import lower_queue_source

        acir = lower_queue_source(
            SOURCE.read_text(encoding="utf-8"), "table_scoreboard"
        )
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "table.ac.mlir"
            frozen = root / "table.frozen.mlir"
            source.write_text(acir, encoding="utf-8")
            verified = subprocess.run(
                (str(tools["opt"]), "--verify-ac-file", str(source), "-o", str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, verified.returncode, verified.stderr)
            planned = subprocess.run(
                (str(tools["plan"]), str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, planned.returncode, planned.stderr)
            document = json.loads(planned.stdout)
            self.assertEqual("0.4", document["contract_epoch"])
            self.assertEqual(1, len(document["tables"]))
            self.assertEqual(2, len(document["table_reads"]))
            self.assertEqual(1, len(document["table_writes"]))

            generated = subprocess.run(
                (str(tools["cxxgen"]), str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, generated.returncode, generated.stderr)
            self.assertIn("gfsim::SimTable<Entry>", generated.stdout)

            rejected = subprocess.run(
                (str(tools["pycgen"]), str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertNotEqual(0, rejected.returncode)
            self.assertIn("unsupported provisional Table", rejected.stderr)


if __name__ == "__main__":
    unittest.main()
