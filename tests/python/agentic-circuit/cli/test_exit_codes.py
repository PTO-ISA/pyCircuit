from __future__ import annotations

import json
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from agentic_circuit._canonical_json import sha256_bytes
from agentic_circuit._commands.build import BuildPublication
from agentic_circuit._run import RunOptions, create_run_manifest
from cli import cli_test_pythonpath


REPOSITORY = Path(__file__).resolve().parents[4]
FIXTURE = Path(__file__).parent / "fixtures" / "compile"
TRACE = Path(__file__).parent / "fixtures" / "run" / "trace.json"


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

    def test_runtime_interrupt_is_130_and_publishes_nothing(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            build = root / "build"
            executable = build / "bin/model"
            executable.parent.mkdir(parents=True)
            executable.write_text(
                "#!/usr/bin/env python3\n"
                "import os,signal\n"
                "os.kill(os.getpid(), signal.SIGINT)\n"
            )
            executable.chmod(0o755)
            build_manifest = build / "build-manifest.json"
            build_manifest.write_text(
                json.dumps(
                    {
                        "artifacts": [
                            {
                                "path": "bin/model",
                                "kind": "executable",
                                "sha256": sha256_bytes(executable.read_bytes()),
                            }
                        ]
                    },
                    sort_keys=True,
                    separators=(",", ":"),
                )
                + "\n"
            )
            trace = root / "trace.json"
            trace.write_bytes(TRACE.read_bytes())
            publication = BuildPublication(
                build, executable, build_manifest, "sha256:" + "0" * 64, False
            )
            options = RunOptions(
                trace, root / "unused", 0, None, None, (), "json", "disabled", "any"
            )
            bundle = root / "bundle"
            bundle.joinpath("bin").mkdir(parents=True)
            shutil.copy2(executable, bundle / "bin/model")
            shutil.copy2(build_manifest, bundle / "build-manifest.json")
            shutil.copy2(trace, bundle / "trace.json")
            manifest = bundle / "run-manifest.json"
            manifest.write_bytes(create_run_manifest(publication, options))
            output = root / "replay"
            result = run_cli(
                "run",
                "--replay-manifest",
                str(manifest),
                "--output-dir",
                str(output),
                "--json",
                cwd=root,
            )
            published = output.exists()

        self.assertEqual(128 + signal.SIGINT, result.returncode, result.stderr)
        self.assertEqual("ACRUN-INTERRUPTED-001", json.loads(result.stdout)["code"])
        self.assertFalse(published)


if __name__ == "__main__":
    unittest.main()
