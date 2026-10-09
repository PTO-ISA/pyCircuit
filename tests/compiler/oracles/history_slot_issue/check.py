"""Check complete independently specified slot/issue histories.

A packed hardware observation must include independently observed transfer
counters and received records. A final output list or an epoch prefix cannot
stand in for the complete history.
"""

import argparse
import json
from pathlib import Path

from generate import data
from models import packed_snapshot

COUNTER_FIELDS = ("pops", "pushes", "received")


def counters(snapshot):
    return {key: snapshot[key] for key in COUNTER_FIELDS}


def check_case(expected, observed):
    rows = observed["rows"] if isinstance(observed, dict) else observed
    assert len(rows) == expected["executed_epochs"], "complete epoch count required"
    for wanted, actual in zip(expected["rows"], rows, strict=True):
        assert actual["epoch"] == wanted["epoch"], "epoch identity/order mismatch"
        for phase in ("before", "work", "after"):
            required = phase == "after"
            if phase in actual:
                assert (
                    actual[phase] == wanted[phase]
                ), f"epoch {wanted['epoch']} {phase} state mismatch"
            elif f"packed_{phase}" in actual:
                value = actual[f"packed_{phase}"]
                assert isinstance(
                    value, str | int
                ), "packed observation must be a decimal scalar"
                bits = expected["snapshot_bits"]
                assert 0 <= int(value) < 1 << bits, "packed observation out of range"
                assert (
                    str(int(value))
                    == packed_snapshot(expected["kind"], wanted[phase])[1]
                ), f"epoch {wanted['epoch']} {phase} packed state mismatch"
                assert (
                    f"{phase}_counters" in actual
                ), "packed observations require actual counters/received"
                assert actual[f"{phase}_counters"] == counters(
                    wanted[phase]
                ), f"epoch {wanted['epoch']} {phase} counter/received mismatch"
            else:
                assert not required, "complete after-state observation required"
        for key in ("taken", "grants", "actions"):
            if key in actual:
                assert (
                    actual[key] == wanted[key]
                ), f"epoch {wanted['epoch']} {key} mismatch"
        if "controls" in actual:
            for key, value in wanted["controls"].items():
                assert (
                    key in actual["controls"] and actual["controls"][key] == value
                ), f"epoch {wanted['epoch']} public control {key} mismatch"
    return len(rows)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--group", choices=("original", "executable"), required=True)
    parser.add_argument("--case", required=True)
    parser.add_argument("--observed", type=Path, required=True)
    args = parser.parse_args()
    case = data()[args.group][args.case]
    observed = json.loads(args.observed.read_text())
    epochs = check_case(case, observed)
    print(  # noqa: T201 - CLI receipt
        json.dumps(
            {"group": args.group, "case": args.case, "epochs": epochs, "checked": True}
        )
    )  # noqa: T201


if __name__ == "__main__":
    main()
