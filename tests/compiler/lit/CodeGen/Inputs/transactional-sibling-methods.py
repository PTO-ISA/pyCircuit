"""Two callers share one accumulator; another instance progresses independently."""

import shutil
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest, log_history

fixture = SourceTest(sys.argv[1:], None, "call_test")
shutil.copytree(
    Path(__file__).with_name("transactional-calls"), fixture.sources, dirs_exist_ok=True
)
# The generic caller compiles before its provider exists. Top sees interfaces only.
fixture.compile_source("Callers")
fixture.compile_source("Accumulator")
with fixture.interfaces_only("Callers", "Accumulator"):
    fixture.compile_source("Top", "Callers", "Accumulator")
final = fixture.scratch / "calls.ac"
linked = fixture.link_saved(
    ("Callers", "Accumulator", "Top"), "call_test.Top.Top", final
)
target = sys.argv[-1]
executable, _ = fixture.build_system(linked if target == "t-gfsim" else final, target)

# Hand recurrence: +3, +5, late guard rejects, +5, then callee guard rejects.
# The separate instance adds one every cycle; caller guards stop after cycle 5.
expected = Counter()
for cycle, (total, first, second) in enumerate(
    (
        (0, 0, 0),
        (3, 1, 0),
        (8, 1, 1),
        (8, 1, 1),
        (13, 1, 2),
        (13, 1, 2),
        (13, 1, 2),
        (13, 1, 2),
    )
):
    for epoch in (2 * cycle, 2 * cycle + 1):
        expected[(epoch, "total", (total,))] += 1
        expected[(epoch, "total", (cycle,))] += 1
        expected[(epoch, "progress", (cycle, first, second))] += 1
        if cycle == 0:
            expected[(epoch, "first", (2, 3, 3))] += 1
        if cycle in (1, 3):
            expected[(epoch, "second", (total,))] += 1

for workers in (1, 2) if target == "cpp" else (1,):
    args = (
        ["+cycles=8"] if target == "verilog" else ["--cycles", 8, "--workers", workers]
    )
    result = fixture.run([executable, *args])
    actual = log_history(result)
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
print(f"cross-module transactions {target}: PASS")

if target == "t-gfsim":
    executable = fixture.build_runtime_consumer(target)
    actual = log_history(fixture.run([executable, "--cycles", 8]))
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
