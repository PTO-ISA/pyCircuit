"""Independent defaults, immutable-local and old-state execution histories."""

import shutil
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest, log_history

fixture = SourceTest(sys.argv[1:], None, "partial_case")
target, case = sys.argv[-2:]
inputs = Path(__file__).parent


def copy_case(name, destination):
    shutil.copyfile(
        inputs / f"transactional-partial-updates-{name}.py",
        fixture.sources / f"{destination}.py",
    )


copy_case("state" if case == "failure" else case, "design")
if case == "constructors":
    copy_case("types", "types")
    fixture.compile_source("types")
    (fixture.sources / "types.py").unlink()
    with fixture.interfaces_only("types"):
        fixture.compile_source("design", "types")
    units = ("types", "design")
    history = [
        [("defaults", (0, 0, 3, 255, 1, 0, 1)), ("explicit", (7, 4, 11, 255, 1, 1, 1))]
    ]
elif case == "locals":
    fixture.compile_source("design")
    units = ("design",)
    # 10xz01zx: value=0x84, known=0xcc, Z=0x12. Replacing other
    # fields must preserve all three planes, not only its visible value bits.
    payload = (0x84, 0xCC, 0x12)
    history = [
        [
            ("local", (18, 199, 1, 3, 160, 3, payload)),
            ("formal", (7, 169, 3, 160)),
            ("formal-output", (0, 0)),
        ],
        [
            ("local", (28, 199, 0, 3, 160, 3, payload)),
            ("formal", (7, 169, 3, 160)),
            ("formal-output", (7, 169)),
        ],
    ]
else:
    if case == "state":
        copy_case("reject", "reject")
        fixture.compile_source("reject")
        for top in ("Overlap", "CrossRule"):
            fixture.cli(
                "link",
                fixture.scratch / "reject",
                "--top",
                f"partial_case.reject.{top}",
                "-o",
                fixture.scratch / f"{top}.ac",
                fails=True,
            )
    fixture.compile_source("design")
    units = ("design",)
    # The phase-1 guard cancels all three partial proposals. Reset at phase 3
    # restores the declared full value; reset Work still observes old Q.
    rows = (
        (0, 2, 165),
        (1, 3, 178),
        (2, 3, 178),
        (3, 4, 195),
        (0, 2, 165),
        (1, 3, 178),
        (2, 3, 178),
    )
    if case == "failure":
        # No logs from the failing phase-4 epoch may become visible, including
        # observations and partial-write Method logs from the other Rule.
        rows = rows[:4]
    history = []
    for phase, tag, word in rows:
        events = [("phase", (phase,)), ("state", (tag, word))]
        if phase != 1:
            events.append(("old", (tag, word)))
        history.append(events)

final = fixture.scratch / "final.ac"
top = "Failure" if case == "failure" else "Top"
linked = fixture.link_saved(units, f"partial_case.design.{top}", final)
# Reopen the original Linked artifact through a saved Routed anchor. The same
# independent histories must hold after both continuation boundaries.
anchor = fixture.scratch / "anchor"
fixture.cli("link", "--from-linked", linked, "--target", "anchor", "-o", anchor)
anchored = fixture.scratch / "anchored.ac"
fixture.cli("link", "--from-anchor", anchor, "--target", "final", "-o", anchored)
executable, generated = fixture.build_system(anchored, target)
expected = Counter(
    (2 * cycle + edge, event, values)
    for cycle, events in enumerate(history)
    for edge in (0, 1)
    for event, values in events
)
for workers in (1, 2) if target == "cpp" else (1,):
    arguments = ["--workers", workers] if target == "cpp" else []
    if case != "state":
        cycles = len(history) + (case == "failure")
        arguments += ["--cycles", cycles] if target == "cpp" else [f"+cycles={cycles}"]
    actual = log_history(
        fixture.run([executable, *arguments], fails=case == "failure"),
        successful=case != "failure",
    )
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
if case == "failure" and target == "cpp":
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
            inputs / "transactional-failure-runtime.cpp",
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
        fixture.run([adapter, workers, 8])
print(f"partial updates {case} {target}: PASS")
