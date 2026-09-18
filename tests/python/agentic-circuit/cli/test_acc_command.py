from __future__ import annotations

import io
import json
import tempfile
import tomllib
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from agentic_circuit import _acc_py

REPOSITORY = Path(__file__).resolve().parents[4]
PYPROJECT = REPOSITORY / "python/agentic-circuit/pyproject.toml"


def frontend(acir: bytes = b"module {}\n") -> SimpleNamespace:
    return SimpleNamespace(acir=acir, diagnostics=())


def native(frozen: bytes = b"module { ac.system @core }\n") -> SimpleNamespace:
    artifact = SimpleNamespace(path="verified.ac.mlir", data=frozen)
    return SimpleNamespace(artifacts=(artifact,), diagnostics=())


class AccCommandTest(unittest.TestCase):
    def test_console_script_is_installed_under_exact_name(self) -> None:
        metadata = tomllib.loads(PYPROJECT.read_text(encoding="utf-8"))
        self.assertEqual(
            "agentic_circuit._acc_py:main", metadata["project"]["scripts"]["acc.py"]
        )

    def test_compiles_frozen_acir_and_atomically_publishes_only_ac_file(self) -> None:
        frozen = b"module { ac.system @core }\n"
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            architecture = root / "architecture.py"
            architecture.write_text("# fixture\n", encoding="utf-8")
            project = root / "agentic-circuit.toml"
            project.write_text("# fixture\n", encoding="utf-8")
            output = root / "artifacts/model.ac"
            workspace = SimpleNamespace(component_roots=())
            with (
                patch.object(_acc_py, "load_workspace", return_value=workspace),
                patch.object(_acc_py, "capture", return_value=frontend()),
                patch.object(
                    _acc_py, "run_native_compiler", return_value=native(frozen)
                ),
            ):
                result = _acc_py.main(
                    [
                        "-c",
                        str(architecture),
                        "-o",
                        str(output),
                        "--project",
                        str(project),
                        "--system",
                        "core",
                    ]
                )

            self.assertEqual(0, result)
            self.assertEqual(frozen, output.read_bytes())
            self.assertEqual(
                ("model.ac",), tuple(path.name for path in output.parent.iterdir())
            )

    def test_failure_does_not_replace_existing_output(self) -> None:
        diagnostic = SimpleNamespace(
            code="ACPY-TEST", message="failed", severity="error"
        )
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            architecture = root / "architecture.py"
            architecture.write_text("# fixture\n", encoding="utf-8")
            output = root / "model.ac"
            output.write_bytes(b"existing")
            workspace = SimpleNamespace(component_roots=())
            with (
                patch.object(_acc_py, "discover_workspace", return_value=workspace),
                patch.object(
                    _acc_py,
                    "capture",
                    return_value=SimpleNamespace(acir=None, diagnostics=(diagnostic,)),
                ),
            ):
                result = _acc_py.main(["-c", str(architecture), "-o", str(output)])
            self.assertEqual(2, result)
            self.assertEqual(b"existing", output.read_bytes())

    def test_missing_frozen_artifact_does_not_publish(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            architecture = root / "architecture.py"
            architecture.write_text("# fixture\n", encoding="utf-8")
            output = root / "model.ac"
            stderr = io.StringIO()
            workspace = SimpleNamespace(component_roots=())
            with (
                patch.object(_acc_py, "discover_workspace", return_value=workspace),
                patch.object(_acc_py, "capture", return_value=frontend()),
                patch.object(
                    _acc_py,
                    "run_native_compiler",
                    return_value=SimpleNamespace(artifacts=(), diagnostics=()),
                ),
                redirect_stderr(stderr),
            ):
                result = _acc_py.main(["-c", str(architecture), "-o", str(output)])
            self.assertEqual(3, result)
            self.assertFalse(output.exists())
            self.assertIn("produced no verified.ac.mlir", stderr.getvalue())

    def test_output_symlink_cannot_redirect_published_ac(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            architecture = root / "architecture.py"
            architecture.write_text("# fixture\n", encoding="utf-8")
            outside = root / "outside.ac"
            outside.write_bytes(b"existing")
            output = root / "model.ac"
            output.symlink_to(outside)
            workspace = SimpleNamespace(component_roots=())
            with (
                patch.object(_acc_py, "discover_workspace", return_value=workspace),
                patch.object(_acc_py, "capture", return_value=frontend()),
                patch.object(_acc_py, "run_native_compiler", return_value=native()),
            ):
                result = _acc_py.main(["-c", str(architecture), "-o", str(output)])
            self.assertEqual(2, result)
            self.assertEqual(b"existing", outside.read_bytes())

    def test_json_reports_published_artifact(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            architecture = root / "architecture.py"
            architecture.write_text("# fixture\n", encoding="utf-8")
            output = root / "model.ac"
            stdout = io.StringIO()
            workspace = SimpleNamespace(component_roots=())
            with (
                patch.object(_acc_py, "discover_workspace", return_value=workspace),
                patch.object(_acc_py, "capture", return_value=frontend()),
                patch.object(_acc_py, "run_native_compiler", return_value=native()),
                redirect_stdout(stdout),
            ):
                result = _acc_py.main(
                    ["-c", str(architecture), "-o", str(output), "--json"]
                )
            payload = json.loads(stdout.getvalue())
        self.assertEqual(0, result)
        self.assertEqual(output.resolve().as_posix(), payload["output"])


if __name__ == "__main__":
    unittest.main()
