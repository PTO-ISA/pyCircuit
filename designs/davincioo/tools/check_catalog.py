#!/usr/bin/env python3
"""Validate the frozen design inventory; this is not a hardware execution gate."""

from __future__ import annotations

import json
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    "SPE": 110,
    "SMT": 26,
    "TMU": 21,
    "VEC": 23,
    "CUBE": 18,
    "MEM": 22,
    "GPE": 20,
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def main() -> None:
    data = json.loads((ROOT / "catalog.json").read_text())
    modules = data["modules"]
    require(set(data["ndf_levels"]) == {"L0", "L1", "L2"}, "NDF refinement axis drift")
    require(data["ndf_levels"]["L0"] == "architectural intent", "NDF L0 drift")
    require(data["ndf_levels"]["L1"] == "observable behavior", "NDF L1 drift")
    require(set(data["hardware_levels"]) == {"H1", "H2", "H3"}, "hardware axis drift")
    ids = [row["candidate_id"] for row in modules]
    require(len(ids) == len(set(ids)) == 240, "expected 240 unique H3 candidates")
    require(set(ids) == set(data["source"]["candidate_ids"]), "source coverage drift")
    require(dict(Counter(row["h1"] for row in modules)) == EXPECTED, "H1 counts drift")
    groups = {(row["h1"], row["h2"]) for row in modules}
    require(len(groups) == 31, "expected 31 H2 groups")
    sources = {item["path"]: item for item in data["source"]["files"]}
    for source in sources.values():
        require(
            bool(re.fullmatch(r"[0-9a-f]{64}", source["sha256"])), "bad source hash"
        )
        require(source["line_count"] > 0, "empty source evidence")
    cards: set[str] = set()
    for row in modules:
        name = row["candidate_id"]
        require(
            row["hierarchy_level"] == "H3" and row["ndf_level"] == "L2",
            f"{name}: axes mixed",
        )
        require(not ({"l1", "l2", "l3"} & row.keys()), f"{name}: legacy hardware keys")
        require(
            name.startswith(f"DAV-{row['h1']}-{row['h2']}-{row['h3']}-"),
            f"{name}: namespace drift",
        )
        require(
            row["disposition_recommendation"]
            in {"leaf", "state_schema", "interface", "alias", "assembly", "review"},
            f"{name}: invalid disposition",
        )
        require(row["evidence"], f"{name}: missing evidence")
        for evidence in row["evidence"]:
            require(evidence["path"] in sources, f"{name}: unhashed evidence")
            require(
                0 < evidence["line"] <= sources[evidence["path"]]["line_count"],
                f"{name}: invalid source line",
            )
        for direction in ("inputs", "outputs"):
            if not row[direction]:
                require(
                    row["disposition_recommendation"]
                    in {"alias", "state_schema", "interface", "review"},
                    f"{name}: leaf without {direction}",
                )
            for port in row[direction]:
                require(
                    all(
                        isinstance(port[k], str) and port[k]
                        for k in ("name", "type", "meaning", "status")
                    ),
                    f"{name}: incomplete port",
                )
                require(
                    port["status"] in {"declared", "proposed", "unresolved"},
                    f"{name}: invalid port evidence",
                )
                require(
                    not port["type"].startswith(("Queue[", "ac.Queue[")),
                    f"{name}: expose payload, not wrapper",
                )
                if port["status"] == "declared":
                    require(
                        "TBD" not in port["type"], f"{name}: unresolved declared type"
                    )
        path = ROOT / row["card"]
        require(
            path.is_relative_to(ROOT) and ".." not in path.parts,
            f"{name}: escaping card",
        )
        require(path.is_file(), f"{name}: missing card")
        require(row["card"] not in cards, f"{name}: duplicate card")
        cards.add(row["card"])
        text = path.read_text()
        require(
            name in text and "## Inputs" in text and "## Outputs" in text,
            f"{name}: card lacks I/O",
        )
        require(
            "NDF refinement: **L2" in text and "Hardware hierarchy: **H3**" in text,
            f"{name}: card axes drift",
        )
    assemblies = data["assemblies"]
    require(len(assemblies) == 38, "expected 7 H1 + 31 H2 assembly cards")
    require(
        Counter(row["hierarchy_level"] for row in assemblies) == {"H1": 7, "H2": 31},
        "assembly hierarchy drift",
    )
    require(
        {(row["h1"], row["h2"]) for row in assemblies if row["hierarchy_level"] == "H2"}
        == groups,
        "assembly coverage drift",
    )
    for row in assemblies:
        require(
            row["ndf_level"] == "L2" and row["inputs"] and row["outputs"],
            f"{row['key']}: incomplete assembly boundary",
        )
        require((ROOT / row["card"]).is_file(), f"{row['key']}: missing assembly card")
    checklist = (ROOT / "MODULE_CHECKLIST.md").read_text()
    for name in ids:
        require(
            checklist.count(f"[{name} —") == 1,
            f"{name}: missing or duplicated checklist entry",
        )
    for path in ROOT.rglob("*.md"):
        for target in re.findall(r"\]\(([^)]+)\)", path.read_text()):
            if target.startswith(("https://", "http://", "#")):
                continue
            require(
                (path.parent / target.split("#", 1)[0]).is_file(),
                f"{path}: broken local link {target}",
            )
    sys.stdout.write(
        json.dumps(
            {
                "status": "passed",
                "ndf": ["L0 intent", "L1 behavior", "L2 microarchitecture"],
                "h1": 7,
                "h2": 31,
                "h3_candidates": 240,
                "cards": len(cards) + len(assemblies),
                "runtime_verified_by_this_check": False,
            }
        )
        + "\n"
    )


if __name__ == "__main__":
    main()
