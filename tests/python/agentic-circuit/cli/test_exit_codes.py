from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[4]


class ExitCodeTest(unittest.TestCase):
    def test_source_checkout_doctor_reports_missing_native_tools(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            environment = os.environ.copy()
            environment["PYTHONPATH"] = str(REPOSITORY / "python/agentic-circuit/src")
            result = subprocess.run(
                [sys.executable, "-m", "agentic_circuit._cli", "doctor", "--json"],
                cwd=temporary,
                env=environment,
                text=True,
                capture_output=True,
                check=False,
            )

        self.assertEqual(3, result.returncode, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual("agentic-circuit-doctor-result", report["schema"])
        self.assertEqual("failed", report["status"])
        self.assertIn(
            "native_extension",
            {
                check["name"]
                for check in report["checks"]
                if check["status"] == "failed"
            },
        )


if __name__ == "__main__":
    unittest.main()
