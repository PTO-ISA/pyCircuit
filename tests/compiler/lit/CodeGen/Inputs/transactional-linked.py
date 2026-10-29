"""Saved Linked counter execution, anchor continuation."""

import sys
from collections import Counter

from transactional_test_support import SourceTest, log_history

fixture = SourceTest(sys.argv[1:], "hello_counter", "linked_counter")
target = sys.argv[-1]
scratch = fixture.scratch
fixture.compile_source("hello_counter")
resumed = scratch / "resumed.ac"
linked = fixture.link_saved(
    ("hello_counter",), "linked_counter.hello_counter.HelloCounter", resumed
)

# Continue through the existing saved Routed anchor boundary as well.
anchor = scratch / "anchor"
fixture.cli("link", "--from-linked", linked, "--target", "anchor", "-o", anchor)
anchored = scratch / "anchored.ac"
fixture.cli("link", "--from-anchor", anchor, "--target", "final", "-o", anchored)

executable, _ = fixture.build_system(anchored, target)
expected = Counter((epoch, "count", (epoch // 2,)) for epoch in range(10))
for workers in (1, 2) if target == "cpp" else (1,):
    arguments = (
        ["--cycles", "5", "--workers", workers] if target == "cpp" else ["+cycles=5"]
    )
    actual = log_history(fixture.run([executable, *arguments]))
    assert actual == expected, (actual, expected)
print(f"saved Linked counter {target}: PASS")
