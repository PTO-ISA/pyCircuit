"""JIT-bound widths and fixed aggregate shapes become concrete before ACIR."""

from __future__ import annotations

import agentic_circuit as ac


ROB_ENTRIES = ac.param[int]("rob_entries")
ISSUE_WIDTH = ac.param[int]("issue_width")


@ac.struct
class RobEntry:
    index: ac.bits[ac.index_width(ROB_ENTRIES)]
    generation: ac.u16
    valid: bool


@ac.struct
class IssueGroup:
    entries: ac.array[ISSUE_WIDTH, RobEntry]
    count: ac.bits[ac.count_width(ROB_ENTRIES)]


@ac.rule
def keep(value: IssueGroup) -> IssueGroup:
    return value


@ac.system
def parameterized_types(
    value: IssueGroup,
    *,
    rob_entries: ac.const[int],
    issue_width: ac.const[int],
) -> IssueGroup:
    result = keep(value)
    return result


specialization = ac.jit(
    parameterized_types,
    rob_entries=128,
    issue_width=4,
)
