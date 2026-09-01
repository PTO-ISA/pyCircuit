from __future__ import annotations

import importlib
import unittest
from dataclasses import FrozenInstanceError


PUBLIC = {
    "system",
    "module",
    "extern_module",
    "generated_module",
    "struct",
    "packet",
    "transaction",
    "protocol",
    "interface",
    "process",
    "scope",
    "array",
    "map",
    "set",
    "instances",
    "view",
    "queue",
    "ResourceRef",
    "address_space",
    "address_map",
    "Static",
    "Flow",
    "Endpoint",
    "source",
    "popcount",
    "memory",
    "sink",
    "observe",
    "expect",
    "atomic",
    "compute",
    "pipeline",
    "config",
    "const",
    "jit",
    "route",
    "merge",
    "schedule",
    "engine",
    "reorder",
    "round_robin",
    "priority",
    "fork",
    "barrier",
    "table",
    "u1",
    "u2",
    "u4",
    "u8",
    "u16",
    "u32",
    "u64",
    "s8",
    "s16",
    "s32",
    "s64",
}


class ReadyValid:
    """Local schema marker used to form a public Flow annotation."""


class PublicApiTest(unittest.TestCase):
    def test_exact_public_inventory_is_importable(self) -> None:
        api = importlib.import_module("agentic_circuit")

        self.assertEqual(PUBLIC, set(api.__all__))
        for name in PUBLIC:
            self.assertIsNotNone(getattr(api, name))

    def test_symbolic_values_reject_python_coercion(self) -> None:
        types = importlib.import_module("agentic_circuit._types")
        value = types._test_symbolic("request", types.Flow[int, ReadyValid])

        for operation in (bool, int, hash, iter):
            with self.subTest(operation=operation.__name__):
                with self.assertRaisesRegex(TypeError, "ACPY-STATIC-002"):
                    operation(value)

    def test_symbolic_values_reject_python_equality(self) -> None:
        types = importlib.import_module("agentic_circuit._types")
        left = types._test_symbolic("left", object())
        right = types._test_symbolic("right", object())

        with self.assertRaisesRegex(TypeError, "ACPY-STATIC-002"):
            left == right

    def test_symbolic_value_repr_uses_only_stable_identity(self) -> None:
        types = importlib.import_module("agentic_circuit._types")

        value = types._test_symbolic("request", object())

        self.assertEqual("SymbolicValue('request')", repr(value))

    def test_decorators_create_immutable_definition_metadata(self) -> None:
        api = importlib.import_module("agentic_circuit")

        @api.module
        def producer() -> None:
            raise AssertionError("decorating a definition must not execute it")

        self.assertEqual("module", producer.kind)
        self.assertEqual(producer.function.__qualname__, producer.qualified_name)
        self.assertTrue(producer.qualified_name.endswith(".<locals>.producer"))
        self.assertEqual((), producer.explicit_options)
        with self.assertRaises(FrozenInstanceError):
            producer.kind = "system"

    def test_decorator_options_are_canonicalized(self) -> None:
        api = importlib.import_module("agentic_circuit")

        @api.generated_module(zeta=2, alpha=1)
        def generated() -> None:
            pass

        self.assertEqual((("alpha", 1), ("zeta", 2)), generated.explicit_options)

    def test_ast_only_markers_reject_runtime_execution(self) -> None:
        api = importlib.import_module("agentic_circuit")

        operations = (
            lambda: api.scope("nested"),
            lambda: api.array(1, 2),
            lambda: api.map({"a": object()}),
            lambda: api.set({object()}),
            lambda: api.instances(1, 2),
            lambda: api.view(object(), "field"),
            lambda: api.source(int),
            lambda: api.popcount(object()),
            lambda: api.sink(object()),
            lambda: api.observe(object()),
            lambda: api.expect(
                object(), predicate=lambda value: True, message="expected"
            ),
            lambda: api.atomic(),
            lambda: api.compute(object(), lambda value: value),
            lambda: api.pipeline(object(), stages=2),
            lambda: api.route(object(), by=object(), outputs=2),
            lambda: api.merge(object(), object()),
            lambda: api.schedule(
                object(),
                by=object(),
                waits_for=object(),
                resource=object(),
                cost=object(),
            ),
            lambda: api.engine(object(), cost=object()),
            lambda: api.reorder(object(), by=object()),
            lambda: api.fork(object(), outputs=2),
            lambda: api.barrier(object(), object()),
        )
        for operation in operations:
            with self.subTest(operation=operation):
                with self.assertRaises(NotImplementedError):
                    operation()

    def test_table_factory_is_subscript_only_and_legacy_call_is_removed(self) -> None:
        import agentic_circuit as api

        with self.assertRaises(NotImplementedError):
            api.table[16, api.u16](init=0)
        with self.assertRaisesRegex(TypeError, "use ac.memory"):
            api.table(object(), address=object())


if __name__ == "__main__":
    unittest.main()
