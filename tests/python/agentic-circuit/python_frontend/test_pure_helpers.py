from __future__ import annotations

import unittest

from agentic_circuit._queue_frontend import (
    QueueFrontendError,
    lower_queue_program,
    lower_queue_source,
    parse_queue_program,
)


def lower(source: str) -> str:
    return lower_queue_program(parse_queue_program(source, "pipeline"))


class PureHelperFrontendTest(unittest.TestCase):
    def test_typed_helper_is_preserved_as_private_func_and_call(self) -> None:
        text = lower(
            """
import agentic_circuit as ac
def add_one(value: ac.u8) -> ac.u8:
    return value + 1
@ac.system
def pipeline(value: ac.u8) -> ac.u8:
    incoming = ac.source(ac.u8)
    outgoing = incoming.apply(lambda item: add_one(item))
    return outgoing
"""
        )
        self.assertIn(
            "func.func private @add_one(%value: !ac.var<i8>) -> !ac.var<i8>", text
        )
        self.assertIn("func.call @add_one(%item) : (!ac.var<i8>) -> !ac.var<i8>", text)

    def test_inline_helper_keeps_marker_nested_calls_and_branches(self) -> None:
        text = lower(
            """
import agentic_circuit as ac
def bias(value: ac.u8) -> ac.u8:
    return value + 1
@ac.inline
def classify(value: ac.u8, valid: bool) -> tuple[ac.u8, bool]:
    adjusted = value
    if valid:
        adjusted = bias(value)
    else:
        adjusted = value + 2
    return adjusted, valid
@ac.system
def pipeline(value: ac.u8) -> ac.u8:
    incoming = ac.source(ac.u8)
    outgoing = incoming.apply(lambda item: classify(item, True)[0])
    return outgoing
"""
        )
        self.assertIn("func.func private @classify", text)
        self.assertIn("attributes {ac.inline = true}", text)
        self.assertIn("func.call @bias", text)
        self.assertIn("func.call @classify", text)
        self.assertIn("ac.var.select", text)

    def test_tuple_helper_can_be_directly_unpacked_in_a_rule(self) -> None:
        text = lower(
            """
import agentic_circuit as ac
def split(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    return value, value + 1
@ac.rule
def transform(item):
    first, second = split(item)
    return first + second
@ac.system
def pipeline(value: ac.u8) -> ac.u8:
    incoming = ac.source(ac.u8)
    outgoing = transform(incoming)
    return outgoing
"""
        )
        self.assertIn("func.call @split", text)
        self.assertGreaterEqual(text.count("ac.var.element"), 2)

    def test_helper_is_available_to_non_transform_queue_expressions(self) -> None:
        text = lower(
            """
import agentic_circuit as ac
@ac.struct
class Item:
    value: ac.u8
    lane: ac.u1
def route_for(item: Item) -> ac.u1:
    return item.lane
@ac.system
def pipeline() -> None:
    incoming = ac.source(Item)
    left, right = incoming.route(outputs=2, key=lambda item: route_for(item))
    ac.sink(left)
    ac.sink(right)
"""
        )
        self.assertIn("func.func private @route_for", text)
        self.assertIn("func.call @route_for", text)

    def test_helpers_are_available_to_pure_and_stateful_modules(self) -> None:
        source = """
import agentic_circuit as ac
def add_one(value: ac.u8) -> ac.u8:
    return value + 1
@ac.inline
def forced(value: ac.u8) -> ac.u8:
    return add_one(value)
@ac.module
def increment(value: ac.u8) -> ac.u8:
    return forced(value)
@ac.module
def accumulator(value: ac.u8) -> ac.u8:
    total: ac.u8 = 0
    total = add_one(value)
    return total
@ac.system
def pipeline(left: ac.u8, right: ac.u8) -> tuple[ac.u8, ac.u8]:
    incremented = increment(left)
    accumulated = accumulator(right)
    return incremented, accumulated
"""
        text = lower_queue_source(source, "pipeline")
        self.assertEqual(1, text.count("func.func private @add_one"))
        self.assertEqual(1, text.count("func.func private @forced"))
        self.assertIn("attributes {ac.inline = true}", text)
        self.assertGreaterEqual(text.count("func.call @add_one"), 2)
        self.assertIn("func.call @forced", text)

    def test_helper_rejects_undefined_branch_local_and_wrong_result(self) -> None:
        undefined = """
import agentic_circuit as ac
def bad(value: ac.u8, valid: bool) -> ac.u8:
    if valid:
        result = value
    return result
@ac.system
def pipeline(value: ac.u8) -> ac.u8:
    incoming = ac.source(ac.u8)
    outgoing = incoming.apply(lambda item: bad(item, True))
    return outgoing
"""
        with self.assertRaisesRegex(
            QueueFrontendError, "not defined on every path"
        ) as caught:
            parse_queue_program(undefined, "pipeline")
        self.assertEqual(caught.exception.code, "ACPY-HELPER-004")
        wrong_result = """
import agentic_circuit as ac
def bad(value: ac.u8) -> bool:
    return value
@ac.system
def pipeline(value: ac.u8) -> ac.u8:
    incoming = ac.source(ac.u8)
    outgoing = incoming.apply(lambda item: bad(item))
    return outgoing
"""
        with self.assertRaisesRegex(QueueFrontendError, "result type does not match"):
            lower(wrong_result)

    def test_helper_rejects_effects_loops_early_returns_and_recursion(self) -> None:
        cases = (
            (
                "def bad(value: ac.u8) -> ac.u8:\n    return print(value)",
                "unknown or effectful",
            ),
            (
                "def bad(value: ac.u8) -> ac.u8:\n    while value:\n        value = value - 1\n    return value",
                "supports only assignments",
            ),
            (
                "def bad(value: ac.u8) -> ac.u8:\n    if value:\n        return value\n    return value + 1",
                "one final return",
            ),
            (
                "def bad(value: ac.u8) -> ac.u8:\n    return bad(value)",
                "recursive helper",
            ),
            (
                "def bad(value: ac.u8) -> ac.u8:\n"
                "    discarded = print(value)\n"
                "    discarded = value\n"
                "    return discarded",
                "unknown or effectful",
            ),
            (
                "def bad(value: ac.u8) -> ac.u8:\n"
                "    discarded = bad(value)\n"
                "    discarded = value\n"
                "    return discarded",
                "recursive helper",
            ),
            (
                "def bad(value: ac.u8) -> ac.u8:\n"
                "    discarded = ambient\n"
                "    discarded = value\n"
                "    return discarded",
                "undefined or captured",
            ),
            (
                "def bad(value: ac.u8) -> ac.u8:\n"
                "    if print(value):\n"
                "        result = value\n"
                "    else:\n"
                "        result = value + 1\n"
                "    return result",
                "unknown or effectful",
            ),
        )
        for helper, message in cases:
            with self.subTest(message=message):
                source = f"""
import agentic_circuit as ac
{helper}
@ac.system
def pipeline(value: ac.u8) -> ac.u8:
    incoming = ac.source(ac.u8)
    outgoing = incoming.apply(lambda item: bad(item))
    return outgoing
"""
                with self.assertRaisesRegex(QueueFrontendError, message):
                    parse_queue_program(source, "pipeline")


if __name__ == "__main__":
    unittest.main()
