"""The seven ``expVal`` contracts of Calendar §4.5.2."""

from __future__ import annotations

import pytest

from wse_model.calendar.receive import (
    CoreIngress,
    DeliveryOutcome,
    ReceiveAccount,
    Segment,
    SegmentId,
)
from wse_model.errors import ReceiveContractError, WseModelError

pytestmark = pytest.mark.unit

OPCODE = 1
EPOCH = 1


def _segment(
    *,
    source: int = 7,
    seq: int = 0,
    address: int = 0x1000,
    nbytes: int = 64,
    opcode: int = OPCODE,
    epoch: int = EPOCH,
    self_sourced: bool = False,
) -> Segment:
    return Segment(
        identity=SegmentId(source=source, seq=seq),
        opcode=opcode,
        epoch=epoch,
        address=address,
        nbytes=nbytes,
        self_sourced=self_sourced,
    )


def _account(*, exp_val: int = 128, capacity: int = 512, **kwargs) -> ReceiveAccount:
    return ReceiveAccount(
        aicore=kwargs.pop("aicore", 0),
        opcode=kwargs.pop("opcode", OPCODE),
        epoch=kwargs.pop("epoch", EPOCH),
        dst=kwargs.pop("dst", 0x1000),
        capacity=capacity,
        exp_val=exp_val,
        **kwargs,
    )


def test_contract_1_the_unit_is_payload_bytes() -> None:
    """Header, padding, retransmits, and the source's own bytes never count."""
    account = _account(exp_val=64)
    assert account.deliver(_segment(self_sourced=True)) is DeliveryOutcome.SELF_SOURCED
    assert account.counted_bytes == 0
    assert not account.complete
    assert account.deliver(_segment()) is DeliveryOutcome.COUNTED
    assert account.counted_bytes == 64
    assert account.complete


def test_contract_2_wrong_epoch_faults() -> None:
    account = _account()
    with pytest.raises(ReceiveContractError) as excinfo:
        account.deliver(_segment(epoch=EPOCH + 1))
    assert excinfo.value.contract == 2


def test_contract_2_wrong_opcode_faults() -> None:
    account = _account()
    with pytest.raises(ReceiveContractError) as excinfo:
        account.deliver(_segment(opcode=2))
    assert excinfo.value.contract == 2


def test_contract_2_out_of_range_write_faults() -> None:
    account = _account(exp_val=64, capacity=64)
    with pytest.raises(ReceiveContractError) as excinfo:
        account.deliver(_segment(address=0x1000 + 32, nbytes=64))
    assert excinfo.value.contract == 2
    with pytest.raises(ReceiveContractError):
        account.deliver(_segment(address=0x1000 - 1))


def test_contract_4_early_arrivals_are_not_missed() -> None:
    """Step3 posts after alignment, but the NoC may already be delivering."""
    ingress = CoreIngress(0)
    early = _segment(source=3)
    assert ingress.deliver(early) is DeliveryOutcome.DEFERRED
    assert ingress.pending[(OPCODE, EPOCH)] == (early,)

    account = ingress.post(_account(exp_val=64))
    assert account.counted_bytes == 64
    assert account.complete
    assert ingress.pending == {}


def test_contract_5_duplicate_segments_are_not_double_counted() -> None:
    account = _account(exp_val=128)
    same = _segment(seq=5)
    assert account.deliver(same) is DeliveryOutcome.COUNTED
    assert account.deliver(same) is DeliveryOutcome.DUPLICATE
    assert account.counted_bytes == 64
    assert not account.complete


def test_contract_5_multicast_copies_dedupe_per_land_point() -> None:
    """Two paths reaching one core with the same segment count once."""
    from wse_model.calendar.receive import SegmentId as Sid

    ingress = CoreIngress(4)
    ingress.post(_account(aicore=4, exp_val=64))
    copy_a = _segment(source=0, seq=9)
    copy_b = _segment(source=0, seq=9)
    assert copy_a.identity == copy_b.identity == Sid(0, 9)
    assert ingress.deliver(copy_a) is DeliveryOutcome.COUNTED
    assert ingress.deliver(copy_b) is DeliveryOutcome.DUPLICATE


def test_contract_6_exp_val_zero_completes_immediately() -> None:
    account = _account(exp_val=0, capacity=64)
    assert account.complete
    assert account.remaining == 0


def test_contract_6_exp_val_above_capacity_faults() -> None:
    with pytest.raises(ReceiveContractError) as excinfo:
        _account(exp_val=1024, capacity=512)
    assert excinfo.value.contract == 6


def test_contract_6_a_compiled_exp_val_mismatch_faults() -> None:
    with pytest.raises(ReceiveContractError) as excinfo:
        _account(exp_val=64, compiled_exp_val=128)
    assert excinfo.value.contract == 6


def test_contract_6_counter_overflow_faults_rather_than_satisfying_exp_val() -> None:
    """Both spans fit; only the accumulated bytes overflow the 96-byte capacity."""
    account = _account(exp_val=64, capacity=96)
    account.deliver(_segment(seq=1, nbytes=64, address=0x1000))
    with pytest.raises(ReceiveContractError) as excinfo:
        account.deliver(_segment(seq=2, nbytes=64, address=0x1020))
    assert excinfo.value.contract == 6
    assert account.counted_bytes == 64
    assert account.seen == {SegmentId(7, 1)}
    # Retry the rejected identity with a legal length: it was never recorded.
    assert account.deliver(_segment(seq=2, nbytes=32, address=0x1020)) is DeliveryOutcome.COUNTED
    assert account.counted_bytes == 96
    assert account.seen == {SegmentId(7, 1), SegmentId(7, 2)}


def test_contract_7_two_active_receives_for_one_epoch_fault() -> None:
    """The epoch must be unique inside its opcode domain (contract 7)."""
    ingress = CoreIngress(0)
    ingress.post(_account(exp_val=128))
    with pytest.raises(ReceiveContractError) as excinfo:
        ingress.post(_account(exp_val=64))
    assert excinfo.value.contract == 7


def test_reserved_opcode_receive_faults() -> None:
    with pytest.raises(ReceiveContractError) as excinfo:
        _account(opcode=7)
    assert "reserved" in str(excinfo.value)


def test_retire_requires_completion() -> None:
    ingress = CoreIngress(0)
    account = ingress.post(_account(exp_val=128))
    with pytest.raises(WseModelError):
        ingress.retire(account)
    account.deliver(_segment(nbytes=64, seq=1))
    account.deliver(_segment(nbytes=64, seq=2))
    assert account.complete
    ingress.retire(account)
    assert ingress.outstanding == ()


def test_contract_3_only_committed_payload_is_counted() -> None:
    """Accounting follows the commit, which is what makes MTE4 completion acquire."""
    from dataclasses import replace

    account = _account(exp_val=64)
    in_flight = replace(_segment(seq=1), committed=False)
    assert account.deliver(in_flight) is DeliveryOutcome.UNCOMMITTED
    assert account.counted_bytes == 0
    assert not account.complete
    # An uncommitted delivery must not enter the de-duplication set, or the commit
    # would be reported as a duplicate and the bytes would be lost.
    assert in_flight.identity not in account.seen

    assert account.deliver(in_flight.committed_copy()) is DeliveryOutcome.COUNTED
    assert account.counted_bytes == 64
    assert account.complete
    # And now a repeat is a duplicate.
    assert account.deliver(in_flight.committed_copy()) is DeliveryOutcome.DUPLICATE
    assert account.counted_bytes == 64


def test_contract_3_commit_through_the_ingress_ledger() -> None:
    from dataclasses import replace

    ingress = CoreIngress(0)
    ingress.post(_account(exp_val=64))
    pending = replace(_segment(seq=9), committed=False)
    assert ingress.deliver(pending) is DeliveryOutcome.UNCOMMITTED
    assert ingress.outstanding[0].counted_bytes == 0
    assert ingress.commit(pending) is DeliveryOutcome.COUNTED
    assert ingress.outstanding[0].counted_bytes == 64
    assert ingress.outstanding[0].complete


def test_a_segment_defaults_to_committed() -> None:
    """Fixtures and the closure treat a delivered segment as already committed."""
    assert _segment(seq=3).committed is True


def test_an_uncommitted_segment_still_faults_out_of_range() -> None:
    """Contract 3 defers accounting; it does not defer the range check."""
    from dataclasses import replace

    account = _account(exp_val=32, capacity=64)
    bad = replace(_segment(seq=4, address=0x1000 + 32, nbytes=64), committed=False)
    with pytest.raises(ReceiveContractError) as excinfo:
        account.deliver(bad)
    assert excinfo.value.contract == 2
