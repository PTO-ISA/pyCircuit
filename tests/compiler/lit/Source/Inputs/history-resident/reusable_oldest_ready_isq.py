"""Oldest-ready resident ISQ core with the exact old-entry readiness snapshot set."""

# ruff: noqa: F841 -- ordinary hardware state proposals.
from pycircuit import module, rule, struct, table, u1, u2, u6, u8, u16


@struct
class IssueEntry:
    index: u2
    age: u8
    src0_tag: u6
    src1_tag: u6
    value: u16
    valid: u1


@struct
class Readiness:
    tag: u6
    ready: u1


@struct
class IsqResult:
    request_take: u1
    readiness_take: u1
    issued_valid: u1
    issued_data: IssueEntry


@rule
def update_ready(ready_tags, event: Readiness, grant):
    if grant:
        ready_tags[event.tag] = event.ready


@rule
def dispatch(entries, index, request: IssueEntry, grant):
    if grant:
        entries[index] = IssueEntry(
            index=index,
            age=request.age,
            src0_tag=request.src0_tag,
            src1_tag=request.src1_tag,
            value=request.value,
            valid=1,
        )


@rule
def issue(entries, index, grant):
    if grant:
        entries[index].valid = 0


@module
def isq(
    request_available: u1,
    request: IssueEntry,
    readiness_available: u1,
    readiness: Readiness,
    issued_space: u1,
) -> IsqResult:
    entries = table[4, IssueEntry](init=0)
    ready_tags = table[64, u1](init=0)
    free_index, free_valid = entries.first(where=lambda entry: not entry.valid)
    issue_index, issue_valid = entries.argmin(
        where=lambda entry: entry.valid
        and ready_tags[entry.src0_tag]
        and ready_tags[entry.src1_tag],
        key=lambda entry: entry.age,
    )
    selected = entries[issue_index]
    readiness_conflict = (
        (readiness.tag == entries[0].src0_tag)
        | (readiness.tag == entries[0].src1_tag)
        | (readiness.tag == entries[1].src0_tag)
        | (readiness.tag == entries[1].src1_tag)
        | (readiness.tag == entries[2].src0_tag)
        | (readiness.tag == entries[2].src1_tag)
        | (readiness.tag == entries[3].src0_tag)
        | (readiness.tag == entries[3].src1_tag)
    )
    readiness_fire = readiness_available
    dispatch_fire = request_available & free_valid
    issue_fire = (
        issue_valid
        & issued_space
        & ~dispatch_fire
        & ~(readiness_fire & readiness_conflict)
    )
    update_ready(ready_tags, readiness, readiness_fire)
    dispatch(entries, free_index, request, dispatch_fire)
    issue(entries, issue_index, issue_fire)
    return IsqResult(
        request_take=dispatch_fire,
        readiness_take=readiness_fire,
        issued_valid=issue_fire,
        issued_data=selected,
    )
