from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from cli import cli_test_pythonpath


REPOSITORY = Path(__file__).resolve().parents[4]
FIXTURE = Path(__file__).parent / "fixtures" / "frontend"


def run_cli(*arguments: str, cwd: Path) -> subprocess.CompletedProcess[str]:
    environment = os.environ.copy()
    environment["PYTHONPATH"] = cli_test_pythonpath(REPOSITORY, environment)
    return subprocess.run(
        [sys.executable, "-m", "agentic_circuit._cli", *arguments],
        cwd=cwd,
        env=environment,
        text=True,
        capture_output=True,
        check=False,
    )


def workspace(temporary: str) -> Path:
    root = Path(temporary) / "project"
    shutil.copytree(FIXTURE, root)
    return root


class FrontendCommandTest(unittest.TestCase):
    def test_check_is_fast_machine_readable_and_writes_no_build(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            result = run_cli(
                "check",
                "architecture.py",
                "--system",
                "main",
                "--json",
                cwd=root,
            )

            self.assertFalse(root.joinpath("build").exists())

        self.assertEqual(0, result.returncode, result.stderr)
        self.assertEqual("passed", json.loads(result.stdout)["status"])

    def test_check_can_stop_after_verified_acpy(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            result = run_cli(
                "check",
                "--stop-after",
                "acpy-verify",
                "--json",
                cwd=root,
            )

        self.assertEqual(0, result.returncode, result.stderr)
        self.assertEqual("acpy-verify", json.loads(result.stdout)["stage"])

    def test_elaborate_is_deterministic_and_captures_project_output(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            first = run_cli(
                "elaborate", "noisy.py", "--emit", "acpy", "-o", "a.json", cwd=root
            )
            second = run_cli(
                "elaborate", "noisy.py", "--emit", "acpy", "-o", "b.json", cwd=root
            )
            first_bytes = root.joinpath("a.json").read_bytes()
            second_bytes = root.joinpath("b.json").read_bytes()

        self.assertEqual(0, first.returncode, first.stderr)
        self.assertEqual(first_bytes, second_bytes)
        self.assertNotIn("project noise", first.stdout)
        self.assertNotIn("project noise", first.stderr)

    def test_elaborate_acir_is_verified_and_atomically_replaced(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            output = root / "model.ac.mlir"
            output.write_text("old\n")
            result = run_cli(
                "elaborate", "architecture.py", "-o", output.name, "--json", cwd=root
            )
            contents = output.read_text()

        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("ac.system @main", contents)
        self.assertEqual("sha256:", json.loads(result.stdout)["sha256"][:7])


if __name__ == "__main__":
    unittest.main()
