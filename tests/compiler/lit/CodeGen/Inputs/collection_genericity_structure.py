"""Generic instances retain one source-owned definition per module."""

import json
import re
import sys
from pathlib import Path

bundle = json.loads(Path(sys.argv[1]).read_text())
groups = bundle["source_groups"]
assert len(groups) == 3, "N3/N5 and bits/struct bindings emitted extra source groups"
by_source = {group["source"]["path"]: group for group in groups}
assert set(by_source) == {"child.py", "family.py", "top.py"}
for name in ("family", "child"):
    header = by_source[f"{name}.py"]["header"]
    matches = re.findall(r"\bclass\s+" + name + r"\s+final\s*:", header)
    assert len(matches) == 1, "generic module definition was cloned for closed actuals"
