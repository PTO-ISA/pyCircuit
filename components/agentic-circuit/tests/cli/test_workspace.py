from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from agentic_circuit._workspace import (
    UserInputError,
    discover_workspace,
    load_workspace,
)


FIXTURE = Path(__file__).parent / "fixtures" / "workspace" / "agentic-circuit.toml"


class WorkspaceTest(unittest.TestCase):
    def test_discovers_upward_and_normalizes_paths(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "agentic-circuit.toml").write_bytes(FIXTURE.read_bytes())
            nested = root / "sources" / "nested"
            nested.mkdir(parents=True)

            config = discover_workspace(nested)

            self.assertEqual(root.resolve(), config.root)
            self.assertEqual(root.resolve() / "architecture.py", config.architecture)
            self.assertEqual((root.resolve() / "components",), config.component_roots)
            self.assertEqual((("max_events", 1000),), config.default_run_inputs)

    def test_explicit_manifest_is_loaded_without_discovery(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            manifest = root / "project" / "agentic-circuit.toml"
            manifest.parent.mkdir()
            manifest.write_bytes(FIXTURE.read_bytes())

            config = load_workspace(manifest)

            self.assertEqual(manifest.parent.resolve(), config.root)

    def test_unknown_key_has_stable_diagnostic(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            manifest = Path(temporary) / "agentic-circuit.toml"
            manifest.write_text(FIXTURE.read_text() + "\nunknown = true\n")

            with self.assertRaises(UserInputError) as caught:
                load_workspace(manifest)

            self.assertEqual("ACPY-CONFIG-002", caught.exception.diagnostic.code)

    def test_paths_cannot_escape_the_workspace(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            manifest = Path(temporary) / "agentic-circuit.toml"
            contents = FIXTURE.read_text().replace(
                'architecture = "architecture.py"', 'architecture = "../outside.py"'
            )
            manifest.write_text(contents)

            with self.assertRaises(UserInputError) as caught:
                load_workspace(manifest)

            self.assertEqual("ACPY-CONFIG-003", caught.exception.diagnostic.code)

    def test_epoch_and_duplicate_ownership_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            manifest = Path(temporary) / "agentic-circuit.toml"
            contents = FIXTURE.read_text().replace(
                'contract_epoch = "0.4"', 'contract_epoch = "0.1"'
            )
            manifest.write_text(contents)
            with self.assertRaises(UserInputError):
                load_workspace(manifest)

            contents = FIXTURE.read_text().replace(
                'protocol_roots = ["protocols"]', 'protocol_roots = ["components"]'
            )
            manifest.write_text(contents)
            with self.assertRaises(UserInputError) as caught:
                load_workspace(manifest)
            self.assertEqual("ACPY-CONFIG-004", caught.exception.diagnostic.code)


if __name__ == "__main__":
    unittest.main()
