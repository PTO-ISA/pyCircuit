"""Supplement: independent natural-width, shift and SSA snapshot oracles."""

import shutil
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest, log_history

fixture = SourceTest(sys.argv[1:], None, "constraints")
scratch, sources = fixture.scratch, fixture.sources
target = sys.argv[-1]
run = fixture.run
shutil.copytree(
    Path(__file__).with_name("transactional_cases") / "constraints",
    sources,
    dirs_exist_ok=True,
)
units = ("provider", "ignored_first", "typed_first", "top")
for unit in units[:-1]:
    fixture.compile_source(unit)
with fixture.interfaces_only(*units[:-1]):
    fixture.compile_source("top", *units[:-1])
final = scratch / "constraints.ac"
fixture.link_saved(units, "constraints.top.Top", final)
executable, _ = fixture.build_system(final, target)


# Natural eight-bit addition wraps to four before the u16 result binding.
# Both shared-literal constraints fix eight-bit arithmetic: 255 + 1 wraps
# to zero before widening. The captured 254 + 1 + 1 also wraps to zero,
# constrained through nested unary uses. Rebinding preserves the old snapshot.
expected = Counter()
for cycle in range(3):
    for epoch in (2 * cycle, 2 * cycle + 1):
        expected[(epoch, "count", (250 + cycle,))] = 2
        expected[(epoch, "sample", (250 + cycle, 7, 250 + cycle, 250 + cycle))] = 2
        expected[(epoch, "wide", (4, 0, 0, 0))] = 1
        # Shifts execute at the left operand's width before result widening;
        # the full wide shift amount is retained (256 is not truncated to 0).
        expected[(epoch, "types", (2, 0, 64, 1, 0))] = 1
for workers in (1, 2) if target == "cpp" else (1,):
    args = ["--cycles", "3", "--workers", workers] if target == "cpp" else ["+cycles=3"]
    actual = log_history(run([executable, *args]))
    assert actual == expected, (actual, expected)
print(f"transactional constraints {target}: PASS")
