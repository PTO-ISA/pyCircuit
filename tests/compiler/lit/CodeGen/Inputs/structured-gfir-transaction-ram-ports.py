"""Use public metadata to find the two legalized response companions."""

import json
import sys
from pathlib import Path

behavior, final = [json.loads(Path(p).read_text()) for p in sys.argv[1:3]]
assert behavior["source_check_count"] == final["source_check_count"] > 0
metadata = behavior["port_availability"]
assert metadata == final["port_availability"]
outputs = metadata["outputs"]
assert [o["name"] for o in outputs] == ["a", "b", "response_bit", "count"]
wires = [o["availability"] for o in outputs[:3]]
assert outputs[3]["availability"] is None
assert all(isinstance(w, str) and w for w in wires) and len(set(wires)) == 3
controls = [c for c in metadata["controls"] if c.get("role") == "availability"]
assert len(controls) == 3 and {c["wire"] for c in controls} == set(wires)
assert all(c["width"] == 1 and c["direction"] == "output" for c in controls)
folder = Path(sys.argv[3])
folder.mkdir(parents=True, exist_ok=True)
macros = list(zip(["PYC_A_VALID", "PYC_B_VALID", "PYC_BIT_VALID"], wires, strict=True))
(folder / "ram_ports.hpp").write_text(
    "#pragma once\n" + "".join(f"#define {key} {value}\n" for key, value in macros)
)
(folder / "ram_ports.svh").write_text(
    "".join(f"`define {key} {value}\n" for key, value in macros)
)
