"""Independent nested packet, old-Q and atomic backpressure oracle."""

import json
import shutil
import sys
from collections import Counter, deque

from transactional_queue_replay import verify_stages
from transactional_test_support import SourceTest

saved_linked = sys.argv[-1] == "linked"
if saved_linked:
    sys.argv.pop()

fixture = SourceTest(sys.argv[1:], "structured_packet", "example_structured_packet")
repo, scratch, sources = fixture.repo, fixture.scratch, fixture.sources
opt = fixture.opt
target, mode = sys.argv[-2:]


if mode == "four-state":
    top = sources / "Top.py"
    top.write_text(
        top.read_text().replace(
            "ac.bits[80](0x123456789ABCDEF01234)",
            'ac.four_state("1010' + "xz01" * 19 + '")',
        )
    )
else:
    assert mode in ("known", "reset"), mode


fixture.compile_source("types")
with fixture.interfaces_only("types"):
    fixture.compile_source("processor", "types")
with fixture.interfaces_only("types", "processor"):
    fixture.compile_source("Top", "types", "processor")

# A different nominal Packet with the same layout must not bind this Module.
if mode == "known" and target == "cpp":
    top = sources / "Top.py"
    original = top.read_text()
    top.write_text(
        original.replace(
            "@ac.system",
            "@ac.struct\nclass ForeignPacket:\n    header: Header\n    data: ac.bits[80]\n\n\n@ac.system",
        )
        .replace("def produce() -> Packet", "def produce() -> ForeignPacket")
        .replace("return Packet(", "return ForeignPacket(")
    )
    fixture.compile_source(
        "Top", "types", "processor", output="wrong-nominal", fails=True
    )
    top.write_text(original)

shutil.rmtree(sources)
final = scratch / "pipeline.ac"
if saved_linked:
    linked = fixture.link_saved(
        ("types", "processor", "Top"),
        "example_structured_packet.Top.Top",
        final,
        retained=("ac.method @", "ac.method.call"),
    )
    fixture.run([opt, final, "--ac-verify-hardware"])
    anchor = scratch / "anchor"
    fixture.cli("link", "--from-linked", linked, "--target", "anchor", "-o", anchor)
    final = scratch / "anchored.ac"
    fixture.cli("link", "--from-anchor", anchor, "--target", "final", "-o", final)
else:
    fixture.cli(
        "link",
        *(scratch / unit for unit in ("types", "processor", "Top")),
        "--top",
        "example_structured_packet.Top.Top",
        "-o",
        final,
    )
fixture.run([opt, final, "--ac-verify-hardware"])
verify_stages(final, opt, fixture.run, mutations=False)
if mode == "known" and target == "cpp":
    routed = scratch / "routed.ac"
    lines = routed.read_text().splitlines(keepends=True)
    # Rename the executable field consistently, preserving a well-typed graph
    # but not the authenticated normalized-source snapshot.
    changed = [
        (
            line
            if "ac.transactional_source =" in line
            else line.replace('"opcode"', '"renamed"')
        )
        for line in lines
    ]
    assert changed != lines
    mutated = scratch / "wrong-nominal-routed.ac"
    mutated.write_text("".join(changed))
    fixture.run([opt, mutated, "--ac-lower-transactions"], fails=True)
executable, generated = fixture.build_system(
    final, target, reset_cycle=5 if mode == "reset" else None
)


def observations(result):
    rows = [
        json.loads(line) for line in result.stdout.splitlines() if line.startswith("{")
    ]
    assert any(row.get("kind") == "result" for row in rows), rows
    logs = Counter()
    for row in rows:
        if row.get("kind") != "log":
            continue
        values = []
        for value in row["values"]:
            if value["kind"] == "bits4":
                planes = tuple(int(value[key], 16) for key in ("value", "known", "z"))
                if value["width"] == 80:
                    values.append(planes)
                else:
                    assert planes[1:] == ((1 << value["width"]) - 1, 0)
                    values.append(planes[0])
            else:
                values.append(int(value["value"]))
        logs[(int(row["evaluation_epoch"]), row["spec"]["event"], tuple(values))] += 1
    return logs


def known(value, width):
    return (value, (1 << width) - 1, 0) if width == 80 else value


pattern = "1010" + "xz01" * 19
payload = known(0x123456789ABCDEF01234, 80)
if mode == "four-state":
    payload = tuple(
        int("".join("1" if bit in chars else "0" for bit in pattern), 2)
        for chars in ("1", "01", "z")
    )

# Old occupancy decides every transaction. No pop lends same-edge capacity.
# Logs observe committed state even after the Method's proposed assignment.
incoming, outgoing = deque(), deque()
sent = received = accepted = cycle = retained = 0
saved = known(0, 80)
expected = Counter()
consumed = []
blocked = 0
steps = 24 if mode == "reset" else 18
for step in range(steps):
    reset = mode == "reset" and step == 5
    send = sent < 4 and len(incoming) < 2
    process = bool(incoming) and len(outgoing) < 2
    consume = cycle >= 10 and bool(outgoing)
    events = [
        (
            "processor",
            (
                known(accepted, 8),
                known(retained, 6),
                known(int(accepted > 0), 1),
                saved,
            ),
        ),
        ("progress", (known(sent, 6), known(received, 8))),
    ]
    if incoming:
        events.append(("head", (known(incoming[0], 6), payload)))
    if process:
        packet = incoming[0]
        events.append(("process", (known(packet, 6), known(retained, 6))))
    if consume:
        events.append(("receive", (known(outgoing[0], 6), payload)))
    if incoming and not process:
        blocked += 1
    # docs/architecture/system-execution.md: successful Work publishes old-Q
    # snapshots, including reset samples;
    # synchronous reset takes effect at the subsequent rising commit.
    for epoch in (2 * step, 2 * step + 1):
        for event, values in events:
            expected[(epoch, event, values)] += 1
    if reset:
        incoming.clear()
        outgoing.clear()
        sent = received = accepted = cycle = retained = 0
        saved = known(0, 80)
        consumed.clear()
        continue
    if process:
        retained = incoming.popleft() + 1
        saved = payload
        accepted += 1
    if consume:
        consumed.append(outgoing.popleft())
        received += 1
    if send:
        incoming.append(sent)
        sent += 1
    if process:
        outgoing.append(retained)
    cycle += 1
assert consumed == [1, 2, 3, 4], consumed
assert blocked > 0
for workers in (1, 2) if target == "cpp" else (1,):
    arguments = ["--workers", workers] if target == "cpp" else []
    arguments += ["--cycles", str(steps)] if target == "cpp" else [f"+cycles={steps}"]
    actual = observations(fixture.run([executable, *arguments]))
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
print(f"transactional structured packet {mode} {target}: PASS")
