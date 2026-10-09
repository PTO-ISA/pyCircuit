"""Independent finite scheduler histories derived from baseline 8887e6de.

Test-only mathematics: no DUT source, compiler, generated artifact or hardware
result is read. QueueDependency uses live Done entries for readiness; Schedule
uses persistent completion and seen-key sets over [0, no_dependency).
"""

from collections import deque


def run(
    tokens,
    *,
    no_dependency,
    persistent=False,
    epochs=32,
    take_low=(),
    offers=None,
    initial_tokens=(),
    host_resets=(),
    capacity=4,
    resources=2,
    queue_depth=4,
    ignore_completion_history=False,
    ignore_resource_busy=False,
    same_edge_admission=False,
    reverse_key_priority=False,
    release_by_key=False,
    reuse_retired_capacity=False,
    commit_on_failure=False,
):
    """Simulate all original edges; tokens are (key, predecessor, resource, cost, value).

    Default producer holds the next token until its input push commits. Explicit
    ``offers`` instead supplies a tuple or None per edge, witnessing overdrive.
    ``initial_tokens`` models the historical direct queue batch at time zero;
    ordinary source-system streams use the default producer instead.
    ``host_resets`` are explicit reset operations before selected edges; they
    clear failure and history. They do not claim physical-reset recovery.
    Rival switches change one semantic decision, never the driven columns.
    """
    assert len(initial_tokens) <= queue_depth
    incoming = deque((tuple(token), 1) for token in initial_tokens)
    outgoing = deque()
    entries = {}
    completed = set()
    seen = set()
    failed = None
    cursor = 0
    rows, frames, events, sinks = [], [], [], []
    for edge in range(epochs):
        host_reset = edge in host_resets
        if host_reset:
            incoming.clear()
            outgoing.clear()
            entries.clear()
            completed.clear()
            seen.clear()
            failed = None
        ready = len(incoming) < queue_depth
        offered = (
            offers[edge]
            if offers is not None
            else (tokens[cursor] if cursor < len(tokens) else None)
        )
        token = tuple(offered) if offered is not None else (0, 0, 0, 0, 0)
        valid = offered is not None
        take = edge not in take_low
        frames.append(
            {
                "valid": int(valid),
                "take": int(take),
                "data": token,
                "host_reset": host_reset,
            }
        )
        available = bool(outgoing) and outgoing[0][1] <= edge
        head = outgoing[0][0] if available else (0, 0, 0, 0, 0)
        before = (
            tuple(incoming),
            tuple(outgoing),
            tuple(sorted(entries.items())),
            frozenset(completed),
            frozenset(seen),
        )
        done = [(key, entry) for key, entry in entries.items() if entry[1] == "done"]
        winner = min(
            done,
            key=lambda item: item[0] if release_by_key else (item[1][2], item[0]),
            default=None,
        )
        release = (
            winner[0] if winner is not None and len(outgoing) < queue_depth else None
        )
        free = len(entries) < capacity or (
            reuse_retired_capacity and release is not None
        )
        input_available = bool(incoming) and incoming[0][1] <= edge
        candidate = incoming[0][0] if input_available and free else None
        error = None
        if candidate is not None:
            key, predecessor, resource, cost, _ = candidate
            if key < 0:
                error = "dependency_negative_key"
            elif predecessor < 0:
                error = "dependency_negative_predecessor"
            elif resource < 0:
                error = "dependency_negative_resource"
            elif cost <= 0:
                error = "dependency_nonpositive_cost"
            elif key in entries or (
                persistent and (key >= no_dependency or key in seen)
            ):
                error = "dependency_duplicate_key"
            elif (
                persistent
                and predecessor != no_dependency
                and predecessor >= no_dependency
            ):
                error = "dependency_predecessor_out_of_range"
            elif resource >= resources:
                error = "dependency_resource_out_of_range"
        newly_completed = [
            key
            for key, entry in entries.items()
            if entry[1] == "running" and entry[2] <= edge
        ]
        issue = []
        issue_entries = dict(entries)
        if same_edge_admission and candidate is not None and error is None:
            issue_entries[candidate[0]] = (candidate, "waiting", None)
        for resource in range(resources):
            busy = any(
                entry[0][2] == resource and entry[1] == "running" and entry[2] > edge
                for entry in entries.values()
            )
            if busy and not ignore_resource_busy:
                continue
            waiting = []
            for key, entry in issue_entries.items():
                packet, phase, _ = entry
                predecessor = packet[1]
                dependency_ready = predecessor == no_dependency
                if not dependency_ready:
                    if persistent and not ignore_completion_history:
                        dependency_ready = predecessor in completed
                    else:
                        dependency_ready = (
                            predecessor in entries and entries[predecessor][1] == "done"
                        )
                if phase == "waiting" and packet[2] == resource and dependency_ready:
                    waiting.append(key)
            if waiting:
                issue.append(max(waiting) if reverse_key_priority else min(waiting))
        if error is not None and failed is None:
            failed = {
                "edge": edge,
                "view": 2 * edge,
                "historical_code": error,
                "current_code": "source_check_failed",
            }
        reject = failed is not None and not commit_on_failure
        observation = (
            None
            if reject
            else {
                "epoch": edge,
                "ready": int(ready),
                "available": int(available),
                "head": head,
            }
        )
        rows.extend((observation, observation))
        accepted = bool(valid and ready and not reject)
        sink = head if available and take and not reject else None
        if not reject:
            if candidate is not None and error is None:
                incoming.popleft()
            if accepted:
                incoming.append((token, edge + 1))
                if offers is None:
                    cursor += 1
            if available and take:
                outgoing.popleft()
                sinks.append((edge, head))
            if release is not None:
                outgoing.append((entries[release][0], edge + 1))
                del entries[release]
            for key in newly_completed:
                packet, _, deadline = entries[key]
                entries[key] = (packet, "done", deadline)
                if persistent:
                    completed.add(key)
            if candidate is not None and error is None:
                entries[candidate[0]] = (candidate, "waiting", None)
                if persistent:
                    seen.add(candidate[0])
            for key in issue:
                packet = entries[key][0]
                entries[key] = (packet, "running", edge + packet[3])
        after = (
            tuple(incoming),
            tuple(outgoing),
            tuple(sorted(entries.items())),
            frozenset(completed),
            frozenset(seen),
        )
        if reject:
            assert before == after
        events.append(
            {
                "edge": edge,
                "admit": candidate[0]
                if candidate is not None and error is None and not reject
                else None,
                "issue": [] if reject else issue,
                "complete": [] if reject else newly_completed,
                "release": None if reject else release,
                "sink": sink,
                "accepted": accepted,
                "free": free,
                "occupied": len(before[2]),
                "input_head": incoming[0][0] if incoming else None,
                "failure": dict(failed) if failed is not None else None,
                "host_reset": host_reset,
                "state_before": before,
                "state_after": after,
            }
        )
        assert (
            len(incoming) <= queue_depth
            and len(outgoing) <= queue_depth
            and len(entries) <= capacity
        )
    return {
        "rows": rows,
        "frames": frames,
        "events": events,
        "sinks": sinks,
        "first_failure": next(
            (event["failure"] for event in events if event["failure"] is not None), None
        ),
        "final": after,
    }


def cases():
    """Full historical scenarios and explicit regular-clock companions."""
    result = {}
    for persistent, sentinel, name in (
        (False, 15, "dependency"),
        (True, 255, "schedule"),
    ):
        tokens = [
            (0, sentinel, 0, 4, 10),
            (1, sentinel, 0, 1, 11),
            (2, sentinel, 1, 1, 12),
            (3, 0, 1, 1, 13),
        ]
        result[name + "_parallel"] = run(
            tokens, no_dependency=sentinel, persistent=persistent, epochs=32
        )
        result[name + "_batch"] = run(
            [],
            initial_tokens=tokens,
            no_dependency=sentinel,
            persistent=persistent,
            epochs=16,
        )
        result[name + "_backpressure"] = run(
            [(i, sentinel, i % 2, 1 + i % 3, 100 + i) for i in range(12)],
            no_dependency=sentinel,
            persistent=persistent,
            epochs=64,
            take_low=range(40),
        )
        result[name + "_key_priority"] = run(
            [
                (0, sentinel, 0, 8, 60),
                (3, sentinel, 0, 1, 63),
                (2, sentinel, 0, 1, 62),
                (1, sentinel, 0, 1, 61),
            ],
            no_dependency=sentinel,
            persistent=persistent,
            epochs=32,
        )
        result[name + "_zero_cost"] = run(
            [(0, sentinel, 0, 0, 20)],
            no_dependency=sentinel,
            persistent=persistent,
            epochs=16,
        )
        result[name + "_live_duplicate"] = run(
            [(2, sentinel, 0, 12, 20), (2, sentinel, 1, 1, 21)],
            no_dependency=sentinel,
            persistent=persistent,
            epochs=24,
        )
        result[name + "_full_defers_zero"] = run(
            [(i, sentinel, i % 2, 15, i) for i in range(4)] + [(4, sentinel, 0, 0, 9)],
            no_dependency=sentinel,
            persistent=persistent,
            epochs=48,
            take_low=range(48),
        )
        # Token 0 has already left the resident table when its successor arrives.
        offers = [None] * 32
        offers[0] = (0, sentinel, 0, 1, 30)
        offers[10] = (1, 0, 1, 1, 31)
        result[name + "_late_successor"] = run(
            [], offers=offers, no_dependency=sentinel, persistent=persistent, epochs=32
        )
    result["schedule_persistent_original"] = run(
        [],
        initial_tokens=[(0, 255, 0, 1, 10), (1, 255, 1, 8, 11), (2, 0, 1, 1, 12)],
        no_dependency=255,
        persistent=True,
        epochs=20,
    )
    reuse = [None] * 32
    reuse[0] = (7, 255, 0, 1, 40)
    reuse[10] = (7, 255, 0, 1, 41)
    result["schedule_retired_key_reuse"] = run(
        [], offers=reuse, no_dependency=255, persistent=True, epochs=32
    )
    result["dependency_retired_key_reuse"] = run(
        [],
        offers=[
            (key, 15, resource, cost, value) if item is not None else None
            for item in reuse
            for key, _, resource, cost, value in [
                item if item is not None else (0, 0, 0, 0, 0)
            ]
        ],
        no_dependency=15,
        epochs=32,
    )
    reset = [None] * 32
    reset[0] = (7, 255, 0, 1, 40)
    reset[10] = (7, 255, 0, 1, 41)
    reset[14] = (8, 7, 0, 1, 42)
    reset[18] = (7, 255, 1, 1, 43)
    result["schedule_reset_history"] = run(
        [],
        offers=reset,
        no_dependency=255,
        persistent=True,
        epochs=32,
        host_resets=(14,),
    )
    result["schedule_outside_domain"] = run(
        [(255, 255, 0, 1, 50)], no_dependency=255, persistent=True, epochs=16
    )
    result["schedule_invalid_resource"] = run(
        [(0, 255, 2, 1, 50)], no_dependency=255, persistent=True, epochs=16
    )
    # A custom completion domain exercises the runtime diagnostic. Original
    # schedule_v2 uses domain/sentinel 255: u8 predecessors cannot exceed it.
    # This is model-only coverage, not a representable original-root vector.
    result["model_custom_domain_invalid_predecessor"] = run(
        [(0, 254, 0, 1, 50)], no_dependency=15, persistent=True, epochs=16
    )
    return result


def self_test():
    records = cases()
    for name in (
        "dependency_parallel",
        "schedule_parallel",
        "dependency_batch",
        "schedule_batch",
    ):
        assert [token[0] for _, token in records[name]["sinks"]] == [2, 0, 1, 3], name
    assert [
        token[0] for _, token in records["schedule_persistent_original"]["sinks"]
    ] == [0, 1, 2]
    assert [token[0] for _, token in records["schedule_late_successor"]["sinks"]] == [
        0,
        1,
    ]
    assert [token[0] for _, token in records["dependency_late_successor"]["sinks"]] == [
        0
    ]
    assert [
        token[0] for _, token in records["dependency_retired_key_reuse"]["sinks"]
    ] == [7, 7]
    assert records["schedule_retired_key_reuse"]["first_failure"]["edge"] == 11
    assert [token[0] for _, token in records["schedule_reset_history"]["sinks"]] == [
        7,
        7,
        8,
    ]
    for name, record in records.items():
        assert len(record["rows"]) == 2 * len(record["events"]), name
        for event in record["events"]:
            if event["failure"] is not None:
                assert event["state_before"] == event["state_after"], name
    for name in ("dependency_backpressure", "schedule_backpressure"):
        record = records[name]
        assert len(record["sinks"]) == 12
        assert any(
            event["occupied"] == 4 and not event["free"] for event in record["events"]
        )
        assert any(len(event["state_before"][1]) == 4 for event in record["events"])
        assert [token[0] for _, token in record["sinks"]] == [
            0,
            1,
            3,
            2,
            4,
            6,
            5,
            7,
            9,
            8,
            10,
            11,
        ]
        assert [edge for edge, _ in record["sinks"]] == list(range(40, 52))
    for name in ("dependency_key_priority", "schedule_key_priority"):
        record = records[name]
        assert [(e["edge"], e["issue"]) for e in record["events"] if e["issue"]] == [
            (2, [0]),
            (10, [1]),
            (11, [2]),
            (12, [3]),
        ]
        assert [(edge, token[0]) for edge, token in record["sinks"]] == [
            (12, 0),
            (13, 1),
            (14, 2),
            (15, 3),
        ]
    # Hand-derived timing tables pin the one-edge stages and shared-resource
    # finishing-edge reuse independently of any DUT or generated output.
    parallel = records["dependency_parallel"]
    assert [(e["edge"], e["issue"]) for e in parallel["events"] if e["issue"]] == [
        (2, [0]),
        (4, [2]),
        (6, [1]),
        (7, [3]),
    ]
    assert [
        (e["edge"], e["complete"]) for e in parallel["events"] if e["complete"]
    ] == [(5, [2]), (6, [0]), (7, [1]), (8, [3])]
    assert [
        (e["edge"], e["release"])
        for e in parallel["events"]
        if e["release"] is not None
    ] == [(6, 2), (7, 0), (8, 1), (9, 3)]
    assert [(edge, token[0]) for edge, token in parallel["sinks"]] == [
        (7, 2),
        (8, 0),
        (9, 1),
        (10, 3),
    ]
    for name in ("dependency_full_defers_zero", "schedule_full_defers_zero"):
        assert records[name]["first_failure"]["edge"] == 19
        assert all(
            not e["free"] and e["failure"] is None
            for e in records[name]["events"][5:19]
        )
    # Rival comparisons hold the entire driven frame stream fixed. This
    # separates scheduler timing/history/resource decisions, not a credit FIFO.
    rival_counts = {}
    for label, baseline, overrides in (
        ("resource_overlap", "dependency_parallel", {"ignore_resource_busy": True}),
        ("same_edge_admission", "dependency_parallel", {"same_edge_admission": True}),
        (
            "highest_key_issue",
            "dependency_key_priority",
            {"reverse_key_priority": True},
        ),
        ("key_ordered_release", "schedule_backpressure", {"release_by_key": True}),
        (
            "lost_persistent_completion",
            "schedule_late_successor",
            {"ignore_completion_history": True},
        ),
        (
            "reuse_retired_capacity",
            "schedule_backpressure",
            {"reuse_retired_capacity": True},
        ),
        ("commit_on_failure", "schedule_zero_cost", {"commit_on_failure": True}),
    ):
        base = records[baseline]
        frames = base["frames"]
        rival = run(
            [],
            offers=[frame["data"] if frame["valid"] else None for frame in frames],
            take_low=[edge for edge, frame in enumerate(frames) if not frame["take"]],
            no_dependency=255 if baseline.startswith("schedule") else 15,
            persistent=baseline.startswith("schedule"),
            epochs=len(frames),
            **overrides,
        )
        different = sum(
            a != b for a, b in zip(base["rows"], rival["rows"], strict=True)
        )
        assert different > 0, label
        rival_counts[label] = different
    records["rivals"] = rival_counts
    return records


if __name__ == "__main__":
    for case_name, record in self_test().items():
        if case_name == "rivals":
            print("rivals", record)
            continue
        print(
            case_name,
            "rows",
            len(record["rows"]),
            "sinks",
            [(edge, token[0]) for edge, token in record["sinks"]],
            "failure",
            record["first_failure"],
        )
