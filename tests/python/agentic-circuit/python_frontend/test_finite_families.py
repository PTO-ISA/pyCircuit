from __future__ import annotations

import dataclasses
import unittest
from enum import Enum

import agentic_circuit as ac


class Mode(Enum):
    FAST = 0
    SAFE = 1


@ac.config
class Nested:
    lanes: ac.static_int(width=8, signed=False)


class FiniteFamilyValuesTest(unittest.TestCase):
    def test_constructs_closed_parameter_types_constraints_and_cases(self) -> None:
        declarations = (
            ac.static_parameter("enabled", ac.static_bool()),
            ac.static_parameter(
                "lanes",
                ac.static_int(width=4, signed=False),
                default=2,
                constraints=(ac.one_of(2, 4), ac.integer_range(1, 8)),
            ),
            ac.static_parameter("mode", ac.static_enum(Mode)),
            ac.static_parameter("nested", ac.static_config(Nested)),
        )
        family_case = ac.case(
            ("enabled", True),
            ("lanes", 4),
            ("mode", Mode.FAST),
            ("nested", Nested(lanes=4)),
        )

        self.assertTrue(declarations[0].required)
        self.assertFalse(declarations[1].required)
        self.assertTrue(declarations[1].accepts(4))
        self.assertEqual(
            tuple(name for name, _ in family_case.bindings),
            ("enabled", "lanes", "mode", "nested"),
        )
        self.assertTrue(dataclasses.is_dataclass(Nested))

    def test_rejects_invalid_closed_values(self) -> None:
        with self.assertRaisesRegex(ValueError, "width must be positive"):
            ac.static_int(width=0, signed=False)
        with self.assertRaisesRegex(ValueError, "one_of values must be unique"):
            ac.one_of(1, 1)
        with self.assertRaisesRegex(ValueError, "bounds are inverted"):
            ac.integer_range(3, 2)
        with self.assertRaisesRegex(ValueError, "out of range"):
            ac.static_parameter(
                "lanes", ac.static_int(width=2, signed=False), default=4
            )
        with self.assertRaisesRegex(ValueError, "binding names must be unique"):
            ac.case(("lanes", 1), ("lanes", 2))
        with self.assertRaisesRegex(TypeError, "payload, lanes, and rate"):
            ac.Queue[ac.u8, 1]
        with self.assertRaisesRegex(ValueError, "rate must not exceed lanes"):
            ac.Queue[ac.u8, 1, 2]

    def test_config_is_an_exact_closed_typed_record(self) -> None:
        with self.assertRaisesRegex(TypeError, "requires ac.static_int"):

            @ac.config
            class BareInteger:
                value: int

        with self.assertRaisesRegex(TypeError, "annotation-only fields"):

            @ac.config
            class Defaulted:
                value: ac.static_int(width=4, signed=False) = 1

        with self.assertRaisesRegex(TypeError, "annotation-only fields"):

            @ac.config
            class Behavioral:
                value: ac.static_int(width=4, signed=False)

                def method(self) -> int:
                    return 0

        class Base:
            pass

        with self.assertRaisesRegex(TypeError, "inheritance"):

            @ac.config
            class Derived(Base):
                value: ac.static_int(width=4, signed=False)

        with self.assertRaisesRegex(ValueError, "out of range"):
            Nested(lanes=256)

    def test_family_ast_rejects_open_config_classes(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        template = """
import agentic_circuit as ac

CONFIG

@ac.module_decl(
    source="stage.py",
    parameters=(ac.static_parameter("cfg", ac.static_config(Config)),),
    finite_cases=(ac.case(("cfg", Config(value=1))),),
)
def stage(value: ac.Queue[ac.u8, 1, 1]) -> ac.Queue[ac.u8, 1, 1]:
    ...

@ac.system
def core(value: ac.u8) -> ac.u8:
    return stage(value, static=ac.case(("cfg", Config(value=1))))
"""
        invalid_configs = (
            ("integers require", "@ac.config\nclass Config:\n    value: int"),
            (
                "defaults",
                "@ac.config\nclass Config:\n"
                "    value: ac.static_int(width=4, signed=False) = 1",
            ),
            (
                "annotation-only",
                "@ac.config\nclass Config:\n"
                "    value: ac.static_int(width=4, signed=False)\n"
                "    def method(self):\n        return 0",
            ),
        )
        for message, declaration in invalid_configs:
            with self.subTest(message=message):
                with self.assertRaisesRegex(QueueFrontendError, message):
                    lower_queue_source(
                        template.replace("CONFIG", declaration),
                        "core",
                        source_path="stage.py",
                    )

    def test_rejects_incompatible_constraint_and_default(self) -> None:
        with self.assertRaisesRegex(TypeError, "requires a static integer"):
            ac.static_parameter(
                "enabled",
                ac.static_bool(),
                constraints=(ac.integer_range(0, 1),),
            )
        with self.assertRaisesRegex(ValueError, "violates its constraints"):
            ac.static_parameter(
                "lanes",
                ac.static_int(width=4, signed=False),
                default=3,
                constraints=(ac.one_of(2, 4),),
            )

    def test_module_declaration_canonicalizes_defaults_and_unused_cases(self) -> None:
        parameters = (
            ac.static_parameter("enabled", ac.static_bool()),
            ac.static_parameter(
                "lanes", ac.static_int(width=4, signed=False), default=2
            ),
        )

        @ac.module_decl(
            source="families/stage.py",
            parameters=parameters,
            finite_cases=(
                ac.case(("enabled", True)),
                ac.case(("enabled", False), ("lanes", 4)),
            ),
        )
        def stage_decl(
            value: ac.Queue[ac.u8, 1, 1],
        ) -> ac.Queue[ac.u8, 1, 1]: ...

        @ac.module(declaration=stage_decl)
        def stage(
            value: ac.Queue[ac.u8, 1, 1],
        ) -> ac.Queue[ac.u8, 1, 1]:
            return value

        options = dict(stage_decl.explicit_options)
        self.assertEqual(
            options["finite_cases"],
            (
                ac.case(("enabled", True), ("lanes", 2)),
                ac.case(("enabled", False), ("lanes", 4)),
            ),
        )
        self.assertIs(dict(stage.explicit_options)["declaration"], stage_decl)

    def test_zero_parameter_declaration_has_one_empty_case(self) -> None:
        @ac.module_decl(source="families/identity.py")
        def identity_decl(value: ac.u8) -> ac.u8: ...

        self.assertEqual(
            dict(identity_decl.explicit_options)["finite_cases"], (ac.case(),)
        )

    def test_rejects_open_or_malformed_family_cases(self) -> None:
        parameter = ac.static_parameter("lanes", ac.static_int(width=4, signed=False))
        with self.assertRaisesRegex(ValueError, "requires non-empty finite_cases"):
            ac.module_decl(source="families/open.py", parameters=(parameter,))
        with self.assertRaisesRegex(ValueError, "missing required binding"):
            ac.module_decl(
                source="families/missing.py",
                parameters=(parameter,),
                finite_cases=(ac.case(),),
            )
        with self.assertRaisesRegex(ValueError, "unknown binding"):
            ac.module_decl(
                source="families/unknown.py",
                parameters=(parameter,),
                finite_cases=(ac.case(("other", 1)),),
            )
        with self.assertRaisesRegex(ValueError, "finite cases must be unique"):
            ac.module_decl(
                source="families/duplicate.py",
                parameters=(parameter,),
                finite_cases=(
                    ac.case(("lanes", 1)),
                    ac.case(("lanes", 1)),
                ),
            )

    def test_literal_family_lowers_typed_schema_cases_and_instance_arguments(
        self,
    ) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = """
import agentic_circuit as ac

@ac.module_decl(
    source="families/stage.py",
    parameters=(
        ac.static_parameter("enabled", ac.static_bool()),
        ac.static_parameter(
            "lanes",
            ac.static_int(width=4, signed=False),
            default=2,
            constraints=(ac.one_of(2, 4),),
        ),
    ),
    finite_cases=(
        ac.case(("enabled", True)),
        ac.case(("enabled", False), ("lanes", 4)),
    ),
)
def stage(value: ac.Queue[ac.u8, 1, 1]) -> ac.Queue[ac.u8, 1, 1]:
    ...

@ac.system
def core(value: ac.u8) -> ac.u8:
    return stage(
        value,
        static=ac.case(("enabled", True), ("lanes", 2)),
    )
"""
        lowered = lower_queue_source(source, "core", source_path="core.py")
        self.assertIn('#ac.static_parameter<"enabled"', lowered)
        self.assertIn('#ac.static_parameter<"lanes"', lowered)
        self.assertEqual(lowered.count("#ac.static_arguments<["), 5)
        self.assertIn("#ac.static_bool_value<true>", lowered)
        self.assertIn('#ac.interface_port<"value", "input"', lowered)
        self.assertIn('#ac.interface_port<"result", "output"', lowered)
        self.assertIn("#ac.type_expr_queue<", lowered)
        self.assertNotIn("stage__", lowered)

    def test_family_queue_preserves_dependent_lanes_and_authored_rate(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = """
import agentic_circuit as ac

@ac.module_decl(
    source="stage.py",
    parameters=(
        ac.static_parameter("lanes", ac.static_int(width=4, signed=False)),
    ),
    finite_cases=(
        ac.case(("lanes", 2)),
        ac.case(("lanes", 4)),
    ),
)
def stage(
    value: ac.Queue[ac.u8, lanes, 2],
) -> ac.Queue[ac.u8, lanes, 2]:
    ...

stage_decl = stage

@ac.module(declaration=stage_decl)
def stage(
    value: ac.Queue[ac.u8, lanes, 2],
) -> ac.Queue[ac.u8, lanes, 2]:
    return value

@ac.system
def core(
    value: ac.Queue[ac.u8, 2, 2],
) -> ac.Queue[ac.u8, 2, 2]:
    return stage(value, static=ac.case(("lanes", 2)))
"""
        lowered = lower_queue_source(source, "core", source_path="stage.py")
        self.assertIn(
            "#ac.type_expr_queue<#ac.type_expr<#ac.type_expr_concrete<i8>>, "
            '#ac.dependent_value<#ac.dependent_parameter<"lanes">>, '
            "#ac.dependent_value<#ac.dependent_integer<2>>>",
            lowered,
        )
        family = lowered[lowered.index("  ac.module @stage ") :]
        family = family[: family.index("  ac.module @Top ")]
        self.assertIn("!ac.queue<i8, lanes=2, rate=2>", family)
        self.assertIn("!ac.queue<i8, lanes=4, rate=2>", family)
        self.assertEqual(family.count("ac.module.case arguments"), 2)

    def test_parameterized_family_uses_fixed_scalar_queue_shape(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = """
import agentic_circuit as ac

@ac.module_decl(
    source="stage.py",
    parameters=(ac.static_parameter("enabled", ac.static_bool()),),
    finite_cases=(ac.case(("enabled", True)),),
)
def stage(value: ac.u8) -> ac.u8:
    ...

stage_decl = stage

@ac.module(declaration=stage_decl)
def stage(value: ac.u8) -> ac.u8:
    return value

@ac.system
def core(value: ac.u8) -> ac.u8:
    return stage(value, static=ac.case(("enabled", True)))
"""
        lowered = lower_queue_source(source, "core", source_path="stage.py")
        self.assertIn("ac.module @stage", lowered)
        self.assertIn("!ac.queue<i8>", lowered)
        self.assertIn("ac.type_expr_queue", lowered)
        self.assertIn(
            "ac.module.case arguments #ac.static_arguments<["
            '#ac.static_argument<"enabled", '
            "#ac.static_value<#ac.static_bool_value<true>>>]",
            lowered,
        )

    def test_parameterized_implementation_preserves_declared_unused_case_body(
        self,
    ) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = """
import agentic_circuit as ac

@ac.module_decl(
    source="stage.py",
    parameters=(ac.static_parameter("enabled", ac.static_bool()),),
    finite_cases=(
        ac.case(("enabled", True)),
        ac.case(("enabled", False)),
    ),
)
def stage(value: ac.Queue[ac.u8, 1, 1]) -> ac.Queue[ac.u8, 1, 1]:
    ...

stage_decl = stage

@ac.module(declaration=stage_decl)
def stage(value: ac.Queue[ac.u8, 1, 1]) -> ac.Queue[ac.u8, 1, 1]:
    return value

@ac.system
def core(value: ac.u8) -> ac.u8:
    return stage(value, static=ac.case(("enabled", True)))
"""
        lowered = lower_queue_source(source, "core", source_path="stage.py")
        family = lowered[lowered.index("  ac.module @stage ") :]
        family = family[: family.index("  ac.module @Top ")]
        self.assertEqual(family.count("ac.module.case arguments"), 2)
        self.assertIn("#ac.static_bool_value<true>", family)
        self.assertIn("#ac.static_bool_value<false>", family)

    def test_dependent_bit_interface_preserves_parameter_expression(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = """
import agentic_circuit as ac

@ac.module_decl(
    source="stage.py",
    parameters=(
        ac.static_parameter("width", ac.static_int(width=8, signed=False)),
    ),
    finite_cases=(
        ac.case(("width", 8)),
        ac.case(("width", 9)),
    ),
)
def stage(
    value: ac.Queue[ac.bits[width], 1, 1],
) -> ac.Queue[ac.bits[width], 1, 1]:
    ...

@ac.system
def core(value: ac.u8) -> ac.u8:
    return stage(value, static=ac.case(("width", 8)))
"""
        lowered = lower_queue_source(source, "core", source_path="core.py")
        self.assertIn(
            '#ac.type_expr_bits<#ac.dependent_value<#ac.dependent_parameter<"width">>, false>',
            lowered,
        )
        self.assertIn(": (!ac.queue<i8>) -> !ac.queue<i8>", lowered)

    def test_cases_lower_independent_bodies_and_concrete_widths(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = """
import agentic_circuit as ac

@ac.module_decl(
    source="stage.py",
    parameters=(
        ac.static_parameter("enabled", ac.static_bool()),
        ac.static_parameter("width", ac.static_int(width=8, signed=False)),
    ),
    finite_cases=(
        ac.case(("enabled", True), ("width", 8)),
        ac.case(("enabled", False), ("width", 9)),
    ),
)
def stage(
    value: ac.Queue[ac.bits[width], 1, 1],
) -> ac.Queue[ac.bits[width], 1, 1]:
    ...

stage_decl = stage

@ac.module(declaration=stage_decl)
def stage(
    value: ac.Queue[ac.bits[width], 1, 1],
) -> ac.Queue[ac.bits[width], 1, 1]:
    if enabled:
        return value + 1
    return value + 2

@ac.system
def core(value: ac.u8) -> ac.u8:
    return stage(
        value,
        static=ac.case(("enabled", True), ("width", 8)),
    )
"""
        lowered = lower_queue_source(source, "core", source_path="stage.py")
        family = lowered[lowered.index("  ac.module @stage ") :]
        family = family[: family.index("  ac.module @Top ")]
        self.assertIn("type (!ac.queue<i8>) -> !ac.queue<i8>", family)
        self.assertIn("type (!ac.queue<i9>) -> !ac.queue<i9>", family)
        self.assertIn("ac.var.constant 1 : i8", family)
        self.assertIn("ac.var.constant 2 : i9", family)

    def test_enum_family_uses_nominal_typed_values(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = """
import agentic_circuit as ac
from enum import Enum

class Mode(Enum):
    FAST = 0
    SAFE = 1

@ac.module_decl(
    source="stage.py",
    parameters=(ac.static_parameter("mode", ac.static_enum(Mode)),),
    finite_cases=(
        ac.case(("mode", Mode.FAST)),
        ac.case(("mode", Mode.SAFE)),
    ),
)
def stage(value: ac.Queue[ac.u8, 1, 1]) -> ac.Queue[ac.u8, 1, 1]:
    ...

@ac.system
def core(value: ac.u8) -> ac.u8:
    return stage(value, static=ac.case(("mode", Mode.FAST)))
"""
        lowered = lower_queue_source(source, "core", source_path="core.py")
        self.assertIn("#ac.static_enum_type<@Mode>", lowered)
        self.assertIn('#ac.static_enum_value<@Mode, "FAST">', lowered)
        self.assertIn('#ac.static_enum_value<@Mode, "SAFE">', lowered)
        self.assertIn("@Mode]>", lowered)

    def test_config_family_uses_recursive_nominal_typed_values(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = """
import agentic_circuit as ac
from enum import Enum

class Mode(Enum):
    FAST = 0
    SAFE = 1

@ac.config
class Geometry:
    entries: ac.static_int(width=8, signed=False)
    enabled: bool

@ac.config
class Config:
    geometry: Geometry
    mode: Mode

@ac.module_decl(
    source="stage.py",
    parameters=(ac.static_parameter("cfg", ac.static_config(Config)),),
    finite_cases=(
        ac.case(("cfg", Config(geometry=Geometry(entries=4, enabled=True), mode=Mode.FAST))),
        ac.case(("cfg", Config(geometry=Geometry(entries=8, enabled=False), mode=Mode.SAFE))),
    ),
)
def stage(value: ac.Queue[ac.u8, 1, 1]) -> ac.Queue[ac.u8, 1, 1]:
    ...

@ac.system
def core(value: ac.u8) -> ac.u8:
    return stage(
        value,
        static=ac.case(("cfg", Config(geometry=Geometry(entries=4, enabled=True), mode=Mode.FAST))),
    )
"""
        lowered = lower_queue_source(source, "core", source_path="core.py")
        self.assertIn("#ac.static_config_type<@Config", lowered)
        self.assertIn("#ac.static_config_type<@Geometry", lowered)
        self.assertIn("#ac.static_config_value<@Config", lowered)
        self.assertIn("#ac.static_config_value<@Geometry", lowered)
        self.assertIn('#ac.static_enum_value<@Mode, "FAST">', lowered)
        self.assertNotIn("ac.static_config_bindings", lowered)


if __name__ == "__main__":
    unittest.main()
