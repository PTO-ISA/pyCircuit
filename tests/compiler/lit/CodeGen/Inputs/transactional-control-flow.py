"""Independent payload and admission histories for sequential control flow."""

import shutil
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest, log_history

# Arithmetic is evaluated from the source constants, independently of the DUT.
# Phase 2's selected false Method guard holds the whole calculate transaction.
scalar_states = ((0, 0), (1, 18), (2, 45), (2, 45), (3, 70), (4, 98))
scalar_results = (
    (10, 11, 3, 16, 18, 20),
    (11, 12, 5, 17, 27, 30),
    None,
    (13, 13, 7, 20, 25, 40),
    (14, 14, 7, 21, 28, 40),
    (15, 15, 7, 22, 29, 40),
)
scalar_history = []
for phase, (state, result) in enumerate(
    zip(scalar_states, scalar_results, strict=True)
):
    events = [("state", (phase, *state)), ("constant_return", (7,))]
    if result is not None:
        events.append(("branch", result))
        if phase != 0:
            events.append(("tail", (result[-2],)))
    scalar_history.append(events)

# The first item 10 is retained by a false guard at phase 1, then consumed once
# despite repeated formal reads at phase 2. The next item 13 arrives at phase 3.
# Early-return paths at phases 0/5 do not require an item in the empty Queue.
input_states = ((0, 0), (0, 5), (0, 5), (1, 21), (1, 21), (2, 28), (2, 5))
input_history = [
    [("state", (phase, *state))]
    + ([("read", (10, 11, 10, 0))] if phase == 2 else [])
    + ([("read", (13, 15, 13, 1))] if phase == 4 else [])
    for phase, state in enumerate(input_states)
]

# Reading the formal in a guard requires a nonempty Queue even when the body
# would return early. The guard observes without consuming: phase 2 still reads
# 10 in its body, once, and phase 4 receives 13.
guarded_states = ((0, 0), (0, 0), (0, 0), (1, 21), (1, 21), (2, 28), (2, 28))
guarded_history = [
    [("state", (phase, *state)), *input_history[phase][1:]]
    for phase, state in enumerate(guarded_states)
]

# Enum/Struct locals join in the Method, then drive independent tuple outputs
# through different Rule return paths. Stored observations lag one rising edge.
nominal_history = [
    [
        ("state", values),
        (
            "four_state",
            (
                ((8, 12, 1), (133, 207, 16), 1)
                if phase == 0
                else ((2, 6, 8), (41, 111, 128), 2)
            ),
        ),
    ]
    for phase, values in enumerate(
        ((0, 0, 1, 0), (1, 11, 0, 1), (2, 22, 0, 2), (3, 13, 0, 3))
    )
]

# The untouched arm of a formal join is not itself a read. Passing the formal
# to ignore at phase 1 still consumes 10; the later read therefore receives 13.
# Other paths can complete with an empty Queue, including after rebinding value.
unused_history = [
    [("state", (phase, phase)), ("done", (phase,))]
    + ([("ignored", (9,))] if phase == 1 else [])
    + ([("read", (13,))] if phase == 4 else [])
    for phase in range(7)
]

arguments = sys.argv[1:]
target = arguments[-1]
cases = [("flow", scalar_history)]
if target != "t-gfsim":
    cases += [
        ("inputs", input_history),
        ("guarded", guarded_history),
        ("nominal", nominal_history),
        ("unused", unused_history),
    ]
for name, history in cases:
    case_arguments = list(arguments)
    case_arguments[6] = str(Path(arguments[6]) / name)
    fixture = SourceTest(case_arguments, None, "control_flow")
    shutil.copyfile(
        Path(__file__).with_name(f"transactional-control-{name}-design.py"),
        fixture.sources / "design.py",
    )
    fixture.compile_source("design")
    final = fixture.scratch / "final.ac"
    linked = fixture.link_saved(("design",), "control_flow.design.Top", final)
    executable, _ = fixture.build_system(
        linked if target == "t-gfsim" else final, target
    )
    expected = Counter(
        (2 * cycle + edge, event, values)
        for cycle, events in enumerate(history)
        for edge in (0, 1)
        for event, values in events
    )
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
    print(f"transactional control flow {name} {target}: PASS")
