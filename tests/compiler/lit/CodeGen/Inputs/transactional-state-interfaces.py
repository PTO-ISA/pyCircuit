"""Supplement: parent-owned outputs, global Rule templates and State aliases."""

import shutil
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest, log_history

fixture = SourceTest(
    sys.argv[1:], "parent_state_interfaces", "example_parent_state_interfaces"
)
target = sys.argv[8]
reset = sys.argv[9:] == ["reset"]
if reset:
    shutil.copytree(
        Path(__file__).with_name("transactional_cases") / "state_interfaces",
        fixture.sources,
        dirs_exist_ok=True,
    )
fixture.compile_source("rules")
fixture.compile_source("stage")
with fixture.interfaces_only("rules", "stage"):
    fixture.compile_source("top", "rules", "stage")
final = fixture.scratch / "state-interfaces.ac"
fixture.link_saved(
    ("rules", "stage", "top"), "example_parent_state_interfaces.top.Top", final
)
if reset:
    executable = fixture.build_module(
        final,
        target,
        Path(__file__).with_suffix(".cpp"),
        Path(__file__).with_suffix(".sv"),
    )
    for workers in (1, 2) if target == "cpp" else (1,):
        fixture.run([executable, workers] if target == "cpp" else ["vvp", executable])
    print(f"parent State reset {target}: PASS")
    sys.exit(0)
executable, _ = fixture.build_system(final, target)


# Derive each row from old Q: the first Stage initially sees incremented=0,
# then 4. Its own accepted update cannot forward into the returned sum.
# Once accepted reaches 2, output and internal state both hold.
expected = Counter()
for cycle, (incremented, first, second, accepted) in enumerate(
    ((0, 20, 30, 0), (4, 0, 10, 1), (4, 5, 11, 2), (4, 5, 11, 2))
):
    for epoch in (cycle * 2, cycle * 2 + 1):
        expected[(epoch, "parent", (3, 10, incremented, first, second, first))] += 1
        expected[(epoch, "stage", (accepted, first))] += 1
        expected[(epoch, "stage", (accepted, second))] += 1
for workers in (1, 2) if target == "cpp" else (1,):
    arguments = (
        ["--cycles", "4", "--workers", workers] if target == "cpp" else ["+cycles=4"]
    )
    actual = log_history(fixture.run([executable, *arguments]))
    assert actual == expected, (actual, expected)
print(f"transactional parent State interfaces {target}: PASS")
