"""Host-side verification only; never compiled into the circuit.

NDF bindings associate the actual observation checkers with verification clauses.
"""

from pathlib import Path


# ndf: verifies=WSEMIG-RX-EXTERNAL-VER-001
def check_run(run: dict, expected: dict, target: str) -> None:
    if run["status"] != "EXECUTED":
        raise AssertionError(f"{target}: backend did not execute: {run['status']}")
    samples = run["samples"]
    rows = expected["results"]
    assert [s["cycle"] for s in samples] == [
        i for i in range(len(rows)) for _ in range(2)
    ], f"{target}: incomplete or reordered cycle observations"
    assert [s["evaluation_epoch"] for s in samples] == list(range(2 * len(rows)))
    assert [s["commit_epoch"] for s in samples] == list(range(1, 2 * len(rows) + 1))
    for index, sample in enumerate(samples):
        assert (
            sample["result"] == rows[index // 2]
        ), f"{target}: sample {index}, actual={sample['result']}, expected={rows[index // 2]}"
    failure = expected["failure_cycle"]
    if failure is None:
        assert run["returncode"] == 0, f"{target}: unexpected simulator failure"
        assert run["terminal"]["status"] == "TERMINATED"
        assert int(run["terminal"]["epoch_time"]) == 2 * len(rows)
    else:
        assert run["returncode"] != 0, f"{target}: missing capacity failure"
        if target == "cpp":
            assert run["terminal"]["status"] == "FAILED"
            assert int(run["terminal"]["epoch_time"]) == 2 * failure
            assert run["terminal"]["error"]["message"] == expected["error"]
        else:
            log = Path(run["stdout"]).read_text() + Path(run["stderr"]).read_text()
            assert f"source or primitive check failed at epoch {2 * failure}" in log


# ndf: verifies=WSEMIG-MULTINODE-VER-001
def check_multinode(events: list, actual: list, wanted: list, cycles: int, target: str) -> None:
    """Require all five nodes at each sample, then compare every observed field."""
    actual.sort(key=lambda row: (row[0], row[1]))
    wanted.sort(key=lambda row: (row[0], row[1]))
    for epoch in range(2 * cycles):
        sample = [event for event in events if int(event["evaluation_epoch"]) == epoch]
        if sorted(int(event["values"][1]["value"]) for event in sample) != [1, 8, 9, 10, 17]:
            raise RuntimeError(f"{target}: incomplete nodes at sampling epoch {epoch}")
        if any(int(event["commit_epoch"]) != epoch + 1 for event in sample):
            raise RuntimeError(f"{target}: unexpected commit epoch")
    if actual != wanted:
        raise RuntimeError(f"{target}: observations differ from the Python core")


# ndf: verifies=WSEMIG-ALLGATHER-VER-001
def check_allgather(events: list, actual: list, wanted: list, cycles: int, target: str) -> None:
    """Check all 32 resident accounts at each low/high sampling point."""
    samples: dict[int, list] = {}
    for event in events:
        epoch = int(event["evaluation_epoch"])
        samples.setdefault(epoch, []).append(event)
        if int(event["commit_epoch"]) != epoch + 1:
            raise RuntimeError(f"{target}: unexpected commit epoch")
    if sorted(samples) != list(range(2 * cycles)):
        raise RuntimeError(f"{target}: incomplete sampling epochs")
    for epoch, sample in samples.items():
        if sorted(int(event["values"][2]["value"]) for event in sample) != list(range(32)):
            raise RuntimeError(f"{target}: incomplete nodes at sampling epoch {epoch}")
        if any(int(event["values"][0]["value"]) != epoch // 2 for event in sample):
            raise RuntimeError(f"{target}: cycle does not match sampling epoch")
    actual.sort(key=lambda row: (row[0], row[2]))
    wanted.sort(key=lambda row: (row[0], row[2]))
    if actual != wanted:
        for observed, expected in zip(actual, wanted, strict=False):
            if observed != expected:
                raise RuntimeError(f"{target}: actual={observed}, core={expected}")
        raise RuntimeError(f"{target}: observation count differs from the Python core")


# ndf: verifies=WSEMIG-NODE-VER-001
def check_node(events: list, actual: list, wanted: list, cycles: int, target: str) -> None:
    """Require every node-integration sample and compare all connected fields."""
    if [int(event["evaluation_epoch"]) for event in events] != list(range(2 * cycles)):
        raise RuntimeError(f"{target}: incomplete or reordered sampling epochs")
    if [int(event["commit_epoch"]) for event in events] != list(range(1, 2 * cycles + 1)):
        raise RuntimeError(f"{target}: unexpected commit epochs")
    if actual != wanted:
        raise RuntimeError(f"{target}: node integration differs from the Python core")


# ndf: verifies=WSEMIG-CLOSEDLOOP-VER-001
def check_allgather_control(
    events: list,
    actual: list,
    wanted: list,
    state_events: list,
    expected_states: list,
    cycles: int,
    target: str,
) -> None:
    """Check all members and the independent controller trace through terminal hold."""
    check_allgather(events, actual, wanted, cycles, target)
    states = [
        [
            1 if v["value"] == "true" else 0 if v["value"] == "false" else int(v["value"])
            for v in event["values"]
        ]
        for event in state_events
    ]
    if [int(e["evaluation_epoch"]) for e in state_events] != list(range(2 * cycles)):
        raise RuntimeError(f"{target}: incomplete controller sampling epochs")
    if [int(e["commit_epoch"]) for e in state_events] != list(range(1, 2 * cycles + 1)):
        raise RuntimeError(f"{target}: unexpected controller commit epochs")
    if states != [row for row in expected_states for _ in range(2)]:
        raise RuntimeError(f"{target}: controller differs from core-driven phase progression")
    if len(states) < 4 or any(row[1:3] != [6, 1] for row in states[-4:]):
        raise RuntimeError(f"{target}: both phases must finish and hold done")
