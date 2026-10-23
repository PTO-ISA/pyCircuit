"""Public numeric composition with module-owned commit state and sticky effects."""
import pycircuit as ac
from functional_float.bit_transform import BitTransform


@ac.struct
class WordCommitState:
    value: ac.u64
    commits: ac.u32
    rejected: ac.u1


@ac.rule
def record_word(response, request, saved, commits, rejected):
    if request:
        if response.valid:
            saved = response.result
            commits = commits + 1
        else:
            rejected = 1


@ac.module
def TrackedBitTransform(  # noqa: N802
    lhs: ac.u64, rhs: ac.u64, alternate: ac.u64, width: ac.u7,
    offset: ac.u6, last: ac.u6, operation: ac.u4, request_valid: ac.u1,
) -> WordCommitState:
    saved: ac.u64 = 0
    commits: ac.u32 = 0
    rejected: ac.u1 = 0
    response = BitTransform(lhs, rhs, alternate, width, offset, last, operation, request_valid)
    record_word(response, request_valid, saved, commits, rejected)
    return WordCommitState(value=saved, commits=commits, rejected=rejected)
