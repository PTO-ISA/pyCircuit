"""State assignment supplies the exact type for conditional integer literals."""

import unittest

from agentic_circuit._queue_frontend import QueueFrontendError, lower_queue_source


class RuleLiteralContextTest(unittest.TestCase):
    def test_conditional_literals_use_the_state_assignment_type(self):
        raw = lower_queue_source(
            """
import agentic_circuit as ac
@ac.rule
def update(count, value):
    count = 1 if value else 0
    return count
@ac.system
def conditional_state(value: bool) -> ac.u5:
    count: ac.u5 = 0
    result = update(count, value)
    return result
""",
            "conditional_state",
        )
        assert "!ac.var<i5>" in raw
        assert "i64" not in raw

    def test_rule_ids_are_unique_across_static_module_specializations(self):
        import re

        raw = lower_queue_source(
            """
import agentic_circuit as ac
@ac.rule
def advance(total, value, bias):
    total = total + value + bias
    return total
@ac.module
def accumulator(value: ac.u8, *, bias: ac.const[int]) -> ac.u8:
    total: ac.u8 = 0
    result = advance(total, value, bias)
    return result
@ac.system
def pair(left: ac.u8, right: ac.u8) -> tuple[ac.u8, ac.u8]:
    a = accumulator(left, bias=1)
    b = accumulator(right, bias=2)
    return a, b
""",
            "pair",
        )
        identities = re.findall(r'ac.rule .*?stable_id "([^"]+)"', raw)
        assert len(identities) == 2
        assert len(set(identities)) == 2

    def test_conditional_effect_counts_runtime_arguments_after_specialization(self):
        raw = lower_queue_source(
            """
import agentic_circuit as ac
@ac.rule
def update(total, value, limit):
    if value > limit:
        return
    total = total + value
@ac.module
def counter(value: ac.u8, *, limit: ac.const[int]) -> ac.u8:
    total: ac.u8 = 0
    update(total, value, limit)
    return value
@ac.system
def bounded(value: ac.u8) -> ac.u8:
    result = counter(value, limit=7)
    return result
""",
            "bounded",
        )
        assert " when %" in raw

    def test_conditional_effect_still_rejects_two_runtime_inputs(self):
        with self.assertRaisesRegex(
            QueueFrontendError, "exactly one payload parameter"
        ):
            lower_queue_source(
                """
import agentic_circuit as ac
@ac.rule
def update(total, value, limit):
    if value > limit:
        return
    total = total + value
@ac.system
def counter(value: ac.u8, limit: ac.u8) -> ac.u8:
    total: ac.u8 = 0
    update(total, value, limit)
    return value
""",
                "counter",
            )

    def test_context_does_not_truncate_typed_runtime_values(self):
        with self.assertRaisesRegex(QueueFrontendError, "one exact type"):
            lower_queue_source(
                """
import agentic_circuit as ac
@ac.rule
def update(count, value):
    count = value if value != 0 else 0
    return count
@ac.system
def counter(value: ac.u8) -> ac.u5:
    count: ac.u5 = 0
    result = update(count, value)
    return result
""",
                "counter",
            )
