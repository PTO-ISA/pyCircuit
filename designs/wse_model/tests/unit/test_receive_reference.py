"""Check test-adapter assumptions separately from generated circuit execution."""

from copy import deepcopy

import pytest

from tools.receive_scenarios import (
    ModelCapacityError,
    ReceiveReference,
    boundary_scenarios,
    expected_trace,
    request,
)
from wse_model.calendar.receive import CoreIngress, ReceiveAccount, Segment, SegmentId

pytestmark = pytest.mark.unit


def test_early_adoption_delays_external_request_without_losing_it():
    case = boundary_scenarios()[0]
    trace = expected_trace(case)
    assert trace["failure_cycle"] is None
    assert [r["status"] for r in trace["results"]] == [6, 6, 8, 1, 2, 2, 7, 1, 2, 7]
    assert [r["accepted"] for r in trace["results"]] == [
        True,
        True,
        False,
        True,
        False,
        True,
        True,
        True,
        False,
        True,
    ]
    assert [r["pending"] for r in trace["results"]] == [1, 2, 2, 2, 1, 1, 1, 1, 0, 0]
    assert [r["counted"] for r in trace["results"]] == [0, 0, 0, 0, 4, 8, 8, 0, 4, 4]


@pytest.mark.parametrize("buffered", [False, True])
def test_capacity_error_preserves_account_and_pending(buffered):
    reference = ReceiveReference(early_slots=2, seen_slots=1)
    if buffered:
        reference.step(request(2))
        reference.step(request(2, source=2))
    reference.step(request(1))
    reference.step(request(0 if buffered else 2))
    before = deepcopy(reference.__dict__)
    with pytest.raises(ModelCapacityError, match="WSE_MODEL_CAPACITY"):
        reference.step(request(0 if buffered else 2, source=2))
    assert reference.__dict__ == before


def test_quiescent_adoption_matches_unbounded_core_ingress():
    """Same two legal arrivals, with only adoption timing changed by the adapter."""
    ingress = CoreIngress(0)
    reference = ReceiveReference(early_slots=2, seen_slots=2)
    for seq in (0, 1):
        ingress.deliver(Segment(SegmentId(1, seq), 1, 1, 100, 4))
        reference.step(request(2, seq=seq))
    account = ingress.post(ReceiveAccount(0, 1, 1, 100, 16, 8, 8))
    assert reference.step(request(1))["counted"] == 0
    reference.step(request())
    result = reference.step(request())
    assert result["counted"] == account.counted_bytes == 8
    assert result["complete"] is account.complete is True
    assert reference.account.seen == account.seen
    assert not ingress.pending
    assert result["pending"] == 0


def test_protocol_overflow_preserves_previous_account_and_sticks():
    trace = expected_trace(next(c for c in boundary_scenarios() if c["name"] == "byte-overflow"))
    assert [r["counted"] for r in trace["results"]] == [0, 4, 4, 4]
    assert [r["status"] for r in trace["results"]] == [1, 2, 9, 9]
    assert [r["accepted"] for r in trace["results"]] == [True, True, True, False]
