"""Independent return-path, transaction and payload histories."""

import json
import sys
from pathlib import Path

from transactional_test_support import SourceTest

arguments = sys.argv[1:]
target = arguments[-1]
sys.path.insert(0, str(Path(arguments[0]) / "cmake"))
from verify_example import verify_expected_observations  # noqa: E402

scalar_source = """import pycircuit as ac

@ac.system
def Top():
    phase: ac.bits[8] = 0
    calls: ac.bits[8] = 0
    committed: ac.bits[8] = 0
    stored: ac.bits[8] = 0

    @ac.method
    def constant_value() -> ac.bits[8]:
        disabled = False
        if disabled:
            return ac.bits[8](99)
        else:
            if True:
                return ac.bits[8](7)

    @ac.method
    def choose(value: ac.bits[8]) -> ac.bits[8]:
        calls = calls + 1
        if phase < 2:
            if phase == 0:
                return value + 1
            partial = value + 2
        else:
            if phase == 2:
                return value + 3
            partial = value + 4
        ac.log("info", "method_tail", partial)
        return partial + 10

    @ac.method(guard=lambda: phase == 99)
    def blocked() -> None:
        ac.log("info", "blocked")

    @ac.method
    def record(value: ac.bits[8]) -> None:
        ac.log("info", "record", value)
        if value < 20:
            return
            assert value >= 20
            ac.log("info", "unreachable_record", value)
        ac.log("info", "record_tail", value)

    @ac.rule
    def produce() -> ac.bits[8]:
        selected = choose(ac.bits[8](10))
        record(selected)
        committed = committed + 1
        if selected < 20:
            return selected
        assert phase != 2
        if (phase == 0) | (phase == 4):
            blocked()
        ac.log("info", "rule_tail", selected)
        return selected + 1

    stored = produce()

    @ac.autorule
    def tick() -> None:
        known = constant_value()
        ac.log("info", "constant_return", known)
        ac.log("info", "state", phase, calls, committed, stored)
        phase = phase + 1
"""

queue_source = """import pycircuit as ac

@ac.system
def Top():
    phase: ac.bits[8] = 0
    popped: ac.bits[8] = 0
    committed: ac.bits[8] = 0
    stored: ac.bits[8] = 0
    queue = ac.queue[ac.bits[8]](depth=2)

    @ac.rule(guard=lambda: (phase == 0) | (phase == 3))
    def produce() -> ac.bits[8]:
        return ac.bits[8](8) if phase == 0 else ac.bits[8](12)

    queue = produce()

    @ac.method(guard=lambda: phase == 99)
    def blocked() -> None:
        ac.log("info", "blocked")

    @ac.method
    def take() -> ac.bits[8]:
        value = queue.pop()
        popped = popped + 1
        ac.log("info", "taken", value)
        if (phase == 1) | (phase == 5):
            return value + 1
        blocked()
        return value + 2

    @ac.rule
    def consume() -> ac.bits[8]:
        committed = committed + 1
        if phase == 0:
            return ac.bits[8](5)
        value = take()
        return value

    stored = consume()

    @ac.autorule
    def tick() -> None:
        ac.log("info", "state", phase, popped, committed, stored)
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
    stored: Packet = Packet(ac.bits[8](0), Mode.RIGHT)
    tag: ac.bits[8] = 0

    @ac.method
    def choose(value: ac.bits[8]) -> tuple[Packet, Mode]:
        if phase == 0:
            return Packet(value + 1, Mode.LEFT), Mode.LEFT
        if phase == 1:
            return Packet(value + 2, Mode.RIGHT), Mode.RIGHT
        return Packet(value + 3, Mode.LEFT), Mode.RIGHT

    @ac.rule
    def produce() -> tuple[Packet, ac.bits[8]]:
        packet, mode = choose(ac.bits[8](10))
        if mode == Mode.LEFT:
            return packet, ac.bits[8](1)
        if phase == 1:
            return Packet(packet.data + 10, Mode.LEFT), ac.bits[8](2)
        return packet, ac.bits[8](3)

    stored, tag = produce()

    @ac.autorule
    def tick() -> None:
        ac.log("info", "state", phase, stored.data, stored.mode == Mode.RIGHT, tag)
        phase = phase + 1
"""

# choose(10) returns 11/22/13/24/24/24. Returns at phases 0 and 2
# bypass the shared suffix; phase 4 reaches a false guard and commits nothing,
# including choose's earlier State update and record's earlier observations.
scalar_history = {
    "constant_return": [[[7]] for _ in range(6)],
    "state": [
        [[0, 0, 0, 0]],
        [[1, 1, 1, 11]],
        [[2, 2, 2, 23]],
        [[3, 3, 3, 13]],
        [[4, 4, 4, 25]],
        [[5, 4, 4, 25]],
    ],
    "method_tail": [[], [[12]], [], [[14]], [], [[14]]],
    "record": [[[11]], [[22]], [[13]], [[24]], [], [[24]]],
    "record_tail": [[], [[22]], [], [[24]], [], [[24]]],
    "rule_tail": [[], [[22]], [], [[24]], [], [[24]]],
}
# The early Rule return at phase 0 bypasses an empty Queue. Method returns
# preserve the successful pops at phases 1/5; the false guard at phase 4
# discards its tentative pop so that phase 5 still receives the same 12.
queue_history = {
    "state": [
        [[0, 0, 0, 0]],
        [[1, 0, 1, 5]],
        [[2, 1, 2, 9]],
        [[3, 1, 2, 9]],
        [[4, 1, 2, 9]],
        [[5, 1, 2, 9]],
        [[6, 2, 3, 13]],
    ],
    "taken": [[], [[8]], [], [], [], [[12]], []],
}
# Method tuple results select the Rule's return path. Independent output State
# bindings receive the returned record and scalar together on the rising edge.
nominal_history = {
    "state": [
        [[0, 0, 1, 0]],
        [[1, 11, 0, 1]],
        [[2, 22, 0, 2]],
        [[3, 13, 0, 3]],
    ]
}

cases = [("scalar", scalar_source, scalar_history)]
if target != "t-gfsim":
    cases += [
        ("queue", queue_source, queue_history),
        ("nominal", nominal_source, nominal_history),
    ]
for name, source, history in cases:
    case_arguments = list(arguments)
    case_arguments[6] = str(Path(arguments[6]) / name)
    fixture = SourceTest(case_arguments, None, "early_returns")
    (fixture.sources / "design.py").write_text(source)
    fixture.compile_source("design")
    final = fixture.scratch / "final.ac"
    linked = fixture.link_saved(("design",), "early_returns.design.Top", final)
    fixture.run([fixture.opt, final, "--ac-verify-hardware"])
    executable, _ = fixture.build_system(
        linked if target == "t-gfsim" else final, target
    )
    cycles = len(history["state"])
    for workers in (1, 2) if target == "cpp" else (1,):
        options = (
            [f"+cycles={cycles}"]
            if target == "verilog"
            else ["--cycles", cycles, "--workers", workers]
        )
        result = fixture.run([executable, *options])
        rows = [
            json.loads(line)
            for line in result.stdout.splitlines()
            if line.startswith("{")
        ]
        assert any(row.get("kind") == "result" for row in rows), rows
        verify_expected_observations(
            history, [row for row in rows if row.get("kind") == "log"], cycles * 2
        )
    print(f"transactional early returns {name} {target}: PASS")

if target == "cpp":
    fixture.sources.mkdir()
    (fixture.sources / "incomplete.py").write_text(
        """import pycircuit as ac
@ac.module
def Incomplete():
    phase: ac.bits[8] = 0
    @ac.method
    def choose() -> ac.bits[8]:
        if phase == 0:
            return ac.bits[8](7)
    @ac.autorule
    def observe() -> None:
        value = choose()
        ac.log("info", "value", value)
"""
    )
    fixture.compile_source("incomplete", fails=True)
    assert not (fixture.scratch / "incomplete" / "unit.json").exists()
    print("incomplete valued return path: rejected")
    (fixture.sources / "arity.py").write_text(
        """import pycircuit as ac
@ac.module
def Arity():
    phase: ac.bits[8] = 0
    @ac.method
    def choose() -> tuple[ac.bits[8], ac.bits[8]]:
        if phase == 0:
            return ac.bits[8](7)
        return ac.bits[8](8), ac.bits[8](9)
    @ac.autorule
    def observe() -> None:
        first, second = choose()
        ac.log("info", "pair", first, second)
"""
    )
    fixture.compile_source("arity", fails=True)
    assert not (fixture.scratch / "arity" / "unit.json").exists()
    print("inconsistent valued return arity: rejected")
