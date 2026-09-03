from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path


WORKSPACE = Path(__file__).resolve().parent / "fixtures" / "scopes"
ZERO_DIGEST = "sha256:" + "0" * 64


def schema(identity: str, ports, results, *, static=()):
    from agentic_circuit._schemas import (
        ComponentSchema,
        ParameterSchema,
        PortSchema,
        ResultSchema,
    )

    return ComponentSchema(
        identity=identity,
        fingerprint=ZERO_DIGEST,
        ports=tuple(
            PortSchema(name, kind, type_key, "in", role, 1)
            for name, kind, type_key, role in ports
        ),
        results=tuple(ResultSchema(name, type_key, None) for name, type_key in results),
        parameters=tuple(
            ParameterSchema(name, "index", True, None, None) for name in static
        ),
        availability="available",
    )


def normalize_scope_fixture(name: str = "nested.py"):
    from agentic_circuit._frontend import CaptureRequest, capture_definitions
    from agentic_circuit._normalize import normalize_program
    from agentic_circuit._schemas import SchemaRegistry

    path = WORKSPACE / name
    spec = importlib.util.spec_from_file_location(f"scope_{path.stem}", path)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot import scope fixture")
    loaded = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(loaded)
    if name == "invalid_escape.py":
        make_private = schema("test.MakePrivate", (), (("private", "!test.resource"),))
        registry = SchemaRegistry({make_private.identity: make_private})
    else:
        schedule = schema(
            "test.Schedule",
            (("input", "flow", "!test.flow", "consumer"),),
            (("scheduled", "!test.flow"),),
        )
        store = schema(
            "test.Store",
            (
                ("input", "flow", "!test.flow", "consumer"),
                ("memory", "endpoint", "!test.endpoint", "target"),
            ),
            (("stored", "!test.flow"),),
        )
        registry = SchemaRegistry({schedule.identity: schedule, store.identity: store})
    captured = capture_definitions(
        CaptureRequest(entry=path, workspace=WORKSPACE, system="main"),
        vars(loaded),
        registry,
    )
    if captured.diagnostics:
        raise AssertionError(captured.diagnostics)
    return normalize_program(captured, definition="pipeline")


def resolved_call(key: str, component, *, lanes: int):
    from agentic_circuit._diagnostics import SourceSpan
    from agentic_circuit._resolve import ResolvedCall

    return ResolvedCall(
        entity_key=key,
        schema=component,
        instance_name=key,
        static_arguments=(("lanes", lanes),),
        inputs=(),
        results=(),
        source=SourceSpan("nested.py", 1, 1, 1, 2),
    )


class ScopeCollectionTest(unittest.TestCase):
    def test_rectangular_homogeneous_collection_selects_array(self) -> None:
        from agentic_circuit._collections import classify_collection

        compute = schema("test.Compute", (), (), static=("lanes",))
        elements = tuple(
            tuple(
                resolved_call(f"worker_{row}_{column}", compute, lanes=2)
                for column in range(4)
            )
            for row in range(2)
        )

        collection = classify_collection(elements)

        self.assertEqual("array", collection.kind)
        self.assertEqual((2, 4), collection.shape)
        self.assertEqual("test.Compute", collection.element_schema)
        self.assertEqual(
            tuple(f"worker_{row}_{column}" for row in range(2) for column in range(4)),
            collection.elements,
        )

    def test_specialization_difference_selects_instances(self) -> None:
        from agentic_circuit._collections import classify_collection

        compute = schema("test.Compute", (), (), static=("lanes",))
        collection = classify_collection(
            (
                resolved_call("worker_0", compute, lanes=2),
                resolved_call("worker_1", compute, lanes=4),
            )
        )

        self.assertEqual("instances", collection.kind)
        self.assertIsNone(collection.element_schema)

    def test_ragged_collection_is_rejected(self) -> None:
        from agentic_circuit._collections import CollectionError, classify_collection

        compute = schema("test.Compute", (), (), static=("lanes",))
        element = resolved_call("worker", compute, lanes=2)

        with self.assertRaisesRegex(CollectionError, "ragged collection"):
            classify_collection(((element,), (element, element)))

    def test_scope_signature_is_minimal_and_ordered(self) -> None:
        from agentic_circuit._scopes import outline_scopes

        program = normalize_scope_fixture()
        self.assertEqual((), program.diagnostics)
        scopes = outline_scopes(program)
        backend = next(scope for scope in scopes if scope.name == "backend")
        memory = next(scope for scope in scopes if scope.name == "memory")

        self.assertEqual(
            ("requests", "memory"),
            tuple(binding.value.source_name for binding in backend.captures),
        )
        self.assertEqual(
            ("stored",), tuple(binding.value.source_name for binding in backend.escapes)
        )
        self.assertEqual(
            ("scheduled", "memory"),
            tuple(binding.value.source_name for binding in memory.captures),
        )
        self.assertEqual(
            ("stored",), tuple(binding.value.source_name for binding in memory.escapes)
        )

    def test_owned_resource_cannot_escape_its_scope(self) -> None:
        from agentic_circuit._scopes import ScopeError, outline_scopes

        program = normalize_scope_fixture("invalid_escape.py")
        self.assertEqual((), program.diagnostics)

        with self.assertRaisesRegex(ScopeError, "ACPY-SCOPE-004"):
            outline_scopes(program)


if __name__ == "__main__":
    unittest.main()
