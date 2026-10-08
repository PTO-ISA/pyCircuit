"""Independent complete graph histories and literal action/state handoff."""

import argparse
import hashlib
import json
from copy import deepcopy
from pathlib import Path

from models import (
    DONE,
    MASK64,
    WAITING,
    Model,
    empty_state,
    flow_counters,
    frame,
    item,
    packed_snapshot,
    public_expected,
    transformed,
)


class Program:
    def __init__(self, initial=None):
        self.model = Model(initial)
        self.initial = self.model.snapshot()
        self.rows = []

    def step(self, label, **actions):
        row = self.model.step(actions, label)
        self.rows.append(row)
        return row

    def wait(self, condition, label, limit=1024, take=True):
        for _ in range(limit):
            if condition():
                return
            assert not self.model.failed, (label, self.rows[-1]["errors"])
            self.step(label, take=take)
        raise AssertionError(f"finite independent scenario exhausted: {label}")

    def offer(self, record, take=True):
        self.wait(
            lambda: len(self.model.state["queues"]["incoming"]) < 16,
            "input-capacity",
            take=take,
        )
        return self.step(f"offer-key-{record['sequence_id']}", offer=record, take=take)

    def drain(self, count):
        self.wait(
            lambda: len(self.model.state["received"]) == count, "complete-root-drain"
        )
        for _ in range(4):
            self.step("settled-empty-pipeline")
        assert all(not values for values in self.model.state["queues"].values())
        assert not any(entry["valid"] for entry in self.model.state["scheduler"])
        assert not any(entry["valid"] for entry in self.model.state["reorder"])


def descriptor(program, scope, executable=True, **metadata):
    for row in program.rows:
        row["frame"] = frame(row)
        row["public_expected"] = public_expected(row)
    return {
        "scope": scope,
        "executable": executable,
        "initial": program.initial,
        "executed_epochs": len(program.rows),
        "attempt_limit": 2048,
        "snapshot_bits": 21934,
        "packed_radix": 16,
        "rows": program.rows,
        "first_failure_attempt": next(
            (row["attempt"] for row in program.rows if row["failed"]), None
        ),
        "host_reset_attempts": [
            row["attempt"] for row in program.rows if row["actions"].get("reset")
        ],
        **metadata,
    }


def expected_output(record):
    return transformed(record, 102 + record["route"])


def four_routes():
    p = Program()
    records = [
        item(
            key,
            opcode=(key * 29 + 7) & 255,
            route=key % 4,
            waits_for=key - 1 if key in (1, 2, 3) else 255,
            cycles=12 if key == 0 else (key % 3) + 2,
            value=MASK64 - key if key % 2 == 0 else key * 1001,
        )
        for key in range(16)
    ]
    for record in records[:4]:
        p.offer(record)
    p.drain(4)
    for record in records[4:]:
        p.offer(record)
    p.drain(16)
    assert p.model.state["received"] == [expected_output(record) for record in records]
    return p


def issue_selection():
    p = Program()
    records = [
        item(0, 71, 0, 255, 24, 100),
        item(3, 73, 0, 255, 2, 300),
        item(1, 17, 0, 255, 2, 101),
        item(2, 29, 1, 255, 3, 201),
    ]
    for record in records:
        p.offer(record)
    p.drain(4)
    assert p.model.state["received"] == [
        expected_output(record)
        for record in sorted(records, key=lambda record: record["sequence_id"])
    ]
    return p


def full_backpressure():
    p = Program()
    records = [
        item(key, (key * 17) & 255, key % 4, 255, 1, MASK64 - key) for key in range(180)
    ]
    offered = 0
    for tick in range(360):
        actions = {"take": False}
        if offered < len(records) and len(p.model.state["queues"]["incoming"]) < 16:
            actions["offer"] = records[offered]
            offered += 1
        p.step(f"paused-output-fill-{tick}", **actions)
    held = p.model.snapshot()
    assert len(held["queues"]["incoming"]) == 16
    assert len(held["queues"]["prepared"]) == 4
    assert len(held["queues"]["scheduled"]) == 16
    assert sum(entry["valid"] for entry in held["scheduler"]) == 8
    assert sum(entry["valid"] for entry in held["reorder"]) == 64
    for _ in range(4):
        row = p.step(
            "complete-topology-backpressure-held", take=False, present=records[offered]
        )
        for key in (
            "queues",
            "scheduler",
            "reorder",
            "cursor",
            "next_key",
            "pops",
            "pushes",
            "received",
            "observations",
            "observer_last",
        ):
            assert row["after"][key] == held[key], key
    while offered < len(records):
        p.offer(records[offered])
        offered += 1
    p.drain(len(records))
    assert p.model.state["received"] == [expected_output(record) for record in records]
    return p


def late_predecessor():
    p = Program()
    first = item(0, 113, 0, 255, 2, 21)
    p.offer(first)
    p.drain(1)
    p.offer(item(1, 211, 1, 0, 1, 52))
    for _ in range(28):
        p.step("retired-predecessor-is-not-a-global-completion-bit")
    live = [entry for entry in p.model.state["scheduler"] if entry["valid"]]
    assert (
        len(live) == 1
        and live[0]["item"]["sequence_id"] == 1
        and live[0]["state"] == WAITING
    )
    assert p.model.state["received"] == [expected_output(first)]
    return p


def deferred_scheduler_fault(zero_cost):
    p = Program()
    records = [item(0, 4, 0, 255, 32, 9)] + [
        item(key, key, key % 4, 200, 1, key) for key in range(1, 8)
    ]
    for record in records:
        p.offer(record)
    p.offer(item(9 if zero_cost else 1, 199, 3, 255, 0 if zero_cost else 1, 900))
    p.wait(
        lambda: (
            sum(entry["valid"] for entry in p.model.state["scheduler"]) == 8
            and bool(p.model.state["queues"]["prepared"])
        ),
        "wait-deferred-invalid-head",
    )
    head = deepcopy(p.model.state["queues"]["prepared"][0])
    for _ in range(3):
        row = p.step("old-full-scheduler-defers-head-validation")
        assert row["committed"] and row["after"]["queues"]["prepared"][0] == head
    for _ in range(64):
        row = p.step("await-first-old-free-capacity-fault")
        if row["failed"]:
            break
    else:
        raise AssertionError("deferred scheduler fault never reached")
    assert row["errors"] == [
        "dependency_nonpositive_cost" if zero_cost else "dependency_duplicate_key"
    ]
    assert row["after"] == row["before"]
    p.step(
        "failed-runner-cannot-continue-without-host-reset",
        offer=item(10, 210, 2, 255, 1, 42),
    )
    p.step("independent-host-reset-retry", reset=True)
    p.offer(item(0, 17, 0, 255, 1, 60))
    p.drain(1)
    return p


def reused_key_stale():
    p = Program()
    p.offer(item(0, 1, 0, 255, 1, 10))
    p.drain(1)
    p.offer(item(0, 2, 3, 255, 1, 20))
    for _ in range(64):
        row = p.step("live-duplicate-check-allows-reuse-but-reorder-rejects-stale")
        if row["failed"]:
            break
    else:
        raise AssertionError("reused key did not reach stale check")
    assert row["errors"] == ["reorder_stale_key"] and row["after"] == row["before"]
    return p


def live_reorder_duplicate():
    p = Program()
    p.offer(item(1, 73, 0, 255, 1, 107))
    p.wait(
        lambda: any(
            entry["valid"] and entry["key"] == 1 for entry in p.model.state["reorder"]
        ),
        "wait-first-key1-resident-in-reorder-without-key0",
    )
    p.offer(item(1, 199, 3, 255, 1, 211))
    p.wait(
        lambda: bool(p.model.state["queues"]["completed"]),
        "wait-live-reorder-duplicate-head",
    )
    row = p.step(
        "reorder-duplicate-protects-concurrent-host-offer",
        offer=item(0, 33, 1, 255, 1, 31),
    )
    assert row["errors"] == ["reorder_duplicate_key"] and row["after"] == row["before"]
    return p


def full_reorder_deferred_fault(duplicate_only=False):
    p = Program()
    if duplicate_only:
        keys, repeated = range(1, 65), 1
    else:
        keys, repeated = range(73), 9
    for key in keys:
        p.offer(item(key, (key + 31) & 255, key % 4, 255, 1, key * 7), take=False)
    p.wait(
        lambda: sum(entry["valid"] for entry in p.model.state["reorder"]) == 64,
        "wait-full64-reorder",
        take=False,
    )
    p.offer(item(repeated, 244, 3, 255, 1, 707), take=False)
    p.wait(
        lambda: bool(p.model.state["queues"]["completed"]),
        "wait-duplicate-head-behind-full64",
        take=False,
    )
    assert p.model.state["queues"]["completed"][0]["sequence_id"] == repeated
    for _ in range(8):
        row = p.step(
            "old-full64-reorder-defers-duplicate-and-stale-validation", take=False
        )
        assert not row["failed"]
    if duplicate_only:
        assert p.model.state["next_key"] == 0
    else:
        assert p.model.state["next_key"] == 9
        for _ in range(64):
            row = p.step(
                "resume-output-until-capacity-reopens-and-stale-fails", take=True
            )
            if row["failed"]:
                break
        else:
            raise AssertionError("deferred reorder stale check never reached")
        assert row["errors"] == ["reorder_stale_key"] and row["after"] == row["before"]
    return p


def full_byte_domain():
    p = Program()
    records = [
        item(key, 255 - key, key % 4, 255, 1, MASK64 - key) for key in range(256)
    ]
    for record in records:
        p.offer(record)
    p.drain(256)
    assert p.model.state["next_key"] == 256
    assert p.model.state["received"] == [expected_output(record) for record in records]
    p.offer(item(0, 199, 2, 255, 1, 123))
    for _ in range(64):
        row = p.step("nextkey256-makes-byte-key0-stale")
        if row["failed"]:
            break
    else:
        raise AssertionError("byte wrap stale check not reached")
    assert row["errors"] == ["reorder_stale_key"] and row["after"] == row["before"]
    return p


def reference_cases():
    result = {}
    state = empty_state()
    state["epoch"] = 65535
    state["scheduler"][0] = {
        "valid": True,
        "state": WAITING,
        "deadline": 0,
        "item": item(0, 13, 0, 255, 1, 29),
    }
    p = Program(state)
    for _ in range(4):
        p.step("absolute-deadline-crosses-u16-domain")
    assert p.rows[0]["after"]["scheduler"][0]["deadline"] == 65536
    result["wide_deadline"] = descriptor(
        p, "seeded-reference-u64-deadline-boundary-not-reset-hardware-trace", False
    )
    state = empty_state()
    state["scheduler"][0] = {
        "valid": True,
        "state": DONE,
        "deadline": 20,
        "item": item(2, 31, 0, 255, 1, 21),
    }
    state["scheduler"][1] = {
        "valid": True,
        "state": DONE,
        "deadline": 10,
        "item": item(1, 33, 1, 255, 1, 23),
    }
    p = Program(state)
    p.step("retire-earliest-deadline-not-first-slot")
    result["retirement_priority"] = descriptor(
        p, "seeded-reference-exact-native-order-not-reset-hardware-trace", False
    )
    state = empty_state()
    state["epoch"] = MASK64 - 1
    state["scheduler"][0] = {
        "valid": True,
        "state": WAITING,
        "deadline": 0,
        "item": item(0, 93, 0, 255, 2, 91),
    }
    state["queues"]["incoming"] = [item(1, 33, 1, 255, 1, 15)]
    state["pushes"]["incoming"] = 1
    p = Program(state)
    row = p.step(
        "deadline-overflow-rejects-whole-proposal", offer=item(2, 17, 2, 255, 1, 16)
    )
    assert (
        row["errors"] == ["dependency_time_overflow"] and row["after"] == row["before"]
    )
    result["deadline_overflow"] = descriptor(
        p,
        "seeded-reference-u64-overflow-precommit-no-hidden-hardware-time-write",
        False,
    )
    p = Program()
    p.offer(item(0, 31, 0, 255, 2, 71))
    p.step(
        "valid-proposals-in-flight-before-control-failure",
        offer=item(1, 33, 1, 255, 1, 29),
    )
    row = p.step(
        "host-clock-error-discards-all-offer-and-tree-effects",
        clock_error=True,
        offer=item(2, 71, 2, 255, 1, 77),
    )
    assert row["after"] == row["before"]
    result["clock_control_discard"] = descriptor(
        p,
        "host-control-reference-requires-existing-runner-fault-injection-not-DUT-port",
        False,
    )
    return result


def data():
    baseline = (
        Path(__file__).resolve().parents[2]
        / "lit/Source/Inputs/history-routed/baseline"
    )
    manifest = json.loads((baseline / "manifest.json").read_text())
    for asset in manifest["assets"]:
        assert (
            hashlib.sha256((baseline / asset["asset"]).read_bytes()).hexdigest()
            == asset["sha256"]
        )
    retired = Path(__file__).resolve().parent / "retired_backend"
    retired_manifest = json.loads((retired / "manifest.json").read_text())
    for asset in retired_manifest["assets"]:
        assert (
            hashlib.sha256((retired / asset["asset"]).read_bytes()).hexdigest()
            == asset["sha256"]
        )
    cases = {
        "four_routes_live_dependencies": four_routes,
        "lowest_key_issue": issue_selection,
        "full_topology_backpressure": full_backpressure,
        "late_retired_predecessor": late_predecessor,
        "scheduler_deferred_zero_cost": lambda: deferred_scheduler_fault(True),
        "scheduler_deferred_duplicate": lambda: deferred_scheduler_fault(False),
        "live_key_reuse_reorder_stale": reused_key_stale,
        "reorder_live_duplicate": live_reorder_duplicate,
        "full64_deferred_stale": full_reorder_deferred_fault,
        "full64_missing_zero_duplicate_stall": lambda: full_reorder_deferred_fault(
            True
        ),
        "full_byte_domain_no_reorder_wrap": full_byte_domain,
    }
    return {
        "schema": 1,
        "topology": {
            "scopes": [
                "frontend",
                "dependency",
                "dispatch",
                "route_0",
                "route_1",
                "route_2",
                "route_3",
                "output",
            ],
            "queue_depths": [16, 4, 16, 8, 8, 8, 8, 1, 1, 1, 1, 8, 8, 1],
            "queue_latency": 1,
            "blocks": 16,
            "scheduler_capacity": 8,
            "resources": 4,
            "reorder_capacity": 64,
        },
        "provenance": manifest,
        "retired_backend_provenance": retired_manifest,
        "historical_runtime_inputs": [],
        "historical_runtime_scope": "The two exact full-root tests build artifacts only; no original full-root runtime stimuli exist in them.",
        "executable": {
            name: descriptor(build(), "new-independent-full-graph-runtime-scenario")
            for name, build in cases.items()
        },
        "reference": reference_cases(),
    }


def compact(result):
    result = deepcopy(result)
    for group in ("executable", "reference"):
        for case in result[group].values():
            for row in case["rows"]:
                row["after_flow"] = flow_counters(row)
                for phase in ("before", "after"):
                    snapshot = row.pop(phase)
                    row[f"packed_{phase}"] = packed_snapshot(snapshot)
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    parser.add_argument("--vectors", type=Path)
    args = parser.parse_args()
    result = data()
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + "\n")
    if args.vectors:
        args.vectors.write_text(
            json.dumps(compact(result), separators=(",", ":")) + "\n"
        )
    print(  # noqa: T201 - CLI receipt
        json.dumps(
            {
                group: {
                    name: case["executed_epochs"]
                    for name, case in result[group].items()
                }
                for group in ("executable", "reference")
            }
        )
    )  # noqa: T201 - CLI receipt


if __name__ == "__main__":
    main()
