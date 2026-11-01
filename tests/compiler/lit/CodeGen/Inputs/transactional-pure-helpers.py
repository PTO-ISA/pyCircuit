"""Independent histories for pure payloads, transactions and saved stages."""

import shutil
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest, log_history

# Fixed arithmetic: u8 addition/shift wraps before explicit u16 conversion.
DATA = [
    [("data", (4, 5, 5, 14, 4, 244, 4, 30)), ("nominal", (5, 1, 1, 1))],
    [("data", (5, 6, 6, 15, 5, 246, 6, 40)), ("nominal", (6, 1, 1, 1))],
    [("data", (6, 7, 7, 16, 6, 248, 8, 50)), ("nominal", (6, 0, 1, 0))],
    [("data", (7, 8, 8, 17, 7, 250, 9, 50)), ("nominal", (7, 0, 1, 0))],
]

# The phase-1 Method guard cancels proposals with capacity still available.
# At phases 3/4 the full output stalls both owners; the phase-4 pop cannot
# lend same-edge capacity. Method logs after writes still observe committed Q.
INTEGRATION = [
    [("total", (0,)), ("state", (0, 0)), ("add", (0, 1))],
    [("total", (1,)), ("state", (1, 1))],
    [("total", (1,)), ("state", (2, 1)), ("add", (1, 4))],
    [("total", (4,)), ("state", (3, 2))],
    [("total", (4,)), ("state", (4, 2)), ("consume", (1,))],
    [("total", (4,)), ("state", (5, 2)), ("add", (4, 10)), ("consume", (4,))],
    [("total", (10,)), ("state", (6, 3)), ("consume", (10,))],
    [("total", (10,)), ("state", (7, 3))],
]
CROSS = [[("cross", values)] for values in ((255, 254), (0, 0), (1, 2), (2, 4))]

fixture = SourceTest(sys.argv[1:], None, "helper_case")
target, case = sys.argv[-2:]
inputs = Path(__file__).parent
if case == "data":
    shutil.copyfile(
        inputs / "transactional-helper-effect-design.py", fixture.sources / "effect.py"
    )
    fixture.compile_source("effect", fails=True)
    (fixture.sources / "effect.py").unlink()
if case == "cross":
    shutil.copytree(
        inputs / "transactional-helper-cross", fixture.sources, dirs_exist_ok=True
    )
    fixture.compile_source("provider")
    # Only the published interface is visible when the consumer is compiled.
    (fixture.sources / "provider.py").unlink()
    with fixture.interfaces_only("provider"):
        fixture.compile_source("consumer", "provider")
    # An interface alone cannot supply the executable helper closure at link.
    fixture.cli(
        "link",
        fixture.scratch / "consumer",
        "--top",
        "helper_case.consumer.Top",
        "-o",
        fixture.scratch / "missing.ac",
        fails=True,
    )
    units, top, history = ("provider", "consumer"), "consumer.Top", CROSS
else:
    shutil.copyfile(
        inputs / f"transactional-helper-{case}-design.py", fixture.sources / "design.py"
    )
    fixture.compile_source("design")
    units, top = ("design",), "design.Top"
    history = DATA if case == "data" else INTEGRATION

final = fixture.scratch / "final.ac"
linked = fixture.link_saved(units, f"helper_case.{top}", final)
artifacts = [final]
if case == "cross":
    # The saved source-level boundary retains callable authority. Expansion is
    # a later lowering step, and repeating it on its helper-free result is inert.
    saved = linked.read_text()
    assert "func.func" in saved and "func.call" in saved
    first, second = fixture.scratch / "inlined.ac", fixture.scratch / "inlined-again.ac"
    fixture.run([fixture.opt, linked, "--ac-inline-pure-helpers", "-o", first])
    fixture.run([fixture.opt, first, "--ac-inline-pure-helpers", "-o", second])
    assert first.read_bytes() == second.read_bytes()
    assert "func.call" not in first.read_text()

    anchor = fixture.scratch / "anchor"
    fixture.cli("link", "--from-linked", linked, "--target", "anchor", "-o", anchor)
    portable_anchor = fixture.scratch / "portable-anchor"
    shutil.copytree(anchor, portable_anchor)
    shutil.rmtree(anchor)
    anchored = fixture.scratch / "anchored.ac"
    fixture.cli(
        "link", "--from-anchor", portable_anchor, "--target", "final", "-o", anchored
    )
    portable_final = fixture.scratch / "portable-final.ac"
    shutil.copyfile(anchored, portable_final)
    anchored.unlink()
    shutil.rmtree(portable_anchor)
    artifacts.append(portable_final)

expected = Counter(
    (2 * cycle + edge, event, values)
    for cycle, events in enumerate(history)
    for edge in (0, 1)
    for event, values in events
)
for index, artifact in enumerate(artifacts):
    executable, _ = fixture.build_system(artifact, target, name=f"{target}-{index}")
    for workers in (1, 2) if target == "cpp" else (1,):
        options = (
            [f"+cycles={len(history)}"]
            if target == "verilog"
            else ["--cycles", len(history), "--workers", workers]
        )
        actual = log_history(fixture.run([executable, *options]))
        assert actual == expected, {
            "unexpected": actual - expected,
            "missing": expected - actual,
        }
print(f"pure helpers {case} {target}: PASS")
