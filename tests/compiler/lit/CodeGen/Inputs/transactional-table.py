"""Independent histories for Table State, static instances and atomic selection."""

import shutil
import sys
import traceback
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest, log_history

fixture = SourceTest(sys.argv[1:], None, "table_case")
case = sys.argv[-1]
inputs = Path(__file__).parent


def copy_source(source, destination):
    shutil.copyfile(
        inputs / f"transactional-table-{source}.py",
        fixture.sources / f"{destination}.py",
    )


if case == "index":
    copy_source("index", "design")
    fixture.compile_source("design")
    fixture.cli(
        "link",
        fixture.scratch / "design",
        "--top",
        "table_case.design.Top",
        "-o",
        fixture.scratch / "final.ac",
        fails=True,
    )
    print("transactional Table index proof: PASS")
    sys.exit(0)

copy_source(case, "design")
if case == "data":
    fixture.compile_source("design")
    units = ("design",)
    # The first five edges update lanes 0,1,2,3,0. Reset observes those
    # committed values before restoring the image and selector together.
    rows = (
        (0, (1, 2, 3, 4), (100, 110, 120, 130)),
        (1, (11, 2, 3, 4), (101, 110, 120, 130)),
        (2, (11, 12, 3, 4), (101, 111, 120, 130)),
        (3, (11, 12, 13, 4), (101, 111, 121, 130)),
        (0, (11, 12, 13, 14), (101, 111, 121, 131)),
        (1, (21, 12, 13, 14), (102, 111, 121, 131)),
        (0, (1, 2, 3, 4), (100, 110, 120, 130)),
        (1, (11, 2, 3, 4), (101, 110, 120, 130)),
    )
    history = []
    for index, regs, counts in rows:
        entries = tuple(
            value
            for pair in zip((10, 20, 30, 40), counts, strict=True)
            for value in pair
        )
        history.append(
            [
                ("regs", regs),
                ("entries", entries),
                ("selected", (index, regs[index], regs[index], counts[index])),
            ]
        )
else:
    copy_source("provider", "provider")
    fixture.compile_source("provider")
    if case == "modules":
        # Compile from the provider interface alone in this representative.
        (fixture.sources / "provider.py").unlink()
        with fixture.interfaces_only("provider"):
            fixture.compile_source("design", "provider")
    else:
        fixture.compile_source("design", "provider")
    units = ("provider", "design")
    history = []
    if case == "modules":
        # A u1 selector visits only lanes 0 and 1. Each selected Method adds
        # STEP=3 plus 1; upper lanes must remain untouched despite the narrow
        # selector having no bit with which to represent their ordinals.
        narrow_rows = ((0, 0, 0, 0), (4, 0, 0, 0), (4, 4, 0, 0), (8, 4, 0, 0))
        for cycle in range(4):
            events = [("narrow", narrow_rows[cycle])]
            for capacity in (4, 2):
                # Default STEP=3 and enumerate index i add 3+i per edge.
                events.extend(
                    ("worker", (capacity, lane, cycle * (3 + lane)))
                    for lane in range(capacity)
                )
                events.append(
                    (
                        "bank",
                        (
                            capacity,
                            cycle * (3 * capacity + capacity * (capacity - 1) // 2),
                            3 * cycle,
                        ),
                    )
                )
            history.append(events)
    else:
        # Phase 0 skips the false Method guard and commits only local/Gate
        # effects. Phase 1 blocks at the selected guard; phase 2 blocks later.
        # Every worker remains zero through both blocked attempts. Phases 3..6
        # write lanes 3,0,1,2 respectively, each adding phase+1 exactly once.
        # Previously used lanes have false guards while the next selected lane
        # remains ready. Phase 7 reads lane 3's nonzero result dynamically.
        rows = (
            (0, (0, 0, 0, 0)),
            (1, (0, 0, 0, 0)),
            (1, (0, 0, 0, 0)),
            (1, (0, 0, 0, 0)),
            (2, (0, 0, 0, 4)),
            (3, (5, 0, 0, 4)),
            (4, (5, 6, 0, 4)),
            (5, (5, 6, 7, 4)),
        )
        for phase, (accepted, workers) in enumerate(rows):
            events = [("progress", (phase, accepted, accepted, workers[phase % 4]))]
            events.extend(
                ("worker", (lane, value)) for lane, value in enumerate(workers)
            )
            if phase in (0, 3, 4, 5, 6):
                events.append(
                    ("commit", (phase, accepted, 0 if phase == 0 else phase + 1))
                )
            if phase in (3, 4, 5, 6):
                events.append(("selected", (phase, 0)))
            history.append(events)

final = fixture.scratch / "final.ac"
if case == "modules":
    for unit in units:
        shutil.copytree(fixture.scratch / unit, fixture.scratch / "evidence" / unit)
    anchor = fixture.scratch / "anchor"
    fixture.link_saved(units, "table_case.design.Top", anchor, resume_target="anchor")
    fixture.cli("link", "--from-anchor", anchor, "--target", "final", "-o", final)
else:
    fixture.cli(
        "link",
        *(fixture.scratch / unit for unit in units),
        "--top",
        "table_case.design.Top",
        "-o",
        final,
    )
expected = Counter(
    (2 * cycle + edge, event, values)
    for cycle, events in enumerate(history)
    for edge in (0, 1)
    for event, values in events
)


def check_backend(target):
    executable, _ = fixture.build_system(final, target)
    for workers in (1, 2) if target == "cpp" else (1,):
        arguments = ["--workers", workers] if target == "cpp" else []
        if case != "data":
            arguments += (
                ["--cycles", len(history)]
                if target == "cpp"
                else [f"+cycles={len(history)}"]
            )
        actual = log_history(fixture.run([executable, *arguments]))
        assert actual == expected, {
            "unexpected": actual - expected,
            "missing": expected - actual,
        }
    print(f"transactional Table {case} {target}: PASS")


failures = []
for target in ("cpp", "verilog"):
    try:
        check_backend(target)
    except Exception as error:
        traceback.print_exc()
        failures.append((target, error))
assert not failures, failures
