"""Check only public execution status and committed epoch, not diagnostics."""

import json
import pathlib
import re
import sys

text = pathlib.Path(sys.argv[1]).read_text()
expected, epoch = sys.argv[2], int(sys.argv[3])
results = []
for line in text.splitlines():
    try:
        row = json.loads(line.replace('\\"', '"'))
    except json.JSONDecodeError:
        continue
    if isinstance(row, dict) and row.get("kind") == "result":
        results.append(row)
if expected == "RTL_FAILED":
    assert not any(row.get("status") == "TERMINATED" for row in results)
    assert re.search(rf"\bat epoch {epoch}\b", text), text
else:
    assert len(results) == 1, text
    assert results[0]["status"] == expected, text
    assert int(results[0]["epoch_time"]) == epoch, text
