"""Closed-loop contracts under fixed geometry and controlled input delays."""

import ast
from pathlib import Path

import pytest

from tools.allgather_control_reference import expectations
from wse_model.fixtures import ffn_example

pytestmark = pytest.mark.unit


def test_control_runs_both_epochs_and_retires_every_member():
    rows, states = expectations()
    assert len(states) == 523 and len(rows) == 523 * 32
    assert [r[0] for r in states if r[1] == 1] == [1, 261]
    for key, total in ((0, 10752), (1, 36864)):
        retired = [r for r in rows if r[1] == key and r[20] == 5]
        assert len(retired) == 32
        assert {r[12] for r in retired} == {7}
        assert {r[15] for r in retired} == {total}
        assert {r[21] for r in retired} == {key + 1}
    assert all(r[1:3] == [6, 1] for r in states[-2:])


def test_one_member_delay_holds_round_and_retries_without_double_counting():
    baseline, normal = expectations()
    rows, states = expectations(delayed=True)
    assert len(states) == len(normal) + 6
    # Other 31 accounts post once; send remains blocked for the delayed member.
    assert [r[1] for r in states[2:6]] == [2] * 4
    assert [r[0] for r in rows if r[2] == 0 and r[12] == 1] == [2, 268]
    assert [r[0] for r in rows if r[2] == 31 and r[12] == 1] == [5, 268]
    # The last multicast remains outstanding; admitted destinations see duplicates.
    assert [(r[1], r[2], r[3]) for r in states[261:265]] == [(3, 0, 255)] * 4
    stalled = [r for r in rows if r[2] == 30 and 261 <= r[0] <= 264]
    assert [r[16] for r in stalled] == [0, 0, 0, 1]
    assert [r[15] for r in stalled] == [10560, 10560, 10560, 10752]
    retried = [r for r in rows if r[2] == 24 and 261 <= r[0] <= 264]
    assert [r[12] for r in retried] == [2, 3, 3, 3]
    assert {r[15] for r in retried} == {10752}
    assert [r[15] for r in rows[-32:]] == [r[15] for r in baseline[-32:]]
    assert [r[0] for r in states if r[1] == 1 and r[2] == 1] == [267]


def test_local_register_metadata_matches_core_for_all_members():
    # The existing route literal test covers the 64 route rows; independently
    # validate the new metadata constants against the complete core entries.
    source = Path(__file__).parents[2] / "models/calendar/allgather_control.py"
    tree = ast.parse(source.read_text())
    helper = next(
        n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == "local_entry"
    )
    high = next(
        n.value for n in helper.body if isinstance(n, ast.AnnAssign) and n.target.id == "high"
    )
    literal = {0: ast.literal_eval(high.orelse), 1: ast.literal_eval(high.body)}
    example = ffn_example()
    for key in (0, 1):
        for node in example.members:
            entry = example.table.entry(key, node)
            packed = int.from_bytes(entry.to_bytes()[8:], "little")
            assert packed == literal[key] | (entry.route_bits.to_int() >> 64)
