"""Lossless host encoding for the current Calendar receive *test profile*.

This is a boundary adapter for models/calendar/receive.py, not a change to the
unbounded Python semantic core or a WSE packet-header specification. Its current
profile is 40 nodes, 8-bit epoch and a 16-bit sequence carrier. An out-of-profile
integer is rejected before conversion, never masked, truncated or renumbered.
Repeated {source, seq} remains a duplicate; a producer must retain the full
integer sequence before calling this adapter rather than wrap it itself.
"""

from __future__ import annotations

from wse_model.calendar.receive import Segment


class ReceiveEncodingError(ValueError):
    """An input cannot be represented losslessly in the selected test profile."""


def _integer(name: str, value: int, maximum: int) -> int:
    if type(value) is not int or not 0 <= value <= maximum:
        raise ReceiveEncodingError(f"{name}={value!r} is outside test profile 0..{maximum}")
    return value


def encode_segment(segment: Segment) -> dict[str, int | bool]:
    """Return the receive Segment fields, rejecting any lossy representation.

    This performs representation checks only. The receiver still owns address
    range, commit, duplicate and expected-byte accounting checks.
    """
    for name in ("committed", "self_sourced"):
        if type(getattr(segment, name)) is not bool:
            raise ReceiveEncodingError(f"{name} must be bool")
    return {
        "opcode": _integer("opcode", segment.opcode, 7),
        "epoch": _integer("epoch", segment.epoch, 255),
        "address": _integer("address", segment.address, (1 << 64) - 1),
        "nbytes": _integer("nbytes", segment.nbytes, (1 << 32) - 1),
        "source": _integer("source", segment.identity.source, 39),
        "seq": _integer("seq", segment.identity.seq, 65535),
        "committed": segment.committed,
        "self_sourced": segment.self_sourced,
    }
