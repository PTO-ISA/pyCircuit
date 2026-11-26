"""External stimulus validation must precede compilation, without truncation."""

from copy import deepcopy

import pytest

from tools.receive_backend import _validate
from tools.receive_scenarios import request, scenario

pytestmark = pytest.mark.unit


@pytest.mark.parametrize(
    "field,value",
    [
        ("source", 40),
        ("source", 63),
        ("seq", 65536),
        ("seq", -1),
        ("seq", True),
        ("epoch", 256),
        ("address", 1 << 64),
        ("nbytes", 1 << 32),
        ("committed", 1),
        ("self_sourced", 0),
    ],
)
def test_reject_unrepresentable_segment_fields(field, value):
    case = scenario("encoding", [request(2)])
    case["requests"][0]["segment"][field] = value
    with pytest.raises(ValueError):
        _validate(case)


@pytest.mark.parametrize(
    "field,value",
    [
        ("kind", 4),
        ("capacity", -1),
        ("expected", 1 << 32),
        ("compiled_expected", True),
    ],
)
def test_reject_unrepresentable_command_fields(field, value):
    case = scenario("encoding", [request(1)])
    case["requests"][0][field] = value
    with pytest.raises(ValueError):
        _validate(case)


def test_allow_representable_protocol_faults_without_rewriting_input():
    case = scenario("invalid-protocol", [request(1, opcode=0, epoch=0, expected=17)])
    before = deepcopy(case)
    _validate(case)
    assert case == before


@pytest.mark.parametrize("field,value", [("early_slots", 0), ("seen_slots", 256)])
def test_reject_invalid_resource_configuration(field, value):
    case = scenario("encoding", [request()])
    case[field] = value
    with pytest.raises(ValueError):
        _validate(case)
