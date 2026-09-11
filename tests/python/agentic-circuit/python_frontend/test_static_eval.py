from __future__ import annotations

import ast
import unittest
from pathlib import Path

WORKSPACE = Path(__file__).resolve().parent / "fixtures" / "static"


def parse_expr(text: str) -> ast.expr:
    return ast.parse(text, mode="eval").body


class StaticEvaluationTest(unittest.TestCase):
    def test_closed_expression_is_deterministic(self) -> None:
        from agentic_circuit._static_eval import StaticEnvironment, evaluate_static

        expression = parse_expr("tuple(i * 2 for i in range(lanes))")
        self.assertEqual(
            (0, 2, 4, 6),
            evaluate_static(expression, StaticEnvironment({"lanes": 4})),
        )

    def test_integer_bitmask_expression_is_deterministic(self) -> None:
        from agentic_circuit._static_eval import StaticEnvironment, evaluate_static

        expression = parse_expr("(rob | dispatch) ^ (local & 0x7f)")
        self.assertEqual(
            0x4C,
            evaluate_static(
                expression,
                StaticEnvironment({"rob": 0x04, "dispatch": 0x08, "local": 0x40}),
            ),
        )

    def test_unapproved_call_is_rejected(self) -> None:
        from agentic_circuit._static_eval import (
            StaticEnvironment,
            StaticEvalError,
            evaluate_static,
        )

        with self.assertRaisesRegex(StaticEvalError, "unapproved call"):
            evaluate_static(parse_expr("open('input.txt')"), StaticEnvironment({}))

    def test_static_shifts_are_bounded_before_evaluation(self) -> None:
        from agentic_circuit._static_eval import (
            StaticEnvironment,
            StaticEvalError,
            evaluate_static,
        )

        for expression in ("1 << -1", "1 >> -1", "1 << 1000000000"):
            with (
                self.subTest(expression=expression),
                self.assertRaises(StaticEvalError),
            ):
                evaluate_static(parse_expr(expression), StaticEnvironment({}))
        for expression, expected in (
            ("1 << 52", 1 << 52),
            ("0 << 1000000000", 0),
            ("1 >> 1000000000", 0),
            ("-1 >> 1000000000", -1),
        ):
            with self.subTest(expression=expression):
                self.assertEqual(
                    expected,
                    evaluate_static(parse_expr(expression), StaticEnvironment({})),
                )

    def test_non_finite_float_is_rejected(self) -> None:
        from agentic_circuit._static_eval import (
            StaticEnvironment,
            StaticEvalError,
            evaluate_static,
        )

        with self.assertRaisesRegex(StaticEvalError, "finite"):
            evaluate_static(parse_expr("1e309"), StaticEnvironment({}))

    def test_comprehension_target_does_not_escape(self) -> None:
        from agentic_circuit._static_eval import (
            StaticEnvironment,
            StaticEvalError,
            evaluate_static,
        )

        expression = parse_expr("(tuple(i for i in range(2)), i)")
        with self.assertRaisesRegex(StaticEvalError, "unknown static name 'i'"):
            evaluate_static(expression, StaticEnvironment({}))


class ValidationTest(unittest.TestCase):
    def test_static_values_use_the_canonical_ijson_rules(self) -> None:
        from agentic_circuit._canonical_json import validate_ijson_value

        for value, message in (
            (-0.0, "negative zero"),
            (float("inf"), "finite binary64"),
            ("\ud800", "Unicode scalar"),
            (1 << 53, "portable I-JSON range"),
        ):
            with self.subTest(value=repr(value)):
                with self.assertRaisesRegex(ValueError, message):
                    validate_ijson_value(value)

    def test_static_evaluation_maps_canonical_ijson_failures(self) -> None:
        from agentic_circuit._static_eval import (
            StaticEnvironment,
            StaticEvalError,
            evaluate_static,
        )

        for expression, message in (
            ("-0.0", "negative zero"),
            ('"\\ud800"', "Unicode scalar"),
        ):
            with self.subTest(expression=expression):
                with self.assertRaisesRegex(StaticEvalError, message):
                    evaluate_static(parse_expr(expression), StaticEnvironment({}))

    def test_static_eval_does_not_export_a_second_ijson_validator(self) -> None:
        from agentic_circuit import _static_eval

        self.assertFalse(hasattr(_static_eval, "validate_ijson_value"))

    def test_supported_static_control_has_no_diagnostics(self) -> None:
        from agentic_circuit._diagnostics import DiagnosticBag
        from agentic_circuit._source import load_source_unit
        from agentic_circuit._validate import ValidationContext, validate_definition

        unit = load_source_unit(WORKSPACE / "supported.py", WORKSPACE)
        site = next(item for item in unit.definitions if item.name == "Supported")
        diagnostics = DiagnosticBag()
        validate_definition(
            site,
            ValidationContext(
                definition_kind="module",
                static_names=frozenset({"lanes", "enabled"}),
                symbolic_names=frozenset(),
                approved_helpers=frozenset({"tuple", "range"}),
            ),
            diagnostics,
        )

        self.assertEqual((), diagnostics.freeze())

    def test_dynamic_truthiness_is_rejected_at_operand(self) -> None:
        from agentic_circuit._diagnostics import DiagnosticBag
        from agentic_circuit._source import load_source_unit
        from agentic_circuit._validate import ValidationContext, validate_definition

        unit = load_source_unit(WORKSPACE / "unsupported.py", WORKSPACE)
        site = next(item for item in unit.definitions if item.name == "Unsupported")
        diagnostics = DiagnosticBag()
        validate_definition(
            site,
            ValidationContext(
                definition_kind="module",
                static_names=frozenset(),
                symbolic_names=frozenset({"request"}),
                approved_helpers=frozenset(),
            ),
            diagnostics,
        )

        matches = [
            item for item in diagnostics.freeze() if item.code == "ACPY-STATIC-002"
        ]
        self.assertEqual(1, len(matches))
        item = matches[0]
        self.assertIsNotNone(item.source)
        assert item.source is not None
        self.assertEqual((8, 7), (item.source.start_line, item.source.start_column))


if __name__ == "__main__":
    unittest.main()
