"""Saved routed Queue controls retain atomic admission and execution."""

from __future__ import annotations

import json
import re
import shutil

import pytest
from test_source_rule_queues import _execute
from test_source_system_execution import FIXTURE, _cli, _compile, _ok

pytestmark = pytest.mark.system


def _assert_unconditional_pop_controls(bodies):
    # Inspect published resource slots, not incidental SSA numbers or op totals.
    # The add Rule has two unconditional pops and no observation suffix.
    found = 0
    for rule in bodies.split('"ac.rule"')[1:]:
        resources = [
            ordinal
            for ordinal, kind in re.findall(
                r'control = (\d+) : i64, declaration = .*?, kind = "(queue_\w+)"',
                rule,
            )
            if kind == "queue_valid"
        ]
        if len(resources) != 2 or "observations = []" not in rule:
            continue
        fire = re.search(r"fire_result = (\d+) : i64", rule)
        yielded = re.search(r'"ac.yield"\(([^)]*)\)', rule)
        assert fire and yielded
        values = [value.strip() for value in yielded[1].split(",")]
        for control in resources:
            raw = values[int(fire[1]) + 1 + 2 * int(control)]
            # Raw valid may be a captured Rule block argument.
            definition = re.search(rf'{re.escape(raw)} = "([^"]+)"', rule)
            assert (
                definition is None or definition[1] != "ac.bits.select"
            ), "unconditional Queue pop must publish raw valid directly"
            assert re.search(
                rf'"ac.bits.compare"\({re.escape(raw)}, [^)]*\).*'
                r'predicate = "case_eq"',
                rule,
            ), "raw four-state control must retain known-one admission"
            found += 1
    assert found == 2, "expected the two unconditional QueueAdd pop controls"


def test_saved_queue_add_controls_refine_without_source(tmp_path):
    source = tmp_path / "source"
    source.mkdir()
    filename = "routed_queue_add.py"
    shutil.copy2(FIXTURE / filename, source)
    unit = tmp_path / "unit"
    _ok(_compile(source, filename, unit))
    direct = tmp_path / "direct.ac"
    anchor = tmp_path / "anchor"
    for target, output in (("final", direct), ("anchor", anchor)):
        _ok(
            _cli(
                "link",
                str(unit),
                "--top",
                "checks.routed_queue_add.QueueAdd",
                "--target",
                target,
                "-o",
                str(output),
            )
        )
    manifest = json.loads((anchor / "anchor.json").read_text())
    bodies = "\n".join(
        (anchor / row["body"]).read_text() for row in manifest["definitions"]
    )
    _assert_unconditional_pop_controls(bodies)

    def anchor_bytes():
        return {path: path.read_bytes() for path in anchor.rglob("*") if path.is_file()}

    original = anchor_bytes()
    shutil.rmtree(source)
    shutil.rmtree(unit)
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    refined = tmp_path / "refined.ac"
    _ok(
        _cli(
            "link",
            "--from-anchor",
            str(anchor),
            "--target",
            "final",
            "-o",
            str(refined),
        )
    )
    # The left operand waits through cycles 1 and 2. The result is available
    # at cycle 4, held until cycle 5, and then consumed exactly once.
    expected = {"queue result": [(5, 42)]}
    for name, final in (("direct", direct), ("refined", refined)):
        root = tmp_path / name
        root.mkdir()
        _execute(final, root, 8, expected)
    assert anchor_bytes() == original
