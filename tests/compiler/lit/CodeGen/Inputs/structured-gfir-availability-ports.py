"""Validate public availability metadata and expose legalized companion names."""
import json
from pathlib import Path
import sys

behavior, final = [json.loads(Path(p).read_text()) for p in sys.argv[1:3]]
assert behavior["source_check_count"] == final["source_check_count"] > 0
metadata = behavior["port_availability"]
assert metadata == final["port_availability"], "behavior/final port metadata differs"
outputs = metadata["outputs"]
assert [o["name"] for o in outputs] == [
    "packet", "projected", "independent", "accepted"
], outputs
wires = [o.get("availability") for o in outputs]
assert all(isinstance(w, str) and w for w in wires[:2]), wires
assert wires[0] != wires[1] and wires[2:] == [None, None], wires
controls = [c for c in metadata["controls"] if c.get("role") == "availability"]
assert len(controls) == 2, controls
assert {c["wire"] for c in controls} == set(wires[:2]), controls
assert all(c["direction"] == "output" and c["width"] == 1 for c in controls)
destination = Path(sys.argv[3])
destination.mkdir(parents=True, exist_ok=True)
macros = list(zip(["PYC_PACKET_VALID", "PYC_PROJECTED_VALID"], wires[:2]))
(destination / "availability_ports.hpp").write_text(
    "#pragma once\n" + "".join(f"#define {name} {wire}\n" for name, wire in macros)
)
(destination / "availability_ports.svh").write_text(
    "".join(f"`define {name} {wire}\n" for name, wire in macros)
)
