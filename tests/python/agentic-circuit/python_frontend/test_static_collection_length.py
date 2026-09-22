"""``len(<static collection>)`` is a compile-time integer.

Issue #150 consolidates static expansion on ``ac.list`` and requires static
``len`` support. A static collection is fully known during elaboration, so its
length is a compile-time constant; before this the frontend expanded ``K``
queues from ``ac.array(K, ...)`` but then required ``K`` to be repeated in the
loop bound, which is exactly the duplication the consolidation removes.
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


class StaticCollectionLengthTest(unittest.TestCase):
    def test_range_over_len_expands_every_lane(self) -> None:
        lowered = lower(
            """
@ac.system
def top() -> None:
    lanes = ac.array(3, lambda i: ac.source(S, depth=2, latency=1))
    for i in range(len(lanes)):
        ac.sink(lanes[i])
"""
        )
        self.assertEqual(3, lowered.count("ac.sink"))
        for index in range(3):
            self.assertIn(f"lanes_element_{index}", lowered)

    def test_len_participates_in_compile_time_arithmetic(self) -> None:
        lowered = lower(
            """
@ac.system
def top() -> None:
    lanes = ac.array(3, lambda i: ac.source(S, depth=2, latency=1))
    for i in range(len(lanes) - 1):
        ac.sink(lanes[i])
"""
        )
        self.assertEqual(2, lowered.count("ac.sink"))

    def test_len_resolves_for_a_set_collection(self) -> None:
        lowered = lower(
            """
@ac.system
def top() -> None:
    first = ac.source(S, depth=2, latency=1)
    second = ac.source(S, depth=2, latency=1)
    lanes = ac.set({first, second})
    for i in range(len(lanes)):
        ac.sink(lanes[i])
"""
        )
        self.assertEqual(2, lowered.count("ac.sink"))

    def test_iterating_the_collection_needs_no_length(self) -> None:
        lowered = lower(
            """
@ac.system
def top() -> None:
    lanes = ac.array(4, lambda i: ac.source(S, depth=2, latency=1))
    for lane in lanes:
        ac.sink(lane)
"""
        )
        self.assertEqual(4, lowered.count("ac.sink"))

    def test_len_of_a_queue_is_not_a_collection_length(self) -> None:
        with self.assertRaises(QueueFrontendError) as caught:
            lower(
                """
@ac.system
def top() -> None:
    queue = ac.source(S, depth=2, latency=1)
    for i in range(len(queue)):
        ac.sink(queue)
"""
            )
        self.assertEqual("ACPY-QUEUE-005", caught.exception.code)
        self.assertIn("range extent must be a compile-time integer", str(caught.exception))

    def test_len_of_an_unknown_name_still_fails_closed(self) -> None:
        with self.assertRaises(QueueFrontendError) as caught:
            lower(
                """
@ac.system
def top() -> None:
    for i in range(len(nothing)):
        ac.sink(nothing)
"""
            )
        self.assertEqual("ACPY-QUEUE-005", caught.exception.code)
        self.assertIn("range extent must be a compile-time integer", str(caught.exception))

    def test_len_is_usable_in_a_static_index(self) -> None:
        """`lanes[len(lanes) - 1]` needs the substitution in both resolvers."""

        lowered = lower(
            """
@ac.system
def top() -> None:
    lanes = ac.array(4, lambda i: ac.source(S, depth=2, latency=1))
    ac.sink(lanes[len(lanes) - 1])
"""
        )
        self.assertEqual(1, lowered.count("ac.sink"))

    def test_len_is_usable_in_the_index_of_a_slice(self) -> None:
        lowered = lower(
            """
@ac.system
def top() -> None:
    lanes = ac.array(4, lambda i: ac.source(S, depth=2, latency=1))
    head = lanes[0:2]
    ac.sink(head[len(head) - 1])
"""
        )
        self.assertEqual(1, lowered.count("ac.sink"))

    def test_a_length_derived_index_out_of_range_names_the_key(self) -> None:
        with self.assertRaises(QueueFrontendError) as caught:
            lower(
                """
@ac.system
def top() -> None:
    lanes = ac.array(3, lambda i: ac.source(S, depth=2, latency=1))
    ac.sink(lanes[len(lanes)])
"""
            )
        self.assertEqual("ACPY-QUEUE-005", caught.exception.code)
        self.assertIn("collection has no key 3", str(caught.exception))

    def test_len_of_a_queue_in_an_index_still_fails_closed(self) -> None:
        with self.assertRaises(QueueFrontendError) as caught:
            lower(
                """
@ac.system
def top() -> None:
    queue = ac.source(S, depth=2, latency=1)
    ac.sink(queue[len(queue) - 1])
"""
            )
        self.assertEqual("ACPY-QUEUE-005", caught.exception.code)

    def test_a_static_collection_stays_statically_indexed(self) -> None:
        """A runtime index over a static collection is still rejected."""

        with self.assertRaises(QueueFrontendError) as caught:
            lower(
                """
@ac.system
def top(index: ac.u8) -> None:
    lanes = ac.array(3, lambda i: ac.source(S, depth=2, latency=1))
    ac.sink(lanes[index])
"""
            )
        self.assertEqual("ACPY-QUEUE-005", caught.exception.code)


if __name__ == "__main__":
    unittest.main()
