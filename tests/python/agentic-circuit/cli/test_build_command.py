from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from jsonschema.validators import Draft202012Validator
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


def workspace(temporary: str) -> Path:
    root = Path(temporary) / "project"
    shutil.copytree(FIXTURE, root)
    return root


def current_manifest(output: Path) -> dict[str, object] | None:
    pointer_path = output / "current.json"
    if not pointer_path.is_file():
        return None
    pointer = json.loads(pointer_path.read_text())
    manifest = output.joinpath(pointer["path"], "build-manifest.json")
    return json.loads(manifest.read_text())


class BuildCommandTest(unittest.TestCase):
    def test_manifest_records_frontend_and_exact_profile(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            output = root / "build/model"
            result = run_cli(
                "build",
                "architecture.py",
                "--profile=validated",
                "-o",
                str(output),
                "--json",
                cwd=root,
            )
            manifest = current_manifest(output)

        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIsNotNone(manifest)
        assert manifest is not None
        schema = json.loads(
            (
                REPOSITORY / "schemas/agentic-circuit" / "build-manifest.schema.json"
            ).read_text()
        )
        Draft202012Validator(schema).validate(manifest)
        self.assertEqual("agentic-circuit-build-manifest", manifest["schema"])
        self.assertEqual("validated", manifest["build_profile"])
        self.assertIn(
            "architecture.py",
            {item["path"] for item in manifest["source_files"]},
        )
        self.assertIn(
            "input/model.acpy.json",
            {item["path"] for item in manifest["artifacts"]},
        )

    def test_identical_build_reports_cache_hit(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            first = run_cli(
                "build",
                "architecture.py",
                "-o",
                "build/model",
                "--json",
                cwd=root,
            )
            second = run_cli(
                "build",
                "architecture.py",
                "-o",
                "build/model",
                "--json",
                cwd=root,
            )

        self.assertEqual(0, first.returncode, first.stderr)
        self.assertEqual(0, second.returncode, second.stderr)
        self.assertFalse(json.loads(first.stdout)["cache_hit"])
        self.assertTrue(json.loads(second.stdout)["cache_hit"])
        self.assertEqual(
            json.loads(first.stdout)["build_fingerprint"],
            json.loads(second.stdout)["build_fingerprint"],
        )

    def test_custom_profile_requires_pipeline_before_output(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            output = root / "build/custom"
            result = run_cli(
                "build",
                "architecture.py",
                "--profile=custom",
                "-o",
                str(output),
                cwd=root,
            )

        self.assertEqual(2, result.returncode, result.stderr)
        self.assertIn("ACPY-CLI-PIPELINE", result.stderr)
        self.assertFalse(output.exists())

    def test_custom_pipeline_is_recorded_verbatim(self) -> None:
        pipeline = "builtin.module(ac-canonicalize-model)"
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            output = root / "build/custom"
            result = run_cli(
                "build",
                "architecture.py",
                "--profile=custom",
                "--pass-pipeline",
                pipeline,
                "-o",
                str(output),
                cwd=root,
            )
            manifest = current_manifest(output)

        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIsNotNone(manifest)
        assert manifest is not None
        self.assertIn(pipeline, manifest["pass_pipeline"])


if __name__ == "__main__":
    unittest.main()
