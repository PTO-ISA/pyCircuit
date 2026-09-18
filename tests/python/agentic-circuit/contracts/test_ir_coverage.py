from __future__ import annotations

import subprocess
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]


class IRCoverageTest(unittest.TestCase):
    def test_read_only_checker_accepts_the_repository(self) -> None:
        completed = subprocess.run(
            [sys.executable, "tools/agentic-circuit/check-ir-coverage.py"],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, completed.returncode, completed.stdout + completed.stderr)

    def test_ledger_contains_only_the_current_acir_surface(self) -> None:
        ledger = (ROOT / "docs/development/acir/verification/ir-coverage.md").read_text(
            encoding="utf-8"
        )
        self.assertIn("# ACIR coverage ledger", ledger)
        self.assertIn("## acir operations", ledger)


if __name__ == "__main__":
    unittest.main()
