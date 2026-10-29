"""Nested nominal packet transport, atomic state updates and reset oracle."""

import shutil
import sys
from collections import Counter, deque
from pathlib import Path

from transactional_test_support import SourceTest, log_history

fixture = SourceTest(sys.argv[1:], None, "packet_case")
target = sys.argv[8]
reset = sys.argv[9:] == ["reset"]
entry = "Reset" if reset else "Top"
shutil.copytree(
    Path(__file__).with_name("structured_packet"), fixture.sources, dirs_exist_ok=True
)
fixture.compile_source("types")
with fixture.interfaces_only("types"):
    fixture.compile_source("processor", "types")
with fixture.interfaces_only("types", "processor"):
    fixture.compile_source("design", "types", "processor")
with fixture.interfaces_only("types", "processor", "design"):
    fixture.compile_source(entry, "types", "processor", "design")
final = fixture.scratch / "packet.ac"
fixture.link_saved(
    ("types", "processor", "design", entry), f"packet_case.{entry}.Top", final
)
executable, _ = fixture.build_system(final, target)

# Packet payloads are test-owned data, independent of the compiled design.
pattern = "1010" + "xz01" * 19
unknown = tuple(
    int("".join("1" if bit in chars else "0" for bit in pattern), 2)
    for chars in ("1", "01", "z")
)
payloads = (0x123456789ABCDEF01234, unknown, 0x123456789ABCDEF01234, unknown)
incoming, outgoing, receipts = deque(), deque(), deque()
sent = received = accepted = cycle = retained = saved = 0
expected = Counter()
consumed = []
steps = 18 if reset else 13
for step in range(steps):
    send = sent < 4 and len(incoming) < 2
    process = bool(incoming) and len(outgoing) < 2 and len(receipts) < 2
    consume = cycle >= 7 and bool(outgoing)
    acknowledge = bool(receipts)
    events = [
        ("processor", (accepted, retained, int(accepted > 0), saved)),
        ("progress", (sent, received)),
    ]
    if incoming:
        events.append(("head", (incoming[0], payloads[incoming[0]])))
    if process:
        packet = incoming[0]
        events.append(("process", (packet, retained)))
    if consume:
        events.append(("receive", (outgoing[0] + 1, payloads[outgoing[0]])))
    if acknowledge:
        events.append(("receipt", (receipts[0],)))
    # Work logs observe old state on both reset samples; reset clears state and
    # all occupied Queues at the rising commit. The second stream starts fresh.
    for epoch in (2 * step, 2 * step + 1):
        for event, values in events:
            expected[(epoch, event, values)] += 1
    if reset and step == 4:
        incoming.clear()
        outgoing.clear()
        receipts.clear()
        sent = received = accepted = cycle = retained = saved = 0
        consumed.clear()
        continue
    if process:
        retained = incoming.popleft() + 1
        saved = payloads[packet]
        accepted += 1
    if consume:
        consumed.append(outgoing.popleft() + 1)
        received += 1
    if acknowledge:
        receipts.popleft()
    if send:
        incoming.append(sent)
        sent += 1
    if process:
        outgoing.append(packet)
        receipts.append(packet)
    cycle += 1
assert consumed == [1, 2, 3, 4]
for workers in (1, 2) if target == "cpp" else (1,):
    arguments = ["--workers", workers] if target == "cpp" else []
    if not reset:
        arguments += ["--cycles", steps] if target == "cpp" else [f"+cycles={steps}"]
    actual = log_history(fixture.run([executable, *arguments]))
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
print(f"transactional structured packet {target}: PASS")
print("t-gfsim: NOT RUN (Queue and nominal payload support remain implementation gaps)")
