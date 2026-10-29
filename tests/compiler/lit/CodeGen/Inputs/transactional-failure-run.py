"""One real assertion failure through generated Systems and public runtime APIs."""

import shutil
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest, log_history

fixture = SourceTest(sys.argv[1:], None, "failure_test")
shutil.copyfile(
    Path(__file__).with_name("transactional-failure.py"), fixture.sources / "failure.py"
)
fixture.compile_source("failure")
final = fixture.scratch / "failure.ac"
linked = fixture.link_saved(("failure",), "failure_test.failure.Failure", final)
target = sys.argv[-1]
executable, generated = fixture.build_system(
    linked if target == "t-gfsim" else final, target
)
expected = Counter()
for epoch in range(7):
    cycle = epoch // 2
    accepted = max(cycle - 1, 0)
    expected[(epoch, "total", (accepted,))] += 1
    expected[(epoch, "progress", (cycle, accepted, cycle))] += 1
    if cycle:
        expected[(epoch, "attempt", (accepted, cycle))] += 1
        expected[(epoch, "received", (cycle,))] += 1
for workers in (1, 2) if target == "cpp" else (1,):
    args = (
        ["+cycles=5"] if target == "verilog" else ["--cycles", 5, "--workers", workers]
    )
    result = fixture.run([executable, *args], fails=True)
    actual = log_history(result, successful=False)
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }

if target == "cpp":
    bundle = fixture.scratch / target
    runtime = Path(fixture.emitter).parent.parent / "runtime/libpyc6_runtime.a"
    if not runtime.is_file():
        runtime = Path(fixture.emitter).parent.parent / "lib/libpyc6_runtime.a"
    adapter = fixture.scratch / "runtime-check"
    fixture.run(
        [
            fixture.cxx,
            "-std=c++20",
            "-pthread",
            f"-I{fixture.repo / 'include'}",
            f"-I{bundle}",
            Path(__file__).with_name("transactional-failure-runtime.cpp"),
            *(
                bundle / row["path"]
                for row in generated["files"]
                if row["path"].endswith(".cpp") and row["path"] != "simulation_main.cpp"
            ),
            runtime,
            "-o",
            adapter,
        ]
    )
    for workers in (1, 2):
        fixture.run([adapter, workers])
print(f"runtime assertion failure {target}: PASS")
