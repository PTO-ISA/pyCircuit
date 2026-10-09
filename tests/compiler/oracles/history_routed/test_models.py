"""Independent full-graph old-Q timing, storage, failure and rival checks."""

import json
import unittest
from copy import deepcopy
from pathlib import Path

from check import check_case
from generate import compact, data, expected_output
from models import (
    DEPTHS,
    DONE,
    EXECUTING,
    FIELDS,
    MASK64,
    OBSERVED,
    QUEUE_NAMES,
    WAITING,
    Model,
    empty_state,
    flow_counters,
    item,
    packed_snapshot,
)
from rivals import RIVALS, witnesses


class RoutedDependencyOracleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cases = data()

    def case(self, name):
        return self.cases["executable"][name]

    def test_original_full_tests_have_no_fabricated_runtime_inputs(self):
        self.assertEqual(self.cases["historical_runtime_inputs"], [])
        manifest = self.cases["provenance"]
        self.assertEqual(len(manifest["assets"]), 4)
        baseline = (
            Path(__file__).resolve().parents[2]
            / "lit/Source/Inputs/history-routed/baseline"
        )
        topology_test = (baseline / "test_queue_codegen.py.txt").read_text()
        start = topology_test.index(
            "    def test_routed_dependency_topology_generates_deterministically"
        )
        end = topology_test.index("    def _assert_typed_module_example", start)
        self.assertNotIn("proposePush", topology_test[start:end])
        self.assertIn("-fsyntax-only", topology_test[start:end])
        self.assertIn(
            "test_routed_dependency_graph_builds_full_pyc_and_verilog",
            (baseline / "test_pyc_backend.py.txt").read_text(),
        )

    def test_full_topology_widths_and_both_persistent_tables(self):
        self.assertEqual(list(DEPTHS), list(QUEUE_NAMES))
        self.assertEqual(
            list(DEPTHS.values()), [16, 4, 16, 8, 8, 8, 8, 1, 1, 1, 1, 8, 8, 1]
        )
        self.assertEqual(sum(DEPTHS.values()), 89)
        state = empty_state()
        self.assertEqual(len(state["scheduler"]), 8)
        self.assertEqual(len(state["reorder"]), 64)
        packed = packed_snapshot(state)
        self.assertEqual(len(packed), 5484)
        self.assertEqual(int(packed, 16), 0)

    def test_single_item_thirteen_edges_preserve_all_fields_and_old_q_latency(self):
        m = Model()
        record = item(0, 113, 0, 255, 1, MASK64)
        rows = [m.step({"offer": record})] + [m.step() for _ in range(12)]
        self.assertEqual(rows[0]["after"]["queues"]["incoming"], [record])
        self.assertEqual(rows[1]["after"]["queues"]["prepared"][0]["value"], 0)
        self.assertEqual(rows[2]["after"]["scheduler"][0]["state"], WAITING)
        self.assertEqual(rows[3]["after"]["scheduler"][0]["state"], EXECUTING)
        self.assertEqual(rows[3]["after"]["scheduler"][0]["deadline"], 4)
        self.assertEqual(rows[4]["after"]["scheduler"][0]["state"], DONE)
        self.assertEqual(rows[5]["after"]["queues"]["scheduled"][0]["value"], 0)
        self.assertEqual(rows[6]["after"]["queues"]["route_0"][0]["value"], 0)
        self.assertEqual(rows[7]["after"]["queues"]["route_0_done"][0]["value"], 1)
        self.assertEqual(rows[8]["after"]["queues"]["completed"][0]["value"], 1)
        self.assertEqual(rows[9]["after"]["reorder"][0]["key"], 0)
        self.assertTrue(rows[9]["after"]["reorder"][0]["valid"])
        self.assertEqual(rows[10]["after"]["next_key"], 1)
        self.assertEqual(
            rows[11]["after"]["queues"]["output"], [expected_output(record)]
        )
        self.assertEqual(rows[11]["after"]["received"], [])
        self.assertEqual(rows[12]["after"]["received"], [expected_output(record)])
        self.assertEqual(m.state["received"][0]["value"], 101)
        self.assertEqual(m.state["received"][0]["opcode"], 113)
        self.assertEqual(m.completed_history, set())
        self.assertEqual(m.seen_history, set())

    def test_four_routes_preserve_opcode_and_unsigned_wrap(self):
        case = self.case("four_routes_live_dependencies")
        inputs = [
            row["actions"]["offer"] for row in case["rows"] if "offer" in row["actions"]
        ]
        final = case["rows"][-1]["after"]
        self.assertEqual(
            final["received"], [expected_output(record) for record in inputs]
        )
        self.assertEqual({record["route"] for record in inputs}, {0, 1, 2, 3})
        self.assertEqual(
            [record["sequence_id"] for record in final["received"]], list(range(16))
        )
        for sent, received in zip(inputs, final["received"], strict=True):
            for field in FIELDS[:-1]:
                self.assertEqual(sent[field], received[field])

    def test_live_done_dependency_issues_on_same_edge_as_predecessor_retirement(self):
        rows = self.case("four_routes_live_dependencies")["rows"]
        witness = next(
            row
            for row in rows
            if row["grants"].get("dependency", {}).get("retire") is not None
            and any(
                row["before"]["scheduler"][index]["item"]["waits_for"] != 255
                for index in row["grants"]["dependency"]["issue"]
            )
        )
        retired = witness["grants"]["dependency"]["retire"]
        predecessor = witness["before"]["scheduler"][retired]
        self.assertEqual(predecessor["state"], DONE)
        self.assertFalse(witness["after"]["scheduler"][retired]["valid"])
        issue = next(
            index
            for index in witness["grants"]["dependency"]["issue"]
            if witness["before"]["scheduler"][index]["item"]["waits_for"]
            == predecessor["item"]["sequence_id"]
        )
        self.assertEqual(witness["after"]["scheduler"][issue]["state"], EXECUTING)

    def test_completion_does_not_forward_to_waiting_dependency(self):
        rows = self.case("four_routes_live_dependencies")["rows"]
        witness = next(
            row
            for row in rows
            if any(
                row["before"]["scheduler"][index]["item"]["sequence_id"] == 0
                for index in row["grants"].get("dependency", {}).get("complete", [])
            )
        )
        self.assertFalse(
            any(
                witness["before"]["scheduler"][index]["item"]["sequence_id"] == 1
                for index in witness["grants"]["dependency"]["issue"]
            )
        )
        self.assertTrue(
            any(
                entry["valid"]
                and entry["item"]["sequence_id"] == 1
                and entry["state"] == WAITING
                for entry in witness["after"]["scheduler"]
            )
        )

    def test_resource_is_free_at_deadline_even_with_old_executing_entry(self):
        rows = self.case("lowest_key_issue")["rows"]
        witness = next(
            row
            for row in rows
            if row["grants"].get("dependency", {}).get("complete")
            and row["grants"]["dependency"]["issue"]
        )
        complete = witness["grants"]["dependency"]["complete"][0]
        issued = witness["grants"]["dependency"]["issue"][0]
        self.assertEqual(witness["before"]["scheduler"][complete]["item"]["route"], 0)
        self.assertEqual(witness["before"]["scheduler"][issued]["item"]["route"], 0)
        self.assertEqual(
            witness["before"]["scheduler"][complete]["deadline"],
            witness["before"]["epoch"],
        )
        self.assertEqual(
            witness["before"]["scheduler"][issued]["item"]["sequence_id"], 1
        )

    def test_retire_uses_deadline_then_key_not_first_physical_slot(self):
        row = self.cases["reference"]["retirement_priority"]["rows"][0]
        self.assertEqual(row["grants"]["dependency"]["retire"], 1)
        self.assertEqual(row["after"]["queues"]["scheduled"][0]["sequence_id"], 1)
        state = deepcopy(row["before"])
        state["scheduler"][0]["deadline"] = 10
        model = Model(state)
        self.assertEqual(model.step()["grants"]["dependency"]["retire"], 1)

    def test_full_topology_has_all_four_branches_and_complete_capacity_hold(self):
        rows = self.case("full_topology_backpressure")["rows"]
        held = [
            row for row in rows if row["label"] == "complete-topology-backpressure-held"
        ]
        self.assertEqual(len(held), 4)
        for row in held:
            state = row["after"]
            self.assertEqual(sum(entry["valid"] for entry in state["scheduler"]), 8)
            self.assertEqual(sum(entry["valid"] for entry in state["reorder"]), 64)
            self.assertEqual(len(state["queues"]["incoming"]), 16)
            self.assertEqual(len(state["queues"]["prepared"]), 4)
            self.assertEqual(len(state["queues"]["scheduled"]), 16)
            for route in range(4):
                self.assertTrue(state["queues"][f"route_{route}"])
                self.assertTrue(state["queues"][f"route_{route}_done"])
            for key in state.keys() - {"epoch"}:
                self.assertEqual(row["before"][key], state[key])
            self.assertTrue(row["frame"]["valid"])
            self.assertFalse(row["public_expected"]["ready"])
            self.assertTrue(row["grants"]["host_blocked"])
        final = rows[-1]["after"]
        self.assertEqual(len(final["received"]), 180)
        self.assertTrue(all(not values for values in final["queues"].values()))

    def test_late_dependency_has_no_persistent_completion_bitmap(self):
        final = self.case("late_retired_predecessor")["rows"][-1]["after"]
        self.assertEqual(len(final["received"]), 1)
        active = [entry for entry in final["scheduler"] if entry["valid"]]
        self.assertEqual(len(active), 1)
        self.assertEqual(active[0]["item"]["waits_for"], 0)
        self.assertEqual(active[0]["state"], WAITING)

    def test_capacity_defers_cost_and_live_duplicate_checks_until_old_free(self):
        for name, error in (
            ("scheduler_deferred_zero_cost", "dependency_nonpositive_cost"),
            ("scheduler_deferred_duplicate", "dependency_duplicate_key"),
        ):
            with self.subTest(case=name):
                rows = self.case(name)["rows"]
                held = [
                    row
                    for row in rows
                    if row["label"] == "old-full-scheduler-defers-head-validation"
                ]
                self.assertEqual(len(held), 3)
                for row in held:
                    self.assertTrue(row["committed"])
                    self.assertFalse(row["failed"])
                    self.assertEqual(
                        sum(entry["valid"] for entry in row["before"]["scheduler"]), 8
                    )
                failed = next(row for row in rows if row["failed"])
                self.assertEqual(failed["errors"], [error])
                self.assertEqual(
                    sum(entry["valid"] for entry in failed["before"]["scheduler"]), 7
                )
                self.assertEqual(failed["after"], failed["before"])
                self.assertFalse(failed["committed"])

    def test_failed_status_is_sticky_until_separate_host_reset_retry(self):
        case = self.case("scheduler_deferred_duplicate")
        rows = case["rows"]
        failed = next(row for row in rows if row["failed"])
        latched = rows[failed["attempt"] + 1]
        self.assertEqual(latched["errors"], ["latched_failure"])
        self.assertEqual(latched["after"], failed["after"])
        reset = rows[case["host_reset_attempts"][0]]
        self.assertEqual(reset["after"], empty_state())
        self.assertFalse(reset["failed"])
        self.assertEqual(
            rows[-1]["after"]["received"], [expected_output(item(0, 17, 0, 255, 1, 60))]
        )

    def test_live_key_reuse_is_allowed_before_reorder_stale_rejection(self):
        rows = self.case("live_key_reuse_reorder_stale")["rows"]
        admissions = [
            row
            for row in rows
            if row["grants"].get("dependency", {}).get("admit") is not None
        ]
        self.assertEqual(len(admissions), 2)
        self.assertEqual(rows[-1]["errors"], ["reorder_stale_key"])

    def test_reorder_duplicate_discards_concurrent_valid_host_offer(self):
        row = self.case("reorder_live_duplicate")["rows"][-1]
        self.assertEqual(row["errors"], ["reorder_duplicate_key"])
        self.assertTrue(row["grants"]["host_offer"])
        self.assertEqual(row["actions"]["offer"]["sequence_id"], 0)
        self.assertEqual(row["before"], row["after"])

    def test_reorder_full64_defers_duplicate_and_stale_without_head_pop(self):
        for name in ("full64_deferred_stale", "full64_missing_zero_duplicate_stall"):
            rows = self.case(name)["rows"]
            held = [
                row
                for row in rows
                if row["label"]
                == "old-full64-reorder-defers-duplicate-and-stale-validation"
            ]
            self.assertEqual(len(held), 8)
            for row in held:
                self.assertEqual(
                    sum(entry["valid"] for entry in row["before"]["reorder"]), 64
                )
                self.assertTrue(row["committed"])
                self.assertFalse(row["failed"])
                self.assertEqual(
                    row["before"]["pops"]["completed"],
                    row["after"]["pops"]["completed"],
                )
        fault = self.case("full64_deferred_stale")["rows"][-1]
        self.assertEqual(fault["errors"], ["reorder_stale_key"])
        self.assertEqual(fault["before"], fault["after"])

    def test_byte_domain_255_is_valid_and_next_key_never_wraps_to_zero(self):
        rows = self.case("full_byte_domain_no_reorder_wrap")["rows"]
        complete = next(row for row in rows if len(row["after"]["received"]) == 256)
        self.assertEqual(complete["after"]["next_key"], 256)
        self.assertEqual(complete["after"]["received"][-1]["sequence_id"], 255)
        self.assertEqual(complete["after"]["received"][-1]["opcode"], 0)
        self.assertEqual(rows[-1]["errors"], ["reorder_stale_key"])
        self.assertEqual(rows[-1]["before"]["next_key"], 256)

    def test_merge_cursor_moves_only_on_committed_old_selected_transfer(self):
        for row in self.case("four_routes_live_dependencies")["rows"]:
            selected = row["grants"].get("merge")
            if selected is None:
                self.assertEqual(row["after"]["cursor"], row["before"]["cursor"])
            else:
                self.assertTrue(row["before"]["queues"][f"route_{selected}_done"])
                self.assertEqual(row["after"]["cursor"], (selected + 1) % 4)

    def test_observation_dedup_uses_pop_count_and_full_head(self):
        held = next(
            row
            for row in self.case("full_topology_backpressure")["rows"]
            if row["label"] == "complete-topology-backpressure-held"
        )
        for name in OBSERVED:
            self.assertEqual(
                held["before"]["observations"][name],
                held["after"]["observations"][name],
            )
        final = self.case("four_routes_live_dependencies")["rows"][-1]["after"]
        self.assertEqual(len(final["observations"]["scheduled"]), 16)
        self.assertEqual(len(final["observations"]["completed"]), 16)
        self.assertEqual(
            {record["route"] for record in final["observations"]["route_1_done"]}, {1}
        )
        self.assertEqual(
            {record["route"] for record in final["observations"]["route_3_done"]}, {3}
        )

    def test_reference_clock_and_u64_overflow_are_explicitly_not_hardware_traces(self):
        self.assertEqual(
            set(self.cases["reference"]),
            {
                "wide_deadline",
                "retirement_priority",
                "deadline_overflow",
                "clock_control_discard",
            },
        )
        for case in self.cases["reference"].values():
            self.assertFalse(case["executable"])
        wide = self.cases["reference"]["wide_deadline"]["rows"][0]
        self.assertEqual(wide["after"]["scheduler"][0]["deadline"], 65536)
        for name in ("deadline_overflow", "clock_control_discard"):
            row = self.cases["reference"][name]["rows"][-1]
            self.assertFalse(row["committed"])
            self.assertTrue(row["failed"])
            self.assertEqual(row["before"], row["after"])

    def test_all_model_rivals_have_complete_reset_reachable_counterexamples(self):
        result = witnesses(self.cases)
        self.assertEqual(set(result), set(RIVALS))
        self.assertEqual(len(result), 27)
        self.assertTrue(all(witness["executable"] for witness in result.values()))
        for name, witness in result.items():
            self.assertTrue(
                witness["changed_physical_fields"] or witness["changed_observations"],
                name,
            )
        self.assertEqual(
            result, json.loads(Path(__file__).with_name("rivals.json").read_text())
        )

    def test_literal_frames_and_all_state_epochs_match_independent_models(self):
        literal = json.loads(Path(__file__).with_name("vectors.json").read_text())
        self.assertEqual(literal["provenance"], self.cases["provenance"])
        self.assertEqual(literal["historical_runtime_inputs"], [])
        self.assertEqual(
            sum(case["executed_epochs"] for case in literal["executable"].values()),
            1792,
        )
        for group in ("executable", "reference"):
            self.assertEqual(set(literal[group]), set(self.cases[group]))
            for name, wanted in self.cases[group].items():
                with self.subTest(group=group, case=name):
                    self.assertEqual(
                        check_case(wanted, literal[group][name]),
                        wanted["executed_epochs"],
                    )
                    for row in wanted["rows"]:
                        self.assertEqual(
                            row["frame"]["valid"],
                            "offer" in row["actions"] or "present" in row["actions"],
                        )
                        self.assertEqual(
                            row["frame"]["data"],
                            row["actions"].get(
                                "offer", row["actions"].get("present", item())
                            ),
                        )

    def test_checker_rejects_prefix_output_only_missing_flow_and_wrong_failure(self):
        case = self.case("reorder_live_duplicate")
        observations = compact({"executable": {"case": case}, "reference": {}})[
            "executable"
        ]["case"]
        with self.assertRaises(AssertionError):
            check_case(case, observations["rows"][:-1])
        missing = deepcopy(observations["rows"])
        missing[0].pop("after_flow")
        with self.assertRaises(AssertionError):
            check_case(case, missing)
        bad = deepcopy(observations["rows"])
        bad[-1]["failed"] = False
        with self.assertRaises(AssertionError):
            check_case(case, bad)
        with self.assertRaises(AssertionError):
            check_case(
                case,
                [
                    {
                        "attempt": row["attempt"],
                        "committed": row["committed"],
                        "failed": row["failed"],
                    }
                    for row in case["rows"]
                ],
            )

    def test_checker_rejects_hidden_payload_queue_counter_and_opcode_mutations(self):
        case = self.case("four_routes_live_dependencies")
        for field in ("scheduler", "reorder", "queue", "counter", "opcode"):
            with self.subTest(field=field):
                rows = deepcopy(case["rows"])
                if field == "scheduler":
                    rows[-1]["after"]["scheduler"][0]["deadline"] ^= 1
                elif field == "reorder":
                    rows[-1]["after"]["reorder"][0]["key"] ^= 1
                elif field == "queue":
                    target = next(
                        row for row in rows if row["after"]["queues"]["incoming"]
                    )
                    target["after"]["queues"]["incoming"][0]["opcode"] ^= 1
                elif field == "counter":
                    rows[-1]["after"]["pops"]["scheduled"] += 1
                else:
                    rows[-1]["after"]["received"][-1]["opcode"] ^= 1
                with self.assertRaises(AssertionError):
                    check_case(case, rows)

    def test_checker_rejects_dropped_intermediate_observation_and_public_old_q(self):
        case = self.case("four_routes_live_dependencies")
        rows = deepcopy(case["rows"])
        target = next(
            row for row in rows if flow_counters(row)["observation_delta"]["scheduled"]
        )
        target["after"]["observations"]["scheduled"][-1]["opcode"] ^= 1
        with self.assertRaises(AssertionError):
            check_case(case, rows)
        rows = deepcopy(case["rows"])
        rows[0]["public"] = deepcopy(rows[0]["public_expected"])
        rows[0]["public"]["ready"] = False
        with self.assertRaises(AssertionError):
            check_case(case, rows)

    def test_payload_cannot_manufacture_invalid_fourth_route_or_256th_key(self):
        for kwargs in (
            {"route": 4},
            {"sequence_id": 256},
            {"cycles": 65536},
            {"value": MASK64 + 1},
        ):
            with self.subTest(kwargs=kwargs), self.assertRaises(AssertionError):
                item(**kwargs)
        self.assertEqual(item(255, 255, 3, 255, 65535, MASK64)["route"], 3)

    def test_exact_at_one_countdown_matches_unit_epoch_timing_with_native_order(self):
        for name in ("four_routes_live_dependencies", "lowest_key_issue"):
            case = self.case(name)
            model = Model(case["initial"], "countdown_at_one")
            for row in case["rows"]:
                self.assertEqual(model.step(row["actions"])["after"], row["after"])

    def test_retired_pyc_policy_witness_is_ordering_not_false_countdown_precision(self):
        case = self.case("lowest_key_issue")
        model = Model(case["initial"], "retired_pyc_scheduler_policy")
        for row in case["rows"]:
            actual = model.step(row["actions"])
            if actual["after"] != row["after"]:
                native_issue = row["grants"]["dependency"]["issue"]
                retired_issue = actual["grants"]["dependency"]["issue"]
                self.assertEqual(
                    [
                        row["before"]["scheduler"][index]["item"]["sequence_id"]
                        for index in native_issue
                    ],
                    [1],
                )
                self.assertEqual(
                    [
                        row["before"]["scheduler"][index]["item"]["sequence_id"]
                        for index in retired_issue
                    ],
                    [3],
                )
                break
        else:
            self.fail("retired physical-slot priority must have a real counterexample")


if __name__ == "__main__":
    unittest.main()
