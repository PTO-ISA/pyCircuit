"""Same-Queue replacement at empty, middle and full occupancy."""

import shutil
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest, log_history

arguments = sys.argv[1:]
reset_samples = arguments[-1] == "--reset-samples"
if reset_samples:
    arguments.pop()
target = arguments[-1]
fixture = SourceTest(arguments, None, "queue_timing")
shutil.copytree(
    Path(__file__).with_name("queue_timing"), fixture.sources, dirs_exist_ok=True
)
fixture.compile_source("design")
final = fixture.scratch / "timing.ac"
units = ("design",)
top = "queue_timing.design.Top"
if reset_samples:
    with fixture.interfaces_only("design"):
        fixture.compile_source("reset", "design")
    units = ("design", "reset")
    top = "queue_timing.reset.Top"
linked = fixture.link_saved(units, top, final)
executable, _ = fixture.build_system(linked if target == "t-gfsim" else final, target)

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
if reset_samples:
    # The reset edge observes phase 3 before clearing a full Queue. Initial
    # values and the entire original timing sequence recur after that edge.
    states = (*states[:4], *states)
expected = Counter(
    (2 * cycle + edge, "state", state)
    for cycle, state in enumerate(states)
    for edge in (0, 1)
)
# The exact 10xz0011 payload keeps X and Z distinct in the Queue. Only the
# consumer's nonempty epochs publish it; reset reruns the same producer.
for epoch in (2, 3, 10, 11) if reset_samples else (2, 3):
    expected[(epoch, "payload", ((0x83, 0xCF, 0x10),))] += 1
for workers in (1, 2) if target == "cpp" else (1,):
    arguments = (
        ["--cycles", "11", "--workers", workers]
        if target != "verilog"
        else ["+cycles=11"]
    )
    if reset_samples:
        arguments = [] if target == "verilog" else ["--workers", workers]
    actual = log_history(fixture.run([executable, *arguments]))
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
print(f"transactional Queue replacement {target}: PASS")
