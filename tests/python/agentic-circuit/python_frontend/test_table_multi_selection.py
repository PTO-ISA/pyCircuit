from __future__ import annotations

import os
import subprocess
import sys
import unittest


MULTI_SOURCE = """
import agentic_circuit as ac

@ac.system
def pipeline() -> None:
    state = ac.table[4, ac.u8](init=0)
    matches = state.match(lambda entry: entry != 0)
    choices = state.choose(matches, count=2)
    first = state.view(choices[0].index).read(when=choices[0].valid)
    second = state.view(choices[1].index).read(when=choices[1].valid)
    ac.sink(first)
    ac.sink(second)
"""


class TableMultiSelectionFrontendTest(unittest.TestCase):
    def test_serialization_is_hash_seed_independent(self) -> None:
        script = (
            "from agentic_circuit._queue_frontend import lower_queue_source\n"
            f"source = {MULTI_SOURCE!r}\n"
            'print(lower_queue_source(source, "pipeline"), end="")\n'
        )
        outputs = []
        for seed in (2, 65537):
            environment = os.environ.copy()
            environment["PYTHONHASHSEED"] = str(seed)
            completed = subprocess.run(
                (sys.executable, "-c", script),
                env=environment,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(completed.returncode, 0, completed.stderr)
            outputs.append(completed.stdout)
        self.assertEqual(outputs[0], outputs[1])

    def test_static_tuple_emits_indices_then_valids(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(MULTI_SOURCE, "pipeline")
        choose = next(
            line for line in lowered.splitlines() if "ac.table.choose" in line
        )
        self.assertIn(
            "%table_choose_2_index_0, %table_choose_2_index_1, "
            "%table_choose_2_valid_0, %table_choose_2_valid_1",
            choose,
        )
        self.assertIn("count 2", choose)
        self.assertIn("policy #ac<table_selection_policy first>", choose)
        self.assertIn('stable_id "table-selection/choices"', choose)
        self.assertIn(
            "-> !ac.var<i2>, !ac.var<i2>, !ac.var<i1>, !ac.var<i1>",
            choose,
        )

    def test_static_tuple_unpack_is_supported(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        source = MULTI_SOURCE.replace(
            "choices = state.choose(matches, count=2)",
            "choice0, choice1 = state.choose(matches, count=2)",
        ).replace("choices[0]", "choice0").replace("choices[1]", "choice1")
        lowered = lower_queue_source(source, "pipeline")
        self.assertEqual(1, lowered.count("ac.table.choose @state"))
        self.assertIn("count 2", lowered)

    def test_dynamic_index_iteration_storage_and_escape_fail_closed(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        cases = (
            ("choices[0]", "choices[index]", "index must be static"),
            ("first = state.view", "ac.sink(choices)\n    first = state.view", "cannot be iterated"),
            ("first = state.view", "stored = [choices]\n    first = state.view", "cannot be iterated"),
        )
        for old, new, diagnostic in cases:
            with (
                self.subTest(new=new),
                self.assertRaisesRegex(QueueFrontendError, diagnostic),
            ):
                lower_queue_source(MULTI_SOURCE.replace(old, new, 1), "pipeline")

    def test_count_is_static_and_within_domain(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        for count in ("0", "5", "dynamic_count"):
            with (
                self.subTest(count=count),
                self.assertRaisesRegex(QueueFrontendError, "count must be a static"),
            ):
                lower_queue_source(
                    MULTI_SOURCE.replace("count=2", f"count={count}", 1),
                    "pipeline",
                )

    def test_signed_key_and_round_robin_policy_are_typed(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        signed = MULTI_SOURCE.replace(
            "@ac.system",
            "@ac.struct\nclass Entry:\n    score: ac.s8\n\n@ac.system",
        ).replace("ac.table[4, ac.u8]", "ac.table[4, Entry]").replace(
            "state.choose(matches, count=2)",
            'state.choose(matches, count=2, policy="min", '
            "key=lambda entry: entry.score)",
        ).replace("entry != 0", "entry.score != 0")
        signed_ir = lower_queue_source(signed, "pipeline")
        self.assertIn("policy #ac<table_selection_policy min>", signed_ir)
        self.assertIn("key_order #ac<table_key_ordering signed>", signed_ir)

        round_robin = MULTI_SOURCE.replace(
            "state.choose(matches, count=2)",
            'state.choose(matches, count=2, policy="round_robin", '
            "initial_cursor=1)",
        )
        rr_ir = lower_queue_source(round_robin, "pipeline")
        self.assertIn("policy #ac<table_selection_policy round_robin>", rr_ir)
        self.assertIn("initial_cursor 1", rr_ir)
        self.assertNotIn("key_order #ac", rr_ir)


if __name__ == "__main__":
    unittest.main()
