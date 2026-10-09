"""Independent full four-route dependency graph, with frozen old-Q proposals.

The selected contract follows the recovered native QueueDependency and
QueueReorder semantics, with whole-system rejection before any commit. No DUT,
compiler, generated hardware, or retired frontend is imported.
"""

from copy import deepcopy

MASK64 = (1 << 64) - 1
FIELDS = ("sequence_id", "opcode", "route", "waits_for", "cycles", "value")
WIDTHS = (8, 8, 2, 8, 16, 64)
QUEUE_NAMES = (
    "incoming",
    "prepared",
    "scheduled",
    "route_0",
    "route_1",
    "route_2",
    "route_3",
    "route_0_done",
    "route_1_done",
    "route_2_done",
    "route_3_done",
    "completed",
    "ordered",
    "output",
)
DEPTHS = dict(
    zip(QUEUE_NAMES, (16, 4, 16, 8, 8, 8, 8, 1, 1, 1, 1, 8, 8, 1), strict=True)
)
OBSERVED = ("scheduled", "route_1_done", "route_3_done", "completed")
WAITING, EXECUTING, DONE = 0, 1, 2


def item(sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0):
    record = {
        "sequence_id": sequence_id,
        "opcode": opcode,
        "route": route,
        "waits_for": waits_for,
        "cycles": cycles,
        "value": value,
    }
    validate(record)
    return record


def validate(record):
    assert set(record) == set(FIELDS)
    assert all(
        isinstance(record[key], int) and 0 <= record[key] < 1 << width
        for key, width in zip(FIELDS, WIDTHS, strict=True)
    ), record


def transformed(record, delta):
    result = deepcopy(record)
    result["value"] = (result["value"] + delta) & MASK64
    return result


def empty_state():
    return {
        "epoch": 0,
        "queues": {name: [] for name in QUEUE_NAMES},
        "scheduler": [
            {"valid": False, "state": WAITING, "deadline": 0, "item": item()}
            for _ in range(8)
        ],
        "reorder": [{"valid": False, "key": 0, "item": item()} for _ in range(64)],
        "cursor": 0,
        "next_key": 0,
        "pops": dict.fromkeys(QUEUE_NAMES, 0),
        "pushes": dict.fromkeys(QUEUE_NAMES, 0),
        "received": [],
        "observations": {name: [] for name in OBSERVED},
        "observer_last": dict.fromkeys(OBSERVED),
    }


class Model:
    def __init__(self, initial=None, rival=None):
        self.state = deepcopy(initial) if initial is not None else empty_state()
        self.rival = rival
        self.failed = False
        self.attempt = 0
        # Deliberately erroneous historical/global-memory variants only.
        self.completed_history = set()
        self.seen_history = set()
        self.countdown = {}

    def snapshot(self):
        return deepcopy(self.state)

    def pop(self, new, name):
        result = new["queues"][name].pop(0)
        new["pops"][name] += 1
        return result

    def push(self, new, name, record):
        validate(record)
        new["queues"][name].append(deepcopy(record))
        new["pushes"][name] += 1

    def space(self, old, new, name):
        image = new if self.rival == "future_pop_capacity" else old
        return len(image["queues"][name]) < DEPTHS[name]

    def transfer(self, old, new, source, target, delta, grants):
        if old["queues"][source] and self.space(old, new, target):
            record = old["queues"][source][0]
            amount = delta
            if self.rival == "wrong_transform" and source == "ordered":
                amount = 99
            mapped = transformed(record, amount)
            if self.rival == "drop_opcode":
                mapped["opcode"] = 0
            self.pop(new, source)
            self.push(new, target, mapped)
            grants[source + "_map"] = True

    def dependency(self, old, new, grants, errors):
        table = old["scheduler"]
        live = [(index, entry) for index, entry in enumerate(table) if entry["valid"]]
        free = next(
            (index for index, entry in enumerate(table) if not entry["valid"]), None
        )
        head = old["queues"]["prepared"][0] if old["queues"]["prepared"] else None
        admit = head is not None and free is not None
        if head is not None and (
            free is not None or self.rival == "validate_full_scheduler"
        ):
            key = head["sequence_id"]
            if head["cycles"] == 0 and self.rival != "ignore_zero_cost":
                errors.append("dependency_nonpositive_cost")
            elif self.rival != "ignore_dependency_duplicate" and (
                any(entry["item"]["sequence_id"] == key for _, entry in live)
                or (self.rival == "persistent_seen" and key in self.seen_history)
            ):
                errors.append("dependency_duplicate_key")
        if errors:
            return
        done = [(index, entry) for index, entry in live if entry["state"] == DONE]
        retiring = None
        if done and self.space(old, new, "scheduled"):
            if self.rival in ("first_slot_retire", "retired_pyc_scheduler_policy"):
                retiring = min(done, key=lambda pair: pair[0])[0]
            else:
                retiring = min(
                    done,
                    key=lambda pair: (
                        pair[1]["deadline"],
                        pair[1]["item"]["sequence_id"],
                    ),
                )[0]
        completing = [
            index
            for index, entry in live
            if entry["state"] == EXECUTING
            and (
                self.countdown.get(index, 0)
                == (0 if self.rival == "countdown_at_zero" else 1)
                if self.rival
                in (
                    "countdown_at_zero",
                    "countdown_at_one",
                    "retired_pyc_scheduler_policy",
                )
                else entry["deadline"] <= old["epoch"]
            )
        ]
        dependency_table = table
        if self.rival == "forward_completion":
            dependency_table = deepcopy(table)
            for index in completing:
                dependency_table[index]["state"] = DONE
        issues = []
        for resource in range(4):
            busy = any(
                entry["item"]["route"] == resource
                and entry["state"] == EXECUTING
                and (
                    self.countdown.get(index, 0)
                    > (0 if self.rival == "countdown_at_zero" else 1)
                    if self.rival
                    in (
                        "countdown_at_zero",
                        "countdown_at_one",
                        "retired_pyc_scheduler_policy",
                    )
                    else entry["deadline"] > old["epoch"]
                    or self.rival == "busy_until_done"
                )
                for index, entry in live
            )
            if busy:
                continue
            ready = []
            for index, entry in live:
                record = entry["item"]
                predecessor = record["waits_for"]
                satisfied = predecessor == 255 or any(
                    candidate["valid"]
                    and candidate["state"] == DONE
                    and candidate["item"]["sequence_id"] == predecessor
                    for candidate in dependency_table
                )
                if (
                    self.rival == "persistent_completed"
                    and predecessor in self.completed_history
                ):
                    satisfied = True
                if (
                    entry["state"] == WAITING
                    and record["route"] == resource
                    and satisfied
                ):
                    ready.append(index)
            if ready:
                chosen = (
                    min(ready)
                    if self.rival
                    in ("first_slot_issue", "retired_pyc_scheduler_policy")
                    else min(
                        ready, key=lambda index: table[index]["item"]["sequence_id"]
                    )
                )
                if old["epoch"] > MASK64 - table[chosen]["item"]["cycles"]:
                    errors.append("dependency_time_overflow")
                else:
                    issues.append(chosen)
        if errors:
            return
        grants["dependency"] = {
            "admit": free if admit else None,
            "retire": retiring,
            "complete": completing,
            "issue": issues,
        }
        if retiring is not None:
            self.push(new, "scheduled", table[retiring]["item"])
            new["scheduler"][retiring]["valid"] = False
        if self.rival in (
            "countdown_at_zero",
            "countdown_at_one",
            "retired_pyc_scheduler_policy",
        ):
            for index, entry in live:
                if index in completing:
                    self.countdown[index] = 0
                elif entry["state"] == EXECUTING and self.countdown.get(index, 0) > 0:
                    self.countdown[index] -= 1
        for index in completing:
            new["scheduler"][index]["state"] = DONE
            if self.rival == "persistent_completed":
                self.completed_history.add(table[index]["item"]["sequence_id"])
        for index in issues:
            new["scheduler"][index]["state"] = EXECUTING
            deadline = old["epoch"] + table[index]["item"]["cycles"]
            new["scheduler"][index]["deadline"] = deadline
            if self.rival in (
                "countdown_at_zero",
                "countdown_at_one",
                "retired_pyc_scheduler_policy",
            ):
                self.countdown[index] = table[index]["item"]["cycles"]
        if (
            self.rival == "reuse_retiring_scheduler_slot"
            and free is None
            and retiring is not None
            and head is not None
        ):
            admit, free = True, retiring
        if admit:
            record = self.pop(new, "prepared")
            new["scheduler"][free] = {
                "valid": True,
                "state": WAITING,
                "deadline": 0,
                "item": record,
            }
            if self.rival == "persistent_seen":
                self.seen_history.add(record["sequence_id"])
        if self.rival == "forward_admission" and admit and head["waits_for"] == 255:
            resource = head["route"]
            if not any(table[index]["item"]["route"] == resource for index in issues):
                new["scheduler"][free]["state"] = EXECUTING
                new["scheduler"][free]["deadline"] = old["epoch"] + head["cycles"]

    def reorder(self, old, new, grants, errors):
        table = old["reorder"]
        free = next(
            (index for index, entry in enumerate(table) if not entry["valid"]), None
        )
        head = old["queues"]["completed"][0] if old["queues"]["completed"] else None
        if head is not None and (
            free is not None or self.rival == "validate_full_reorder"
        ):
            key = head["sequence_id"]
            if key < old["next_key"] and self.rival != "ignore_reorder_stale":
                errors.append("reorder_stale_key")
            elif self.rival != "ignore_reorder_duplicate" and any(
                entry["valid"] and entry["key"] == key for entry in table
            ):
                errors.append("reorder_duplicate_key")
        if errors:
            return
        retiring = next(
            (
                index
                for index, entry in enumerate(table)
                if entry["valid"] and entry["key"] == old["next_key"]
            ),
            None,
        )
        if not self.space(old, new, "ordered"):
            retiring = None
        grants["reorder"] = {
            "admit": free if head is not None and free is not None else None,
            "retire": retiring,
        }
        if retiring is not None:
            self.push(new, "ordered", table[retiring]["item"])
            new["reorder"][retiring]["valid"] = False
            new["next_key"] += 1
            if self.rival == "u8_next_key":
                new["next_key"] &= 255
        if (
            self.rival == "reuse_retiring_reorder_slot"
            and free is None
            and retiring is not None
        ):
            free = retiring
        if head is not None and free is not None:
            self.pop(new, "completed")
            new["reorder"][free] = {
                "valid": True,
                "key": head["sequence_id"],
                "item": deepcopy(head),
            }
            if (
                self.rival == "forward_reorder_admission"
                and head["sequence_id"] == old["next_key"]
                and retiring is None
                and self.space(old, new, "ordered")
            ):
                self.push(new, "ordered", head)
                new["reorder"][free]["valid"] = False
                new["next_key"] += 1

    def dispatch(self, old, new, grants):
        if not old["queues"]["scheduled"]:
            return
        head = old["queues"]["scheduled"][0]
        target = f"route_{head['route']}"
        space = self.space(old, new, target)
        if self.rival == "route_requires_all_space":
            space = all(self.space(old, new, f"route_{index}") for index in range(4))
        if space:
            self.pop(new, "scheduled")
            self.push(new, target, head)
            grants["route"] = head["route"]

    def merge(self, old, new, grants):
        start = 0 if self.rival == "priority_merge" else old["cursor"]
        selected = next(
            (
                index
                for offset in range(4)
                if old["queues"][f"route_{(index := (start + offset) % 4)}_done"]
            ),
            None,
        )
        if selected is not None and self.space(old, new, "completed"):
            self.push(new, "completed", self.pop(new, f"route_{selected}_done"))
            new["cursor"] = (selected + 1) % 4
            grants["merge"] = selected
        elif self.rival == "merge_cursor_on_stall":
            new["cursor"] = (old["cursor"] + 1) % 4

    def observe(self, old, new):
        for name in OBSERVED:
            if old["queues"][name]:
                pending = {"pops": old["pops"][name], "item": old["queues"][name][0]}
                if (
                    self.rival == "observe_every_held_head"
                    or old["observer_last"][name] != pending
                ):
                    new["observations"][name].append(deepcopy(pending["item"]))
                    new["observer_last"][name] = deepcopy(pending)

    def step(self, actions=None, label=""):
        actions = deepcopy(actions or {})
        old = self.snapshot()
        assert set(actions) <= {"offer", "present", "take", "reset", "clock_error"}
        assert not ("offer" in actions and "present" in actions)
        grants, errors = {}, []
        if actions.get("reset"):
            assert len(actions) == 1
            self.state = empty_state()
            self.failed = False
            self.completed_history.clear()
            self.seen_history.clear()
            self.countdown.clear()
            new, committed = self.snapshot(), True
        elif self.failed:
            new, committed, errors = old, False, ["latched_failure"]
        else:
            new = deepcopy(old)
            offered = actions.get("offer")
            if offered is not None:
                validate(offered)
                assert (
                    len(old["queues"]["incoming"]) < 16
                ), "ordinary offer requires OLD input capacity"
            if "present" in actions:
                validate(actions["present"])
                if len(old["queues"]["incoming"]) < 16:
                    offered = actions["present"]
                else:
                    grants["host_blocked"] = True
            # The actual sink is after the final map in the recovered graph;
            # grants still use OLD output capacity. Publishing a physical pop
            # first only exposes the deliberate future-pop-capacity rival.
            if actions.get("take", True) and old["queues"]["output"]:
                record = self.pop(new, "output")
                new["received"].append(record)
                grants["sink"] = deepcopy(record)
            self.transfer(old, new, "ordered", "output", 100, grants)
            self.transfer(old, new, "incoming", "prepared", 1, grants)
            self.dependency(old, new, grants, errors)
            self.dispatch(old, new, grants)
            for route in range(4):
                self.transfer(
                    old, new, f"route_{route}", f"route_{route}_done", route + 1, grants
                )
            self.merge(old, new, grants)
            self.reorder(old, new, grants, errors)
            self.observe(old, new)
            if offered is not None:
                self.push(new, "incoming", offered)
                grants["host_offer"] = True
            if actions.get("clock_error"):
                errors.append("clock_control_failure")
            if old["epoch"] == MASK64:
                errors.append("epoch_time_overflow")
            committed = not errors
            if committed:
                new["epoch"] += 1
            else:
                self.failed = True
                if self.rival == "partial_commit_failure":
                    new["epoch"] += 1
                else:
                    new = old
            for name in QUEUE_NAMES:
                assert len(new["queues"][name]) <= DEPTHS[name], (name, label)
            self.state = new
        row = {
            "attempt": self.attempt,
            "label": label,
            "actions": actions,
            "before": old,
            "after": self.snapshot(),
            "committed": committed,
            "failed": self.failed,
            "errors": errors,
            "grants": grants,
        }
        self.attempt += 1
        return row


def packed_snapshot(snapshot):
    """21934 bits: all queue cells, retained table cells and three scalars."""
    packed, total = 0, 0

    def append(width, value):
        nonlocal packed, total
        assert 0 <= value < 1 << width
        packed = (packed << width) | int(value)
        total += width

    def record(value):
        for key, width in zip(FIELDS, WIDTHS, strict=True):
            append(width, value[key])

    append(64, snapshot["epoch"])
    append(2, snapshot["cursor"])
    append(64, snapshot["next_key"])
    for name in QUEUE_NAMES:
        values, depth = snapshot["queues"][name], DEPTHS[name]
        append(depth.bit_length(), len(values))
        for index in range(depth):
            record(values[index] if index < len(values) else item())
    for value in snapshot["scheduler"]:
        append(1, value["valid"])
        append(2, value["state"])
        append(64, value["deadline"])
        record(value["item"])
    for value in snapshot["reorder"]:
        append(1, value["valid"])
        append(64, value["key"])
        record(value["item"])
    assert total == 21934, total
    return f"{packed:0{(total + 3) // 4}x}"


def counters(snapshot):
    return {
        key: deepcopy(snapshot[key])
        for key in ("pops", "pushes", "received", "observations", "observer_last")
    }


def flow_counters(row):
    """Complete append-only observations as per-edge deltas, avoiding prefixes."""
    old, new = row["before"], row["after"]
    reset = row["actions"].get("reset", False)

    def delta(previous, current):
        if reset:
            return deepcopy(current)
        assert current[: len(previous)] == previous
        return deepcopy(current[len(previous) :])

    return {
        "pops": deepcopy(new["pops"]),
        "pushes": deepcopy(new["pushes"]),
        "received_count": len(new["received"]),
        "received_delta": delta(old["received"], new["received"]),
        "observation_counts": {
            name: len(new["observations"][name]) for name in OBSERVED
        },
        "observation_delta": {
            name: delta(old["observations"][name], new["observations"][name])
            for name in OBSERVED
        },
        "observer_last": deepcopy(new["observer_last"]),
    }


def public_expected(row):
    old = row["before"]
    result = {"ready": len(old["queues"]["incoming"]) < 16}
    for name in ("output", *OBSERVED):
        values = old["queues"][name]
        result[name] = {"available": bool(values)}
        if values:
            result[name]["head"] = deepcopy(values[0])
    return result


def frame(row):
    actions = row["actions"]
    return {
        "valid": "offer" in actions or "present" in actions,
        "take": actions.get("take", True),
        "data": deepcopy(actions.get("offer", actions.get("present", item()))),
        "host_reset": actions.get("reset", False),
        "clock_error": actions.get("clock_error", False),
    }
