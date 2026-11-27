"""The full FFN oracle and host checker retain all members and both phases."""

import ast
from copy import deepcopy
from pathlib import Path

import pytest

from models.calendar.host_checks import check_allgather
from tools.allgather_reference import expectations
from wse_model.fixtures import ffn_example

pytestmark = pytest.mark.unit


def test_full_core_trace_and_frozen_route_inputs():
    rows = expectations()
    assert len(rows) == 516 * 32
    for cycle, total in ((257, 10752), (515, 36864)):
        final = [row for row in rows if row[0] == cycle]
        assert {row[2] for row in final} == set(range(32))
        assert {row[15] for row in final} == {total}
        assert {row[12] for row in final} == {7}  # Retired after completion.
    tree = ast.parse((Path(__file__).parents[2] / "models/calendar/allgather.py").read_text())
    function = next(
        n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == "route_rows"
    )
    literals = next(n.value for n in function.body if isinstance(n, ast.AnnAssign))
    example = ffn_example()
    assert ast.literal_eval(literals) == tuple(
        example.table.entry(key, source).route_bits.to_int()
        for key in (0, 1)
        for source in range(32)
    )


@pytest.mark.parametrize("mutation", [None, "missing_node", "wrong_count", "wrong_address"])
def test_host_checker_requires_every_node_and_correct_counts(mutation):
    # One post cycle, both sampling points, with 32 distinct account observations.
    wanted = [row for row in expectations()[:32] for _ in range(2)]
    actual = deepcopy(wanted)
    events = [
        {
            "evaluation_epoch": epoch,
            "commit_epoch": epoch + 1,
            "values": [{"value": value} for value in row],
        }
        for epoch in range(2)
        for row in expectations()[:32]
    ]
    if mutation == "missing_node":
        events.pop()
    elif mutation == "wrong_count":
        actual[0][15] = 1
    elif mutation == "wrong_address":
        actual[0][18] += 1
    if mutation:
        with pytest.raises(RuntimeError):
            check_allgather(events, actual, wanted, 1, "test")
    else:
        check_allgather(events, actual, wanted, 1, "test")
