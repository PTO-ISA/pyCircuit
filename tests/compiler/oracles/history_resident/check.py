"""Check complete observed epoch snapshots against independent resident rows.

Input is JSON ``{"rows": [{"epoch": 0, "after": <complete snapshot>}, ...]}``
or the equivalent fixed-width ``packed_after`` decimal integer. Exact row count
is mandatory. Optional before/work snapshots are checked when provided.
"""

import argparse
import json
from pathlib import Path

from generate import data
from models import packed_snapshot


def compare(kind, actual, expected, group="cases"):
    rows = actual["rows"]
    golden = expected[group][kind]["rows"]
    assert len(rows) == len(golden), ("epoch count", len(rows), len(golden))
    for observed, reference in zip(rows, golden, strict=True):
        assert observed["epoch"] == reference["epoch"], ("epoch", observed)
        assert (
            "after" in observed or "packed_after" in observed
        ), "missing full after snapshot"
        for phase in ("before", "work", "after"):
            if phase in observed:
                assert observed[phase] == reference[phase], (
                    kind,
                    reference["epoch"],
                    reference["label"],
                    phase,
                )
                packed_snapshot(kind, observed[phase])
            if f"packed_{phase}" in observed:
                assert int(observed[f"packed_{phase}"]) == int(
                    reference[f"packed_{phase}"]
                ), (kind, reference["epoch"], reference["label"], f"packed_{phase}")
        for key in ("taken", "grants", "writes"):
            if key in observed:
                assert observed[key] == reference[key], (kind, reference["epoch"], key)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--kind", choices=("rob", "isq"), required=True)
    parser.add_argument("--observed", type=Path, required=True)
    parser.add_argument(
        "--scenario", choices=("history", "reservations", "system"), default="history"
    )
    args = parser.parse_args()
    expected = data()
    group = {"history": "cases", "reservations": "witnesses", "system": "systems"}[
        args.scenario
    ]
    compare(args.kind, json.loads(args.observed.read_text()), expected, group)
    print(  # noqa: T201 - command-line receipt
        f"{args.kind} {args.scenario}: complete {expected[group][args.kind]['executed_epochs']}-epoch snapshots matched"
    )


if __name__ == "__main__":
    main()
