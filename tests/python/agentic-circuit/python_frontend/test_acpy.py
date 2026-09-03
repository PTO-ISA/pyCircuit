from __future__ import annotations

import json
import unittest
from pathlib import Path

from jsonschema import Draft202012Validator


REPOSITORY = Path(__file__).resolve().parents[4]
FIXTURES = Path(__file__).resolve().parent / "fixtures"
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
        expected = (FIXTURES / "acpy" / "minimal.acpy.json").read_bytes().rstrip(b"\n")

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


if __name__ == "__main__":
    unittest.main()
