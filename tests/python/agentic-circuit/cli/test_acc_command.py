from __future__ import annotations

import io
import json
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

import tomllib
from agentic_circuit import _acc_py

REPOSITORY = Path(__file__).resolve().parents[4]
PYPROJECT = REPOSITORY / "python/agentic-circuit/pyproject.toml"


def frontend(acir: bytes = b"module {}\n") -> SimpleNamespace:
    return SimpleNamespace(acir=acir, diagnostics=())


def native(core: bytes = b"module { ac.system @core }\n") -> SimpleNamespace:
    artifacts = (
        SimpleNamespace(path="core.ac", kind="verified-acir", data=core),
        SimpleNamespace(
            path="interfaces/pkg/types.ac",
            kind="verified-acir",
            data=b'module attributes {ac.unit_kind = "interface"} {}\n',
        ),
        SimpleNamespace(
            path="sources/bank.ac",
            kind="verified-acir",
            data=(
                b"module {\n"
                b"  ac.module @bank__index_0() parameters {index = 0 : i64} "
                b"graph { ac.return }\n"
                b"  ac.module @bank__index_1() parameters {index = 1 : i64} "
                b"graph { ac.return }\n"
                b"}\n"
            ),
        ),
        SimpleNamespace(
            path="interfaces/modules/bank.ac",
            kind="verified-acir",
            data=(
                b"module { ac.module.import @bank__index_0 : () -> () "
                b"parameters {index = 0 : i64} from {source = \"bank.py\"} }\n"
            ),
        ),
    )
    return SimpleNamespace(artifacts=artifacts, diagnostics=())


class AccCommandTest(unittest.TestCase):
    def test_console_script_is_installed_under_exact_name(self) -> None:
        metadata = tomllib.loads(PYPROJECT.read_text(encoding="utf-8"))
        self.assertEqual(
            "agentic_circuit._acc_py:main", metadata["project"]["scripts"]["acc.py"]
        )

    def test_compiles_and_atomically_publishes_core_ac_unit(self) -> None:
        frozen = b"module { ac.system @core }\n"
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            architecture = root / "architecture.py"
            architecture.write_text("# fixture\n", encoding="utf-8")
            project = root / "agentic-circuit.toml"
            project.write_text("# fixture\n", encoding="utf-8")
            output = root / "artifacts/model.ac"
            workspace = SimpleNamespace(component_roots=(), root=root)
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
            code="ACPY-TEST", message="failed", severity="error", source=None
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
            self.assertIn("produced no unique 'core.ac' AC unit", stderr.getvalue())

    def test_interface_mode_publishes_interface_source_tree(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            architecture = root / "architecture.py"
            architecture.write_text("# fixture\n", encoding="utf-8")
            output = root / "interfaces"
            workspace = SimpleNamespace(component_roots=())
            with (
                patch.object(_acc_py, "discover_workspace", return_value=workspace),
                patch.object(_acc_py, "capture", return_value=frontend()),
                patch.object(_acc_py, "run_native_compiler", return_value=native()),
            ):
                result = _acc_py.main(
                    [
                        "-c",
                        str(architecture),
                        "--unit",
                        "interfaces",
                        "-o",
                        str(output),
                    ]
            )
            self.assertEqual(0, result)
            self.assertEqual(
                b'module attributes {ac.unit_kind = "interface"} {}\n',
                (output / "pkg/types.ac").read_bytes(),
            )

    def test_interface_mode_publishes_empty_tree_for_zero_type_closure(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            architecture = root / "architecture.py"
            architecture.write_text("# fixture\n", encoding="utf-8")
            output = root / "interfaces"
            workspace = SimpleNamespace(component_roots=())
            empty_native = SimpleNamespace(
                artifacts=(
                    SimpleNamespace(
                        path="core.ac",
                        kind="verified-acir",
                        data=b"module { ac.system @core }\n",
                    ),
                ),
                diagnostics=(),
            )
            with (
                patch.object(_acc_py, "discover_workspace", return_value=workspace),
                patch.object(_acc_py, "capture", return_value=frontend()),
                patch.object(
                    _acc_py, "run_native_compiler", return_value=empty_native
                ),
            ):
                result = _acc_py.main(
                    [
                        "-c",
                        str(architecture),
                        "--unit",
                        "interfaces",
                        "-o",
                        str(output),
                    ]
                )
            self.assertEqual(0, result)
            self.assertTrue(output.is_dir())
            self.assertEqual((), tuple(output.iterdir()))

    def test_legacy_unit_names_are_rejected(self) -> None:
        for legacy in ("root", "shared"):
            with self.subTest(unit=legacy), self.assertRaises(SystemExit):
                _acc_py.main(
                    [
                        "-c",
                        "architecture.py",
                        "--unit",
                        legacy,
                        "-o",
                        "model.ac",
                    ]
                )

    def test_source_unit_groups_definitions_and_specializations(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            architecture = root / "bank.py"
            architecture.write_text(
                """import agentic_circuit as ac

@ac.module
def bank(value: ac.u8, *, index: ac.const[int]) -> ac.u8:
    return value
""",
                encoding="utf-8",
            )
            source_unit = root / "source-unit.json"
            source_unit.write_text(
                json.dumps(
                    {
                        "schema": "agentic-circuit-specializations",
                        "version": "0.1",
                        "specializations": [
                            {"module": "bank", "static": {"index": 0}},
                            {"module": "bank", "static": {"index": 1}},
                        ],
                    }
                ),
                encoding="utf-8",
            )
            output = root / "bank.ac"
            header = root / "interfaces" / "modules" / "bank.ac"
            workspace = SimpleNamespace(component_roots=(), root=root)

            def captured(arguments, _workspace):
                self.assertIsNone(arguments.module)
                self.assertEqual(
                    ("bank", "bank"),
                    tuple(name for name, _ in arguments.source_specializations),
                )
                return frontend()

            with (
                patch.object(_acc_py, "discover_workspace", return_value=workspace),
                patch.object(_acc_py, "capture", side_effect=captured),
                patch.object(
                    _acc_py,
                    "run_native_compiler",
                    return_value=native(),
                ),
            ):
                result = _acc_py.main(
                    [
                        "-c",
                        str(architecture),
                        "--specializations-json",
                        str(source_unit),
                        "--header-output",
                        str(header),
                        "-o",
                        str(output),
                    ]
                )
            self.assertEqual(0, result)
            rendered = output.read_bytes()
            self.assertEqual(1, rendered.count(b"ac.module @bank__index_0"))
            self.assertEqual(1, rendered.count(b"ac.module @bank__index_1"))
            self.assertIn(b"ac.module.import @bank__index_0", header.read_bytes())

    def test_definition_level_module_mode_is_removed(self) -> None:
        with self.assertRaises(SystemExit):
            _acc_py.main(
                [
                    "-c",
                    "stage.py",
                    "--module",
                    "stage",
                    "-o",
                    "stage.ac",
                ]
            )

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

    def test_static_json_reaches_capture_as_canonical_arguments(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            architecture = root / "architecture.py"
            architecture.write_text("# fixture\n", encoding="utf-8")
            bindings = root / "bindings.json"
            bindings.write_text(
                '{"width":8,"cfg":{"lanes":4,"entries":16}}\n',
                encoding="utf-8",
            )
            output = root / "model.ac"
            workspace = SimpleNamespace(component_roots=())

            def captured(arguments, _workspace):
                self.assertTrue(arguments.jit_source_closure)
                self.assertEqual(
                    (
                        ("cfg", {"entries": 16, "lanes": 4}),
                        ("width", 8),
                    ),
                    arguments.static_arguments,
                )
                return frontend()

            with (
                patch.object(_acc_py, "discover_workspace", return_value=workspace),
                patch.object(_acc_py, "capture", side_effect=captured),
                patch.object(_acc_py, "run_native_compiler", return_value=native()),
            ):
                result = _acc_py.main(
                    [
                        "-c",
                        str(architecture),
                        "-o",
                        str(output),
                        "--static-json",
                        str(bindings),
                    ]
                )
        self.assertEqual(0, result)

    def test_static_json_requires_a_closed_object(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            architecture = root / "architecture.py"
            architecture.write_text("# fixture\n", encoding="utf-8")
            bindings = root / "bindings.json"
            bindings.write_text("[]\n", encoding="utf-8")
            with self.assertRaises(SystemExit):
                _acc_py.main(
                    [
                        "-c",
                        str(architecture),
                        "-o",
                        str(root / "model.ac"),
                        "--static-json",
                        str(bindings),
                    ]
                )

    def test_timeout_must_be_positive(self) -> None:
        with self.assertRaises(SystemExit):
            _acc_py.main(
                [
                    "-c",
                    "architecture.py",
                    "-o",
                    "model.ac",
                    "--timeout",
                    "0",
                ]
            )


if __name__ == "__main__":
    unittest.main()
