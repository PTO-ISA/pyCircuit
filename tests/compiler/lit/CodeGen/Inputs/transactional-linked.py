"""Saved Linked counter execution, anchor continuation and failed publication."""

import json
import re
import shutil
import sys

from transactional_test_support import SourceTest

fixture = SourceTest(sys.argv[1:], "hello_counter", "linked_counter")
target = sys.argv[-1]
scratch = fixture.scratch
fixture.compile_source("hello_counter")
shutil.copyfile(
    fixture.repo
    / "examples/module_control_state_machine/module_control_state_machine.py",
    fixture.sources / "control.py",
)
fixture.compile_source("control")
direct = scratch / "direct.ac"
fixture.cli(
    "link",
    scratch / "hello_counter",
    "--top",
    "linked_counter.hello_counter.HelloCounter",
    "-o",
    direct,
)
resumed = scratch / "resumed.ac"
linked = fixture.link_saved(
    ("hello_counter",), "linked_counter.hello_counter.HelloCounter", resumed
)
control = scratch / "control.ac"
fixture.link_saved(
    ("control",),
    "linked_counter.control.ModuleControlStateMachine",
    control,
    retained=("scf.if",),
)
fixture.run([fixture.opt, control, "--ac-verify-hardware"])

# Continue through the existing saved Routed anchor boundary as well.
anchor = scratch / "anchor"
fixture.cli("link", "--from-linked", linked, "--target", "anchor", "-o", anchor)
anchored = scratch / "anchored.ac"
fixture.cli("link", "--from-anchor", anchor, "--target", "final", "-o", anchored)

# Reject one invalid replacement without changing the published artifact.
before = resumed.read_bytes()
invalid = scratch / "invalid-linked.ac"
text = linked.read_text()
invalid.write_text(
    re.sub(r"ac.source_units = \[[^\]]*\]", "ac.source_units = []", text, count=1)
)
assert invalid.read_text() != text
fixture.cli("link", "--from-linked", invalid, "-o", resumed, "--replace", fails=True)
assert resumed.read_bytes() == before

for name, final in (("direct", direct), ("resumed", resumed), ("anchor", anchored)):
    executable, _ = fixture.build_system(final, target, name=f"{name}-{target}")
    for workers in (1, 2) if target == "cpp" else (1,):
        arguments = (
            ["--cycles", "5", "--workers", workers]
            if target == "cpp"
            else ["+cycles=5"]
        )
        result = fixture.run([executable, *arguments])
        rows = [
            json.loads(line)
            for line in result.stdout.splitlines()
            if line.startswith("{")
        ]
        assert any(row.get("kind") == "result" for row in rows), rows
        actual = []
        for row in rows:
            if row.get("kind") != "log":
                continue
            assert row["spec"]["event"] == "count", row
            (value,) = row["values"]
            if value["kind"] == "bits4":
                assert value["width"] == 8, value
                assert int(value["known"], 16) == 255 and int(value["z"], 16) == 0
                count = int(value["value"], 16)
            else:
                count = int(value["value"])
            actual.append((int(row["evaluation_epoch"]), count))
        assert actual == [(epoch, epoch // 2) for epoch in range(10)], actual
print(f"saved Linked counter {target}: PASS")
