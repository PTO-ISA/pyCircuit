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
FIXTURE = Path(__file__).parent / "fixtures" / "compile"


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

    def test_missing_cpp_compiler_is_four(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "project"
            shutil.copytree(FIXTURE, root)
            manifest = root / "agentic-circuit.toml"
            manifest.write_text(
                manifest.read_text().replace(
                    'compiler = "c++"', 'compiler = "missing-agentic-cxx"'
                )
            )
            result = run_cli(
                "build",
                "architecture.py",
                "--output-dir",
                "build/model",
                "--json",
                cwd=root,
            )

        self.assertEqual(4, result.returncode, result.stderr)
        self.assertEqual("ACBUILD-COMPILER-001", json.loads(result.stdout)["code"])



if __name__ == "__main__":
    unittest.main()
