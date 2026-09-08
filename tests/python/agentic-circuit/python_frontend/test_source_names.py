"""Source labels survive frontend rewrites independently of execution identities."""

import re
import unittest

from agentic_circuit._queue_frontend import lower_queue_source

SOURCE = """
import agentic_circuit as ac
@ac.module
def accumulator(value: ac.u8, *, bias: ac.const[int]) -> ac.u8:
    total: ac.u8 = 0
    @ac.rule
    def consume(item):
        nonlocal total
        total = total + item
    @ac.rule
    def advance(item):
        nonlocal total
        total = total + item
        return total
    consume(value)
    result = advance(value)
    return result
@ac.system
def pair(left: ac.u8, right: ac.u8) -> tuple[ac.u8, ac.u8]:
    first = accumulator(left, bias=1)
    second = accumulator(right, bias=2)
    return first, second
"""


class SourceNamesTest(unittest.TestCase):
    def test_nested_rules_and_parameter_specializations_preserve_source_names(self):
        raw = lower_queue_source(SOURCE, "pair")
        self.assertEqual(2, raw.count('ac.source_name = "consume"'))
        self.assertEqual(2, raw.count('ac.source_name = "advance"'))
        self.assertEqual(2, raw.count('ac.source_name = "accumulator"'))
        identities = re.findall(r'ac.rule .*?stable_id "([^"]+)"', raw)
        self.assertEqual(4, len(set(identities)))

    def test_result_renaming_does_not_rename_source_labels(self):
        a = lower_queue_source(SOURCE, "pair")
        b = lower_queue_source(SOURCE.replace("result", "renamed_result"), "pair")
        self.assertEqual(
            re.findall(r'ac.source_name = "([^"]+)"', a),
            re.findall(r'ac.source_name = "([^"]+)"', b),
        )
