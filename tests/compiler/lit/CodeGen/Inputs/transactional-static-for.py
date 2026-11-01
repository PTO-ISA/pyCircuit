"""Independent arithmetic and pipeline histories for static loop expansion."""

import json
import shutil
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest, log_history

fixture = SourceTest(sys.argv[1:], None, "static_for_case")
target, case = sys.argv[-2:]
inputs = Path(__file__).parent
if case == "main":
    shutil.copyfile(
        inputs / "transactional-static-for-runtime.py", fixture.sources / "runtime.py"
    )
    fixture.compile_source("runtime", fails=True)
    (fixture.sources / "runtime.py").unlink()
    shutil.copyfile(
        inputs / "transactional-static-for-overlap.py", fixture.sources / "overlap.py"
    )
    fixture.compile_source("overlap")
    fixture.cli(
        "link",
        fixture.scratch / "overlap",
        "--top",
        "static_for_case.overlap.Overlap",
        "-o",
        fixture.scratch / "overlap.ac",
        fails=True,
    )
    (fixture.sources / "overlap.py").unlink()
shutil.copytree(
    inputs / "transactional-static-for", fixture.sources, dirs_exist_ok=True
)
fixture.compile_source("provider")
saved_source = fixture.scratch / "source.ac"
shutil.copyfile(fixture.artifact("provider", "body"), saved_source)
source = saved_source.read_text()
assert "scf.for" in source
# A fresh parser verifies the unexpanded, still-parameterized source artifact.
fixture.run([fixture.opt, saved_source, "-o", fixture.scratch / "source-roundtrip.ac"])

# The consumer sees only the provider's published interface. Parameter values
# specialize its saved loop template later, without importing Python bodies.
(fixture.sources / "provider.py").unlink()
with fixture.interfaces_only("provider"):
    fixture.compile_source("consumer", "provider")

if case == "main" and target == "cpp":
    parameters = fixture.scratch / "parameters.json"
    parameters.write_text(json.dumps([{"name": "COUNT", "value": "2"}]))
    budget = fixture.scratch / "budget.ac"
    arguments = [
        "link",
        fixture.scratch / "provider",
        "--top",
        "static_for_case.provider.Kernel",
        "--target",
        "linked",
        "--parameters",
        parameters,
        "-o",
        budget,
    ]
    fixture.cli(*arguments)
    original = budget.read_bytes()
    # Expansion reserves common work before cloning. An excessive bound must
    # fail without replacing the last successfully published artifact.
    parameters.write_text(json.dumps([{"name": "COUNT", "value": "10000000"}]))
    fixture.cli(*arguments, "--replace", fails=True)
    assert budget.read_bytes() == original

    repeated = [fixture.scratch / f"deterministic-{index}.ac" for index in (1, 2)]
    for output in repeated:
        fixture.cli(
            "link",
            fixture.scratch / "provider",
            fixture.scratch / "consumer",
            "--top",
            "static_for_case.consumer.Top",
            "--target",
            "linked",
            "-o",
            output,
        )
    assert repeated[0].read_bytes() == repeated[1].read_bytes()

final = fixture.scratch / "final.ac"
top = {"main": "Top", "reset": "ResetTop", "failure": "FailureTop"}[case]
linked = fixture.link_saved(
    ("provider", "consumer"), f"static_for_case.consumer.{top}", final
)
normalized = linked.read_text()
assert "scf.for" not in normalized
assert "ac.source.static_index" not in normalized
assert "ac.source.unbound" not in normalized
first, second = fixture.scratch / "expanded.ac", fixture.scratch / "expanded-again.ac"
fixture.run([fixture.opt, linked, "--ac-expand-static-for", "-o", first])
fixture.run([fixture.opt, first, "--ac-expand-static-for", "-o", second])
assert first.read_bytes() == second.read_bytes()

# Resume the Routed boundary after the original final and portable Linked
# inputs are gone. Python sources and published source units are already gone.
anchor = fixture.scratch / "anchor"
fixture.cli("link", "--from-linked", linked, "--target", "anchor", "-o", anchor)
final.unlink()
linked.unlink()
fixture.cli("link", "--from-anchor", anchor, "--target", "final", "-o", final)


def kernel_event(count, phase):
    triangle = count * (count - 1) // 2
    weighted = count * (count - 1) * (count + 1) // 6
    if phase < 2:
        total = triangle + count * phase
        running = 3 + weighted + phase * count * (count + 1) // 2
    else:
        total = 2 * triangle
        running = 3 + 2 * weighted
    # Each iteration reads old Q into a local; the final State proposal
    # increases saved once. Distinct bit writes compose the low COUNT bits.
    saved = 5 + phase if count else 5
    mask = (1 << count) - 1 if phase else 0
    accumulated = count * (saved - 1) + triangle if phase else 0
    # "10xz01zx" starts with independent value/known/Z planes. Partial
    # writes affect only selected low bits; the remaining X/Z survive.
    lanes = (0x84 | mask, 0xCC | mask, 0x12 & ~mask)
    # Helper offsets: nested sums 1+3+6, stride 1+3+5, three nines,
    # retained/final targets 7+9, and widened/escaped locals 1+7.
    # The narrower addition wraps 14 -> 15 -> 0 -> 1 before widening.
    # A selected static branch defines 6; u8(255)+1 wraps to zero and
    # selects the hardware branch adding 2. The complete offset is 78.
    return "kernel", (
        count,
        phase,
        total,
        running,
        total + 78,
        saved,
        mask,
        accumulated,
        lanes,
    )


if case == "reset":
    # Reset Work sees phase 2 and the effects of the first two successful
    # edges. The reset rising edge restores every initializer for the last pair.
    history = [[kernel_event(2, phase)] for phase in (0, 1, 2, 0)]
elif case == "failure":
    # The phase-2 low Work fails after tentatively logging loop effects.
    # None of that epoch's logs may publish. This does not observe failed State.
    history = [
        [("attempt", (phase, bit, 0 if phase == 0 else 3)) for bit in (0, 1)]
        for phase in (0, 1)
    ]
else:
    history = []
    for cycle in range(8):
        events = [("template", (min(cycle, 4) + 3,))]
        events.append(("loop-state", (0 if cycle == 0 else 5,)))
        events.extend(kernel_event(count, cycle) for count in (0, 2, 4))
        if cycle < 4:
            events.append(("send", (10 + cycle,)))
        # Each connection is an old-occupancy Queue. Stage j receives the first
        # item at cycle j+1 and adds j+1; all four input items remain in order.
        for stage in range(3):
            item = cycle - stage - 1
            if 0 <= item < 4:
                incoming = 10 + item + stage * (stage + 1) // 2
                events.append(("stage", (stage, item, incoming)))
        if 4 <= cycle < 8:
            events.append(("receive", (16 + cycle - 4,)))
        # Another specialization has one distinct Stage, with its own accepted
        # count and queues. Its payloads reach the sink after two queue edges.
        if 1 <= cycle < 3:
            events.append(("stage", (0, cycle - 1, 30 + cycle - 1)))
        if 2 <= cycle < 4:
            events.append(("receive_short", (31 + cycle - 2,)))
        history.append(events)

expected = Counter(
    (2 * cycle + edge, event, values)
    for cycle, events in enumerate(history)
    for edge in (0, 1)
    for event, values in events
)

executable, _ = fixture.build_system(final, target)
for workers in (1, 2) if target == "cpp" else (1,):
    options = ["--workers", workers] if target == "cpp" else []
    if case != "reset":
        cycles = 3 if case == "failure" else 8
        options += ["--cycles", cycles] if target == "cpp" else [f"+cycles={cycles}"]
    actual = log_history(
        fixture.run([executable, *options], fails=case == "failure"),
        successful=case != "failure",
    )
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
print(f"transactional static for {case} {target}: PASS")
