"""Same-Queue replacement at empty, middle and full occupancy."""

import shutil
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest, log_history

fixture = SourceTest(sys.argv[1:], None, "queue_timing")
target = sys.argv[-1]
shutil.copytree(
    Path(__file__).with_name("queue_timing"), fixture.sources, dirs_exist_ok=True
)
fixture.compile_source("design")
final = fixture.scratch / "timing.ac"
fixture.cli(
    "link", fixture.scratch / "design", "--top", "queue_timing.design.Top", "-o", final
)
executable, _ = fixture.build_system(final, target)

# phase, pushes, replacements, pops, last popped payload. Empty phase 0
# cannot bypass; full phase 4 cannot borrow room. Replacement can commit only
# at middle occupancy (phases 1 and 5), never full phase 3 or empty phase 7.
states = (
    (0, 0, 0, 0, 0),
    (1, 1, 0, 0, 0),
    (2, 1, 1, 0, 0),
    (3, 2, 1, 0, 0),
    (4, 2, 1, 0, 0),
    (5, 2, 1, 1, 12),
    (6, 2, 2, 1, 12),
    (7, 2, 2, 2, 23),
    (8, 2, 2, 2, 23),
    (9, 3, 2, 2, 23),
    (10, 3, 2, 3, 22),
)
expected = Counter(
    (2 * cycle + edge, "state", state)
    for cycle, state in enumerate(states)
    for edge in (0, 1)
)
for workers in (1, 2) if target == "cpp" else (1,):
    arguments = (
        ["--cycles", "11", "--workers", workers] if target == "cpp" else ["+cycles=11"]
    )
    actual = log_history(fixture.run([executable, *arguments]))
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
print(f"transactional Queue replacement {target}: PASS")
print("t-gfsim: NOT RUN (Queue support remains an implementation gap)")
