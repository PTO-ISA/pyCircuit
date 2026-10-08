"""Independent hostile variants and explicit physical-state witnesses."""

import argparse
import json
from pathlib import Path

from generate import data
from models import Model, packed_snapshot

RIVALS = (
    "first_slot_issue",
    "first_slot_retire",
    "countdown_at_zero",
    "retired_pyc_scheduler_policy",
    "u8_next_key",
    "persistent_completed",
    "persistent_seen",
    "forward_completion",
    "busy_until_done",
    "forward_admission",
    "reuse_retiring_scheduler_slot",
    "validate_full_scheduler",
    "reuse_retiring_reorder_slot",
    "validate_full_reorder",
    "forward_reorder_admission",
    "priority_merge",
    "merge_cursor_on_stall",
    "route_requires_all_space",
    "future_pop_capacity",
    "drop_opcode",
    "wrong_transform",
    "observe_every_held_head",
    "ignore_zero_cost",
    "ignore_dependency_duplicate",
    "ignore_reorder_stale",
    "ignore_reorder_duplicate",
    "partial_commit_failure",
)
PHYSICAL = ("epoch", "queues", "scheduler", "reorder", "cursor", "next_key")
PREFERRED = {
    "retired_pyc_scheduler_policy": "lowest_key_issue",
    "first_slot_issue": "lowest_key_issue",
    "first_slot_retire": "full_topology_backpressure",
    "u8_next_key": "full_byte_domain_no_reorder_wrap",
    "persistent_completed": "late_retired_predecessor",
    "persistent_seen": "live_key_reuse_reorder_stale",
    "reuse_retiring_scheduler_slot": "full_topology_backpressure",
    "validate_full_scheduler": "scheduler_deferred_zero_cost",
    "reuse_retiring_reorder_slot": "full64_deferred_stale",
    "validate_full_reorder": "full64_missing_zero_duplicate_stall",
    "priority_merge": "full_topology_backpressure",
    "route_requires_all_space": "full_topology_backpressure",
    "observe_every_held_head": "full_topology_backpressure",
    "ignore_zero_cost": "scheduler_deferred_zero_cost",
    "ignore_dependency_duplicate": "scheduler_deferred_duplicate",
    "ignore_reorder_stale": "live_key_reuse_reorder_stale",
    "ignore_reorder_duplicate": "reorder_live_duplicate",
    "partial_commit_failure": "scheduler_deferred_duplicate",
}


def witnesses(result=None):
    result = result or data()
    found = {}
    for rival in RIVALS:
        candidates = list(result["executable"].items())
        preference = PREFERRED.get(rival)
        if preference:
            candidates.sort(key=lambda pair: pair[0] != preference)
        candidates += list(result["reference"].items())
        for name, case in candidates:
            model = Model(case["initial"], rival)
            for row in case["rows"]:
                actual = model.step(row["actions"], row["label"])
                changed = [
                    key for key in PHYSICAL if actual["after"][key] != row["after"][key]
                ]
                flow_changed = (
                    actual["after"]["observations"] != row["after"]["observations"]
                )
                if changed or flow_changed:
                    found[rival] = {
                        "case": name,
                        "executable": case["executable"],
                        "attempt": row["attempt"],
                        "label": row["label"],
                        "changed_physical_fields": changed,
                        "changed_observations": flow_changed,
                        "reference_packed_after": packed_snapshot(row["after"]),
                        "rival_packed_after": packed_snapshot(actual["after"]),
                    }
                    break
            if rival in found:
                break
        assert rival in found, f"no counterexample for {rival}"
    return found


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = witnesses()
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(  # noqa: T201 - CLI receipt
        json.dumps(
            {
                "model_rivals": len(result),
                "reference_only": [
                    name
                    for name, witness in result.items()
                    if not witness["executable"]
                ],
            }
        )
    )  # noqa: T201 - CLI receipt


if __name__ == "__main__":
    main()
