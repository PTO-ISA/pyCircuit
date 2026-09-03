from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from agentic_circuit._cli import EXACT_COMMANDS, build_parser, command_names
from agentic_circuit._staging import ArtifactStage
from cli import cli_test_pythonpath


FIXTURE = Path(__file__).parent / "fixtures" / "workspace" / "agentic-circuit.toml"
REPOSITORY = Path(__file__).resolve().parents[4]


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


class CliParserTest(unittest.TestCase):
    def test_exact_command_inventory(self) -> None:
        self.assertEqual(EXACT_COMMANDS, command_names(build_parser()))

    def test_json_stdout_contains_one_value_and_no_prose(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            result = run_cli("init", "--dry-run", "--json", cwd=Path(temporary))

        self.assertEqual(0, result.returncode)
        self.assertIsInstance(json.loads(result.stdout), dict)
        self.assertEqual("", result.stderr)

    def test_unknown_toml_key_is_exit_two(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            root.joinpath("agentic-circuit.toml").write_text(
                FIXTURE.read_text() + "\nunknown = true\n"
            )
            result = run_cli("check", "architecture.py", "--json", cwd=root)

        self.assertEqual(2, result.returncode)
        self.assertEqual("ACPY-CONFIG-002", json.loads(result.stdout)["code"])
        self.assertEqual("", result.stderr)

    def test_explicit_project_bypasses_current_directory_discovery(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            project = base / "project"
            unrelated = base / "unrelated"
            project.mkdir()
            unrelated.mkdir()
            manifest = project / "agentic-circuit.toml"
            manifest.write_bytes(FIXTURE.read_bytes())
            project.joinpath("architecture.py").write_text(
                "from agentic_circuit import module, system\n"
                "@module\n"
                "def top() -> None:\n"
                "    return\n"
                "@system(root='top')\n"
                "def main() -> None:\n"
                "    return\n"
            )
            result = run_cli(
                "check",
                "architecture.py",
                "--project",
                str(manifest),
                "--json",
                cwd=unrelated,
            )

        self.assertEqual(0, result.returncode, result.stderr)
        self.assertEqual("fixture", json.loads(result.stdout)["project"])

    def test_init_refuses_conflicts_unless_each_is_forced(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            root.joinpath("architecture.py").write_text("preserve me\n")
            refused = run_cli("init", "--json", cwd=root)
            preserved = root.joinpath("architecture.py").read_text()
            forced = run_cli("init", "--force", "architecture.py", "--json", cwd=root)

            self.assertEqual(2, refused.returncode)
            self.assertEqual("preserve me\n", preserved)
            self.assertEqual(0, forced.returncode)
            self.assertTrue(root.joinpath("agentic-circuit.toml").is_file())
            self.assertNotEqual(
                "preserve me\n", root.joinpath("architecture.py").read_text()
            )

    def test_stage_failure_preserves_published_files(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            destination = Path(temporary) / "output"
            destination.mkdir()
            destination.joinpath("old.txt").write_text("old\n")

            with self.assertRaises(ValueError):
                with ArtifactStage(destination, expected=("new.txt",)) as stage:
                    stage.write_text("unexpected.txt", "new\n")
                    stage.commit()

            self.assertEqual("old\n", destination.joinpath("old.txt").read_text())
            self.assertFalse(destination.joinpath("new.txt").exists())

    def test_aliases_and_repeated_singleton_options_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            root.joinpath("agentic-circuit.toml").write_bytes(FIXTURE.read_bytes())
            alias = run_cli("chk", cwd=root)
            repeated = run_cli(
                "check", "--system", "first", "--system", "second", cwd=root
            )

        self.assertEqual(2, alias.returncode)
        self.assertEqual(2, repeated.returncode)


if __name__ == "__main__":
    unittest.main()
