from __future__ import annotations

import hashlib
import json
import tempfile
import unittest
from pathlib import Path

from jsonschema import Draft202012Validator


REPOSITORY = Path(__file__).resolve().parents[4]
WORKSPACE = Path(__file__).resolve().parent / "fixtures" / "source"


class SourceCaptureTest(unittest.TestCase):
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
