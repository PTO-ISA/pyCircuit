"""Independent histories for payload joins and selected-path admission."""

import json
import re
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest

scalar_source = """import pycircuit as ac

@ac.system
def Top():
    phase: ac.bits[8] = 0
    committed: ac.bits[8] = 0
    total: ac.bits[16] = 0

    base = phase + ac.bits[8](10)
    snapshot = base
    if phase < 2:
        base = base + 1
        if phase == 0:
            extra = ac.bits[8](3)
        else:
            extra = ac.bits[8](5)
    else:
        extra = ac.bits[8](7)

    @ac.method
    def adjust(value: ac.bits[8]) -> tuple[ac.bits[8], ac.bits[8]]:
        adjusted = value + 2
        if phase == 0:
            adjusted = adjusted + 1
            tag = ac.bits[8](20)
        elif phase == 1:
            adjusted = adjusted + 3
            tag = ac.bits[8](30)
        else:
            tag = ac.bits[8](40)
        return adjusted, tag

    @ac.method(guard=lambda: phase == 99)
    def blocked(value: ac.bits[8]) -> ac.bits[8]:
        return value + 100

    @ac.rule
    def parameter(value: ac.bits[8]) -> None:
        if phase == 0:
            value = value + 1
        ac.log("info", "parameter", value)

    parameter(phase)

    @ac.autorule
    def calculate() -> None:
        value = base + extra
        fallback = value
        if phase == 0:
            selected = value + 1
            fallback = value + 2
        elif phase == 3:
            selected = value + 3
        else:
            selected = value + 5
        result, tag = adjust(selected)
        committed = committed + 1
        if phase == 2:
            result = blocked(result)
        total = total + result
        ac.log("info", "branch", snapshot, base, extra, fallback, result, tag)

    @ac.autorule
    def tick() -> None:
        ac.log("info", "state", phase, committed, total)
        phase = phase + 1
"""

nominal_source = """from enum import Enum
import pycircuit as ac

@ac.encoding(width=2)
class Mode(Enum):
    LEFT = 1
    RIGHT = 2

@ac.struct
class Packet:
    data: ac.bits[8]
    mode: Mode

@ac.system
def Top():
    phase: ac.bits[8] = 0
    seed = Packet(ac.bits[8](10), Mode.LEFT)
    if phase == 2:
        seed = Packet(ac.bits[8](20), Mode.RIGHT)
    if phase == 0:
        unknown = ac.four_state("10xz")
        probe = Packet(ac.four_state("10xz0101"), Mode.RIGHT)
    else:
        unknown = ac.four_state("z01x")
        probe = Packet(ac.four_state("z01x1001"), Mode.LEFT)

    @ac.method
    def transform(value: Packet) -> tuple[Packet, Mode]:
        mode = Mode.LEFT
        if phase < 2:
            if phase == 0:
                packet = Packet(value.data + 1, Mode.RIGHT)
                mode = Mode.RIGHT
            else:
                packet = Packet(value.data + 2, Mode.LEFT)
        else:
            packet = Packet(value.data + 3, Mode.RIGHT)
            mode = Mode.RIGHT
        return packet, mode

    @ac.autorule
    def calculate() -> None:
        packet, mode = transform(seed)
        saved = packet
        if phase == 1:
            packet = Packet(packet.data + 10, Mode.RIGHT)
            mode = Mode.RIGHT
        ac.log("info", "packet", seed.data, seed.mode == Mode.RIGHT,
               saved.data, saved.mode == Mode.RIGHT,
               packet.data, packet.mode == Mode.RIGHT, mode == Mode.RIGHT)

    @ac.autorule
    def observe_unknown() -> None:
        if probe.mode == Mode.RIGHT:
            selected = ac.bits[8](1)
        else:
            selected = ac.bits[8](2)
        ac.log("info", "four_state", unknown, probe.data, selected)

    @ac.autorule
    def tick() -> None:
        phase = phase + 1
"""

queue_source = """import pycircuit as ac

@ac.system
def Top():
    phase: ac.bits[8] = 0
    committed: ac.bits[8] = 0
    total: ac.bits[16] = 0
    queue = ac.queue[ac.bits[8]](depth=2)
    parameter_queue = ac.queue[ac.bits[8]](depth=2)
    empty = ac.queue[ac.bits[8]](depth=2)

    @ac.rule(guard=lambda: phase == 1)
    def produce() -> tuple[ac.bits[8], ac.bits[8]]:
        return ac.bits[8](9), ac.bits[8](20)

    queue, parameter_queue = produce()

    @ac.rule
    def parameter(value: ac.bits[8]) -> None:
        if phase == 2:
            ac.log("info", "parameter_branch", value)
        adjusted = value + 1
        ac.log("info", "parameter", adjusted)

    parameter(parameter_queue)

    @ac.rule
    def unused(value: ac.bits[8]) -> None:
        if phase == 1:
            value = value + 1
        ac.log("info", "unused", phase)

    unused(empty)

    @ac.autorule
    def consume() -> None:
        committed = committed + 1
        if (phase == 0) | (phase == 2) | (phase == 4):
            value = queue.pop()
        else:
            value = ac.bits[8](5)
        adjusted = value + 1
        total = total + adjusted
        ac.log("info", "consume", adjusted)

    @ac.autorule
    def tick() -> None:
        ac.log("info", "state", phase, committed, total)
        phase = phase + 1
"""


def observations(result):
    rows = [
        json.loads(line) for line in result.stdout.splitlines() if line.startswith("{")
    ]
    assert any(row.get("kind") == "result" for row in rows), rows
    actual = Counter()
    for row in rows:
        if row.get("kind") != "log":
            continue
        values = []
        for value in row["values"]:
            if value["kind"] == "bits4":
                planes = tuple(int(value[key], 16) for key in ("value", "known", "z"))
                values.append(
                    planes[0]
                    if planes[1:] == ((1 << value["width"]) - 1, 0)
                    else planes
                )
            else:
                values.append(int(value["value"]))
        actual[(int(row["evaluation_epoch"]), row["spec"]["event"], tuple(values))] += 1
    return actual


# These tables follow the source arithmetic and committed-state semantics.
# Both Work samples see the same old state. The selected false Method guard at
# phase 2 suppresses the scalar result and every proposed update in calculate.
scalar_states = ((0, 0), (1, 18), (2, 45), (2, 45), (3, 70), (4, 98))
scalar_results = (
    (10, 11, 3, 16, 18, 20),
    (11, 12, 5, 17, 27, 30),
    None,
    (13, 13, 7, 20, 25, 40),
    (14, 14, 7, 21, 28, 40),
    (15, 15, 7, 22, 29, 40),
)
scalar_history = [
    [("state", (cycle, *state)), ("parameter", (1 if cycle == 0 else cycle,))]
    + ([] if result is None else [("branch", result)])
    for cycle, (state, result) in enumerate(
        zip(scalar_states, scalar_results, strict=False)
    )
]
# Struct snapshots keep their original fields after rebinding the joined local.
nominal_history = [
    [
        ("packet", values),
        (
            "four_state",
            (
                ((8, 12, 1), (133, 207, 16), 1)
                if cycle == 0
                else ((2, 6, 8), (41, 111, 128), 2)
            ),
        ),
    ]
    for cycle, values in enumerate(
        (
            (10, 0, 11, 1, 11, 1, 1),
            (10, 0, 12, 0, 22, 1, 1),
            (20, 1, 23, 1, 23, 1, 1),
            (10, 0, 13, 1, 13, 1, 1),
        )
    )
]
# The Queue starts empty; cycle 1 enqueues 9 while consuming the non-Queue arm.
# Only cycle 2 pops it. Selected empty reads at cycles 0 and 4 hold both owners.
queue_history = [
    [("state", (cycle, *state))]
    + ([] if result is None else [("consume", (result,))])
    + ([("parameter_branch", (20,)), ("parameter", (21,))] if cycle == 2 else [])
    + ([] if cycle == 1 else [("unused", (cycle,))])
    for cycle, (state, result) in enumerate(
        zip(
            ((0, 0), (0, 0), (1, 6), (2, 16), (3, 22), (3, 22)),
            (None, 6, 10, 6, None, 6),
            strict=False,
        )
    )
]

arguments = sys.argv[1:]
target = arguments[-1]
cases = [("scalar", scalar_source, scalar_history)]
if target != "t-gfsim":
    cases += [
        ("nominal", nominal_source, nominal_history),
        ("queue", queue_source, queue_history),
    ]
for name, source, history in cases:
    case_arguments = list(arguments)
    case_arguments[6] = str(Path(arguments[6]) / name)
    fixture = SourceTest(case_arguments, None, "branch_results")
    (fixture.sources / "design.py").write_text(source)
    fixture.compile_source("design")
    final = fixture.scratch / "final.ac"
    linked = fixture.link_saved(("design",), "branch_results.design.Top", final)
    fixture.run([fixture.opt, final, "--ac-verify-hardware"])
    if name == "scalar" and target == "cpp":
        fixture.env["PYCIRCUIT_PRINTER"] = str(
            Path(fixture.opt).with_name("pycircuit-print")
        )
        readable = fixture.cli("print", linked).stdout
        assert re.search(r"%\w+.* = scf\.if ", readable)
        assert re.search(r"scf\.yield\(\s*%", readable)
        # A locator cannot authenticate swapped executable arm values. Both
        # payload operands retain their type and all provenance stays intact.
        lines = final.read_text().splitlines(keepends=True)
        index = next(
            i
            for i, line in enumerate(lines)
            if '"ac.bits.select"(' in line and "ac.transactional_step" in line
        )
        original = lines[index]
        lines[index] = re.sub(
            r'("ac.bits.select"\()([^,]+), ([^,]+), ([^)]+)(\))',
            r"\1\2, \4, \3\5",
            original,
            count=1,
        )
        assert lines[index] != original
        changed = fixture.scratch / "swapped-branch.ac"
        changed.write_text("".join(lines))
        fixture.run([fixture.opt, changed, "--ac-verify-hardware"], fails=True)
    executable, _ = fixture.build_system(
        linked if target == "t-gfsim" else final, target
    )
    expected = Counter(
        (2 * cycle + half, event, values)
        for cycle, events in enumerate(history)
        for half in (0, 1)
        for event, values in events
    )
    for workers in (1, 2) if target == "cpp" else (1,):
        options = (
            [f"+cycles={len(history)}"]
            if target == "verilog"
            else ["--cycles", len(history), "--workers", workers]
        )
        actual = observations(fixture.run([executable, *options]))
        assert actual == expected, {
            "unexpected": actual - expected,
            "missing": expected - actual,
        }
    print(f"transactional branch results {name} {target}: PASS")

if target == "cpp":
    # A local defined only on one continuing path has no value after the join.
    fixture.sources.mkdir()
    (fixture.sources / "undefined.py").write_text(
        """import pycircuit as ac
@ac.module
def Undefined():
    phase: ac.bits[8] = 0
    @ac.autorule
    def observe() -> None:
        if phase == 0:
            value = ac.bits[8](7)
        ac.log("info", "undefined", value)
"""
    )
    fixture.compile_source("undefined", fails=True)
    assert not (fixture.scratch / "undefined" / "unit.json").exists()
    print("possibly undefined branch local: rejected")
