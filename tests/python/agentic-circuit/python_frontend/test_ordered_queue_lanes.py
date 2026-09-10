from __future__ import annotations

import unittest


LANE_SOURCE = """
import agentic_circuit as ac

@ac.system
def pipeline() -> None:
    incoming = ac.source(ac.u8, depth=4, lanes=3, rate=2)
    computed = ac.compute(incoming, lambda item: item, depth=4)
    staged = ac.pipeline(computed, stages=2, depth=4)
    ac.sink(staged)
"""


class OrderedQueueLaneFrontendTest(unittest.TestCase):
    def test_lanes_and_rate_share_one_queue_identity(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(LANE_SOURCE, "pipeline")
        queue_type = "!ac.queue<i8, lanes = 3, rate = 2>"
        self.assertGreaterEqual(lowered.count(queue_type), 6)
        self.assertEqual(1, lowered.count("%incoming = ac.source"))
        self.assertNotIn("incoming__lane", lowered)

    def test_lane_count_is_separate_from_payload_shape(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = LANE_SOURCE.replace(
            "@ac.system",
            "@ac.struct\nclass Packet:\n    values: ac.array[3, ac.u8]\n\n"
            "@ac.system",
        ).replace("ac.source(ac.u8", "ac.source(Packet")
        lowered = lower_queue_source(source, "pipeline")
        self.assertIn(
            "!ac.queue<!ac.struct<@types::@Packet>, lanes = 3, rate = 2>",
            lowered,
        )

    def test_zero_dynamic_and_rate_exceeding_lanes_fail_closed(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        cases = (
            ("lanes=3", "lanes=0", "lanes must be positive"),
            ("lanes=3", "lanes=runtime_lanes", "compile-time integer"),
            ("lanes=3, rate=2", "lanes=2, rate=3", "must not exceed lanes"),
        )
        for old, new, diagnostic in cases:
            with (
                self.subTest(new=new),
                self.assertRaisesRegex(QueueFrontendError, diagnostic),
            ):
                lower_queue_source(LANE_SOURCE.replace(old, new, 1), "pipeline")

    def test_merge_rejects_lane_scalarization_mismatch(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        source = """
import agentic_circuit as ac

@ac.system
def pipeline() -> None:
    left = ac.source(ac.u8, depth=4, lanes=2)
    right = ac.source(ac.u8, depth=4, lanes=3)
    merged = ac.merge(left, right)
    ac.sink(merged)
"""
        with self.assertRaisesRegex(QueueFrontendError, "matching lanes and rate"):
            lower_queue_source(source, "pipeline")


if __name__ == "__main__":
    unittest.main()
