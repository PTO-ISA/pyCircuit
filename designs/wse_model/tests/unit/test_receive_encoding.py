"""40-node/8-bit-epoch/16-bit-seq test boundary; core remains unbounded."""

from dataclasses import replace

import pytest

from wse_model.calendar.receive import DeliveryOutcome, ReceiveAccount, Segment, SegmentId
from wse_model.calendar.receive_encoding import ReceiveEncodingError, encode_segment

pytestmark = pytest.mark.unit


def segment(source=1, seq=0, epoch=1):
    return Segment(SegmentId(source, seq), opcode=1, epoch=epoch, address=100, nbytes=4)


@pytest.mark.parametrize("seq", [0, 65535])
def test_sequence_endpoints_are_lossless(seq):
    encoded = encode_segment(segment(seq=seq))
    assert encoded["seq"] == seq
    assert encoded["source"] == 1


@pytest.mark.parametrize("seq", [-1, 65536, 1 << 80, True])
def test_unrepresentable_sequence_is_rejected_before_conversion(seq):
    with pytest.raises(ReceiveEncodingError, match="seq"):
        encode_segment(segment(seq=seq))


def test_overflow_does_not_alias_zero_and_core_semantics_are_unchanged():
    account = ReceiveAccount(0, 1, 1, 100, 16, 8)
    assert account.deliver(segment(seq=0)) == DeliveryOutcome.COUNTED
    with pytest.raises(ReceiveEncodingError):
        encode_segment(segment(seq=65536))
    assert account.counted_bytes == 4
    # The ordinary core has no 16-bit truncation and sees a distinct identity.
    assert account.deliver(segment(seq=65536)) == DeliveryOutcome.COUNTED
    assert account.counted_bytes == 8


def test_duplicates_sources_and_epoch_reuse_follow_core_contract():
    first, other = segment(1, 65535), segment(2, 65535)
    assert encode_segment(first)["seq"] == encode_segment(other)["seq"]
    assert encode_segment(first)["source"] != encode_segment(other)["source"]
    account = ReceiveAccount(0, 1, 1, 100, 16, 8)
    assert account.deliver(first) == DeliveryOutcome.COUNTED
    assert account.deliver(first) == DeliveryOutcome.DUPLICATE
    assert account.deliver(other) == DeliveryOutcome.COUNTED
    next_round = ReceiveAccount(0, 1, 2, 100, 16, 4)
    reused = replace(first, epoch=2)
    assert encode_segment(reused)["epoch"] == 2
    assert next_round.deliver(reused) == DeliveryOutcome.COUNTED


@pytest.mark.parametrize("field,value", [("epoch", 256), ("address", 1 << 64), ("nbytes", 1 << 32)])
def test_other_integer_fields_are_not_silently_narrowed(field, value):
    with pytest.raises(ReceiveEncodingError, match=field):
        encode_segment(replace(segment(), **{field: value}))


def test_source_outside_the_40_node_profile_is_rejected():
    with pytest.raises(ReceiveEncodingError, match="source"):
        encode_segment(segment(source=40))
