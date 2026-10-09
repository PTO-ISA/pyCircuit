"""Independent complete-state and hostile-variant checks for slot/issue."""

import json
import unittest
from copy import deepcopy
from pathlib import Path

from check import check_case, counters
from generate import data, initial_issue_entries
from models import Model, entry, event, packed_snapshot
from rivals import ISSUE_RIVALS, MAILBOX_RIVALS, witnesses


class HistoricalSlotIssueTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.all_cases = data()

    def case(self, name):
        return self.all_cases["executable"][name]

    def row(self, case, label):
        return next(row for row in self.case(case)["rows"] if row["label"] == label)

    def test_original_mailbox_retains_all_six_epochs(self):
        case = self.all_cases["original"]["mailbox"]
        self.assertFalse(case["compiled_gate"])
        self.assertEqual([row["epoch"] for row in case["rows"]], list(range(1, 7)))
        rows = case["rows"]
        self.assertEqual(rows[0]["grants"], {"left": ["capture"], "right": ["capture"]})
        self.assertEqual(
            rows[0]["after"]["received"], {"left_output": [], "right_output": []}
        )
        self.assertEqual(rows[1]["grants"], {"left": ["consume"], "right": ["consume"]})
        self.assertEqual(
            rows[-1]["after"]["received"],
            {"left_output": [event(7)], "right_output": [event(7)]},
        )
        self.assertEqual(self.case("mailbox_original_body")["executed_epochs"], 7)

    def test_original_issue_preserves_initial_host_state_and_thirty_epochs(self):
        case = self.all_cases["original"]["issue"]
        self.assertFalse(case["compiled_gate"])
        self.assertEqual(case["initial"]["entries"], initial_issue_entries())
        self.assertEqual(case["executed_epochs"], 30)
        self.assertEqual(case["rows"][5]["after"]["pops"]["allocations"], 1)
        self.assertEqual(case["rows"][5]["after"]["entries"], initial_issue_entries())
        self.assertTrue(case["rows"][5]["after"]["slots"]["allocation"]["valid"])
        final = case["rows"][-1]["after"]
        self.assertEqual(
            [record["age"] for record in final["received"]["output"]], [0, 99]
        )
        self.assertEqual(final["pops"]["allocations"], 1)

    def test_reset_reachable_prelude_is_separate_and_retains_original_body(self):
        case = self.case("issue_original_body")
        self.assertEqual(case["initial"]["entries"], [entry() for _ in range(4)])
        start = case["body_start_epoch"]
        self.assertEqual(start, 12)
        self.assertEqual(case["executed_epochs"], 42)
        warm = case["rows"][start]["before"]
        self.assertEqual(warm["entries"], initial_issue_entries())
        self.assertEqual(warm["pops"]["allocations"], 4)
        self.assertNotEqual(warm, self.all_cases["original"]["issue"]["initial"])
        self.assertEqual(case["rows"][start + 5]["after"]["pops"]["allocations"] - 4, 1)
        self.assertEqual(case["rows"][-1]["after"]["pops"]["allocations"] - 4, 1)

    def test_all_executable_cases_use_real_ordinary_boundary_actions(self):
        for name, case in self.all_cases["executable"].items():
            with self.subTest(case=name):
                model = Model(case["kind"])
                self.assertEqual(model.snapshot(), case["initial"])
                self.assertLessEqual(case["executed_epochs"], case["epoch_limit"])
                for row in case["rows"]:
                    self.assertNotIn("inject", row["actions"])
                    for offers in row["actions"].get("offer", {}).values():
                        self.assertEqual(len(offers), 1)
                    self.assertEqual(
                        model.step(row["actions"], row["label"])["after"], row["after"]
                    )

    def test_mailbox_hold_release_retains_payload_and_has_no_refill(self):
        case = self.case("mailbox_backpressure")
        held = [
            row
            for row in case["rows"]
            if row["label"] == "both-output-full-slot-and-input-held"
        ]
        self.assertEqual(len(held), 4)
        for row in held:
            self.assertEqual(row["before"], row["after"])
        take = self.row("mailbox_backpressure", "take-right_output")
        self.assertEqual(take["grants"]["right"], [])
        publish = self.row("mailbox_backpressure", "right-publish-102-left-still-held")
        self.assertEqual(publish["grants"]["right"], ["consume"])
        self.assertFalse(publish["after"]["slots"]["right"]["valid"])
        self.assertEqual(publish["after"]["queues"]["right_input"], [event(103)])
        final = case["rows"][-1]["after"]
        self.assertEqual(final["slots"]["left"], {"valid": False, "payload": event(33)})
        self.assertEqual(
            final["slots"]["right"], {"valid": False, "payload": event(103)}
        )

    def test_false_payload_wakeup_sets_both_operands_over_separate_old_edges(self):
        case = self.case("issue_wakeup_flags")
        patches = [
            row
            for row in case["rows"]
            if row["grants"]["wakeup_src0"] or row["grants"]["wakeup_src1"]
        ]
        self.assertEqual(len(patches), 2)
        self.assertFalse(patches[0]["before"]["slots"]["wakeup"]["payload"]["valid"])
        self.assertTrue(patches[0]["after"]["entries"][0]["src0_ready"])
        self.assertFalse(patches[0]["after"]["entries"][0]["src1_ready"])
        self.assertTrue(patches[1]["after"]["entries"][0]["src1_ready"])
        self.assertIsNone(patches[1]["grants"]["selected"])
        self.assertEqual(
            case["rows"][-1]["after"]["received"]["output"],
            [entry(9, 7, 8, True, True, True)],
        )

    def test_same_edge_wakeup_does_not_apply_to_new_allocation(self):
        row = self.row(
            "issue_allocation_no_forward", "install-and-wakeup-old-empty-table"
        )
        self.assertEqual(row["grants"]["allocated"], 0)
        self.assertEqual(row["grants"]["release"], ["allocation", "wakeup"])
        self.assertEqual(row["after"]["entries"][0], entry(17, 7, 7, True))
        self.assertEqual(row["grants"]["wakeup_src0"], [])
        self.assertEqual(row["grants"]["wakeup_src1"], [])

    def test_table_full_still_captures_pending_slot_and_depth_two_queue(self):
        captured = self.row(
            "issue_full_slots_and_output", "capture-allocation-boundary-even-table-full"
        )
        self.assertEqual(captured["after"]["pops"]["allocations"], 5)
        self.assertTrue(captured["after"]["slots"]["allocation"]["valid"])
        self.assertEqual(captured["grants"]["allocated"], None)
        held = [
            row
            for row in self.case("issue_full_slots_and_output")["rows"]
            if row["label"] == "full-table-allocation-slot-and-depth2-boundary-held"
        ]
        self.assertEqual(len(held), 4)
        for row in held:
            self.assertEqual(row["before"], row["after"])
            self.assertEqual(
                [item["age"] for item in row["after"]["queues"]["allocations"]],
                [100, 101],
            )
            self.assertEqual(row["after"]["slots"]["allocation"]["payload"]["age"], 99)

    def test_selected_valid_clears_even_when_output_full(self):
        row = self.row(
            "issue_full_slots_and_output",
            "output-full-still-clears-selected99-and-captures100",
        )
        self.assertEqual(row["before"]["queues"]["output"][0]["age"], 0)
        self.assertEqual(row["before"]["entries"][0]["age"], 99)
        self.assertTrue(row["before"]["entries"][0]["valid"])
        self.assertFalse(row["after"]["entries"][0]["valid"])
        self.assertFalse(row["grants"]["output_push"])
        self.assertEqual(row["after"]["queues"]["output"][0]["age"], 0)

    def test_old_free_mask_prevents_same_edge_release_refill(self):
        issue = self.row(
            "issue_full_slots_and_output", "issue-old-full-table-no-same-edge-install"
        )
        self.assertEqual(issue["grants"]["selected"], 0)
        self.assertIsNone(issue["grants"]["allocated"])
        install = self.row(
            "issue_full_slots_and_output", "install-slot99-on-following-old-free-edge"
        )
        self.assertEqual(install["grants"]["allocated"], 0)
        self.assertNotIn("allocation", install["grants"]["capture"])
        self.assertFalse(install["after"]["slots"]["allocation"]["valid"])
        self.assertEqual(install["after"]["queues"]["allocations"][0]["age"], 100)

    def test_selection_is_oldest_with_stable_tie_and_physical_output_bubble(self):
        self.assertEqual(
            self.row("issue_oldest_ties", "oldest-ready-first-stable-index1")["grants"][
                "selected"
            ],
            1,
        )
        pop = self.row(
            "issue_oldest_ties", "physical-pop-does-not-give-future-space-to-read"
        )
        self.assertEqual(pop["grants"]["selected"], 2)
        self.assertTrue(pop["grants"]["valid_clear"])
        self.assertFalse(pop["grants"]["output_push"])
        self.assertEqual(pop["after"]["queues"]["output"], [])

    def test_independent_fields_cofire_and_read_returns_old_valid_entry(self):
        wake = self.row(
            "issue_independent_fields", "wakeup-other-entry-and-old-ready-issue-cofire"
        )
        self.assertEqual(wake["grants"]["wakeup_src0"], [0])
        self.assertEqual(wake["grants"]["wakeup_src1"], [0])
        self.assertEqual(wake["grants"]["selected"], 1)
        self.assertTrue(wake["after"]["queues"]["output"][0]["valid"])
        self.assertFalse(wake["after"]["entries"][1]["valid"])
        allocation = self.row(
            "issue_independent_fields",
            "old-ready-clear-and-different-free-allocation-cofire",
        )
        self.assertEqual(allocation["grants"]["selected"], 0)
        self.assertEqual(allocation["grants"]["allocated"], 1)
        self.assertEqual(allocation["after"]["entries"][1], entry(30, 30, 31, True))

    def test_complete_allocation_replaces_stale_ready_and_keeps_false_valid(self):
        final = self.case("issue_full_replacement")["rows"][-1]["after"]
        replacement = entry(33, 51, 53)
        self.assertEqual(final["entries"][0], replacement)
        self.assertEqual(
            final["slots"]["allocation"], {"valid": False, "payload": replacement}
        )
        self.assertEqual(final["queues"]["output"], [])

    def test_literal_vectors_match_every_epoch_including_counters(self):
        literal = json.loads(Path(__file__).with_name("vectors.json").read_text())
        self.assertEqual(literal["provenance"], self.all_cases["provenance"])
        for group in ("original", "executable"):
            self.assertEqual(set(literal[group]), set(self.all_cases[group]))
            for name, expected in self.all_cases[group].items():
                with self.subTest(group=group, case=name):
                    self.assertEqual(
                        check_case(expected, literal[group][name]),
                        expected["executed_epochs"],
                    )

    def test_all_rivals_have_reset_reachable_physical_state_counterexamples(self):
        result = witnesses(self.all_cases["executable"])
        self.assertEqual(set(result), {*MAILBOX_RIVALS, *ISSUE_RIVALS})
        self.assertEqual(len(result), 26)
        for witness in result.values():
            self.assertNotEqual(
                witness["reference_packed_after"], witness["rival_packed_after"]
            )
        self.assertEqual(
            result, json.loads(Path(__file__).with_name("rivals.json").read_text())
        )

    def test_checker_rejects_final_output_only_and_shortened_histories(self):
        case = self.case("issue_original_body")
        with self.assertRaises(AssertionError):
            check_case(case, {"rows": case["rows"][:-1]})
        with self.assertRaises(AssertionError):
            check_case(
                case,
                {
                    "rows": [
                        {"epoch": row["epoch"], "received": row["after"]["received"]}
                        for row in case["rows"]
                    ]
                },
            )

    def test_checker_rejects_wrong_epoch_and_missing_counter_observations(self):
        case = self.case("mailbox_backpressure")
        rows = deepcopy(case["rows"])
        rows[0]["epoch"] += 1
        with self.assertRaises(AssertionError):
            check_case(case, rows)
        rows = [
            {"epoch": row["epoch"], "packed_after": row["packed_after"]}
            for row in case["rows"]
        ]
        with self.assertRaises(AssertionError):
            check_case(case, rows)

    def test_checker_rejects_retained_payload_readiness_queue_and_counter_mutations(
        self,
    ):
        for name, key in (
            ("mailbox_backpressure", "payload"),
            ("issue_wakeup_flags", "ready"),
            ("issue_full_slots_and_output", "queue"),
            ("issue_original_body", "counter"),
        ):
            with self.subTest(field=key):
                case = self.case(name)
                rows = deepcopy(case["rows"])
                # Only supply after, so rejection cannot accidentally come from optional before/work.
                observations = [
                    {"epoch": row["epoch"], "after": row["after"]} for row in rows
                ]
                if key == "payload":
                    observations[-1]["after"]["slots"]["left"]["payload"]["value"] ^= 1
                elif key == "ready":
                    observations[-1]["after"]["entries"][0]["src1_ready"] ^= 1
                elif key == "queue":
                    target = next(
                        row
                        for row in observations
                        if row["after"]["queues"]["allocations"]
                    )
                    target["after"]["queues"]["allocations"][0]["age"] ^= 1
                else:
                    observations[-1]["after"]["pops"]["allocations"] += 1
                with self.assertRaises(AssertionError):
                    check_case(case, observations)

    def test_packed_checker_requires_and_checks_actual_received_and_transfer_counts(
        self,
    ):
        case = self.case("issue_original_body")
        rows = [
            {
                "epoch": row["epoch"],
                "packed_after": row["packed_after"],
                "after_counters": counters(row["after"]),
            }
            for row in case["rows"]
        ]
        self.assertEqual(check_case(case, rows), 42)
        rows[-1]["after_counters"] = deepcopy(rows[-1]["after_counters"])
        rows[-1]["after_counters"]["received"]["output"][0]["valid"] = False
        with self.assertRaises(AssertionError):
            check_case(case, rows)

    def test_complete_packing_has_exact_width_and_observes_invalid_storage(self):
        for name in ("mailbox_backpressure", "issue_full_replacement"):
            case = self.case(name)
            final = deepcopy(case["rows"][-1]["after"])
            bits, packed = packed_snapshot(case["kind"], final)
            self.assertEqual(bits, case["snapshot_bits"])
            if case["kind"] == "mailbox":
                final["slots"]["left"]["payload"]["value"] ^= 1
            else:
                final["entries"][0]["src0_tag"] ^= 1
            self.assertNotEqual(packed_snapshot(case["kind"], final)[1], packed)

    def test_public_flags_use_old_capacity_and_payloads(self):
        row = self.row(
            "issue_full_slots_and_output",
            "output-full-still-clears-selected99-and-captures100",
        )
        controls = row["controls"]
        self.assertFalse(controls["issued_ready"])
        self.assertTrue(controls["issued_available"])
        self.assertEqual(controls["issued_head"]["age"], 0)
        self.assertTrue(controls["selected"])
        self.assertFalse(controls["publish"])
        self.assertEqual(controls["publish_data"], entry(99, 20, 21, True, True, True))
        self.assertTrue(controls["allocation_capture"])
        self.assertFalse(controls["allocation_install"])
        case = self.case("issue_full_slots_and_output")
        observations = deepcopy(case["rows"])
        target = next(item for item in observations if item["epoch"] == row["epoch"])
        target["controls"]["selected"] = False
        with self.assertRaises(AssertionError):
            check_case(case, observations)


if __name__ == "__main__":
    unittest.main()
