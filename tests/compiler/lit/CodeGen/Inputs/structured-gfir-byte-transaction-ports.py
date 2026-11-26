"""Resolve the public read availability wire from both target manifests."""

import json
import sys
from pathlib import Path

behavior, final = [json.loads(Path(p).read_text()) for p in sys.argv[1:3]]
assert behavior["source_check_count"] == final["source_check_count"] > 0
ports = behavior["port_availability"]
assert ports == final["port_availability"]
outputs = ports["outputs"]
assert [o["name"] for o in outputs] == ["value", "count"]
wire = outputs[0]["availability"]
assert isinstance(wire, str) and wire and outputs[1]["availability"] is None
controls = [c for c in ports["controls"] if c.get("role") == "availability"]
assert len(controls) == 1 and controls[0]["wire"] == wire
assert controls[0]["width"] == 1 and controls[0]["direction"] == "output"
folder = Path(sys.argv[3])
folder.mkdir(parents=True, exist_ok=True)
(folder / "byte_ports.hpp").write_text(
    f"#pragma once\n#define PYC_VALUE_VALID {wire}\n"
)
(folder / "byte_ports.svh").write_text(f"`define PYC_VALUE_VALID {wire}\n")
