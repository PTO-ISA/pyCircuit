"""Selected Method guards gate the complete outer transaction."""

import json
import sys
from collections import Counter

from transactional_test_support import SourceTest

fixture = SourceTest(sys.argv[1:], None, "conditional")
target = sys.argv[-1]
(fixture.sources / "design.py").write_text(
    """import pycircuit as ac

@ac.module
def Receiver():
    total: ac.u16 = 0

    @ac.method
    def accept(x: ac.u8) -> None:
        if x == 99:
            assert x == 0
            ac.log("info", "unreachable", x)
        elif x == 3:
            contribution = x
            total = total + contribution
        else:
            total = total + x

    @ac.method(guard=lambda: total == 99)
    def blocked(x: ac.u8) -> None:
        total = total + x

    @ac.autorule
    def observe() -> None:
        ac.log("info", "total", total)

@ac.system
def Top():
    receiver: Receiver
    phase: ac.u8 = 0
    sent: ac.u8 = 0
    accepted: ac.u8 = 0

    @ac.rule(guard=lambda: sent < 2)
    def produce() -> ac.u8:
        sent = sent + 1
        return 3 if sent == 0 else 5

    incoming = produce()

    @ac.rule
    def consume(src: ac.u8) -> None:
        value = src
        accepted = accepted + 1
        if phase == 2:
            receiver.blocked(value)
        else:
            receiver.accept(value)
        ac.log("info", "consume", value)

    consume(incoming)

    @ac.autorule
    def tick() -> None:
        phase = phase + 1
        ac.log("info", "progress", sent, accepted)
"""
)
fixture.compile_source("design")
final = fixture.scratch / "conditional.ac"
fixture.link_saved(("design",), "conditional.design.Top", final)
executable, _ = fixture.build_system(final, target)
# Queue begins empty: cycle 1 consumes 3; cycle 2's selected false guard
# preserves the second item and both owners; cycle 3 consumes that same 5.
states = ((0, 0, 0), (0, 1, 0), (3, 2, 1), (3, 2, 1), (8, 2, 2), (8, 2, 2))
expected = Counter()
for cycle, (total, sent, accepted) in enumerate(states):
    events = [("total", (total,)), ("progress", (sent, accepted))]
    if cycle in (1, 3):
        events.append(("consume", (3 if cycle == 1 else 5,)))
    for epoch in (cycle * 2, cycle * 2 + 1):
        for event, values in events:
            expected[(epoch, event, values)] += 1
for workers in (1, 2) if target == "cpp" else (1,):
    args = ["--cycles", 6, "--workers", workers] if target == "cpp" else ["+cycles=6"]
    result = fixture.run([executable, *args])
    actual = Counter()
    for line in result.stdout.splitlines():
        if not line.startswith("{"):
            continue
        row = json.loads(line)
        if row.get("kind") != "log":
            continue
        values = []
        for value in row["values"]:
            if value["kind"] == "bits4":
                assert int(value["known"], 16) == (1 << value["width"]) - 1
                assert int(value["z"], 16) == 0
                values.append(int(value["value"], 16))
            else:
                values.append(int(value["value"]))
        actual[(int(row["evaluation_epoch"]), row["spec"]["event"], tuple(values))] += 1
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
print(f"conditional Method admission {target}: PASS")

# Two representative boundaries: runtime paths cannot select resource identity,
# and ordinary registrations do not silently arbitrate overlapping writers.
if target == "cpp":
    fixture.sources.mkdir()
    (fixture.sources / "ambiguous.py").write_text(
        """import pycircuit as ac
@ac.module
def Ambiguous() -> ac.u8:
    select: ac.u1 = 0
    left: ac.u8 = 1
    right: ac.u8 = 2
    if select:
        out = left
    else:
        out = right
    return out
"""
    )
    fixture.compile_source("ambiguous", fails=True)
    (fixture.sources / "conflict.py").write_text(
        """import pycircuit as ac
@ac.module
def Conflict() -> ac.u8:
    total: ac.u8 = 0
    @ac.rule
    def first() -> None:
        total = 1
    @ac.rule
    def second() -> None:
        total = 2
    first()
    second()
    return total
"""
    )
    fixture.compile_source("conflict")
    rejected = fixture.scratch / "conflict.ac"
    fixture.cli(
        "link",
        fixture.scratch / "conflict",
        "--top",
        "conditional.conflict.Conflict",
        "-o",
        rejected,
        fails=True,
    )
    assert not rejected.exists()
