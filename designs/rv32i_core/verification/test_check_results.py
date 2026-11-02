"""Verifier inventory regressions; these fixtures do not execute a hardware DUT."""

import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch


class ResultInventoryTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="rv32i-check-inventory-")
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)
        self.results = root / "results"
        self.results.mkdir()
        scripts = Path(__file__).resolve().parent
        with patch.object(sys, "path", [str(scripts), *sys.path]):
            spec = importlib.util.spec_from_file_location(
                "rv32i_check_results", scripts / "check_results.py"
            )
            self.checker = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(self.checker)
        # Numerical oracles have their own generated-model campaign. This test
        # isolates inventory admission and must never be cited as DUT evidence.
        oracle = patch.object(self.checker, "compare", return_value={"status": "PASS"})
        oracle.start()
        self.addCleanup(oracle.stop)
        arguments = patch.object(
            sys,
            "argv",
            [
                "check_results.py",
                "--programs",
                str(root / "programs"),
                "--results",
                str(self.results),
            ],
        )
        arguments.start()
        self.addCleanup(arguments.stop)
        self.rows = []
        for strategy in ("timing", "functional"):
            for target in ("cpp", "verilog"):
                view = f"{strategy}-{target}"
                build = self.results / "build" / view
                build.mkdir(parents=True)
                (build / "core.ac").write_text("same final artifact\n")
                output = self.results / "runs" / "smoke" / "fast" / view
                output.mkdir(parents=True)
                (output / "state.bits").write_text("0101\n")
                self.rows.append(
                    {
                        "case": "smoke",
                        "schedule": "fast",
                        "strategy": strategy,
                        "target": target,
                        "status": "PASS",
                        "returncode": 0,
                        "basis": "spike",
                    }
                )

    def reject(self, rows):
        (self.results / "summary.json").write_text(json.dumps(rows))
        receipt = self.results / "raw-validation.json"
        receipt.write_text(json.dumps({"status": "PASS", "views": 4}))
        with self.assertRaises((AssertionError, ValueError, SystemExit)):
            self.checker.main()
        self.assertNotEqual(json.loads(receipt.read_text())["status"], "PASS")

    def test_empty_inventory(self):
        self.reject([])

    def test_missing_view(self):
        self.reject(self.rows[:-1])

    def test_duplicate_view(self):
        self.rows[-1] = dict(self.rows[0])
        self.reject(self.rows)

    def test_failed_view(self):
        self.rows[-1]["status"] = "FAIL"
        self.reject(self.rows)

    def test_complete_four_view_inventory(self):
        (self.results / "summary.json").write_text(json.dumps(self.rows))
        self.checker.main()
        report = json.loads((self.results / "raw-validation.json").read_text())
        self.assertEqual(report["status"], "PASS")
        self.assertEqual(report["views"], 4)
        self.assertEqual(report["complete_state_groups"], 1)


if __name__ == "__main__":
    unittest.main()
