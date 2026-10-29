"""Independent histories for paused transitions, old-Q reads and wrapping."""

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
fixture = SourceTest(arguments, None, "state_machine")
shutil.copyfile(
    Path(__file__).with_name("transactional-state-machine-design.py"),
    fixture.sources / "design.py",
)
fixture.compile_source("design")
shutil.copyfile(
    Path(__file__).with_name(
        "transactional-state-reset-system.py"
        if reset_samples
        else "transactional-state-machine-system.py"
    ),
    fixture.sources / "system.py",
)
with fixture.interfaces_only("design"):
    fixture.compile_source("system", "design")
final = fixture.scratch / "final.ac"
linked = fixture.link_saved(("design", "system"), "state_machine.system.Top", final)
executable, _ = fixture.build_system(linked if target == "t-gfsim" else final, target)

# Receive pauses when permit is zero. Each selected action observes its entry
# stage, including after assignment; it never executes the next stage early.
# The separate reset System fixture retains the currently unsupported sample
# schedule. --reset-samples retains its executable oracle but is NOT RUN in lit
# until the transactional frontend admits that public decorator option.
states = (
    (0, 0, 0, 3, 6, 250),
    (0, 1, 0, 3, 7, 4),
    (1, 0, 0, 0, 0, 14),
    (2, 1, 0, 0, 1, 24),
    (0, 0, 1, 0, 2, 34),
)
if reset_samples:
    # Reset still observes old Q at both Work samples. Its rising edge restores
    # initial State instead of committing the Rule proposals made at that epoch.
    states += ((0, 1, 1, 0, 3, 44), *states[:4])
expected = Counter()
for cycle, state in enumerate(states):
    events = [("state", state)]
    stage, permit, *_ = state
    if stage == 0 and permit:
        events.append(("receive", (0, 0)))
    elif stage == 1:
        events.append(("compute", (1,)))
    elif stage == 2:
        events.append(("publish", (2,)))
    for edge in (0, 1):
        for event, values in events:
            expected[(2 * cycle + edge, event, values)] += 1

for workers in (1, 2) if target == "cpp" else (1,):
    options = (
        ["--cycles", len(states), "--workers", workers]
        if target != "verilog"
        else [f"+cycles={len(states)}"]
    )
    if reset_samples:
        options = [] if target == "verilog" else ["--workers", workers]
    actual = log_history(fixture.run([executable, *options]))
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
print(f"transactional state machine {target}: PASS")
