"""Independent admission checks for the published-artifact verification summary.

The numerical comparator and compilation are stubbed here to isolate cross-view
aggregation. These tests do not execute a DUT or claim hardware conformance.
"""

import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch


class ArtifactCycleValidationTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="artifact-validation-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        scripts = Path(__file__).resolve().parent
        with patch.object(sys, "path", [str(scripts), *sys.path]):
            spec = importlib.util.spec_from_file_location(
                "review_run_generated", scripts / "run_generated.py"
            )
            self.runner = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(self.runner)
        self.program = self.root / "program"
        self.program.mkdir()
        (self.program / "simulation.json").write_text('{"image": "image.bin"}')

    def run_summary(self, difference):
        output = self.root / "output"

        def execute(command, ports, config, destination):
            destination.mkdir()
            (destination / "state.bits").write_text("0101\n")
            rows = [
                {
                    "cycle": n,
                    "reset": False,
                    "outputs": {"retire_valid": int(n == 1), "stage_if": 0},
                }
                for n in range(2)
            ]
            if destination.name.endswith("verilog"):
                if difference == "shift":
                    for row in rows:
                        row["cycle"] += 1
                elif difference == "output":
                    rows[0]["outputs"]["stage_if"] = 1
            (destination / "cycles.jsonl").write_text(
                "".join(json.dumps(row) + "\n" for row in rows)
            )

        arguments = [
            "run_generated.py",
            "--prefix",
            str(self.root / "runtime"),
            "--program",
            str(self.program),
            "--output",
            str(output),
        ]
        with (
            patch.object(sys, "argv", arguments),
            patch.object(self.runner, "invoke"),
            patch.object(self.runner, "execute", side_effect=execute),
            patch.object(
                self.runner,
                "compare",
                side_effect=lambda *args: {
                    "status": "PASS",
                    "retired": 1,
                    "model_cycles": 2,
                },
            ),
        ):
            self.runner.main()
        return json.loads((output / "verification.json").read_text())

    def test_timed_out_driver_is_reaped_and_streams_close(self):
        driver = self.runner.Driver(
            [sys.executable, "-c", "import time; time.sleep(120)"],
            self.root / "unresponsive-driver.log",
        )
        original_wait = driver.process.wait
        first_wait = True

        def bounded_wait(timeout=None):
            nonlocal first_wait
            if first_wait:
                first_wait = False
                return original_wait(timeout=0.02)
            return original_wait(timeout=5)

        def cleanup():
            if driver.process.poll() is None:
                driver.process.kill()
                original_wait(timeout=5)
            for stream in (driver.process.stdin, driver.process.stdout, driver.log):
                stream.close()

        self.addCleanup(cleanup)
        with patch.object(driver.process, "wait", side_effect=bounded_wait):
            with self.assertRaises((RuntimeError, subprocess.TimeoutExpired)):
                driver.close()
        self.assertIsNotNone(driver.process.poll())
        self.assertTrue(driver.process.stdin.closed)
        self.assertTrue(driver.process.stdout.closed)
        self.assertTrue(driver.log.closed)

    def test_matching_cycles_publish_two_comparisons(self):
        report = self.run_summary(None)
        self.assertEqual(report["status"], "PASS")
        self.assertEqual(report["native_rtl_cycle_comparisons"], 2)

    def test_equal_state_and_isa_with_shifted_cycles_is_rejected(self):
        with self.assertRaises((AssertionError, ValueError)):
            self.run_summary("shift")
        self.assertFalse((self.root / "output/verification.json").exists())

    def test_equal_cycle_counts_with_different_outputs_is_rejected(self):
        with self.assertRaises((AssertionError, ValueError)):
            self.run_summary("output")
        self.assertFalse((self.root / "output/verification.json").exists())


if __name__ == "__main__":
    unittest.main()
