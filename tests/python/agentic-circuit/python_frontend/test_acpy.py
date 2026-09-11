from __future__ import annotations

import json
import subprocess
import tempfile
import unittest
from pathlib import Path

from jsonschema import Draft202012Validator

REPOSITORY = Path(__file__).resolve().parents[4]
FIXTURES = Path(__file__).resolve().parent / "fixtures"
GOLDENS = REPOSITORY / "tests" / "goldens" / "agentic-circuit" / "frontend"
ZERO_DIGEST = "sha256:" + "0" * 64


def minimal_document():
    from agentic_circuit._acpy import AcpyDocument, Entity, SourceFile
    from agentic_circuit._diagnostics import SourceSpan

    return AcpyDocument(
        entry="e0",
        sources=(SourceFile("architecture.py", ZERO_DIGEST),),
        entities=(
            Entity(
                id="e0",
                kind="system",
                source=SourceSpan("architecture.py", 1, 1, 2, 1),
                parent=None,
                scope="Architecture",
                type=None,
                definition=None,
                uses=(),
                schema_ref=None,
                properties=(),
            ),
        ),
    )


class AcpyContractTest(unittest.TestCase):
    def test_minimal_document_matches_golden_and_schema(self) -> None:
        expected = (GOLDENS / "minimal.acpy.json").read_bytes().rstrip(b"\n")

        actual = minimal_document().canonical_bytes()

        self.assertEqual(expected, actual)
        schema = json.loads(
            (REPOSITORY / "schemas/agentic-circuit" / "acpy.schema.json").read_text(
                encoding="utf-8"
            )
        )
        Draft202012Validator(schema).validate(json.loads(actual))

    def test_ids_are_dense_and_references_resolve(self) -> None:
        from agentic_circuit._acpy import AcpyDocument, EntityAllocator, SourceFile

        allocator = EntityAllocator()
        root = allocator.allocate(kind="system", scope="Architecture")
        call = allocator.allocate(kind="call", scope="Architecture", parent=root.id)
        allocator.allocate(
            kind="result", scope="Architecture", parent=call.id, uses=(call.id,)
        )
        document = AcpyDocument(
            entry=root.id,
            sources=(SourceFile("architecture.py", ZERO_DIGEST),),
            entities=allocator.freeze(),
        )

        self.assertEqual(
            [f"e{index}" for index in range(len(document.entities))],
            [entity.id for entity in document.entities],
        )
        self.assertEqual((), document.verify())

    def test_rfc_8785_vector_and_utf16_key_order(self) -> None:
        from agentic_circuit._canonical_json import canonical_json_bytes

        value = {
            "numbers": [333333333.33333329, 1e30, 4.50, 2e-3, 1e-27],
            "string": '€$\u000f\nA\'B"\\"/',
            "literals": [None, True, False],
        }
        expected = (
            '{"literals":[null,true,false],'
            '"numbers":[333333333.3333333,1e+30,4.5,0.002,1e-27],'
            '"string":"€$\\u000f\\nA\'B\\"\\\\\\"/"}'
        ).encode("utf-8")
        self.assertEqual(expected, canonical_json_bytes(value))
        self.assertEqual(
            '{"€":"euro","😀":"emoji","דּ":"hebrew"}'.encode(),
            canonical_json_bytes({"דּ": "hebrew", "😀": "emoji", "€": "euro"}),
        )

    def test_mlir_strings_use_canonical_byte_escaping_after_ijson_validation(self) -> None:
        from agentic_circuit._canonical_json import (
            canonical_json_bytes,
            canonical_mlir_string,
        )

        value = 'quote" slash\\ backspace\b formfeed\f return\r newline\n tab\t euro€'
        self.assertEqual(
            '"quote\\" slash\\\\ backspace\\b formfeed\\f return\\r '
            'newline\\n tab\\t euro€"'.encode(),
            canonical_json_bytes(value),
        )
        self.assertEqual(
            '"quote\\22 slash\\\\ backspace\\08 formfeed\\0C return\\0D '
            'newline\\0A tab\\09 euro\\E2\\82\\AC"',
            canonical_mlir_string(value),
        )
        with self.assertRaisesRegex(ValueError, "Unicode scalar"):
            canonical_mlir_string("\ud800")

    def test_canonical_mlir_string_round_trips_through_native_parser(self) -> None:
        from agentic_circuit._canonical_json import canonical_mlir_string
        from agentic_circuit._contract import CONTRACT_EPOCH

        acir_opt = REPOSITORY / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"
        if not acir_opt.is_file():
            self.skipTest("acir-opt is not built")
        value = 'quote" slash\\ backspace\b formfeed\f return\r euro€ emoji😀'
        source = (
            "module attributes {ac.contract_epoch = "
            + canonical_mlir_string(CONTRACT_EPOCH)
            + ", ac.test = "
            + canonical_mlir_string(value)
            + "} {}\n"
        )
        with tempfile.TemporaryDirectory() as temporary:
            input_path = Path(temporary) / "input.mlir"
            output_path = Path(temporary) / "output.mlir"
            input_path.write_text(source, encoding="utf-8")
            completed = subprocess.run(
                [str(acir_opt), str(input_path), "-o", str(output_path)],
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, completed.returncode, completed.stderr)
            rendered = output_path.read_text(encoding="utf-8")
        for escape in (r"\22", r"\\", r"\08", r"\0C", r"\0D", r"\E2\82\AC"):
            self.assertIn(escape, rendered)


if __name__ == "__main__":
    unittest.main()
