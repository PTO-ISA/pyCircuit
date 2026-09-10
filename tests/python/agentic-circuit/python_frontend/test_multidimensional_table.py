from __future__ import annotations

import unittest

SOURCE = """
import agentic_circuit as ac

@ac.system
def pipeline() -> None:
    state = ac.table[(2, 3), ac.u8](
        init={"version": 1, "entry": ac.u8, "values": [1, 2, 3, 4, 5, 6]}
    )
    snapshots = state.view((1, 2)).read()
    matches = state.match(lambda entry: entry != 0)
    selected = state.choose(matches)
    selected_snapshot = state.view(selected.index).read(when=selected.valid)
    ac.sink(snapshots)
    ac.sink(selected_snapshot)
"""


class MultidimensionalTableFrontendTest(unittest.TestCase):
    def test_shape_image_mask_domain_and_flattened_choice_are_canonical(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        first = lower_queue_source(SOURCE, "pipeline")
        second = lower_queue_source(SOURCE, "pipeline")
        self.assertEqual(first, second)
        self.assertIn("entries 6 init 0", first)
        self.assertIn("ac.table_shape = [2 : i64, 3 : i64]", first)
        self.assertIn('ac.table_layout = "row-major-v1"', first)
        self.assertIn('ac.table_init_image = "{\\"entry\\":\\"i8\\"', first)
        self.assertIn("ac.table_mask_domain_shape = [2 : i64, 3 : i64]", first)
        self.assertIn("ac.var.constant 5 : i64", first)
        self.assertIn("ac.table.choose @state", first)
        self.assertIn("-> !ac.var<i3>, !ac.var<i1>", first)
        self.assertNotIn("coordinates", first)

    def test_rank_one_integer_shape_remains_compatible(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = SOURCE.replace("[(2, 3), ac.u8]", "[6, ac.u8]").replace(
            "state.view((1, 2))", "state.view(5)"
        )
        lowered = lower_queue_source(source, "pipeline")
        self.assertIn("ac.table_shape = [6 : i64]", lowered)
        self.assertIn("ac.var.constant 5 : i64", lowered)

    def test_shape_and_per_axis_static_bounds_are_rejected(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        replacements = (
            ("(2, 3)", "()", "non-empty"),
            ("(2, 3)", "(2, 0)", "positive"),
            ("(2, 3)", "(2, 'x')", "positive static"),
            ("state.view((1, 2))", "state.view((2, 0))", "axis 0"),
            ("state.view((1, 2))", "state.view((1, 3))", "axis 1"),
            ("state.view((1, 2))", "state.view((1,))", "rank"),
        )
        for old, new, diagnostic in replacements:
            with (
                self.subTest(new=new),
                self.assertRaisesRegex(QueueFrontendError, diagnostic),
            ):
                lower_queue_source(SOURCE.replace(old, new, 1), "pipeline")

    def test_shape_product_overflow_is_rejected(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        source = SOURCE.replace("(2, 3)", f"({1 << 62}, 4)", 1)
        with self.assertRaisesRegex(QueueFrontendError, "flattened size overflows"):
            lower_queue_source(source, "pipeline")

    def test_dynamic_multidimensional_index_fails_closed_until_native_bounds(
        self,
    ) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        source = SOURCE.replace(
            "snapshots = state.view((1, 2)).read()",
            "requests = ac.source(ac.u8)\n"
            "    snapshots = state.view(lambda request: (request, 2)).read(requests)",
        )
        with self.assertRaisesRegex(QueueFrontendError, "native per-axis bounds"):
            lower_queue_source(source, "pipeline")

    def test_aggregate_image_is_typed_and_field_order_is_canonical(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = (
            SOURCE.replace(
                "@ac.system",
                "@ac.struct\nclass Entry:\n    value: ac.u8\n    valid: bool\n\n@ac.system",
            )
            .replace(
                "ac.table[(2, 3), ac.u8](\n"
                '        init={"version": 1, "entry": ac.u8, "values": [1, 2, 3, 4, 5, 6]}\n'
                "    )",
                "ac.table[(2, 3), Entry](\n"
                '        init={"values": [Entry(valid=True, value=1), '
                "Entry(value=2, valid=False), Entry(value=3, valid=False), "
                "Entry(value=4, valid=False), Entry(value=5, valid=False), "
                'Entry(value=6, valid=False)], "entry": Entry, "version": 1}\n'
                "    )",
            )
            .replace("lambda entry: entry != 0", "lambda entry: entry.valid")
        )
        lowered = lower_queue_source(source, "pipeline")
        self.assertIn('\\"values\\":[{\\"valid\\":true,\\"value\\":1}', lowered)

    def test_typed_image_version_entry_count_and_value_are_rejected(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        cases = (
            ('"version": 1', '"version": 2', "version"),
            ('"entry": ac.u8', '"entry": ac.u16', "Entry type"),
            ("1, 2, 3, 4, 5, 6", "1, 2", "exactly 6"),
            ("1, 2, 3, 4, 5, 6", "1, 2, 3, 4, 5, 256", "does not fit"),
            ("1, 2, 3, 4, 5, 6", "1, 2, 3, 4, 5, ROOT", "closed literals"),
        )
        for old, new, diagnostic in cases:
            with (
                self.subTest(new=new),
                self.assertRaisesRegex(QueueFrontendError, diagnostic),
            ):
                lower_queue_source(SOURCE.replace(old, new, 1), "pipeline")


if __name__ == "__main__":
    unittest.main()
