from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from cli import cli_test_pythonpath


REPOSITORY = Path(__file__).resolve().parents[4]
COMPILE_FIXTURE = Path(__file__).parent / "fixtures" / "compile"
RUN_FIXTURE = Path(__file__).parent / "fixtures" / "run"


def run_cli(*arguments: str, cwd: Path) -> subprocess.CompletedProcess[str]:
    environment = os.environ.copy()
    environment["PYTHONHASHSEED"] = "0"
    environment["PYTHONPATH"] = cli_test_pythonpath(REPOSITORY, environment)
    return subprocess.run(
        [sys.executable, "-m", "agentic_circuit._cli", *arguments],
        cwd=cwd,
        env=environment,
        text=True,
        capture_output=True,
        check=False,
    )


def copy_fixture(source: Path, root: Path) -> None:
    shutil.copytree(source, root)


def file_bytes(root: Path) -> tuple[tuple[str, bytes], ...]:
    return tuple(
        (path.relative_to(root).as_posix(), path.read_bytes())
        for path in sorted(root.rglob("*"))
        if path.is_file()
    )


class CliDeterminismTest(unittest.TestCase):
    def test_compile_outputs_are_identical_across_workspace_roots(self) -> None:
        with (
            tempfile.TemporaryDirectory() as first,
            tempfile.TemporaryDirectory() as second,
        ):
            first_root = Path(first) / "project"
            second_root = Path(second) / "project"
            copy_fixture(COMPILE_FIXTURE, first_root)
            copy_fixture(COMPILE_FIXTURE, second_root)
            first_result = run_cli(
                "compile",
                "architecture.py",
                "--emit",
                "acpy,acir,acsim",
                "--output-dir",
                "out",
                cwd=first_root,
            )
            second_result = run_cli(
                "compile",
                "architecture.py",
                "--emit",
                "acpy,acir,acsim",
                "--output-dir",
                "out",
                cwd=second_root,
            )
            first_files = file_bytes(first_root / "out")
            second_files = file_bytes(second_root / "out")

        self.assertEqual(0, first_result.returncode, first_result.stderr)
        self.assertEqual(0, second_result.returncode, second_result.stderr)
        self.assertEqual(first_files, second_files)

    def test_build_and_run_manifests_are_root_independent(self) -> None:
        with (
            tempfile.TemporaryDirectory() as first,
            tempfile.TemporaryDirectory() as second,
        ):
            first_root = Path(first) / "project"
            second_root = Path(second) / "project"
            copy_fixture(RUN_FIXTURE, first_root)
            copy_fixture(RUN_FIXTURE, second_root)
            arguments = (
                "run",
                "capped_architecture.py",
                "--trace",
                "trace.json",
                "--max-ticks",
                "1",
                "--output-dir",
                "runs/one",
            )
            first_result = run_cli(*arguments, cwd=first_root)
            second_result = run_cli(*arguments, cwd=second_root)
            first_manifest = first_root.joinpath(
                "runs/one/run-manifest.json"
            ).read_bytes()
            second_manifest = second_root.joinpath(
                "runs/one/run-manifest.json"
            ).read_bytes()
            first_run_result = first_root.joinpath(
                "runs/one/run-result.json"
            ).read_bytes()
            second_run_result = second_root.joinpath(
                "runs/one/run-result.json"
            ).read_bytes()

        self.assertEqual(7, first_result.returncode, first_result.stderr)
        self.assertEqual(7, second_result.returncode, second_result.stderr)
        self.assertEqual(first_manifest, second_manifest)
        self.assertEqual(first_run_result, second_run_result)


if __name__ == "__main__":
    unittest.main()
