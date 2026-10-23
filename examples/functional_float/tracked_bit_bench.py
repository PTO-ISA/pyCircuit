"""Independent small state oracle for the public composed word provider."""
import pycircuit as ac
from functional_float.tracked_bit_transform import TrackedBitTransform


@ac.struct
class Row:
    lhs: ac.u64
    rhs: ac.u64
    width: ac.u7
    first: ac.u6
    last: ac.u6
    operation: ac.u4
    request: ac.u1
    saved: ac.u64
    commits: ac.u32
    rejected: ac.u1


def rows() -> ac.table[8, Row]:
    entries: ac.table[8, Row] = (
        Row(lhs=2, rhs=2, width=64, first=32, last=63, operation=2, request=1,
            saved=0, commits=0, rejected=0),
        Row(lhs=0, rhs=99, width=0, first=0, last=63, operation=7, request=0,
            saved=0x200000002, commits=1, rejected=0),
        Row(lhs=0, rhs=99, width=0, first=0, last=63, operation=2, request=1,
            saved=0x200000002, commits=1, rejected=0),
        Row(lhs=0, rhs=99, width=64, first=0, last=63, operation=7, request=1,
            saved=0x200000002, commits=1, rejected=1),
        Row(lhs=1, rhs=1, width=64, first=63, last=32, operation=2, request=1,
            saved=0x200000002, commits=1, rejected=1),
        Row(lhs=0, rhs=0x123456789ABCDEF0, width=64, first=0, last=63, operation=2, request=1,
            saved=0x8000000000000000, commits=2, rejected=1),
        Row(lhs=0, rhs=0, width=64, first=0, last=63, operation=2, request=0,
            saved=0x123456789ABCDEF0, commits=3, rejected=1),
        Row(lhs=0, rhs=0, width=0, first=0, last=63, operation=7, request=0,
            saved=0x123456789ABCDEF0, commits=3, rejected=1),
    )
    return entries


@ac.rule
def advance(phase):
    phase = phase + 1


@ac.system
def CommitSystem():  # noqa: N802
    phase: ac.u3 = 0
    row = rows()[phase]
    state = TrackedBitTransform(row.lhs, row.rhs, 0, row.width, row.first,
                                row.last, row.operation, row.request)

    @ac.rule
    def verify():
        assert state.value == row.saved, "word commit value"
        assert state.commits == row.commits, "word commit count"
        assert state.rejected == row.rejected, "word sticky rejection"

    verify()
    advance(phase)
