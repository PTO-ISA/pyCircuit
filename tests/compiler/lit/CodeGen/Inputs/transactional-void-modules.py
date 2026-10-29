"""Void instances retain identity and autonomous work across source-unit reload."""

import shutil
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest, log_history

fixture = SourceTest(sys.argv[1:], None, "void_test")
shutil.copytree(
    Path(__file__).with_name("transactional-void"), fixture.sources, dirs_exist_ok=True
)
fixture.compile_source("Owners")
# Parent compilation must need neither the provider's source nor its body.
(fixture.sources / "Owners.py").unlink()
with fixture.interfaces_only("Owners"):
    fixture.compile_source("Top", "Owners")
final = fixture.scratch / "void.ac"
linked = fixture.link_saved(("Owners", "Top"), "void_test.Top.Top", final)
target = sys.argv[-1]
executable, _ = fixture.build_system(linked if target == "t-gfsim" else final, target)

# Each cycle samples committed state twice, then commits one rising-edge update.
# Input step starts at two and increases once per cycle. Method reads after add
# must still see old Q; both names for the first instance must see the same Q.
expected = Counter()
first = second = registered = automatic = 0
for cycle in range(7):
    step = cycle + 2
    for epoch in (2 * cycle, 2 * cycle + 1):
        expected[
            (epoch, "counters", (first, first, second, first + step, second + 5))
        ] += 1
        expected[(epoch, "registered", (registered,))] += 1
        expected[(epoch, "automatic", (automatic,))] += 1
    first += step
    second += 5
    registered += step
    automatic += step + 1

for workers in (1, 2) if target == "cpp" else (1,):
    arguments = (
        ["+cycles=7"] if target == "verilog" else ["--cycles", 7, "--workers", workers]
    )
    actual = log_history(fixture.run([executable, *arguments]))
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
print(f"void Modules {target}: PASS")
