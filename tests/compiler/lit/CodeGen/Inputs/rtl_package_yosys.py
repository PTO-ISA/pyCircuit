import json
import sys
from pathlib import Path

modules = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))["modules"]
(implementation,) = modules["pyc_root"]["cells"].values()
ports = modules[implementation["type"]]["ports"]

# The fixture adds eight to Python's -7 // 3 and -7 % 3.
widths = (len(ports["q"]["bits"]), len(ports["r"]["bits"]))
assert widths == (5, 10), widths
