from __future__ import annotations

import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[4]


class DavinciOOTraceTest(unittest.TestCase):
    def test_generated_gfsim_matches_reference_projection_and_artifacts(self) -> None:
        if shutil.which("c++") is None:
            self.skipTest("C++ compiler is unavailable")
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "run"
            completed = subprocess.run(
                (
                    sys.executable,
                    str(ROOT / "compiler/acir/tools/run-davincioo.py"),
                    "--output-dir",
                    str(output),
                ),
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, completed.returncode, completed.stderr)
            report = json.loads((output / "run.json").read_text(encoding="utf-8"))
            self.assertEqual(15, report["record_count"])
            self.assertEqual(453, report["cycles"])
            self.assertEqual(report["reference_cycles"], report["cycles"])
            self.assertEqual(list(range(15)), report["retirement_order"])
            self.assertEqual(
                [0, 2, 3, 5, 7, 9, 10, 12, 1, 4, 6, 8, 11, 13, 14],
                report["completion_order"],
            )
            artifacts = ROOT / "tests/goldens/agentic-circuit/davincioo"
            self.assertEqual(
                (artifacts / "davincioo-softmax-run.json").read_bytes(),
                (output / "run.json").read_bytes(),
            )
            self.assertEqual(
                (artifacts / "davincioo-softmax-swimlane.svg").read_bytes(),
                (output / "swimlane.svg").read_bytes(),
            )


if __name__ == "__main__":
    unittest.main()
