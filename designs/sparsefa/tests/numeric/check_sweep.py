"""Check emitted exponential values against independent Python libm."""

import json
import math
import struct
import sys
from pathlib import Path


def check(path):
    rows = {}
    terminated = False
    for line in Path(path).read_text().splitlines():
        record = json.loads(line)
        if record.get("kind") == "result":
            terminated = record["status"] == "TERMINATED" and record["error"] is None
        if record.get("kind") != "log":
            continue
        a, b = [int(value["value"]) for value in record["values"]]
        x, y = [struct.unpack("<f", struct.pack("<I", v))[0] for v in (a, b)]
        expected = math.exp(x)
        rows[a] = (abs(y - expected) / expected, abs(y - expected))
    assert terminated, "generated System did not terminate successfully"
    assert len(rows) == 4096, f"incomplete sweep: {len(rows)} samples"
    relative = max(v[0] for v in rows.values())
    absolute = max(v[1] for v in rows.values())
    assert relative <= 4e-5, f"exp relative error {relative}"
    return {
        "points": len(rows),
        "max_relative_error": relative,
        "max_absolute_error": absolute,
    }


if __name__ == "__main__":
    print(
        json.dumps(check(sys.argv[1]), indent=2)
    )  # noqa: T201 -- command-line validation summary.
