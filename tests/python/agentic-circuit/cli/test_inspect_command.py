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
FIXTURE = Path(__file__).parent / "fixtures" / "inspect"
KINDS = (
    "graph",
    "hierarchy",
    "ports",
    "resources",
    "address-map",
    "protocols",
    "specialization",
)


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


class InspectCommandTest(unittest.TestCase):
    def test_every_exact_view_is_machine_readable_and_read_only(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            documents: dict[str, dict[str, object]] = {}
            for kind in KINDS:
                result = run_cli("inspect", kind, "--json", cwd=root)
                self.assertEqual(0, result.returncode, (kind, result.stderr))
                documents[kind] = json.loads(result.stdout)
            formatted = run_cli("inspect", "graph", "--format", "json", cwd=root)
            build_exists = root.joinpath("build").exists()

        self.assertFalse(build_exists)
        self.assertEqual("graph", json.loads(formatted.stdout)["kind"])
        for kind, document in documents.items():
            self.assertEqual("agentic-circuit-inspection", document["schema"])
            self.assertEqual(kind, document["kind"])
            self.assertEqual("main", document["system"])
            self.assertIsInstance(document["records"], list)

    def test_hierarchy_path_is_canonical_and_unknown_path_is_diagnostic(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            found = run_cli("inspect", "ports", "--path", "chip", "--json", cwd=root)
            missing = run_cli(
                "inspect", "ports", "--path", "missing", "--json", cwd=root
            )

        self.assertEqual(0, found.returncode, found.stderr)
        self.assertEqual("chip", json.loads(found.stdout)["path"])
        self.assertEqual(2, missing.returncode)
        self.assertEqual("ACPY-INSPECT-001", json.loads(missing.stdout)["code"])

    def test_graphviz_output_is_deterministic_and_host_independent(self) -> None:
        with (
            tempfile.TemporaryDirectory() as first,
            tempfile.TemporaryDirectory() as second,
        ):
            first_root = workspace(first)
            second_root = workspace(second)
            first_dot = run_cli("inspect", "graph", "--format", "dot", cwd=first_root)
            second_dot = run_cli("inspect", "graph", "--format", "dot", cwd=second_root)

        self.assertEqual(0, first_dot.returncode, first_dot.stderr)
        self.assertEqual(first_dot.stdout, second_dot.stdout)
        self.assertTrue(first_dot.stdout.startswith("digraph agentic_circuit {\n"))
        self.assertNotIn(str(first_root), first_dot.stdout)
        self.assertNotIn(str(second_root), first_dot.stdout)


if __name__ == "__main__":
    unittest.main()
