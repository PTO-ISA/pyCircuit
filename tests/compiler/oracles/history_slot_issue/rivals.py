"""Adversarial algorithm variants; these are test models, never DUT inputs."""

import argparse
import json
from pathlib import Path

from generate import data
from models import Model, packed_snapshot

MAILBOX_RIVALS = (
    "mailbox_output_replace",
    "mailbox_release_without_output",
    "mailbox_release_refill",
    "mailbox_capture_forward",
    "mailbox_clear_payload",
    "mailbox_shared_state",
)
ISSUE_RIVALS = (
    "issue_payload_valid_guard",
    "issue_capture_requires_free",
    "issue_capture_forward",
    "issue_wakeup_whole_entry_last_writer",
    "issue_forward_wakeup",
    "issue_src0_only_ready",
    "issue_first_instead_of_oldest",
    "issue_reverse_ties",
    "issue_serialize_wakeup",
    "issue_serialize_allocation",
    "issue_output_replace_for_take",
    "issue_read_new_candidate",
    "issue_clear_requires_output_space",
    "issue_refill_cleared_slot",
    "issue_force_allocated_valid",
    "issue_partial_allocation",
    "issue_forward_wakeup_to_allocation",
    "issue_drop_full_allocation",
    "issue_slot_release_refill",
    "issue_slot_clear_payload",
)


def witnesses(cases=None):
    cases = cases or data()["executable"]
    found = {}
    for rival in (*MAILBOX_RIVALS, *ISSUE_RIVALS):
        kind = "mailbox" if rival.startswith("mailbox_") else "issue"
        for name, case in cases.items():
            if case["kind"] != kind:
                continue
            model = Model(kind, case["initial"]["entries"], case["rows"][0]["epoch"])
            assert (
                model.snapshot() == case["initial"]
            ), "rivals start from the same reset state"
            for row in case["rows"]:
                actual = model.step(row["actions"], row["label"], rival)
                wanted_bits = packed_snapshot(kind, row["after"])[1]
                rival_bits = packed_snapshot(kind, actual["after"])[1]
                if wanted_bits != rival_bits:
                    found[rival] = {
                        "case": name,
                        "epoch": row["epoch"],
                        "label": row["label"],
                        "reference_packed_after": wanted_bits,
                        "rival_packed_after": rival_bits,
                        "snapshot_bits": case["snapshot_bits"],
                    }
                    break
            if rival in found:
                break
        assert rival in found, f"no complete physical-state counterexample for {rival}"
    return found


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = witnesses()
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(  # noqa: T201
        json.dumps({"model_rivals": len(result), "all_observable": True})
    )


if __name__ == "__main__":
    main()
