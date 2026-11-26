"""Authenticate the two partial outputs and four independent total outputs."""

import json
import sys
from pathlib import Path

behavior, final = [json.loads(Path(path).read_text()) for path in sys.argv[1:3]]
assert behavior["source_check_count"] == final["source_check_count"] > 0
ports = behavior["port_availability"]
assert ports == final["port_availability"]
outputs = ports["outputs"]
assert [row["name"] for row in outputs] == [
    "packet",
    "right_gate",
    "left_committed",
    "right_committed",
    "left_seen",
    "right_seen",
]
wires = [row["availability"] for row in outputs]
assert all(isinstance(wire, str) and wire for wire in wires[:2])
assert wires[0] != wires[1] and wires[2:] == [None] * 4
controls = [row for row in ports["controls"] if row.get("role") == "availability"]
assert len(controls) == 2 and {row["wire"] for row in controls} == set(wires[:2])
assert all(row["width"] == 1 and row["direction"] == "output" for row in controls)
folder = Path(sys.argv[3])
folder.mkdir(parents=True, exist_ok=True)
macros = list(zip(["PYC_PACKET_VALID", "PYC_RIGHT_VALID"], wires[:2], strict=True))
(folder / "child_availability_ports.hpp").write_text(
    "#pragma once\n" + "".join(f"#define {name} {wire}\n" for name, wire in macros)
)
(folder / "child_availability_ports.svh").write_text(
    "".join(f"`define {name} {wire}\n" for name, wire in macros)
)
