from __future__ import annotations

import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


REPOSITORY = Path(__file__).resolve().parents[4]
BUILD = REPOSITORY / ".pycircuit_out" / "acir" / "dev-llvm22"
FIXTURE = Path(__file__).parent / "fixtures" / "inspect"


def install_to(prefix: Path) -> subprocess.CompletedProcess[str]:
    toolchain_root = os.environ.get("AC_GATE_TOOLCHAIN_ROOT")
    if toolchain_root:
        shutil.copytree(Path(toolchain_root), prefix, symlinks=True)
        return subprocess.CompletedProcess(
            args=("copytree", toolchain_root, os.fspath(prefix)),
            returncode=0,
            stdout="",
            stderr="",
        )
    return subprocess.run(
        ["cmake", "--install", str(BUILD), "--prefix", str(prefix)],
        cwd=REPOSITORY,
        text=True,
        capture_output=True,
        check=False,
    )


def run_installed(
    prefix: Path, *arguments: str, cwd: Path
) -> subprocess.CompletedProcess[str]:
    environment = os.environ.copy()
    environment.pop("PYTHONPATH", None)
    return subprocess.run(
        [str(prefix / "bin/agentic-circuit"), *arguments],
        cwd=cwd,
        env=environment,
        text=True,
        capture_output=True,
        check=False,
    )


class InstallationTest(unittest.TestCase):
    def test_installed_prefix_runs_without_source_checkout(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            prefix = root / "prefix"
            project = root / "project"
            unrelated = root / "outside"
            shutil.copytree(FIXTURE, project)
            unrelated.mkdir()
            installed = install_to(prefix)
            self.assertEqual(0, installed.returncode, installed.stderr)
            installed_bins = {path.name for path in (prefix / "bin").iterdir()}

            doctor = run_installed(prefix, "doctor", "--json", cwd=unrelated)
            capabilities = run_installed(
                prefix, "schema", "capabilities", "--json", cwd=unrelated
            )
            opcode = run_installed(
                prefix,
                "schema",
                "opcode",
                "ac.reorder",
                "--json",
                cwd=unrelated,
            )
            checked = run_installed(
                prefix,
                "check",
                "architecture.py",
                "--project",
                str(project / "agentic-circuit.toml"),
                "--json",
                cwd=unrelated,
            )
            initialized = run_installed(
                prefix, "init", str(root / "initialized"), "--json", cwd=unrelated
            )
            built = run_installed(
                prefix,
                "build",
                "architecture.py",
                "--project",
                str(project / "agentic-circuit.toml"),
                "--output-dir",
                str(project / "build/main"),
                "--json",
                cwd=unrelated,
            )

        self.assertEqual(0, doctor.returncode, doctor.stderr)
        self.assertEqual(0, capabilities.returncode, capabilities.stderr)
        self.assertEqual(
            "agentic-circuit-capabilities",
            json.loads(capabilities.stdout)["schema"],
        )
        self.assertEqual(0, opcode.returncode, opcode.stderr)
        self.assertEqual("ac.reorder", json.loads(opcode.stdout)["operation"])
        self.assertEqual(0, checked.returncode, checked.stderr)
        self.assertEqual("passed", json.loads(checked.stdout)["status"])
        self.assertEqual(0, initialized.returncode, initialized.stderr)
        self.assertEqual(0, built.returncode, built.stderr)
        self.assertNotIn("import-davincioo-pto-trace.py", installed_bins)
        self.assertNotIn("pack-perfetto-trace.py", installed_bins)


if __name__ == "__main__":
    unittest.main()
