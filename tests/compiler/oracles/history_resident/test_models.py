"""Hand-computed reservation witnesses and independent checker mutation probes."""

import json
import unittest
from copy import deepcopy
from pathlib import Path

from check import compare
from generate import data, isq_history, rob_history
from models import ResidentModel, completion, entry, event, packed_snapshot, readiness


def rob_residents():
    m = ResidentModel("rob")
    m.instances["left"].update(head=0, tail=2, count=2)
    m.instances["left"]["entries"][0] = event(10, generation=2, done=True)
    m.instances["left"]["entries"][1] = event(20, index=1, generation=1, done=True)
    return m


def isq_residents():
    m = ResidentModel("isq")
    m.instances["left"]["entries"][0] = entry(100, 3, 5, 7, valid=True)
    m.instances["left"]["entries"][1] = entry(200, 1, 11, 13, index=1, valid=True)
    for tag in (5, 7, 11, 13):
        m.instances["left"]["ready"][tag] = True
    return m


class HistoricalResidentOracleTests(unittest.TestCase):
    def assert_rival(self, initial, actions, rival, grants, writes=None):
        correct = deepcopy(initial).step(actions)
        wrong = deepcopy(initial).step(actions, rival=rival)
        self.assertEqual(correct["grants"]["left"], grants)
        if writes is not None:
            self.assertEqual(correct["writes"]["left"], writes)
        self.assertNotEqual(
            (correct["after"], correct["grants"], correct["writes"]),
            (wrong["after"], wrong["grants"], wrong["writes"]),
        )
        return correct

    def test_original_histories_and_complete_state(self):
        rob, isq = rob_history(), isq_history()
        self.assertEqual((len(rob.rows), len(isq.rows)), (61, 81))
        for kind, p, count in (("rob", rob, 10), ("isq", isq, 6)):
            for row in p.rows:
                self.assertEqual(len(row["after"]["queues"]), count)
                self.assertEqual(
                    sum(len(s["entries"]) for s in row["after"]["instances"].values()),
                    8,
                )
                packed_snapshot(kind, row["after"])
                if kind == "isq":
                    self.assertEqual(
                        sum(
                            len(s["ready"]) for s in row["after"]["instances"].values()
                        ),
                        128,
                    )

    def test_rob_stale_same_head_completion_does_not_lock_done_write(self):
        m = rob_residents()
        m.queues["left_completion"] = [event(index=0, generation=1)]
        row = self.assert_rival(
            m,
            {},
            "rob_stale_completion_whole_entry_lock",
            ["complete", "retire"],
            ["entries[0].done", "head", "count"],
        )
        self.assertEqual(
            row["after"]["queues"]["left_retired"], [event(10, generation=2, done=True)]
        )
        self.assertEqual(row["after"]["instances"]["left"]["count"], 1)

    def test_rob_matching_same_head_completion_locks_retire(self):
        m = rob_residents()
        m.queues["left_completion"] = [completion(m.instances["left"]["entries"][0])]
        self.assert_rival(
            m, {}, "rob_complete_retire_cofire", ["complete"], ["entries[0].done"]
        )

    def test_rob_matching_other_entry_completion_and_retire_cofire(self):
        m = rob_residents()
        m.queues["left_completion"] = [completion(m.instances["left"]["entries"][1])]
        row = m.step()
        self.assertEqual(row["grants"]["left"], ["complete", "retire"])
        self.assertEqual(
            row["writes"]["left"],
            ["entries[1].done", "entries[0].done", "head", "count"],
        )

    def test_rob_allocation_epoch_identity_write_blocks_completion(self):
        m = rob_residents()
        m.queues["left_allocate"] = [event(30)]
        m.queues["left_completion"] = [completion(m.instances["left"]["entries"][0])]
        row = self.assert_rival(m, {}, "rob_allocate_complete_cofire", ["allocate"])
        self.assertTrue(row["after"]["queues"]["left_completion"])
        self.assertEqual(row["after"]["instances"]["left"]["count"], 3)

    def test_rob_flush_retains_entries_and_tail(self):
        m = rob_residents()
        m.queues["left_flush"] = [event()]
        row = self.assert_rival(m, {}, "rob_flush_clear_entries", ["recover"])
        s = row["after"]["instances"]["left"]
        self.assertEqual((s["head"], s["tail"], s["count"], s["epoch"]), (2, 2, 0, 1))
        self.assertEqual(s["entries"], m.instances["left"]["entries"])

    def test_rob_stale_generation_and_epoch_do_not_write(self):
        for rival, tag in (
            ("rob_ignore_generation", event(generation=1)),
            ("rob_ignore_epoch", event(generation=2, epoch=65535)),
        ):
            with self.subTest(rival=rival):
                m = rob_residents()
                m.instances["left"]["entries"][0]["done"] = False
                m.queues["left_completion"] = [tag]
                self.assert_rival(m, {}, rival, ["complete"], [])

    def test_generation_and_epoch_wrap_are_finite(self):
        m = ResidentModel("rob")
        m.instances["left"]["entries"][0]["generation"] = 65535
        m.instances["left"]["epoch"] = 65535
        m.queues["left_allocate"] = [event(90)]
        m.step()
        self.assertEqual(m.queues["left_allocated"][0]["generation"], 0)
        self.assertEqual(m.queues["left_allocated"][0]["epoch"], 65535)
        m.queues["left_flush"] = [event()]
        m.step()
        self.assertEqual(m.instances["left"]["epoch"], 0)

    def test_output_take_bubble_both_algorithms(self):
        for kind, m, name, held, grants in (
            ("rob", rob_residents(), "left_retired", event(1), []),
            ("isq", isq_residents(), "left_issued", entry(1), []),
        ):
            with self.subTest(kind=kind):
                m.queues[name] = [held]
                row = self.assert_rival(m, {"take": [name]}, "output_replace", grants)
                self.assertEqual(row["after"]["queues"][name], [])

    def test_isq_readiness_clear_and_restore_have_priority_over_issue(self):
        for ready, rival in (
            (False, "isq_ignore_ready_reservation"),
            (True, "isq_new_ready_visibility"),
        ):
            with self.subTest(ready=ready):
                row = self.assert_rival(
                    isq_residents(),
                    {"inject": {"left_readiness": readiness(11, ready)}},
                    rival,
                    ["update_ready"],
                    ["ready[11]"],
                )
                self.assertFalse(row["after"]["queues"]["left_issued"])

    def test_isq_unrelated_ready_writer_and_oldest_issue_cofire(self):
        row = self.assert_rival(
            isq_residents(),
            {"inject": {"left_readiness": readiness(63)}},
            "isq_full_ready_lock",
            ["update_ready", "issue"],
            ["ready[63]", "entries[1].valid"],
        )
        self.assertEqual(row["after"]["queues"]["left_issued"][0]["value"], 200)

    def test_isq_invalid_entry_src1_reservation_is_retained(self):
        m = isq_residents()
        m.instances["left"]["entries"][3] = entry(80, 9, 40, 63, index=3, valid=False)
        for rival in ("isq_valid_only_reservation", "isq_src0_only_reservation"):
            with self.subTest(rival=rival):
                self.assert_rival(
                    m,
                    {"inject": {"left_readiness": readiness(63)}},
                    rival,
                    ["update_ready"],
                    ["ready[63]"],
                )

    def test_isq_dispatch_and_ready_cofire_but_issue_stalls(self):
        m = isq_residents()
        m.queues["left_request"] = [entry(300, 0, 5)]
        row = self.assert_rival(
            m,
            {"inject": {"left_readiness": readiness(40)}},
            "isq_dispatch_issue_cofire",
            ["update_ready", "dispatch"],
        )
        self.assertEqual(
            row["after"]["instances"]["left"]["entries"][2],
            entry(300, 0, 5, index=2, valid=True),
        )

    def test_full_entry_set_does_not_reuse_issued_slot_on_same_edge(self):
        m = isq_residents()
        for i in (2, 3):
            m.instances["left"]["entries"][i] = entry(
                300 + i, 10 + i, 5, index=i, valid=True
            )
        m.queues["left_request"] = [entry(600, 0, 5)]
        row = m.step()
        self.assertEqual(row["grants"]["left"], ["issue"])
        self.assertTrue(row["after"]["queues"]["left_request"])
        self.assertFalse(row["after"]["instances"]["left"]["entries"][1]["valid"])

    def test_exact_snapshot_checker_rejects_hidden_state_and_shortened_runs(self):
        expected = data()
        for kind, case in expected["cases"].items():
            actual = {
                "rows": [
                    {"epoch": row["epoch"], "after": deepcopy(row["after"])}
                    for row in case["rows"]
                ]
            }
            compare(kind, actual, expected)
            with self.assertRaises(AssertionError):
                compare(kind, {"rows": actual["rows"][:-1]}, expected)
            bad = deepcopy(actual)
            bad["rows"][0]["after"]["instances"]["right"]["entries"][3]["value"] = 1
            with self.assertRaises(AssertionError):
                compare(kind, bad, expected)
            if kind == "isq":
                bad = deepcopy(actual)
                bad["rows"][0]["after"]["instances"]["right"]["ready"][63] = True
                with self.assertRaises(AssertionError):
                    compare(kind, bad, expected)

    def test_frozen_literal_handoff_matches_independent_model(self):
        path = Path(__file__).with_name("vectors.json")
        if not path.exists():
            self.fail("missing literal vector handoff")
        frozen = json.loads(path.read_text())
        expected = data()
        self.assertEqual(frozen["provenance"], expected["provenance"])
        for group in ("cases", "witnesses", "systems"):
            for kind, case in frozen[group].items():
                self.assertEqual(
                    case["executed_epochs"], expected[group][kind]["executed_epochs"]
                )
                compare(kind, {"rows": case["rows"]}, expected, group)

    def test_regular_clock_cases_use_only_ordinary_queue_operations(self):
        expected = data()
        self.assertEqual(expected["systems"]["rob"], expected["cases"]["rob"])
        rows = expected["systems"]["isq"]["rows"]
        self.assertFalse(any(row["actions"].get("inject") for row in rows))
        clear = next(
            row
            for row in rows
            if row["label"] == "ordinary-ready-clear-competes-with-old-eligible-issue"
        )
        self.assertEqual(clear["grants"]["left"], ["update_ready"])
        self.assertFalse(clear["after"]["queues"]["left_issued"])
        values = [
            row["taken"]["left_issued"]["value"]
            for row in rows
            if "left_issued" in row["taken"]
        ]
        self.assertEqual(
            values, [100, 300, 200, 400, 500, 600, 700, 800, 801, 802, 803]
        )

    def test_all_rivals_are_discriminated_by_reset_reachable_programs(self):
        expected = data()
        rivals = {
            "rob": (
                "output_replace",
                "rob_allocate_complete_cofire",
                "rob_ignore_generation",
                "rob_ignore_epoch",
                "rob_stale_completion_whole_entry_lock",
                "rob_complete_retire_cofire",
                "rob_flush_clear_entries",
            ),
            "isq": (
                "output_replace",
                "isq_valid_only_reservation",
                "isq_src0_only_reservation",
                "isq_full_ready_lock",
                "isq_new_ready_visibility",
                "isq_ignore_ready_reservation",
                "isq_dispatch_issue_cofire",
            ),
        }
        for kind, mutations in rivals.items():
            for rival in mutations:
                with self.subTest(kind=kind, rival=rival):
                    detected = False
                    for group in ("cases", "witnesses"):
                        m = ResidentModel(kind)
                        for reference in expected[group][kind]["rows"]:
                            observed = m.step(reference["actions"], rival=rival)
                            if observed["after"] != reference["after"]:
                                detected = True
                                break
                        if detected:
                            break
                    self.assertTrue(detected, (kind, rival))


if __name__ == "__main__":
    unittest.main()
