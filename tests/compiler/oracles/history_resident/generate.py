"""Serialize complete independent historical resident scenarios.

Only test-owned models determine waits and expected snapshots. Generated data
is handed to the ordinary source-unit bench/harness; it never enters the DUT.
"""

import argparse
import hashlib
import json
from copy import deepcopy
from pathlib import Path

from models import ResidentModel, completion, entry, event, packed_snapshot, readiness


class Program:
    def __init__(self, kind):
        self.model = ResidentModel(kind)
        self.rows = []
        self.tags = {}

    def step(self, label, **actions):
        row = self.model.step(actions, label)
        for phase in ("before", "work", "after"):
            row[f"packed_{phase}"] = packed_snapshot(self.model.kind, row[phase])[1]
        self.rows.append(row)
        return row

    def wait(self, predicate, label):
        for _ in range(32):
            if predicate():
                return
            self.step(label)
        raise AssertionError(f"independent bounded wait failed: {label}")

    def wait_queue(self, name):
        self.wait(lambda: bool(self.model.queues[name]), f"wait-{name}")

    def wait_empty(self, name):
        self.wait(lambda: not self.model.queues[name], f"wait-empty-{name}")

    def offer(self, name, value, label=None):
        self.step(
            label or f"offer-{name}-{value.get('value', value.get('tag', 0))}",
            offer={name: value},
        )

    def take(self, name, expected):
        self.wait_queue(name)
        value = self.model.queues[name][0]
        assert value["value"] == expected, (name, expected, value)
        self.step(f"take-{name}-{expected}", take=[name])
        return value

    def allocate(self, value, side="left"):
        self.offer(f"{side}_allocate", event(value))
        self.wait_queue(f"{side}_allocated")
        tag = self.model.queues[f"{side}_allocated"][0]
        self.tags[value] = tag.copy()
        return tag

    def complete(self, tag, side="left"):
        self.offer(
            f"{side}_completion", completion(tag), f"complete-{side}-{tag['value']}"
        )
        self.wait_empty(f"{side}_completion")

    def ready(self, tag, ready=True, side="left"):
        self.offer(f"{side}_readiness", readiness(tag, ready))
        self.wait(
            lambda: self.model.instances[side]["ready"][tag] == ready,
            f"wait-ready-{side}-{tag}-{int(ready)}",
        )

    def dispatch(self, value, age, tag, side="left", src1=None):
        self.offer(f"{side}_request", entry(value, age, tag, src1))
        self.wait_empty(f"{side}_request")


def rob_history():
    p = Program("rob")
    p.step(
        "offer-two-instances",
        offer={"left_allocate": event(10), "right_allocate": event(100)},
    )
    p.wait(
        lambda: p.model.queues["left_allocated"] and p.model.queues["right_allocated"],
        "wait-two-instance-allocation",
    )
    left10 = p.model.queues["left_allocated"][0].copy()
    right100 = p.take("right_allocated", 100)
    p.complete(right100, "right")
    p.take("right_retired", 100)
    p.offer("left_allocate", event(20))
    p.step("held-allocation-output")
    assert p.model.instances["left"]["tail"] == p.model.instances["left"]["count"] == 1
    assert p.model.queues["left_allocate"]
    p.take("left_allocated", 10)
    p.wait_queue("left_allocated")
    left20 = p.take("left_allocated", 20)
    assert left20["index"] == 1
    left30 = p.allocate(30)
    p.take("left_allocated", 30)
    left40 = p.allocate(40)
    p.take("left_allocated", 40)
    assert (
        p.model.instances["left"]["tail"] == 0
        and p.model.instances["left"]["count"] == 4
    )
    p.offer("left_allocate", event(50))
    p.step("full-resident-input-hold")
    assert p.model.queues["left_allocate"] and p.model.instances["left"]["count"] == 4
    p.complete(left30)
    p.complete(left10)
    p.wait_queue("left_retired")
    assert (
        p.model.instances["left"]["head"] == 1
        and p.model.instances["left"]["count"] == 3
    )
    p.wait_queue("left_allocated")
    left50 = p.take("left_allocated", 50)
    assert left50["index"] == 0 and left50["generation"] == 2
    p.complete(left10)
    stale_rows = [row for row in p.rows[-2:] if "complete" in row["grants"]["left"]]
    assert stale_rows and all(not row["writes"]["left"] for row in stale_rows)
    assert not p.model.instances["left"]["entries"][0]["done"]
    for tag in (left40, left20, left50):
        p.complete(tag)
    held = p.model.snapshot()
    for _ in range(3):
        p.step("held-retirement-output")
        assert p.model.snapshot() == held
    for value in (10, 20, 30, 40, 50):
        p.take("left_retired", value)
    assert (
        p.model.instances["left"]["head"] == 1
        and p.model.instances["left"]["count"] == 0
    )
    left60 = p.allocate(60)
    p.take("left_allocated", 60)
    p.offer("left_flush", event())
    p.wait_empty("left_flush")
    s = p.model.instances["left"]
    assert (s["head"], s["tail"], s["count"], s["epoch"]) == (2, 2, 0, 1)
    p.complete(left60)
    assert not p.model.instances["left"]["entries"][1]["done"]
    left70 = p.allocate(70)
    assert left70["index"] == 2 and left70["epoch"] == 1
    p.take("left_allocated", 70)
    p.complete(left70)
    p.take("left_retired", 70)
    s = p.model.instances["right"]
    assert (s["head"], s["tail"], s["count"], s["epoch"]) == (1, 1, 0, 0)
    assert p.model.instances["left"]["count"] == 0
    return p


def isq_history(*, regular_clock=False):
    p = Program("isq")
    p.step(
        "readiness-and-dispatch-offer",
        offer={"left_readiness": readiness(1), "left_request": entry(100, 1, 1)},
    )
    p.wait_queue("left_issued")
    assert p.model.queues["left_issued"][0]["value"] == 100
    p.ready(5)
    p.dispatch(300, 3, 5)
    p.step(
        "same-epoch-lost-wakeup-offer",
        offer={"left_readiness": readiness(9), "left_request": entry(400, 4, 9)},
    )
    p.wait(
        lambda: not p.model.queues["left_readiness"]
        and not p.model.queues["left_request"],
        "wait-readiness-and-dispatch",
    )
    p.dispatch(200, 2, 7)
    p.step("resident-not-ready-hold")
    p.step("resident-not-ready-hold")
    assert not p.model.instances["left"]["ready"][7]
    p.ready(7)
    p.step(
        "fill-fourth-resident-offer",
        offer={"left_readiness": readiness(11), "left_request": entry(500, 5, 11)},
    )
    p.wait(
        lambda: not p.model.queues["left_readiness"]
        and not p.model.queues["left_request"],
        "wait-fourth-resident",
    )
    held = p.model.snapshot()
    assert sum(e["valid"] for e in p.model.instances["left"]["entries"]) == 4
    for _ in range(3):
        p.step("full-output-holds-all-residents")
        assert p.model.snapshot() == held
    p.offer("left_request", entry(600, 6, 1))
    p.step("full-resident-input-hold")
    p.step("full-resident-input-hold")
    assert p.model.queues["left_request"]
    p.dispatch(900, 1, 5, "right")
    p.step("right-readiness-isolation")
    p.step("right-readiness-isolation")
    assert (
        not p.model.queues["right_issued"]
        and not p.model.instances["right"]["ready"][5]
    )
    p.offer("right_readiness", readiness(5))
    p.take("right_issued", 900)
    if regular_clock:
        p.step(
            "ordinary-clear-offer-on-held-output-take-edge",
            take=["left_issued"],
            offer={"left_readiness": readiness(7, False)},
        )
        clear = p.step("ordinary-ready-clear-competes-with-old-eligible-issue")
    else:
        p.take("left_issued", 100)
        clear = p.step(
            "committed-ready-clear-competes-with-issue",
            inject={"left_readiness": readiness(7, False)},
        )
    assert (
        clear["grants"]["left"] == ["update_ready"]
        and not clear["after"]["queues"]["left_issued"]
    )
    if regular_clock:
        restore = p.step(
            "ordinary-restore-offer-observes-old-cleared-ready",
            offer={"left_readiness": readiness(7)},
        )
        assert restore["grants"]["left"] == ["issue"]
        assert restore["after"]["queues"]["left_issued"][0]["value"] == 300
        restore = p.step("ordinary-restore-commits-behind-held-output")
        assert restore["grants"]["left"] == ["update_ready", "dispatch"]
        order = (300, 200, 400, 500, 600)
    else:
        restore = p.step(
            "committed-ready-set-competes-with-issue",
            inject={"left_readiness": readiness(7)},
        )
        assert (
            restore["grants"]["left"] == ["update_ready"]
            and not restore["after"]["queues"]["left_issued"]
        )
        order = (200, 300, 400, 500, 600)
    for i, value in enumerate(order):
        p.wait_queue("left_issued")
        if i == 0:
            p.step("issued-output-held-after-first-drain")
            p.step("issued-output-held-after-first-drain")
            assert sum(e["valid"] for e in p.model.instances["left"]["entries"]) == 4
        p.take("left_issued", value)
    p.ready(12)
    p.ready(12, False)
    p.dispatch(700, 7, 12)
    p.step("reused-tag-cleared-before-dispatch")
    p.step("reused-tag-cleared-before-dispatch")
    assert not p.model.queues["left_issued"]
    p.offer("left_readiness", readiness(12))
    p.take("left_issued", 700)
    for i, tag in enumerate((20, 21, 22, 63)):
        p.ready(tag)
        p.dispatch(800 + i, 20 + i, tag)
    assert sum(e["valid"] for e in p.model.instances["left"]["entries"]) == 3
    p.take("left_issued", 800)
    for i in range(3):
        p.offer("left_readiness", readiness(40 + i, bool(i & 1)))
        assert p.model.queues[
            "left_issued"
        ], "unrelated readiness must not serialize issue"
        p.take("left_issued", 801 + i)
    assert not any(e["valid"] for s in p.model.instances.values() for e in s["entries"])
    assert not p.model.queues["left_request"]
    return p


def rob_reservations():
    """Reach each challenged ROB grant from reset using ordinary host actions."""
    p = Program("rob")
    tags = {}
    for value in (10, 20, 30, 40):
        tags[value] = p.allocate(value)
        p.take("left_allocated", value)
    p.complete(tags[10])
    p.wait_queue("left_retired")
    for value in (20, 30, 40):
        p.complete(tags[value])
    wrong = completion(tags[20])
    wrong["generation"] = 0
    p.step(
        "preload-stale-completion-and-drain-held-output",
        offer={"left_completion": wrong},
        take=["left_retired"],
    )
    row = p.step("witness-stale-same-head-completion-and-retire-cofire")
    assert row["grants"]["left"] == ["complete", "retire"]
    assert row["after"]["queues"]["left_retired"][0]["value"] == 20
    # Now head 2 is done; matching completion must lock its retirement.
    p.step(
        "preload-matching-head-completion-and-drain-output",
        offer={"left_completion": completion(tags[30])},
        take=["left_retired"],
    )
    row = p.step("witness-matching-head-completion-blocks-retire")
    assert row["grants"]["left"] == ["complete"]
    p.wait_queue("left_retired")
    assert p.model.queues["left_retired"][0]["value"] == 30
    # Matching completion of a different slot does not lock head retirement.
    p.step(
        "preload-other-slot-completion-and-drain-output",
        offer={"left_completion": completion(tags[30])},
        take=["left_retired"],
    )
    row = p.step("witness-other-slot-completion-and-retire-cofire")
    assert row["grants"]["left"] == ["complete", "retire"]
    assert row["after"]["queues"]["left_retired"][0]["value"] == 40
    p.take("left_retired", 40)
    left50 = p.allocate(50)
    p.take("left_allocated", 50)
    p.step(
        "preload-allocation-and-completion",
        offer={"left_allocate": event(60), "left_completion": completion(left50)},
    )
    row = p.step("witness-allocation-epoch-identity-write-blocks-completion")
    assert row["grants"]["left"] == ["allocate"]
    assert row["after"]["queues"]["left_completion"]
    p.wait_empty("left_completion")
    p.wait_queue("left_retired")
    p.take("left_allocated", 60)
    p.take("left_retired", 50)
    return p


def isq_reservations():
    """Reach invalid src1 and unrelated-write witnesses without seeding DUT state."""
    p = Program("isq")
    for tag in (1, 2, 3, 5, 7, 40, 63):
        p.ready(tag)
    p.dispatch(100, 1, 1)
    p.wait_queue("left_issued")
    p.dispatch(200, 2, 5, src1=7)
    p.dispatch(300, 3, 2)
    p.dispatch(400, 4, 3)
    p.dispatch(90, 0, 40, src1=63)
    assert sum(e["valid"] for e in p.model.instances["left"]["entries"]) == 4
    p.take("left_issued", 100)
    p.take("left_issued", 90)
    e = p.model.instances["left"]["entries"][3]
    assert not e["valid"] and e["src0_tag"] == 40 and e["src1_tag"] == 63
    row = p.step(
        "witness-invalid-entry-src1-reservation-stalls-issue",
        inject={"left_readiness": readiness(63)},
    )
    assert row["grants"]["left"] == ["update_ready"]
    row = p.step(
        "witness-unrelated-ready-write-and-oldest-issue-cofire",
        inject={"left_readiness": readiness(62)},
    )
    assert row["grants"]["left"] == ["update_ready", "issue"]
    assert row["after"]["queues"]["left_issued"][0]["value"] == 200
    p.step(
        "preload-dispatch-ready-and-drain-output",
        take=["left_issued"],
        offer={"left_request": entry(500, 5, 1), "left_readiness": readiness(61)},
    )
    row = p.step("witness-dispatch-and-ready-cofire-issue-stalls")
    assert row["grants"]["left"] == ["update_ready", "dispatch"]
    p.wait_queue("left_issued")
    assert p.model.queues["left_issued"][0]["value"] == 300
    p.dispatch(600, 6, 1)
    p.dispatch(700, 7, 1)
    assert sum(e["valid"] for e in p.model.instances["left"]["entries"]) == 4
    p.step(
        "preload-request-while-full-and-drain-output",
        take=["left_issued"],
        offer={"left_request": entry(800, 8, 1)},
    )
    row = p.step("witness-full-resident-does-not-reuse-issued-slot")
    assert row["grants"]["left"] == ["issue"]
    assert row["after"]["queues"]["left_request"]
    row = p.step("witness-freed-slot-reused-on-following-edge")
    assert row["grants"]["left"] == ["dispatch"]
    for value in (400, 500, 600, 700, 800):
        p.take("left_issued", value)
    return p


def data():
    root = (
        Path(__file__).resolve().parents[2]
        / "lit/Source/Inputs/history-resident/baseline"
    )
    manifest = json.loads((root / "manifest.json").read_text())
    for asset in manifest["assets"]:
        assert (
            hashlib.sha256((root / asset["asset"]).read_bytes()).hexdigest()
            == asset["sha256"]
        )
    cases = {}
    for kind, generator in (("rob", rob_history), ("isq", isq_history)):
        p = generator()
        assert len(p.rows) < 256
        cases[kind] = {
            "snapshot_bits": 974 if kind == "rob" else 616,
            "epoch_limit": 256,
            "executed_epochs": len(p.rows),
            "rows": p.rows,
        }
    witnesses = {}
    for kind, generator in (("rob", rob_reservations), ("isq", isq_reservations)):
        p = generator()
        assert len(p.rows) < 256
        witnesses[kind] = {
            "snapshot_bits": 974 if kind == "rob" else 616,
            "epoch_limit": 256,
            "executed_epochs": len(p.rows),
            "rows": p.rows,
        }
    system_isq = isq_history(regular_clock=True)
    systems = {
        "rob": deepcopy(cases["rob"]),
        "isq": {
            "snapshot_bits": 616,
            "epoch_limit": 256,
            "executed_epochs": len(system_isq.rows),
            "rows": system_isq.rows,
        },
    }
    return {
        "schema": 1,
        "provenance": manifest,
        "cases": cases,
        "witnesses": witnesses,
        "systems": systems,
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
        compact = {
            "schema": result["schema"],
            "provenance": result["provenance"],
            "cases": {},
            "witnesses": {},
            "systems": {},
        }
        for group in ("cases", "witnesses", "systems"):
            for kind, case in result[group].items():
                compact[group][kind] = {
                    key: value for key, value in case.items() if key != "rows"
                }
                compact[group][kind]["rows"] = [
                    {
                        key: value
                        for key, value in row.items()
                        if key not in ("before", "work", "after")
                    }
                    for row in case["rows"]
                ]
        args.vectors.write_text(json.dumps(compact, indent=2) + "\n")
    print(  # noqa: T201 - command-line receipt
        json.dumps(
            {
                group: {
                    kind: case["executed_epochs"]
                    for kind, case in result[group].items()
                }
                for group in ("cases", "witnesses", "systems")
            }
        )
    )


if __name__ == "__main__":
    main()
