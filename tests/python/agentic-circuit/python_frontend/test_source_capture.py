from __future__ import annotations

import hashlib
import json
import tempfile
import unittest
from pathlib import Path
from types import ModuleType, SimpleNamespace
from unittest.mock import patch, sentinel

from jsonschema import Draft202012Validator

REPOSITORY = Path(__file__).resolve().parents[4]
WORKSPACE = Path(__file__).resolve().parent / "fixtures" / "source"


class SourceCaptureTest(unittest.TestCase):
    def test_frontend_kind_uses_registered_rule_definitions(self) -> None:
        from agentic_circuit import module, rule
        from agentic_circuit._capture_worker import _contains_registered_rule

        @module
        def structural() -> None:
            pass

        @rule
        def imported_rule(value):
            return value

        class Rules:
            nested = imported_rule

        helper = ModuleType("helper")
        helper.imported_rule = imported_rule

        self.assertFalse(_contains_registered_rule({"structural": structural}))
        self.assertTrue(_contains_registered_rule({"imported": imported_rule}))
        self.assertTrue(_contains_registered_rule({"Rules": Rules}))
        self.assertTrue(_contains_registered_rule({"helper": helper}))

    def test_queue_rule_capture_preserves_frontend_diagnostics(self) -> None:
        from agentic_circuit._capture_worker import _capture_queue_rule
        from agentic_circuit._diagnostics import Diagnostic

        diagnostic = Diagnostic(
            stage="queue-frontend",
            code="ACPY-VERIFY-001",
            severity="warning",
            message="captured rule warning",
        )
        program = SimpleNamespace(diagnostics=(diagnostic,))
        with (
            patch(
                "agentic_circuit._queue_frontend.parse_queue_program",
                return_value=program,
            ),
            patch(
                "agentic_circuit._queue_frontend.build_queue_acpy",
                return_value=sentinel.document,
            ),
            patch(
                "agentic_circuit._queue_frontend.lower_queue_program",
                return_value="module {}\n",
            ),
        ):
            document, acir, diagnostics = _capture_queue_rule(
                "@ac.system\ndef top(): ...\n",
                "top",
                "architecture.py",
                {},
            )

        self.assertIs(sentinel.document, document)
        self.assertEqual("module {}\n", acir)
        self.assertEqual((diagnostic,), diagnostics)

    def test_identity_is_workspace_relative_and_hashed(self) -> None:
        from agentic_circuit._source import load_source_unit

        entry = WORKSPACE / "basic.py"
        unit = load_source_unit(entry, WORKSPACE)

        self.assertEqual("basic.py", unit.path)
        self.assertEqual(
            "sha256:" + hashlib.sha256(entry.read_bytes()).hexdigest(), unit.sha256
        )
        self.assertEqual(
            ["Worker", "Architecture"],
            [site.qualified_name for site in unit.definitions],
        )
        self.assertEqual([0, 1], [site.lexical_index for site in unit.definitions])
        self.assertEqual(4, unit.definitions[0].span.start_line)

    def test_source_outside_workspace_is_rejected(self) -> None:
        from agentic_circuit._source import SourceCaptureError, load_source_unit

        with tempfile.TemporaryDirectory() as temporary:
            workspace = Path(temporary) / "workspace"
            workspace.mkdir()
            outside = Path(temporary) / "outside.py"
            outside.write_text("value = 1\n", encoding="utf-8")

            with self.assertRaisesRegex(SourceCaptureError, "outside workspace"):
                load_source_unit(outside, workspace)

    def test_non_utf8_source_is_rejected(self) -> None:
        from agentic_circuit._source import SourceCaptureError, load_source_unit

        with tempfile.TemporaryDirectory() as temporary:
            workspace = Path(temporary)
            entry = workspace / "invalid.py"
            entry.write_bytes(b"\xff\n")

            with self.assertRaisesRegex(SourceCaptureError, "not UTF-8"):
                load_source_unit(entry, workspace)

    def test_duplicate_qualified_definitions_are_rejected(self) -> None:
        from agentic_circuit._source import SourceCaptureError, load_source_unit

        with tempfile.TemporaryDirectory() as temporary:
            workspace = Path(temporary)
            entry = workspace / "duplicate.py"
            entry.write_text(
                "def repeated():\n    pass\n\ndef repeated():\n    pass\n",
                encoding="utf-8",
            )

            with self.assertRaisesRegex(SourceCaptureError, "duplicate definition"):
                load_source_unit(entry, workspace)

    def test_nested_definition_keeps_qualified_name_and_column(self) -> None:
        from agentic_circuit._source import load_source_unit

        with tempfile.TemporaryDirectory() as temporary:
            workspace = Path(temporary)
            entry = workspace / "nested.py"
            entry.write_text(
                "def outer():\n" "    @module\n" "    def inner():\n" "        pass\n",
                encoding="utf-8",
            )

            unit = load_source_unit(entry, workspace)

        inner = unit.definitions[1]
        self.assertEqual("outer.<locals>.inner", inner.qualified_name)
        self.assertEqual(2, inner.span.start_line)
        self.assertEqual(5, inner.span.start_column)


class DiagnosticTest(unittest.TestCase):
    def test_structured_exception_converts_without_string_parsing(self) -> None:
        from agentic_circuit._diagnostics import (
            AgenticCircuitError,
            DiagnosticError,
            SourceSpan,
            diagnostic_from_exception,
        )

        source = SourceSpan("basic.py", 9, 5, 9, 12)
        error = DiagnosticError("ACPY-QUEUE-001", "queue is invalid", source)

        diagnostic = diagnostic_from_exception(
            error,
            stage="queue-frontend",
            default_code="ACPY-VERIFY-001",
        )

        self.assertIsInstance(error, ValueError)
        self.assertIsInstance(error, AgenticCircuitError)
        self.assertEqual("ACPY-QUEUE-001: queue is invalid", str(error))
        self.assertEqual("ACPY-QUEUE-001", diagnostic.code)
        self.assertEqual("queue is invalid", diagnostic.message)
        self.assertEqual(source, diagnostic.source)

    def test_legacy_queue_frontend_exception_is_structured_at_boundary(self) -> None:
        from agentic_circuit._diagnostics import diagnostic_from_exception
        from agentic_circuit._queue_frontend import QueueFrontendError

        error = QueueFrontendError("ACPY-QUEUE-002: unsupported field type")

        diagnostic = diagnostic_from_exception(
            error,
            stage="queue-frontend",
            default_code="ACPY-VERIFY-001",
        )

        self.assertIsInstance(error, ValueError)
        self.assertEqual("ACPY-QUEUE-002", error.code)
        self.assertEqual("unsupported field type", error.message)
        self.assertIsNone(error.source)
        self.assertEqual("ACPY-QUEUE-002", diagnostic.code)
        self.assertEqual("unsupported field type", diagnostic.message)

    def test_unstructured_exception_cannot_smuggle_a_code_in_its_message(self) -> None:
        from agentic_circuit._diagnostics import diagnostic_from_exception

        diagnostic = diagnostic_from_exception(
            ValueError("ACPY-RULE-001: text is not diagnostic identity"),
            stage="queue-frontend",
            default_code="ACPY-VERIFY-001",
        )

        self.assertEqual("ACPY-VERIFY-001", diagnostic.code)
        self.assertEqual(
            "ACPY-RULE-001: text is not diagnostic identity", diagnostic.message
        )

    def test_jit_and_lowering_errors_are_structured_at_the_raise_site(self) -> None:
        from agentic_circuit._diagnostics import (
            AgenticCircuitError,
            DiagnosticRuntimeError,
            DiagnosticTypeError,
        )
        from agentic_circuit._jit import jit
        from agentic_circuit._lower_acir import _symbol

        with self.assertRaises(TypeError) as jit_raised:
            jit(object())
        with self.assertRaises(ValueError) as lowering_raised:
            _symbol("not a symbol")

        self.assertIsInstance(jit_raised.exception, AgenticCircuitError)
        self.assertEqual("ACPY-JIT-001", jit_raised.exception.code)
        self.assertFalse(issubclass(DiagnosticTypeError, ValueError))
        self.assertFalse(issubclass(DiagnosticRuntimeError, ValueError))
        self.assertIsInstance(lowering_raised.exception, AgenticCircuitError)
        self.assertEqual("ACPY-VERIFY-001", lowering_raised.exception.code)

    def test_diagnostic_contract_identity_cannot_be_overridden(self) -> None:
        from agentic_circuit._diagnostics import Diagnostic

        with self.assertRaisesRegex(ValueError, "schema identity"):
            Diagnostic(
                stage="source-capture",
                code="ACPY-SYNTAX-SOURCE",
                severity="error",
                message="source cannot be captured",
                schema="different-schema",
            )

    def test_diagnostics_sort_by_source_then_code(self) -> None:
        from agentic_circuit._diagnostics import (
            Diagnostic,
            DiagnosticBag,
            SourceSpan,
        )

        bag = DiagnosticBag()
        bag.add(
            Diagnostic(
                stage="type-check",
                code="ACPY-TYPE-002",
                severity="error",
                message="second",
                source=SourceSpan("basic.py", 8, 1, 8, 4),
            )
        )
        bag.add(
            Diagnostic(
                stage="type-check",
                code="ACPY-TYPE-001",
                severity="error",
                message="first",
                source=SourceSpan("basic.py", 3, 1, 3, 4),
            )
        )

        self.assertEqual(
            ["ACPY-TYPE-001", "ACPY-TYPE-002"],
            [item.code for item in bag.freeze()],
        )

    def test_diagnostic_json_matches_the_closed_public_schema(self) -> None:
        from agentic_circuit._diagnostics import (
            Diagnostic,
            FixIt,
            RelatedLocation,
            SourceSpan,
        )

        source = SourceSpan("basic.py", 9, 5, 9, 12)
        diagnostic = Diagnostic(
            stage="source-capture",
            code="ACPY-SYNTAX-SOURCE",
            severity="error",
            message="source cannot be captured",
            source=source,
            related=(RelatedLocation("definition", source, None),),
            fixits=(FixIt("Keep the definition in the workspace"),),
        )
        schema = json.loads(
            (
                REPOSITORY / "schemas/agentic-circuit" / "diagnostic.schema.json"
            ).read_text(encoding="utf-8")
        )

        value = diagnostic.to_json()
        Draft202012Validator(schema).validate(value)
        self.assertEqual({"file": "basic.py", "line": 9, "column": 5}, value["source"])
        self.assertIsNone(value["expected"])
        self.assertIsNone(value["actual"])
        self.assertEqual(
            {"file": "basic.py", "line": 9, "column": 5},
            value["related"][0]["source"],
        )
        self.assertIsNone(value["related"][0]["object_path"])
        self.assertEqual(
            [{"message": "Keep the definition in the workspace"}], value["fixits"]
        )


if __name__ == "__main__":
    unittest.main()
