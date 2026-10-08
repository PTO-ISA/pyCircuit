"""Check every full-graph epoch, protected failure, and observed item."""

import argparse
import json
from pathlib import Path

from generate import data
from models import flow_counters, packed_snapshot


def wanted_packed(row, phase):
    return packed_snapshot(row[phase]) if phase in row else row[f"packed_{phase}"]


def wanted_flow(row):
    return flow_counters(row) if "after" in row else row["after_flow"]


def check_case(expected, observed):
    rows = observed["rows"] if isinstance(observed, dict) else observed
    assert len(rows) == expected["executed_epochs"], "complete attempt history required"
    previous = expected["initial"]
    previous_was_packed = False
    for wanted, actual in zip(expected["rows"], rows, strict=True):
        attempt = wanted["attempt"]
        assert actual["attempt"] == attempt, "attempt order/identity mismatch"
        assert (
            actual["committed"] == wanted["committed"]
        ), f"attempt {attempt} commit mismatch"
        assert (
            actual["failed"] == wanted["failed"]
        ), f"attempt {attempt} protected failure mismatch"
        for phase in ("before", "after"):
            if phase in actual:
                assert packed_snapshot(actual[phase]) == wanted_packed(
                    wanted, phase
                ), f"attempt {attempt} {phase} complete physical-state mismatch"
            elif f"packed_{phase}" in actual:
                value = actual[f"packed_{phase}"]
                assert (
                    isinstance(value, str)
                    and len(value) == (expected["snapshot_bits"] + 3) // 4
                )
                assert (
                    int(value, 16) < 1 << expected["snapshot_bits"]
                ), "packed width overflow"
                assert value.lower() == wanted_packed(
                    wanted, phase
                ), f"attempt {attempt} {phase} packed mismatch"
            else:
                assert phase != "after", "complete after-state required"
        if "after_flow" in actual:
            assert actual["after_flow"] == wanted_flow(
                wanted
            ), f"attempt {attempt} actual transfer/observation mismatch"
        elif "after" in actual:
            # Full observed snapshots carry actual ledgers; derive their deltas
            # from the previous actual snapshot, never from expected results.
            assert (
                not previous_was_packed or "before" in actual
            ), "switching packed to full observations requires a full actual before snapshot"
            source = {
                "before": actual.get("before", previous),
                "after": actual["after"],
                "actions": wanted["actions"],
            }
            assert flow_counters(source) == wanted_flow(
                wanted
            ), f"attempt {attempt} actual ledger mismatch"
        else:
            raise AssertionError("packed state requires complete actual after_flow")
        if "after" in actual:
            previous = actual["after"]
        elif "after_flow" not in actual:
            raise AssertionError("mixed observations lost actual transfer history")
        previous_was_packed = "after" not in actual
        for key in ("errors", "grants", "frame", "actions"):
            if key in actual:
                assert actual[key] == wanted[key], f"attempt {attempt} {key} mismatch"
        if "public_expected" in actual:
            assert (
                actual["public_expected"] == wanted["public_expected"]
            ), f"attempt {attempt} public old-Q mismatch"
        if "public" in actual:
            desired = wanted["public_expected"]
            assert actual["public"]["ready"] == desired["ready"]
            for name, view in desired.items():
                if name == "ready":
                    continue
                assert actual["public"][name]["available"] == view["available"]
                if view["available"]:
                    assert (
                        actual["public"][name]["head"] == view["head"]
                    ), f"attempt {attempt} {name} payload mismatch"
    return len(rows)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--group", choices=("executable", "reference"), default="executable"
    )
    parser.add_argument("--case", required=True)
    parser.add_argument("--observed", type=Path, required=True)
    args = parser.parse_args()
    expected = data()[args.group][args.case]
    epochs = check_case(expected, json.loads(args.observed.read_text()))
    print(  # noqa: T201 - CLI receipt
        json.dumps({"case": args.case, "attempts": epochs, "checked": True})
    )


if __name__ == "__main__":
    main()
