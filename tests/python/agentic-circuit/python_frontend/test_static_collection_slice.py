"""A static collection can be sliced during elaboration.

Issue #150 requires ``ac.list`` to support static slicing alongside static
``len``, indexing, and iteration, and the existing ordered collections carry the
same requirement. A static collection is fully known during elaboration, so a
slice selects members at compile time and yields another static collection with
the selected members re-keyed from zero; ``slice.indices`` supplies Python's
clamping and negative-bound rules instead of a reimplementation.
"""

from __future__ import annotations

import unittest

from agentic_circuit._queue_frontend import QueueFrontendError, lower_queue_source

PRELUDE = """
import agentic_circuit as ac

@ac.struct
class S:
    lane: ac.u8
"""


def lower(body: str) -> str:
    return lower_queue_source(PRELUDE + body, "top", source_path="generated/module.py")


def lanes(extent: int = 4) -> str:
    return (
        f"    lanes = ac.array({extent}, lambda i: ac.source(S, depth=2, latency=1))\n"
    )


class SliceSelectionTest(unittest.TestCase):
    def test_slice_selects_the_requested_members(self) -> None:
        lowered = lower(
            "@ac.system\ndef top() -> None:\n"
            + lanes()
            + "    head = lanes[0:2]\n"
            "    for queue in head:\n"
            "        ac.sink(queue)\n"
        )
        self.assertEqual(2, lowered.count("ac.sink"))

    def test_slice_step_selects_every_other_member(self) -> None:
        lowered = lower(
            "@ac.system\ndef top() -> None:\n"
            + lanes()
            + "    evens = lanes[0:4:2]\n"
            "    for queue in evens:\n"
            "        ac.sink(queue)\n"
        )
        self.assertEqual(2, lowered.count("ac.sink"))

    def test_open_ended_slice_keeps_the_tail(self) -> None:
        lowered = lower(
            "@ac.system\ndef top() -> None:\n"
            + lanes()
            + "    rest = lanes[1:]\n"
            "    for queue in rest:\n"
            "        ac.sink(queue)\n"
        )
        self.assertEqual(3, lowered.count("ac.sink"))

    def test_negative_bounds_follow_python(self) -> None:
        lowered = lower(
            "@ac.system\ndef top() -> None:\n"
            + lanes()
            + "    tail = lanes[-2:]\n"
            "    for queue in tail:\n"
            "        ac.sink(queue)\n"
        )
        self.assertEqual(2, lowered.count("ac.sink"))

    def test_negative_step_reverses_the_order(self) -> None:
        lowered = lower(
            "@ac.system\ndef top() -> None:\n"
            + lanes()
            + "    reversed_lanes = lanes[::-1]\n"
            "    for queue in reversed_lanes:\n"
            "        ac.sink(queue)\n"
        )
        self.assertEqual(4, lowered.count("ac.sink"))

    def test_a_slice_is_indexed_from_zero(self) -> None:
        lowered = lower(
            "@ac.system\ndef top() -> None:\n"
            + lanes()
            + "    middle = lanes[1:3]\n"
            "    ac.sink(middle[0])\n"
        )
        self.assertEqual(1, lowered.count("ac.sink"))

    def test_a_slice_has_a_static_length(self) -> None:
        lowered = lower(
            "@ac.system\ndef top() -> None:\n"
            + lanes()
            + "    head = lanes[0:3]\n"
            "    for index in range(len(head)):\n"
            "        ac.sink(head[index])\n"
        )
        self.assertEqual(3, lowered.count("ac.sink"))

    def test_a_set_collection_can_be_sliced(self) -> None:
        lowered = lower(
            """
@ac.system
def top() -> None:
    first = ac.source(S, depth=2, latency=1)
    second = ac.source(S, depth=2, latency=1)
    third = ac.source(S, depth=2, latency=1)
    members = ac.set({first, second, third})
    part = members[0:2]
    for queue in part:
        ac.sink(queue)
"""
        )
        self.assertEqual(2, lowered.count("ac.sink"))


class SliceRejectionTest(unittest.TestCase):
    def assert_rejected(self, body: str, fragment: str) -> None:
        with self.assertRaises(QueueFrontendError) as caught:
            lower(body)
        self.assertEqual("ACPY-QUEUE-005", caught.exception.code)
        self.assertIn(fragment, str(caught.exception))

    def test_dynamic_bounds_fail_closed(self) -> None:
        self.assert_rejected(
            "@ac.system\ndef top(index: ac.u8) -> None:\n"
            + lanes()
            + "    head = lanes[0:index]\n"
            "    for queue in head:\n"
            "        ac.sink(queue)\n",
            "static slice bounds must be compile-time integers",
        )

    def test_zero_step_fails_closed(self) -> None:
        self.assert_rejected(
            "@ac.system\ndef top() -> None:\n"
            + lanes()
            + "    head = lanes[0:2:0]\n"
            "    for queue in head:\n"
            "        ac.sink(queue)\n",
            "static slice step must not be zero",
        )

    def test_empty_slice_fails_closed(self) -> None:
        self.assert_rejected(
            "@ac.system\ndef top() -> None:\n"
            + lanes()
            + "    head = lanes[2:2]\n"
            "    for queue in head:\n"
            "        ac.sink(queue)\n",
            "static slice selects no members",
        )

    def test_a_keyed_collection_cannot_be_sliced(self) -> None:
        self.assert_rejected(
            """
@ac.system
def top() -> None:
    only = ac.source(S, depth=2, latency=1)
    keyed = ac.map({"a": only})
    part = keyed[0:1]
    for queue in part:
        ac.sink(queue)
""",
            "a keyed collection cannot be sliced",
        )

    def test_a_queue_cannot_be_sliced(self) -> None:
        self.assert_rejected(
            """
@ac.system
def top() -> None:
    queue = ac.source(S, depth=2, latency=1)
    part = queue[0:1]
    ac.sink(queue)
""",
            "static slicing requires a collection",
        )


class SlicedSelectReceiverTest(unittest.TestCase):
    def test_select_accepts_a_sliced_collection(self) -> None:
        lowered = lower(
            """
@ac.system
def top() -> None:
    control = ac.source(S, depth=2, latency=1)
    lanes = ac.array(4, lambda i: ac.source(S, depth=2, latency=1))
    window = lanes[0:3]
    chosen = window.select(control, key=lambda item: item.lane, depth=2, latency=1)
    ac.sink(chosen)
"""
        )
        self.assertIn("ac.select", lowered)

    def test_select_rejects_a_single_member_slice(self) -> None:
        with self.assertRaises(QueueFrontendError) as caught:
            lower(
                """
@ac.system
def top() -> None:
    control = ac.source(S, depth=2, latency=1)
    lanes = ac.array(4, lambda i: ac.source(S, depth=2, latency=1))
    window = lanes[0:1]
    chosen = window.select(control, key=lambda item: item.lane, depth=2, latency=1)
    ac.sink(chosen)
"""
            )
        self.assertEqual("ACPY-QUEUE-018", caught.exception.code)
        self.assertIn("select requires two unique data Queues", str(caught.exception))


if __name__ == "__main__":
    unittest.main()
