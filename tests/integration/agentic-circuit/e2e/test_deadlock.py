from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[4]
EXAMPLE = ROOT / "examples/agentic-circuit" / "pipelines" / "pyc_barrier_pipeline.py"


class GeneratedDeadlockTest(unittest.TestCase):
    def test_generated_barrier_reports_missing_peer_as_no_progress(self) -> None:
        cxx = shutil.which("c++")
        runtime = ROOT / ".pycircuit_out/acir/dev-llvm22/lib/gfsim/libgfsim.a"
        if cxx is None or not runtime.is_file():
            self.skipTest("C++ compiler or gfsim runtime is unavailable")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            model = root / "barrier.cpp"
            acir = root / "barrier.ac.mlir"
            plan = root / "barrier.queue-plan.json"
            generated = subprocess.run(
                (
                    str(ROOT / "compiler/acir/tools/ac-queue-cxxgen.py"),
                    str(EXAMPLE),
                    "--system",
                    "pyc_barrier_pipeline",
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
            self.assertIn('block_0_("barrier_left_ready"', content)

            harness = root / "harness.cpp"
            executable = root / "deadlock"
            harness.write_text(
                f"""#include "{model.name}"
#include "gfsim/object.h"
#include <iostream>

int main() {{
  ac_generated::PycBarrierPipeline model;
  if (!model.left().proposePush(ac_generated::LeftToken{{11}}))
    return 1;
  model.left().doXfer({{0, 0}});
  auto rows = model.dispatch_rows();
  gfsim::SimSystem system("deadlock");
  if (!system.setDispatchTable(rows))
    return 2;
  for (const auto &row : rows)
    if (!system.scheduleWork(row.id, {{0, 0}}))
      return 3;
  auto result = system.run();
  if (result.classification != gfsim::TerminationClass::Failed ||
      result.diagnosticCode != "no_progress") {{
    std::cerr << result.diagnosticCode << "\\n";
    return 4;
  }}
  return 0;
}}
""",
                encoding="utf-8",
            )
            compiled = subprocess.run(
                (
                    cxx,
                    "-std=c++20",
                    "-I",
                    str(ROOT / "simulator/gfsim/include"),
                    str(harness),
                    str(runtime),
                    "-o",
                    str(executable),
                ),
                cwd=root,
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


if __name__ == "__main__":
    unittest.main()
