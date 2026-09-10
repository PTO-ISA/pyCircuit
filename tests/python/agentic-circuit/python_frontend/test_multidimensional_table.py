from __future__ import annotations

import os
import subprocess
import sys
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
    def test_serialization_is_hash_seed_independent(self) -> None:
        script = (
            "from agentic_circuit._queue_frontend import lower_queue_source\n"
            f"source = {SOURCE!r}\n"
            'print(lower_queue_source(source, "pipeline"), end="")\n'
        )

        def lower_with_seed(seed: int) -> str:
            environment = os.environ.copy()
            environment["PYTHONHASHSEED"] = str(seed)
            completed = subprocess.run(
                (sys.executable, "-c", script),
                cwd=os.getcwd(),
                env=environment,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(completed.returncode, 0, completed.stderr)
            return completed.stdout

        self.assertEqual(lower_with_seed(1), lower_with_seed(99991))

    def test_shape_image_mask_domain_and_flattened_choice_are_canonical(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        first = lower_queue_source(SOURCE, "pipeline")
        second = lower_queue_source(SOURCE, "pipeline")
        self.assertEqual(first, second)
        self.assertIn("entries 6 init 0", first)
        self.assertIn("shape = array<i64: 2, 3>", first)
        self.assertIn("axis_widths = array<i64: 1, 2>", first)
        self.assertIn('layout = "row_major"', first)
        self.assertIn("layout_version = 1 : i64", first)
        self.assertIn(
            'schema_id = "sha256:c62fc75ed67ce361c35684c21e695d03'
            '6e8bae7ee6f525dfaf9b13940b86a4b0"',
            first,
        )
        self.assertIn("init_version = 1 : i64", first)
        self.assertIn(
            "init_image = [1 : i8, 2 : i8, 3 : i8, 4 : i8, 5 : i8, 6 : i8]",
            first,
        )
        self.assertIn("domain_axes = array<i64: 0, 1>", first)
        self.assertIn("domain_shape = array<i64: 2, 3>", first)
        self.assertIn("domain_strides = array<i64: 3, 1>", first)
        self.assertIn("domain_offset = 0 : i64", first)
        self.assertIn("ac.table.index @state", first)
        self.assertIn("ac.table.choose @state", first)
        self.assertIn("-> !ac.var<i3>, !ac.var<i1>", first)
        self.assertNotIn("coordinates", first)

    def test_rank_one_zero_shorthand_remains_byte_compatible(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = (
            SOURCE.replace("[(2, 3), ac.u8]", "[6, ac.u8]")
            .replace(
                'init={"version": 1, "entry": ac.u8, '
                '"values": [1, 2, 3, 4, 5, 6]}',
                "init=0",
            )
            .replace("state.view((1, 2))", "state.view(5)")
        )
        lowered = lower_queue_source(source, "pipeline")
        declaration = next(
            line for line in lowered.splitlines() if "ac.table @state" in line
        )
        self.assertEqual(
            '  ac.table @state entry i8 entries 6 init 0 owner "/" '
            'stable_id "table/state"',
            declaration,
        )
        self.assertIn("ac.var.constant 5 : i3", lowered)
        self.assertNotIn("ac.table.index @state", lowered)

    def test_rank_three_extent_one_and_non_power_of_two_shape(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = SOURCE.replace("(2, 3)", "(1, 3, 5)", 1).replace(
            "state.view((1, 2))", "state.view((0, 2, 4))"
        ).replace(
            'init={"version": 1, "entry": ac.u8, '
            '"values": [1, 2, 3, 4, 5, 6]}',
            "init=0",
        )
        lowered = lower_queue_source(source, "pipeline")
        self.assertIn("shape = array<i64: 1, 3, 5>", lowered)
        self.assertIn("axis_widths = array<i64: 1, 2, 3>", lowered)
        self.assertIn(
            "ac.table.index @state [%v0, %v1, %v2] : "
            "!ac.var<i1>, !ac.var<i2>, !ac.var<i3> -> !ac.var<i4>",
            lowered,
        )

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
            ("state.view((1, 2))", "state.view((1, 2, 0))", "rank"),
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

    def test_dynamic_multidimensional_index_uses_typed_flattening_boundary(
        self,
    ) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = SOURCE.replace(
            'init={"version": 1, "entry": ac.u8, '
            '"values": [1, 2, 3, 4, 5, 6]}',
            "init=0",
        ).replace(
            "snapshots = state.view((1, 2)).read()",
            "requests = ac.source(ac.u1)\n"
            "    snapshots = state.view(lambda request: (request, 2)).read(requests)",
        )
        lowered = lower_queue_source(source, "pipeline")
        self.assertIn(
            "ac.table.index @state [%item, %v0] : "
            "!ac.var<i1>, !ac.var<i2> -> !ac.var<i3>",
            lowered,
        )

    def test_projected_view_has_explicit_row_major_mask_domain(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = SOURCE.replace(
            "snapshots = state.view((1, 2)).read()",
            "row = state.view(1)\n"
            "    snapshots = row.view(2).read()",
        ).replace("matches = state.match", "matches = row.match").replace(
            "selected = state.choose", "selected = row.choose"
        ).replace(
            "selected_snapshot = state.view(selected.index)",
            "selected_snapshot = row.view(selected.index)",
        )
        lowered = lower_queue_source(source, "pipeline")
        self.assertIn("domain_axes = array<i64: 1>", lowered)
        self.assertIn("domain_shape = array<i64: 3>", lowered)
        self.assertIn("domain_strides = array<i64: 1>", lowered)
        self.assertIn("domain_offset = 3 : i64", lowered)
        self.assertIn("-> !ac.var<i3>, !ac.var<i1>", lowered)
        self.assertIn("ac.table.index @state", lowered)

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
        self.assertIn(
            "init_image = [{value = 1 : i8, valid = 1 : i1}, "
            "{value = 2 : i8, valid = 0 : i1}",
            lowered,
        )

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
