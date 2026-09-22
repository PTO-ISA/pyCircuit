from __future__ import annotations

import builtins
import importlib
import unittest
from collections.abc import Callable
from dataclasses import FrozenInstanceError

CAPTURE_ONLY = {
    "scope",
    "map",
    "set",
    "view",
    "concat",
    "literal",
    "zero",
    "zext",
    "sext",
    "truncate",
    "wrap",
    "saturate",
    "checked",
    "refine",
    "static_assert",
    "insert",
    "matches",
    "source",
    "count_leading_zeros",
    "count_trailing_zeros",
    "popcount",
    "priority_encode",
    "onehot_encode",
    "onehot_enum",
    "match_enum",
    "memory",
    "sink",
    "observe",
    "expect",
    "compute",
    "pipeline",
    "route",
    "merge",
    "schedule",
    "engine",
    "reorder",
    "fork",
    "barrier",
    "table",
    "slot",
}

RUNTIME = {
    "system",
    "module",
    "module_decl",
    "struct",
    "rule",
    "invariant",
    "inline",
    "writer_priority",
    "array",
    "bits",
    "BitfieldSpec",
    "queue",
    "ResourceRef",
    "Queue",
    "address_space",
    "address_map",
    "Static",
    "Flow",
    "Endpoint",
    "config",
    "const",
    "param",
    "index_width",
    "index",
    "range",
    "count_width",
    "encoding",
    "case",
    "integer_range",
    "one_of",
    "static_bool",
    "static_config",
    "static_enum",
    "static_int",
    "static_parameter",
    "round_robin",
    "priority",
    *(f"u{width}" for width in range(1, 65)),
    "s8",
    "s16",
    "s32",
    "s64",
}

RESERVED = {
    "extern_module",
    "interface",
    "packet",
    "process",
    "protocol",
    "transaction",
}

# ``__all__`` is the wildcard surface derived from ``RUNTIME_API`` minus every
# name that would shadow a Python builtin.  ``range`` stays reachable through
# the explicit attribute ``ac.range``.
WILDCARD = RUNTIME - {"range"}


class ReadyValid:
    """Local schema marker used to form a public Flow annotation."""


def _runtime_evidence() -> dict[str, Callable[[], object]]:
    """Executable evidence that each runtime inventory name authors something.

    Only names with an executable example live here.  ``priority`` and
    ``round_robin`` are policy *tokens* rather than call targets: authored
    source writes their literal string value as ``policy="priority"`` /
    ``policy="round_robin"``, and the test below exercises that value.
    """

    api = importlib.import_module("agentic_circuit")
    lower_queue_source = importlib.import_module(
        "agentic_circuit._queue_frontend"
    ).lower_queue_source

    def author_model() -> None:
        lowered = lower_queue_source(
            """
import agentic_circuit as ac

@ac.struct
class Item:
    lane: ac.u8

@ac.rule
def keep(item):
    return item

@ac.system
def top() -> None:
    incoming = ac.source(Item, depth=2, latency=1)
    outgoing = keep(incoming)
    ac.sink(outgoing)
""",
            "top",
        )
        assert "ac.source" in lowered and "ac.sink" in lowered

    def declare_module() -> None:
        @api.module_decl(source="tests/producer.py")
        def producer() -> None:
            ...

        @api.module(declaration=producer)
        def producer_body() -> None:
            ...

    def declare_config() -> None:
        @api.config
        class Geometry:
            entries: api.static_int(width=8, signed=False)

        api.static_config(Geometry)

    def declare_enum() -> None:
        from enum import Enum

        class Opcode(Enum):
            NONE = 0

        @api.encoding(width=4)
        class Encoded(Enum):
            NONE = 0

        api.static_enum(Opcode)

    def declare_parameter() -> None:
        entries = api.param[int]("entries")
        assert api.index_width(entries) is not None
        assert api.count_width(entries) is not None
        api.static_parameter(
            "entries",
            api.static_int(width=8, signed=False),
            constraints=(api.one_of(1, 2), api.integer_range(1, 4)),
        )

    def declare_resources() -> None:
        api.queue("q", payload_type="Item", protocol="ready_valid", depth=2)
        space = api.address_space("mem", width=32)
        reference = api.ResourceRef(
            stable_name="bank", annotation=object(), role="master"
        )
        api.address_map(space, (0, 16, reference, 0))

    def declare_decorators() -> None:
        @api.rule
        def rule(item):
            return item

        @api.invariant
        def invariant(item):
            return True

        @api.inline
        def helper(item):
            return item

    evidence: dict[str, Callable[[], object]] = {
        "system": author_model,
        "struct": author_model,
        "rule": declare_decorators,
        "invariant": declare_decorators,
        "inline": declare_decorators,
        "module": declare_module,
        "module_decl": declare_module,
        "writer_priority": lambda: api.writer_priority(1),
        "array": lambda: api.array[4, api.u8],
        "bits": lambda: api.bits[5],
        "BitfieldSpec": lambda: api.BitfieldSpec(width=8, fields={"a": (3, 0)}),
        "queue": declare_resources,
        "ResourceRef": declare_resources,
        "address_space": declare_resources,
        "address_map": declare_resources,
        "Queue": lambda: api.Queue[api.u8, 1, 1],
        "Static": lambda: api.Static[int],
        "const": lambda: api.const[int],
        "Flow": lambda: api.Flow[int, ReadyValid],
        "Endpoint": lambda: api.Endpoint[ReadyValid, int],
        "config": declare_config,
        "static_config": declare_config,
        "encoding": declare_enum,
        "static_enum": declare_enum,
        "param": declare_parameter,
        "index_width": declare_parameter,
        "count_width": declare_parameter,
        "static_parameter": declare_parameter,
        "one_of": declare_parameter,
        "integer_range": declare_parameter,
        "index": lambda: api.index[5],
        "range": lambda: api.range[0, 5],
        "static_bool": lambda: api.static_bool(),
        "static_int": lambda: api.static_int(width=8, signed=False),
        "case": lambda: api.case(("x", 1)),
    }
    for width in range(1, 65):
        evidence[f"u{width}"] = (
            lambda width=width: getattr(api, f"u{width}")
        )
    for name in ("s8", "s16", "s32", "s64"):
        evidence[name] = (lambda name=name: getattr(api, name))
    return evidence


class PublicApiTest(unittest.TestCase):
    def test_exact_public_inventory_is_importable(self) -> None:
        api = importlib.import_module("agentic_circuit")

        self.assertEqual(RUNTIME, set(api.RUNTIME_API))
        self.assertEqual(WILDCARD, set(api.__all__))
        for name in RUNTIME | RESERVED:
            self.assertIsNotNone(getattr(api, name))

    def test_wildcard_surface_never_shadows_a_python_builtin(self) -> None:
        api = importlib.import_module("agentic_circuit")

        self.assertEqual(set(), set(api.__all__) & set(dir(builtins)))
        self.assertNotIn("range", api.__all__)
        self.assertIsNotNone(api.range[0, 5])

    def test_reserved_inventory_is_explicit_and_disjoint(self) -> None:
        api = importlib.import_module("agentic_circuit")

        self.assertEqual(RESERVED, set(api.RESERVED_API))
        self.assertTrue(RESERVED.isdisjoint(RUNTIME))
        self.assertTrue(RESERVED.isdisjoint(CAPTURE_ONLY))
        self.assertTrue(RESERVED.isdisjoint(api.__all__))

    def test_capture_only_inventory_has_a_dedicated_namespace(self) -> None:
        api = importlib.import_module("agentic_circuit")
        markers = importlib.import_module("agentic_circuit.markers")

        self.assertEqual(CAPTURE_ONLY, set(api.CAPTURE_ONLY_API))
        self.assertEqual(CAPTURE_ONLY, set(markers.__all__))
        self.assertTrue(CAPTURE_ONLY.isdisjoint(api.__all__))
        self.assertTrue(CAPTURE_ONLY.isdisjoint(api.RESERVED_API))
        for name in CAPTURE_ONLY:
            self.assertIs(getattr(markers, name), getattr(api, name))

    def test_runtime_inventory_contains_no_capture_only_stubs(self) -> None:
        api = importlib.import_module("agentic_circuit")

        marker_objects = {getattr(api.markers, name) for name in CAPTURE_ONLY}
        self.assertTrue(
            all(getattr(api, name) not in marker_objects for name in api.RUNTIME_API)
        )

    def test_runtime_inventory_names_have_executable_evidence(self) -> None:
        api = importlib.import_module("agentic_circuit")

        documented = {
            "priority": "policy token accepted as the literal policy=\"priority\"",
            "round_robin": "policy token accepted as the literal policy=\"round_robin\"",
        }
        evidence = _runtime_evidence()

        self.assertEqual(RUNTIME, set(evidence) | set(documented))
        self.assertTrue(set(evidence).isdisjoint(documented))
        self.assertTrue(set(documented).issubset(api.RUNTIME_API))
        for name, invoke in evidence.items():
            with self.subTest(name=name):
                invoke()
        lower_queue_source = importlib.import_module(
            "agentic_circuit._queue_frontend"
        ).lower_queue_source
        for name in documented:
            with self.subTest(policy=name):
                token = getattr(api, name)
                self.assertIsInstance(token, str)
                lowered = lower_queue_source(
                    f"""
import agentic_circuit as ac

@ac.struct
class Item:
    lane: ac.u8

@ac.system
def top() -> None:
    left = ac.source(Item)
    right = ac.source(Item)
    merged = left.merge(right, policy="{token}", depth=1, latency=1)
    ac.sink(merged)
""",
                    "top",
                )
                self.assertIn(f'policy "{token}"', lowered)

    def test_reserved_declarations_fail_fast_with_a_targeted_diagnostic(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
            parse_queue_program,
        )

        for name in sorted(RESERVED):
            with self.subTest(name=name):
                source = (
                    "import agentic_circuit as ac\n\n"
                    f"@ac.{name}\n"
                    "def reserved() -> None:\n"
                    "    ...\n"
                )
                for entry in (parse_queue_program, lower_queue_source):
                    with self.assertRaisesRegex(
                        QueueFrontendError, f"ACPY-API-001: ac.{name} is reserved"
                    ):
                        entry(source, "reserved")

    def test_instances_marker_is_removed_with_a_migration_diagnostic(self) -> None:
        api = importlib.import_module("agentic_circuit")
        markers = importlib.import_module("agentic_circuit.markers")
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
            parse_queue_program,
        )

        self.assertNotIn("instances", api.CAPTURE_ONLY_API)
        self.assertNotIn("instances", markers.__all__)
        self.assertFalse(hasattr(api, "instances"))
        self.assertFalse(hasattr(markers, "instances"))
        source = (
            "import agentic_circuit as ac\n\n"
            "@ac.system\n"
            "def top() -> None:\n"
            "    ac.instances()\n"
        )
        for entry in (parse_queue_program, lower_queue_source):
            with self.assertRaisesRegex(
                QueueFrontendError,
                r"ACPY-API-002: ac\.instances has no lowering.*ac\.list",
            ):
                entry(source, "top")

    def test_writer_priority_is_an_immutable_checked_compile_descriptor(self) -> None:
        api = importlib.import_module("agentic_circuit")

        policy = api.writer_priority(3)
        self.assertEqual(3, policy.rank)
        with self.assertRaises(FrozenInstanceError):
            policy.rank = 4
        for invalid in (-1, True, 1.0):
            with self.subTest(invalid=invalid):
                with self.assertRaisesRegex(ValueError, "non-negative static integer"):
                    api.writer_priority(invalid)

    def test_variable_has_no_long_form_public_alias(self) -> None:
        api = importlib.import_module("agentic_circuit")

        self.assertFalse(hasattr(api, "variable"))

    def test_unsigned_bit_types_cover_every_width_from_one_through_sixty_four(
        self,
    ) -> None:
        api = importlib.import_module("agentic_circuit")

        for width in range(1, 65):
            bit_type = getattr(api, f"u{width}")
            self.assertEqual(width, bit_type.width)
            self.assertFalse(bit_type.signed)

    def test_bits_factory_uses_the_same_static_width_contract(self) -> None:
        api = importlib.import_module("agentic_circuit")

        for width in (1, 3, 5, 17, 64):
            self.assertEqual(getattr(api, f"u{width}"), api.bits[width])
        for width in (0, 65):
            with self.assertRaisesRegex(ValueError, r"\[1, 64\]"):
                api.bits[width]

    def test_dependent_width_annotations_are_runtime_metadata(self) -> None:
        api = importlib.import_module("agentic_circuit")

        entries = api.param[int]("entries")
        index_type = api.bits[api.index_width(entries)]
        array_type = api.array[entries, api.u8]

        self.assertEqual("entries", entries.name)
        self.assertEqual(7, index_type.width.evaluate({"entries": 128}))
        self.assertEqual(128, array_type.length.evaluate({"entries": 128}))
        with self.assertRaisesRegex(TypeError, "only integer"):
            api.param[str]("name")

    def test_bounded_range_annotations_are_half_open_and_nominal(self) -> None:
        api = importlib.import_module("agentic_circuit")

        self.assertEqual(api.index[5], api.range[0, 5])
        self.assertEqual(3, api.index[5].width)
        self.assertEqual(4, api.range[4, 9].width)
        self.assertNotEqual(api.index[5], api.u3)
        self.assertNotEqual(api.index[5], api.index[6])
        self.assertEqual("!ac.range<0, 4>", api.index[5].mlir())
        full = api.range[0, 1 << 64]
        self.assertEqual(64, full.width)
        self.assertEqual("18446744073709551616", full.canonical()["upper"])
        self.assertEqual("!ac.range<0, 18446744073709551615>", full.mlir())
        entries = api.param[int]("entries")
        nested = api.array[2, api.index[entries]]
        self.assertEqual(2, nested.length)
        self.assertEqual(0, nested.element.lower)
        self.assertEqual(entries, nested.element.upper)
        for bounds in ((0, 0), (-1, 4), (4, 3), (0, (1 << 64) + 1)):
            with self.subTest(bounds=bounds):
                with self.assertRaisesRegex(ValueError, "range bounds"):
                    api.range[bounds]

    def test_nested_config_fields_are_dependent_integer_parameters(self) -> None:
        api = importlib.import_module("agentic_circuit")

        @api.config
        class Geometry:
            entries: api.static_int(width=64, signed=False)

        @api.config
        class Config:
            geometry: Geometry
            name: api.static_int(width=64, signed=False)
            value_type: api.static_int(width=64, signed=False)

        cfg = api.param[Config]("cfg")
        entries = cfg.geometry.entries
        index_type = api.bits[api.index_width(entries)]

        self.assertEqual("cfg.geometry.entries", entries.name)
        self.assertEqual(
            7,
            index_type.width.evaluate({"cfg.geometry.entries": 128}),
        )
        self.assertEqual("cfg.name", cfg.name.name)
        self.assertEqual("cfg.value_type", cfg.value_type.name)
        with self.assertRaisesRegex(AttributeError, "unknown config field"):
            _ = cfg.geometry.missing
        with self.assertRaisesRegex(TypeError, "integer config leaf"):
            api.index_width(cfg.geometry)

    def test_enum_encoding_decorator_preserves_the_standard_enum_class(self) -> None:
        from enum import Enum

        api = importlib.import_module("agentic_circuit")

        @api.encoding(width=4)
        class Opcode(Enum):
            NONE = 0
            READ = 3

        self.assertIsInstance(Opcode.READ, Opcode)
        self.assertEqual(4, Opcode.compiler_encoding_width__)
        with self.assertRaisesRegex(ValueError, r"\[1, 64\]"):
            api.encoding(width=0)

    def test_bitfield_spec_is_immutable_and_has_stable_layout_metadata(self) -> None:
        api = importlib.import_module("agentic_circuit")

        first = api.BitfieldSpec(
            width=32,
            fields={"opcode": (31, 26), "rd": (25, 21), "imm26": (25, 0)},
        )
        reordered = api.BitfieldSpec(
            width=32,
            fields={"imm26": (25, 0), "rd": (25, 21), "opcode": (31, 26)},
        )

        self.assertEqual(first.fields, reordered.fields)
        self.assertEqual((26, 6), first.field_slices()["opcode"])
        self.assertEqual(26, first.field_width("imm26"))
        with self.assertRaises(TypeError):
            first.fields["new"] = (1, 0)
        with self.assertRaises(FrozenInstanceError):
            first._layout = reordered._layout

    def test_bitfield_spec_rejects_invalid_layouts_and_wide_values(self) -> None:
        api = importlib.import_module("agentic_circuit")

        with self.assertRaisesRegex(ValueError, "out of range"):
            api.BitfieldSpec(width=8, fields={"bad": (8, 0)})
        with self.assertRaisesRegex(ValueError, r"\[1, 64\]"):
            api.BitfieldSpec(width=65, fields={"wide": (64, 0)})

    def test_array_annotation_uses_existing_pythonic_array_intrinsic(self) -> None:
        api = importlib.import_module("agentic_circuit")

        descriptor = api.array[4, api.bits[5]]
        self.assertEqual("!ac.value_array<4 x i5>", descriptor.mlir())
        self.assertEqual(20, descriptor.bit_width())
        with self.assertRaisesRegex(ValueError, "positive"):
            api.array[0, api.bits[5]]

    def test_scalar_type_rejects_out_of_range_widths(self) -> None:
        types = importlib.import_module("agentic_circuit._types")

        for width in (0, 65):
            with self.subTest(width=width):
                with self.assertRaisesRegex(ValueError, r"\[1, 64\]"):
                    types.ScalarType(width)

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

        @api.module_decl(source="tests/producer.py")
        def producer() -> None:
            ...

        producer_decl = producer

        @api.module(declaration=producer_decl)
        def producer() -> None:
            raise AssertionError("decorating a definition must not execute it")

        self.assertEqual("module", producer.kind)
        self.assertEqual(producer.function.__qualname__, producer.qualified_name)
        self.assertTrue(producer.qualified_name.endswith(".<locals>.producer"))
        self.assertEqual(
            (("declaration", producer_decl),), producer.explicit_options
        )
        with self.assertRaises(FrozenInstanceError):
            producer.kind = "system"

    def test_decorator_options_are_canonicalized(self) -> None:
        api = importlib.import_module("agentic_circuit")

        with self.assertRaisesRegex(TypeError, "unexpected keyword"):
            api.module(zeta=2, alpha=1)

    def test_generated_module_is_not_a_public_compatibility_alias(self) -> None:
        api = importlib.import_module("agentic_circuit")

        self.assertFalse(hasattr(api, "generated_module"))

    def test_rule_decorator_captures_without_executing(self) -> None:
        api = importlib.import_module("agentic_circuit")

        @api.rule
        def complete(item):
            raise AssertionError("decorating a rule must not execute it")

        self.assertEqual("rule", complete.kind)
        self.assertEqual("complete", complete.__name__)

    def test_invariant_decorator_captures_without_executing(self) -> None:
        api = importlib.import_module("agentic_circuit")

        @api.invariant
        def valid(item: object) -> bool:
            raise AssertionError("decorating an invariant must not execute it")

        self.assertEqual("invariant", valid.kind)
        self.assertEqual("valid", valid.__name__)

    def test_ast_only_markers_reject_runtime_execution(self) -> None:
        api = importlib.import_module("agentic_circuit")

        operations = (
            lambda: api.scope("nested"),
            lambda: api.array(1, 2),
            lambda: api.map({"a": object()}),
            lambda: api.set({object()}),
            lambda: api.view(object(), "field"),
            lambda: api.concat(object(), object()),
            lambda: api.insert(object(), object(), lsb=0),
            lambda: api.matches(object(), "1xx0"),
            lambda: api.source(int),
            lambda: api.count_leading_zeros(object()),
            lambda: api.count_trailing_zeros(object()),
            lambda: api.popcount(object()),
            lambda: api.priority_encode(object()),
            lambda: api.sink(object()),
            lambda: api.observe(object()),
            lambda: api.expect(
                object(), predicate=lambda value: True, message="expected"
            ),
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
                no_dependency=255,
            ),
            lambda: api.engine(object(), cost=object()),
            lambda: api.reorder(object(), by=object()),
            lambda: api.fork(object(), outputs=2),
            lambda: api.barrier(object(), object()),
        )
        for operation in operations:
            with self.subTest(operation=operation):
                with self.assertRaisesRegex(
                    NotImplementedError,
                    "capture-time only|AST intrinsic inside Agentic definitions",
                ):
                    operation()

    def test_marker_namespace_and_explicit_root_imports_are_captured(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        namespaced = lower_queue_source(
            """
import agentic_circuit as ac
import agentic_circuit.markers as markers

@ac.system
def pipeline() -> None:
    incoming = markers.source(int)
    markers.sink(incoming)
""",
            "pipeline",
        )
        explicit = lower_queue_source(
            """
from agentic_circuit import sink, source, system

@system
def pipeline() -> None:
    incoming = source(int)
    sink(incoming)
""",
            "pipeline",
        )

        for lowered in (namespaced, explicit):
            self.assertIn("ac.source depth 1 latency 1", lowered)
            self.assertIn("ac.sink", lowered)

    def test_table_factory_is_subscript_only_and_legacy_call_is_removed(self) -> None:
        import agentic_circuit as api

        with self.assertRaises(NotImplementedError):
            api.table[16, api.u16](init=0)
        with self.assertRaisesRegex(TypeError, "use ac.memory"):
            api.table(object(), address=object())


if __name__ == "__main__":
    unittest.main()
