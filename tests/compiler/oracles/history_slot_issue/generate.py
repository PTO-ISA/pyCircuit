"""Complete historical and reset-reachable slot/issue vector handoff."""

import argparse
import hashlib
import json
from copy import deepcopy
from pathlib import Path

from models import Model, entry, event, expected_controls, packed_snapshot, wakeup


class Program:
    def __init__(self, kind, initial_entries=None, first_epoch=0):
        self.model = Model(kind, initial_entries, first_epoch)
        self.initial = self.model.snapshot()
        self.rows = []

    def step(self, label, drain=False, **actions):
        if drain:
            assert "sink" not in actions and "take" not in actions
            actions["sink"] = list(self.model.received)
        row = self.model.step(actions, label)
        for phase in ("before", "work", "after"):
            row[f"packed_{phase}"] = packed_snapshot(self.model.kind, row[phase])[1]
        row["controls"] = expected_controls(self.model.kind, row)
        self.rows.append(row)
        return row

    def wait(self, predicate, label, drain=False):
        for _ in range(32):
            if predicate():
                return
            self.step(label, drain=drain)
        raise AssertionError(f"bounded independent wait failed: {label}")

    def offer(self, name, value, drain=False):
        return self.step(f"offer-{name}", drain=drain, offer={name: [value]})

    def install(self, value):
        self.offer("allocations", value)
        self.wait(
            lambda: value in self.model.entries, f"wait-install-age-{value['age']}"
        )

    def wake(self, tag, payload_valid=False):
        self.offer("wakeups", wakeup(tag, payload_valid))
        self.wait(
            lambda: (
                not self.model.queues["wakeups"]
                and not self.model.slots["wakeup"]["valid"]
            ),
            f"wait-wakeup-{tag}",
        )

    def take(self, name):
        self.wait(lambda: bool(self.model.queues[name]), f"wait-{name}")
        return self.step(f"take-{name}", take=[name])


def mailbox_original():
    # These are paired projections of two separately executed flat/nested
    # original fixtures: committed input7, six epochs starting at tick1.
    p = Program("mailbox", first_epoch=1)
    for tick in range(1, 7):
        actions = (
            {"inject": {"left_input": [event(7)], "right_input": [event(7)]}}
            if tick == 1
            else {}
        )
        p.step(f"original-flat-and-nested-cycle-{tick}", drain=True, **actions)
        if tick == 1:
            assert (
                not p.model.received["left_output"]
                and not p.model.received["right_output"]
            )
        if tick >= 3:
            assert p.model.received == {
                "left_output": [event(7)],
                "right_output": [event(7)],
            }
    return p


def mailbox_reset_reachable():
    p = Program("mailbox")
    p.step(
        "ordinary-offer-before-original-six-cycles",
        offer={"left_input": [event(7)], "right_input": [event(7)]},
    )
    for tick in range(1, 7):
        p.step(f"reset-reachable-flat-and-nested-cycle-{tick}", drain=True)
    assert p.model.received == {"left_output": [event(7)], "right_output": [event(7)]}
    return p


def mailbox_backpressure():
    p = Program("mailbox")
    p.step(
        "offer-independent-first-payloads",
        offer={"left_input": [event(11)], "right_input": [event(101)]},
    )
    p.step("capture-both-old-boundary-heads")
    p.step(
        "publish-first-and-offer-next",
        offer={"left_input": [event(22)], "right_input": [event(102)]},
    )
    p.step("capture-residents-behind-full-outputs")
    p.step(
        "fill-last-boundary-capacity",
        offer={"left_input": [event(33)], "right_input": [event(103)]},
    )
    held = p.model.snapshot()
    for _ in range(4):
        p.step("both-output-full-slot-and-input-held")
        assert p.model.snapshot() == held
    p.take("right_output")
    p.step("right-publish-102-left-still-held")
    assert p.model.queues["right_output"] == [event(102)]
    assert p.model.queues["left_output"] == [event(11)]
    p.take("right_output")
    p.step("right-publish-103-after-slot-capture")
    p.take("right_output")
    for value in (11, 22, 33):
        p.wait(lambda: bool(p.model.queues["left_output"]), "wait-left-output")
        assert p.model.queues["left_output"] == [event(value)]
        p.take("left_output")
    for _ in range(3):
        p.step("retained-payloads-after-complete-drain")
    assert p.model.received == {
        "left_output": [event(11), event(22), event(33)],
        "right_output": [event(101), event(102), event(103)],
    }
    assert p.model.slots == {
        "left": {"valid": False, "payload": event(33)},
        "right": {"valid": False, "payload": event(103)},
    }
    return p


def initial_issue_entries():
    return [
        entry(i, 7 if i == 0 else i + 10, 7 if i == 0 else i + 10, valid=True)
        for i in range(4)
    ]


def issue_body(p, label_prefix):
    baseline_pops = p.model.pops["allocations"]
    body_start = p.model.epoch
    for tick in range(30):
        actions = {}
        if tick == 0:
            actions["offer"] = {
                "allocations": [
                    entry(99, 20, 21, valid=True, src0_ready=True, src1_ready=True)
                ]
            }
        if tick == 6:
            actions["offer"] = {"wakeups": [wakeup(7, True)]}
        p.step(f"{label_prefix}-tick-{tick}", drain=True, **actions)
        if tick == 5:
            assert p.model.pops["allocations"] - baseline_pops == 1
            assert p.model.entries == initial_issue_entries()
            assert p.model.slots["allocation"]["valid"]
    assert p.model.pops["allocations"] - baseline_pops == 1
    ages = [value["age"] for value in p.model.received["output"]]
    assert ages.count(0) == ages.count(99) == 1 and ages == [0, 99]
    assert not any(value["valid"] and value["age"] == 99 for value in p.model.entries)
    return body_start


def issue_original():
    p = Program("issue", initial_issue_entries())
    issue_body(p, "original-heterogeneous-host-initializeEntry")
    return p


def issue_reset_reachable():
    p = Program("issue")
    for i, value in enumerate(initial_issue_entries()):
        p.install(value)
        assert p.model.entries[i] == value
    assert p.model.entries == initial_issue_entries()
    assert p.model.pops["allocations"] == 4
    body_start = issue_body(p, "reset-reachable-original-body")
    assert p.model.pops["allocations"] == 5
    return p, body_start


def issue_wakeup_flags():
    p = Program("issue")
    p.install(entry(9, 7, 8, valid=True))
    p.wake(7, False)
    assert p.model.entries[0]["src0_ready"] and not p.model.entries[0]["src1_ready"]
    assert not p.model.queues["output"]
    p.wake(8, False)
    assert p.model.entries[0]["src0_ready"] and p.model.entries[0]["src1_ready"]
    assert not p.model.queues[
        "output"
    ], "wakeup must not forward to same-edge selection"
    p.take("output")
    assert p.model.received["output"] == [
        entry(9, 7, 8, valid=True, src0_ready=True, src1_ready=True)
    ]
    return p


def issue_allocation_no_forward():
    p = Program("issue")
    p.step(
        "offer-allocation-and-wakeup-same-edge",
        offer={
            "allocations": [entry(17, 7, 7, valid=True)],
            "wakeups": [wakeup(7, False)],
        },
    )
    p.step("capture-inputs-before-any-field-use")
    row = p.step("install-and-wakeup-old-empty-table")
    assert row["grants"]["allocated"] == 0
    assert row["grants"]["wakeup_src0"] == row["grants"]["wakeup_src1"] == []
    assert not p.model.entries[0]["src0_ready"] and not p.model.entries[0]["src1_ready"]
    for _ in range(4):
        p.step("no-global-readiness-or-forwarding-after-old-wakeup")
        assert not p.model.queues["output"]
    p.wake(7, False)
    assert p.model.entries[0]["src0_ready"] and p.model.entries[0]["src1_ready"]
    p.take("output")
    assert p.model.received["output"][0]["age"] == 17
    return p


def issue_full_slots_and_output():
    p = Program("issue")
    for value in initial_issue_entries():
        p.install(value)
    p.offer(
        "allocations", entry(99, 20, 21, valid=True, src0_ready=True, src1_ready=True)
    )
    p.step("capture-allocation-boundary-even-table-full")
    assert p.model.pops["allocations"] == 5 and p.model.slots["allocation"]["valid"]
    p.offer("allocations", entry(100, 22, 23, valid=True))
    p.offer("allocations", entry(101, 24, 25, valid=True))
    assert len(p.model.queues["allocations"]) == 2
    held = p.model.snapshot()
    for _ in range(4):
        p.step("full-table-allocation-slot-and-depth2-boundary-held")
        assert p.model.snapshot() == held
    p.wake(7, False)
    row = p.step("issue-old-full-table-no-same-edge-install")
    assert row["grants"]["selected"] == 0 and row["grants"]["allocated"] is None
    assert p.model.slots["allocation"]["valid"]
    row = p.step("install-slot99-on-following-old-free-edge")
    assert row["grants"]["allocated"] == 0
    assert p.model.queues["allocations"][0]["age"] == 100
    assert not p.model.slots["allocation"]["valid"]
    row = p.step("output-full-still-clears-selected99-and-captures100")
    assert row["grants"]["selected"] == 0 and row["grants"]["valid_clear"]
    assert not row["grants"]["output_push"]
    assert p.model.queues["output"][0]["age"] == 0
    assert p.model.slots["allocation"]["payload"]["age"] == 100
    p.step("install100-after99-clear")
    p.step("capture101-with-separate-empty-slot")
    p.step("install101-in-old-next-free-slot")
    p.take("output")
    assert [value["age"] for value in p.model.received["output"]] == [0]
    assert not any(value["valid"] and value["age"] == 99 for value in p.model.entries)
    assert p.model.pops["allocations"] == 7
    return p


def issue_oldest_ties_and_pop_bubble():
    p = Program("issue")
    for value in (
        entry(8, 7, 7, valid=True),
        entry(3, 7, 7, valid=True),
        entry(3, 7, 7, valid=True),
    ):
        p.install(value)
    p.wake(7, False)
    row = p.step("oldest-ready-first-stable-index1")
    assert row["grants"]["selected"] == 1
    row = p.step("physical-pop-does-not-give-future-space-to-read", take=["output"])
    assert row["grants"]["selected"] == 2 and row["grants"]["valid_clear"]
    assert not row["grants"]["output_push"]
    assert not p.model.queues["output"]
    row = p.step("remaining-older-age8-after-clear-loss")
    assert row["grants"]["selected"] == 0 and row["grants"]["output_push"]
    p.take("output")
    assert [value["age"] for value in p.model.received["output"]] == [3, 8]
    return p


def issue_independent_fields():
    p = Program("issue")
    p.install(entry(2, 11, 11, valid=True))
    p.offer(
        "allocations", entry(1, 20, 21, valid=True, src0_ready=True, src1_ready=True)
    )
    p.offer("wakeups", wakeup(11, False))
    p.offer("allocations", entry(30, 30, 31, valid=True))
    assert p.model.entries[1]["age"] == 1 and p.model.slots["wakeup"]["valid"]
    row = p.step("wakeup-other-entry-and-old-ready-issue-cofire")
    assert row["grants"]["wakeup_src0"] == row["grants"]["wakeup_src1"] == [0]
    assert row["grants"]["selected"] == 1 and row["grants"]["capture"] == ["allocation"]
    row = p.step("old-ready-clear-and-different-free-allocation-cofire")
    assert row["grants"]["selected"] == 0 and row["grants"]["allocated"] == 1
    assert not row["grants"]["output_push"] and row["grants"]["valid_clear"]
    assert p.model.entries[1] == entry(30, 30, 31, valid=True)
    p.take("output")
    assert p.model.received["output"][0]["age"] == 1
    return p


def issue_full_replacement():
    p = Program("issue")
    p.install(entry(4, 17, 19, valid=True, src0_ready=True, src1_ready=True))
    p.take("output")
    assert not p.model.entries[0]["valid"] and p.model.entries[0]["src0_ready"]
    replacement = entry(33, 51, 53, valid=False, src0_ready=False, src1_ready=False)
    p.install(replacement)
    assert p.model.entries[0] == replacement
    for _ in range(4):
        p.step("invalid-allocation-keeps-fresh-fields-and-no-output")
        assert not p.model.queues["output"]
    assert not p.model.slots["allocation"]["valid"]
    assert p.model.slots["allocation"]["payload"] == replacement
    return p


def descriptor(p, scope, **metadata):
    assert len(p.rows) <= 256
    return dict(
        kind=p.model.kind,
        scope=scope,
        epoch_limit=256,
        snapshot_bits=54 if p.model.kind == "mailbox" else 250,
        initial=p.initial,
        executed_epochs=len(p.rows),
        rows=p.rows,
        **metadata,
    )


def data():
    baseline = (
        Path(__file__).resolve().parents[2]
        / "lit/Source/Inputs/history-slot-issue/baseline"
    )
    manifest = json.loads((baseline / "manifest.json").read_text())
    for asset in manifest["assets"]:
        assert (
            hashlib.sha256((baseline / asset["asset"]).read_bytes()).hexdigest()
            == asset["sha256"]
        )
    reset_issue, body_start = issue_reset_reachable()
    return {
        "schema": 1,
        "provenance": manifest,
        "original": {
            "mailbox": descriptor(
                mailbox_original(),
                "paired-projections-of-original-flat-and-nested-six-cycle-driver",
                compiled_gate=False,
            ),
            "issue": descriptor(
                issue_original(),
                "exact-original-heterogeneous-host-initializeEntry-30-ticks",
                compiled_gate=False,
            ),
        },
        "executable": {
            "mailbox_original_body": descriptor(
                mailbox_reset_reachable(),
                "ordinary-offer-plus-original-six-cycle-body",
                body_start_epoch=1,
            ),
            "mailbox_backpressure": descriptor(
                mailbox_backpressure(),
                "complete-two-instance-backpressure-and-retained-payload",
            ),
            "issue_original_body": descriptor(
                reset_issue,
                "reset-reachable-four-allocation-prelude-plus-original30-tick-body",
                body_start_epoch=body_start,
            ),
            "issue_wakeup_flags": descriptor(
                issue_wakeup_flags(), "payload-valid-ignored-and-both-operands-oldQ"
            ),
            "issue_allocation_no_forward": descriptor(
                issue_allocation_no_forward(), "no-forwarding-and-no-global-readiness"
            ),
            "issue_full_slots_and_output": descriptor(
                issue_full_slots_and_output(),
                "full-boundary-slot-table-and-output-selected-clear",
            ),
            "issue_oldest_ties": descriptor(
                issue_oldest_ties_and_pop_bubble(),
                "oldest-stable-ties-and-physical-output-bubble",
            ),
            "issue_independent_fields": descriptor(
                issue_independent_fields(),
                "independent-wakeup-issue-allocation-field-cofire",
            ),
            "issue_full_replacement": descriptor(
                issue_full_replacement(),
                "complete-replacement-retains-input-valid-and-clears-stale-ready",
            ),
        },
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    parser.add_argument("--vectors", type=Path)
    args = parser.parse_args()
    result = data()
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + "\n")
    if args.vectors:
        compact = deepcopy(result)
        for group in ("original", "executable"):
            for case in compact[group].values():
                for row in case["rows"]:
                    for phase in ("before", "work", "after"):
                        snapshot = row.pop(phase)
                        row[f"{phase}_counters"] = {
                            key: snapshot[key] for key in ("pops", "pushes", "received")
                        }
        args.vectors.write_text(json.dumps(compact, indent=2) + "\n")
    print(  # noqa: T201 - CLI receipt
        json.dumps(
            {
                group: {
                    name: case["executed_epochs"]
                    for name, case in result[group].items()
                }
                for group in ("original", "executable")
            }
        )
    )  # noqa: T201 - CLI receipt


if __name__ == "__main__":
    main()
