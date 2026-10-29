"""Uninitialized State is X before a write and after reset, on both targets."""

import shutil
import sys
from pathlib import Path

from transactional_test_support import SourceTest

fixture = SourceTest(sys.argv[1:], None, "uninitialized")
target = sys.argv[-1]
shutil.copyfile(
    Path(__file__).with_name("transactional_cases") / "uninitialized/design.py",
    fixture.sources / "design.py",
)
fixture.compile_source("design")
final = fixture.scratch / "uninitialized.ac"
fixture.link_saved(("design",), "uninitialized.design.Top", final)
executable = fixture.build_module(
    final, target, Path(__file__).with_suffix(".cpp"), Path(__file__).with_suffix(".sv")
)
for workers in (1, 2) if target == "cpp" else (1,):
    fixture.run([executable, workers] if target == "cpp" else ["vvp", executable])
print(f"uninitialized State {target}: PASS")
