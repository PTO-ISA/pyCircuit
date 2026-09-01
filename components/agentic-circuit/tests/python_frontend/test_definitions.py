from __future__ import annotations

import dataclasses
import importlib.util
import inspect
import json
import tempfile
import unittest
from pathlib import Path


REPOSITORY = Path(__file__).resolve().parents[2]
WORKSPACE = Path(__file__).resolve().parent / "fixtures" / "definitions"


def load_fixture(name: str):
    path = WORKSPACE / name
    module_name = f"agentic_circuit_test_{path.stem}"
    spec = importlib.util.spec_from_file_location(module_name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import test fixture {path}")
    loaded = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(loaded)
    return loaded


def stdlib_registry():
    from agentic_circuit._schemas import SchemaRegistry

    return SchemaRegistry.from_catalog(
        REPOSITORY / "schemas" / "stdlib" / "catalog.json", REPOSITORY
    )


class DefinitionCaptureTest(unittest.TestCase):
    def test_decorated_definition_binds_without_executing_its_body(self) -> None:
        loaded = load_fixture("basic.py")

        self.assertEqual("Definition(kind='system', qualified_name='main')", repr(loaded.main))
        self.assertEqual(["lanes"], list(inspect.signature(loaded.main).parameters))
        call = loaded.main(lanes=8)

        self.assertEqual("main", call.definition.qualified_name)
        self.assertEqual((('lanes', 8),), call.arguments)
        with self.assertRaisesRegex(TypeError, "ACPY-CALL-003"):
            loaded.main(surprise=8)

    def test_definition_matches_ast_and_system_selection(self) -> None:
        from agentic_circuit._frontend import CaptureRequest, capture_definitions

        loaded = load_fixture("basic.py")
        result = capture_definitions(
            CaptureRequest(
                entry=WORKSPACE / "basic.py",
                workspace=WORKSPACE,
                system="main",
            ),
            vars(loaded),
            stdlib_registry(),
        )

        self.assertEqual((), result.diagnostics)
        self.assertIsNotNone(result.selected_system)
        assert result.selected_system is not None
        self.assertEqual("main", result.selected_system.qualified_name)
        self.assertEqual(
            ["worker", "main"],
            [definition.qualified_name for definition in result.definitions],
        )
        self.assertEqual("basic.py", result.source.path)

    def test_source_line_mismatch_rejects_fabricated_definition(self) -> None:
        from agentic_circuit._frontend import CaptureRequest, capture_definitions

        loaded = load_fixture("basic.py")
        loaded.main = dataclasses.replace(loaded.main, source_line=999)
        result = capture_definitions(
            CaptureRequest(
                entry=WORKSPACE / "basic.py",
                workspace=WORKSPACE,
                system="main",
            ),
            vars(loaded),
            stdlib_registry(),
        )

        self.assertIsNone(result.selected_system)
        self.assertIn(
            "ACPY-SYMBOL-DEFINITION", [item.code for item in result.diagnostics]
        )

    def test_non_static_defaults_and_variadic_parameters_are_rejected(self) -> None:
        from agentic_circuit._frontend import CaptureRequest, capture_definitions

        loaded = load_fixture("invalid.py")
        result = capture_definitions(
            CaptureRequest(
                entry=WORKSPACE / "invalid.py",
                workspace=WORKSPACE,
                system="invalid_defaults",
            ),
            vars(loaded),
            stdlib_registry(),
        )

        codes = [item.code for item in result.diagnostics]
        self.assertIn("ACPY-STATIC-DEFAULT", codes)
        self.assertIn("ACPY-TYPE-SIGNATURE", codes)


class SchemaCallableTest(unittest.TestCase):
    def test_recomputed_fingerprint_cannot_hide_invalid_closed_field(self) -> None:
        from agentic_circuit._canonical_json import canonical_json_bytes, sha256_bytes
        from agentic_circuit._schemas import SchemaError, SchemaRegistry

        record = json.loads(
            (REPOSITORY / "schemas" / "stdlib" / "Queue.json").read_text()
        )
        record["effect"]["kind"] = "ambient"
        digest_record = dict(record)
        digest_record.pop("schema_fingerprint")
        record["schema_fingerprint"] = sha256_bytes(
            canonical_json_bytes(digest_record)
        )
        catalog = {
            "catalog": "ac",
            "version": "0.1",
            "contract_epoch": "0.4",
            "entries": [
                {
                    "canonical_name": "ac.Queue",
                    "availability": "available",
                    "schema_path": "schemas/stdlib/Queue.json",
                    "schema_fingerprint": record["schema_fingerprint"],
                }
            ],
        }
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            stdlib = root / "schemas" / "stdlib"
            stdlib.mkdir(parents=True)
            (stdlib / "Queue.json").write_text(json.dumps(record))
            (stdlib / "catalog.json").write_text(json.dumps(catalog))

            with self.assertRaisesRegex(SchemaError, "effect.kind"):
                SchemaRegistry.from_catalog(stdlib / "catalog.json", root)

    def test_queue_signature_is_derived_from_the_closed_schema(self) -> None:
        queue = stdlib_registry().callable("ac.Queue")

        signature = inspect.signature(queue)

        self.assertEqual("ComponentCallable('ac.Queue')", repr(queue))
        self.assertEqual(
            ["input", "output", "T", "capacity", "byteCapacity", "name"],
            list(signature.parameters),
        )
        self.assertEqual(
            [
                inspect.Parameter.POSITIONAL_OR_KEYWORD,
                inspect.Parameter.POSITIONAL_OR_KEYWORD,
                inspect.Parameter.KEYWORD_ONLY,
                inspect.Parameter.KEYWORD_ONLY,
                inspect.Parameter.KEYWORD_ONLY,
                inspect.Parameter.KEYWORD_ONLY,
            ],
            [parameter.kind for parameter in signature.parameters.values()],
        )

    def test_callable_rejects_undocumented_keyword(self) -> None:
        from agentic_circuit._types import _test_symbolic

        queue = stdlib_registry().callable("ac.Queue")
        input_value = _test_symbolic("input", object())
        output_value = _test_symbolic("output", object())

        with self.assertRaisesRegex(TypeError, "ACPY-CALL-003"):
            queue(
                input_value,
                output_value,
                T="Packet",
                capacity=4,
                surprise=True,
            )

    def test_valid_call_records_schema_defaults_in_signature_order(self) -> None:
        from agentic_circuit._types import _test_symbolic

        queue = stdlib_registry().callable("ac.Queue")
        call = queue(
            _test_symbolic("input", object()),
            _test_symbolic("output", object()),
            T="Packet",
            capacity=4,
        )

        self.assertEqual("ac.Queue", call.schema.identity)
        self.assertEqual(
            ["input", "output", "T", "capacity", "byteCapacity", "name"],
            [name for name, _ in call.arguments],
        )
        self.assertEqual("unbounded", dict(call.arguments)["byteCapacity"])
        self.assertIsNone(dict(call.arguments)["name"])

    def test_declared_unavailable_component_has_no_callable(self) -> None:
        registry = stdlib_registry()

        with self.assertRaisesRegex(LookupError, "declared unavailable"):
            registry.callable("ac.Arbiter")


if __name__ == "__main__":
    unittest.main()
