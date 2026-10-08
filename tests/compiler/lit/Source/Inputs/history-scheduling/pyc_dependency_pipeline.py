"""Four-entry dependency scheduling with live-entry predecessor readiness."""

from pycircuit import bits, concat, module, queue, rule, struct, table, u1, u2, u64


@struct
class Token:
    sequence: bits[4]
    waits_for: bits[4]
    resource: bits[1]
    cycles: bits[4]
    value: bits[16]


@struct
class Entry:
    valid: u1
    state: u2
    deadline: u64
    data: Token


@struct
class Result:
    ready: u1
    available: u1
    head: Token


@struct
class Deadline:
    value: u64


@rule
def execution_deadline(epoch: u64, cost: bits[4]) -> Deadline:
    extended: u64 = cost
    return Deadline(value=epoch + extended)


@rule
def advance(
    entries,
    epoch,
    admit,
    free_index,
    head,
    duplicate,
    issue0,
    issue_index0,
    deadline0,
    issue1,
    issue_index1,
    deadline1,
    retire,
    done_index,
    complete0,
    complete1,
    complete2,
    complete3,
    old0,
    old1,
    old2,
    old3,
):
    resource_check: u2 = head.resource
    if admit:
        assert head.cycles != 0, "dependency_nonpositive_cost"
        assert duplicate == 0, "dependency_duplicate_key"
        assert resource_check < 2, "dependency_resource_out_of_range"
    if issue0:
        assert deadline0 >= epoch, "dependency_time_overflow"
    if issue1:
        assert deadline1 >= epoch, "dependency_time_overflow"

    if retire:
        entries[done_index].valid = 0
    if complete0:
        entries[0].state = 2
    if complete1:
        entries[1].state = 2
    if complete2:
        entries[2].state = 2
    if complete3:
        entries[3].state = 2
    if issue0:
        entries[issue_index0].state = 1
        entries[issue_index0].deadline = deadline0
    if issue1:
        entries[issue_index1].state = 1
        entries[issue_index1].deadline = deadline1
    if admit:
        entries[free_index].valid = 1
        entries[free_index].state = 0
        entries[free_index].deadline = 0
        entries[free_index].data = head
    epoch = epoch + 1


@module
def DependencyPipeline(valid: u1, data: Token, take: u1) -> Result:  # noqa: N802
    entries = table[4, Entry](init=0)
    epoch: u64 = 0
    old0 = entries[0]
    old1 = entries[1]
    old2 = entries[2]
    old3 = entries[3]
    free_index, free_valid = entries.first(where=lambda entry: entry.valid == 0)
    ready, available, head = queue[Token](
        valid, data, free_valid, depth=4, latency=1, ready_policy="local_occupancy"
    )
    admit = available & free_valid
    duplicate = (
        (old0.valid & (old0.data.sequence == head.sequence))
        | (old1.valid & (old1.data.sequence == head.sequence))
        | (old2.valid & (old2.data.sequence == head.sequence))
        | (old3.valid & (old3.data.sequence == head.sequence))
    )
    busy0 = (
        (
            old0.valid
            & (old0.state == 1)
            & (old0.deadline > epoch)
            & (old0.data.resource == 0)
        )
        | (
            old1.valid
            & (old1.state == 1)
            & (old1.deadline > epoch)
            & (old1.data.resource == 0)
        )
        | (
            old2.valid
            & (old2.state == 1)
            & (old2.deadline > epoch)
            & (old2.data.resource == 0)
        )
        | (
            old3.valid
            & (old3.state == 1)
            & (old3.deadline > epoch)
            & (old3.data.resource == 0)
        )
    )
    issue_index0, issue_valid0 = entries.argmin(
        where=lambda entry: entry.valid
        & (entry.state == 0)
        & (entry.data.resource == 0)
        & (
            (entry.data.waits_for == 15)
            | (
                old0.valid
                & (old0.state == 2)
                & (old0.data.sequence == entry.data.waits_for)
            )
            | (
                old1.valid
                & (old1.state == 2)
                & (old1.data.sequence == entry.data.waits_for)
            )
            | (
                old2.valid
                & (old2.state == 2)
                & (old2.data.sequence == entry.data.waits_for)
            )
            | (
                old3.valid
                & (old3.state == 2)
                & (old3.data.sequence == entry.data.waits_for)
            )
        ),
        key=lambda entry: entry.data.sequence,
    )
    issue0 = issue_valid0 & ~busy0
    deadline_result0 = execution_deadline(epoch, entries[issue_index0].data.cycles)
    deadline0 = deadline_result0.value
    busy1 = (
        (
            old0.valid
            & (old0.state == 1)
            & (old0.deadline > epoch)
            & (old0.data.resource == 1)
        )
        | (
            old1.valid
            & (old1.state == 1)
            & (old1.deadline > epoch)
            & (old1.data.resource == 1)
        )
        | (
            old2.valid
            & (old2.state == 1)
            & (old2.deadline > epoch)
            & (old2.data.resource == 1)
        )
        | (
            old3.valid
            & (old3.state == 1)
            & (old3.deadline > epoch)
            & (old3.data.resource == 1)
        )
    )
    issue_index1, issue_valid1 = entries.argmin(
        where=lambda entry: entry.valid
        & (entry.state == 0)
        & (entry.data.resource == 1)
        & (
            (entry.data.waits_for == 15)
            | (
                old0.valid
                & (old0.state == 2)
                & (old0.data.sequence == entry.data.waits_for)
            )
            | (
                old1.valid
                & (old1.state == 2)
                & (old1.data.sequence == entry.data.waits_for)
            )
            | (
                old2.valid
                & (old2.state == 2)
                & (old2.data.sequence == entry.data.waits_for)
            )
            | (
                old3.valid
                & (old3.state == 2)
                & (old3.data.sequence == entry.data.waits_for)
            )
        ),
        key=lambda entry: entry.data.sequence,
    )
    issue1 = issue_valid1 & ~busy1
    deadline_result1 = execution_deadline(epoch, entries[issue_index1].data.cycles)
    deadline1 = deadline_result1.value
    complete0 = old0.valid & (old0.state == 1) & (old0.deadline <= epoch)
    complete1 = old1.valid & (old1.state == 1) & (old1.deadline <= epoch)
    complete2 = old2.valid & (old2.state == 1) & (old2.deadline <= epoch)
    complete3 = old3.valid & (old3.state == 1) & (old3.deadline <= epoch)
    done_index, done_valid = entries.argmin(
        where=lambda entry: entry.valid & (entry.state == 2),
        key=lambda entry: concat(entry.deadline, entry.data.sequence),
    )
    space, completed_valid, completed_head = queue[Token](
        done_valid,
        entries[done_index].data,
        take,
        depth=4,
        latency=1,
        ready_policy="local_occupancy",
    )
    retire = done_valid & space
    advance(
        entries,
        epoch,
        admit,
        free_index,
        head,
        duplicate,
        issue0,
        issue_index0,
        deadline0,
        issue1,
        issue_index1,
        deadline1,
        retire,
        done_index,
        complete0,
        complete1,
        complete2,
        complete3,
        old0,
        old1,
        old2,
        old3,
    )
    return Result(ready=ready, available=completed_valid, head=completed_head)
