"""Resolve public view/transfer/derived companions without private-name guesses."""

import json
import sys
from pathlib import Path

behavior, final = [json.loads(Path(path).read_text()) for path in sys.argv[1:3]]
assert behavior["source_check_count"] == final["source_check_count"] > 0
ports = behavior["port_availability"]
assert ports == final["port_availability"]
outputs = ports["outputs"]
assert [row["name"] for row in outputs] == [
    "viewed",
    "transferred",
    "derived",
    "in_ready",
    "in_valid",
    "head",
    "out_valid",
    "out_data",
    "moves",
    "seen",
]
wires = [row["availability"] for row in outputs[:3]]
assert all(isinstance(wire, str) and wire for wire in wires)
assert len(set(wires)) == 3
assert all(row["availability"] is None for row in outputs[3:])
controls = [row for row in ports["controls"] if row.get("role") == "availability"]
assert len(controls) == 3 and {row["wire"] for row in controls} == set(wires)
assert all(row["direction"] == "output" and row["width"] == 1 for row in controls)
folder = Path(sys.argv[3])
folder.mkdir(parents=True, exist_ok=True)
macros = list(
    zip(["PYC_VIEW_VALID", "PYC_MOVE_VALID", "PYC_DERIVED_VALID"], wires, strict=True)
)
(folder / "fifo_ports.hpp").write_text(
    "#pragma once\n" + "".join(f"#define {name} {wire}\n" for name, wire in macros)
)
(folder / "fifo_ports.svh").write_text(
    "".join(f"`define {name} {wire}\n" for name, wire in macros)
)
