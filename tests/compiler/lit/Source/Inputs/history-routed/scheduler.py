"""Global eight-entry dependency scheduler with live retained Done readiness."""

# ruff: noqa: F841 -- hardware state proposals.
from history_routed.records import WorkItem
from pycircuit import concat, module, rule, struct, table, u1, u2, u3, u64


@struct
class SchedulerEntry:
    valid: u1
    state: u2
    deadline: u64
    item: WorkItem


@struct
class SchedulerMove:
    take: u1
    push: u1
    item: WorkItem


@struct
class Deadline:
    value: u64


@rule
def execution_deadline(epoch: u64, cost) -> Deadline:
    extended: u64 = cost
    return Deadline(value=epoch + extended)


@rule
def advance(
    entries,
    epoch,
    head,
    admit,
    free_index,
    duplicate,
    issue0,
    issue_index0,
    deadline0,
    issue1,
    issue_index1,
    deadline1,
    issue2,
    issue_index2,
    deadline2,
    issue3,
    issue_index3,
    deadline3,
    retire,
    done_index,
    complete0,
    complete1,
    complete2,
    complete3,
    complete4,
    complete5,
    complete6,
    complete7,
):
    if admit:
        assert head.cycles != 0, "dependency_nonpositive_cost"
        assert not duplicate, "dependency_duplicate_key"
        resource_check: u3 = head.route
        assert resource_check < 4, "dependency_resource_out_of_range"
    if issue0:
        assert deadline0 >= epoch, "dependency_time_overflow"
    if issue1:
        assert deadline1 >= epoch, "dependency_time_overflow"
    if issue2:
        assert deadline2 >= epoch, "dependency_time_overflow"
    if issue3:
        assert deadline3 >= epoch, "dependency_time_overflow"
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
    if complete4:
        entries[4].state = 2
    if complete5:
        entries[5].state = 2
    if complete6:
        entries[6].state = 2
    if complete7:
        entries[7].state = 2
    if issue0:
        entries[issue_index0].state = 1
        entries[issue_index0].deadline = deadline0
    if issue1:
        entries[issue_index1].state = 1
        entries[issue_index1].deadline = deadline1
    if issue2:
        entries[issue_index2].state = 1
        entries[issue_index2].deadline = deadline2
    if issue3:
        entries[issue_index3].state = 1
        entries[issue_index3].deadline = deadline3
    if admit:
        entries[free_index].valid = 1
        entries[free_index].state = 0
        entries[free_index].deadline = 0
        entries[free_index].item = head
    epoch = epoch + 1


@module
def dependency(available: u1, head: WorkItem, space: u1) -> SchedulerMove:
    entries = table[8, SchedulerEntry](init=0)
    epoch: u64 = 0
    old0 = entries[0]
    old1 = entries[1]
    old2 = entries[2]
    old3 = entries[3]
    old4 = entries[4]
    old5 = entries[5]
    old6 = entries[6]
    old7 = entries[7]
    free_index, free_valid = entries.first(where=lambda row: not row.valid)
    admit = available & free_valid
    duplicate = (
        (old0.valid & (old0.item.sequence_id == head.sequence_id))
        | (old1.valid & (old1.item.sequence_id == head.sequence_id))
        | (old2.valid & (old2.item.sequence_id == head.sequence_id))
        | (old3.valid & (old3.item.sequence_id == head.sequence_id))
        | (old4.valid & (old4.item.sequence_id == head.sequence_id))
        | (old5.valid & (old5.item.sequence_id == head.sequence_id))
        | (old6.valid & (old6.item.sequence_id == head.sequence_id))
        | (old7.valid & (old7.item.sequence_id == head.sequence_id))
    )
    busy0 = (
        (
            old0.valid
            & (old0.state == 1)
            & (old0.deadline > epoch)
            & (old0.item.route == 0)
        )
        | (
            old1.valid
            & (old1.state == 1)
            & (old1.deadline > epoch)
            & (old1.item.route == 0)
        )
        | (
            old2.valid
            & (old2.state == 1)
            & (old2.deadline > epoch)
            & (old2.item.route == 0)
        )
        | (
            old3.valid
            & (old3.state == 1)
            & (old3.deadline > epoch)
            & (old3.item.route == 0)
        )
        | (
            old4.valid
            & (old4.state == 1)
            & (old4.deadline > epoch)
            & (old4.item.route == 0)
        )
        | (
            old5.valid
            & (old5.state == 1)
            & (old5.deadline > epoch)
            & (old5.item.route == 0)
        )
        | (
            old6.valid
            & (old6.state == 1)
            & (old6.deadline > epoch)
            & (old6.item.route == 0)
        )
        | (
            old7.valid
            & (old7.state == 1)
            & (old7.deadline > epoch)
            & (old7.item.route == 0)
        )
    )
    issue_index0, issue_valid0 = entries.argmin(
        where=lambda row: row.valid
        & (row.state == 0)
        & (row.item.route == 0)
        & (
            (row.item.waits_for == 255)
            | (
                old0.valid
                & (old0.state == 2)
                & (old0.item.sequence_id == row.item.waits_for)
            )
            | (
                old1.valid
                & (old1.state == 2)
                & (old1.item.sequence_id == row.item.waits_for)
            )
            | (
                old2.valid
                & (old2.state == 2)
                & (old2.item.sequence_id == row.item.waits_for)
            )
            | (
                old3.valid
                & (old3.state == 2)
                & (old3.item.sequence_id == row.item.waits_for)
            )
            | (
                old4.valid
                & (old4.state == 2)
                & (old4.item.sequence_id == row.item.waits_for)
            )
            | (
                old5.valid
                & (old5.state == 2)
                & (old5.item.sequence_id == row.item.waits_for)
            )
            | (
                old6.valid
                & (old6.state == 2)
                & (old6.item.sequence_id == row.item.waits_for)
            )
            | (
                old7.valid
                & (old7.state == 2)
                & (old7.item.sequence_id == row.item.waits_for)
            )
        ),
        key=lambda row: row.item.sequence_id,
    )
    issue0 = issue_valid0 & ~busy0
    deadline_result0 = execution_deadline(epoch, entries[issue_index0].item.cycles)
    deadline0 = deadline_result0.value
    busy1 = (
        (
            old0.valid
            & (old0.state == 1)
            & (old0.deadline > epoch)
            & (old0.item.route == 1)
        )
        | (
            old1.valid
            & (old1.state == 1)
            & (old1.deadline > epoch)
            & (old1.item.route == 1)
        )
        | (
            old2.valid
            & (old2.state == 1)
            & (old2.deadline > epoch)
            & (old2.item.route == 1)
        )
        | (
            old3.valid
            & (old3.state == 1)
            & (old3.deadline > epoch)
            & (old3.item.route == 1)
        )
        | (
            old4.valid
            & (old4.state == 1)
            & (old4.deadline > epoch)
            & (old4.item.route == 1)
        )
        | (
            old5.valid
            & (old5.state == 1)
            & (old5.deadline > epoch)
            & (old5.item.route == 1)
        )
        | (
            old6.valid
            & (old6.state == 1)
            & (old6.deadline > epoch)
            & (old6.item.route == 1)
        )
        | (
            old7.valid
            & (old7.state == 1)
            & (old7.deadline > epoch)
            & (old7.item.route == 1)
        )
    )
    issue_index1, issue_valid1 = entries.argmin(
        where=lambda row: row.valid
        & (row.state == 0)
        & (row.item.route == 1)
        & (
            (row.item.waits_for == 255)
            | (
                old0.valid
                & (old0.state == 2)
                & (old0.item.sequence_id == row.item.waits_for)
            )
            | (
                old1.valid
                & (old1.state == 2)
                & (old1.item.sequence_id == row.item.waits_for)
            )
            | (
                old2.valid
                & (old2.state == 2)
                & (old2.item.sequence_id == row.item.waits_for)
            )
            | (
                old3.valid
                & (old3.state == 2)
                & (old3.item.sequence_id == row.item.waits_for)
            )
            | (
                old4.valid
                & (old4.state == 2)
                & (old4.item.sequence_id == row.item.waits_for)
            )
            | (
                old5.valid
                & (old5.state == 2)
                & (old5.item.sequence_id == row.item.waits_for)
            )
            | (
                old6.valid
                & (old6.state == 2)
                & (old6.item.sequence_id == row.item.waits_for)
            )
            | (
                old7.valid
                & (old7.state == 2)
                & (old7.item.sequence_id == row.item.waits_for)
            )
        ),
        key=lambda row: row.item.sequence_id,
    )
    issue1 = issue_valid1 & ~busy1
    deadline_result1 = execution_deadline(epoch, entries[issue_index1].item.cycles)
    deadline1 = deadline_result1.value
    busy2 = (
        (
            old0.valid
            & (old0.state == 1)
            & (old0.deadline > epoch)
            & (old0.item.route == 2)
        )
        | (
            old1.valid
            & (old1.state == 1)
            & (old1.deadline > epoch)
            & (old1.item.route == 2)
        )
        | (
            old2.valid
            & (old2.state == 1)
            & (old2.deadline > epoch)
            & (old2.item.route == 2)
        )
        | (
            old3.valid
            & (old3.state == 1)
            & (old3.deadline > epoch)
            & (old3.item.route == 2)
        )
        | (
            old4.valid
            & (old4.state == 1)
            & (old4.deadline > epoch)
            & (old4.item.route == 2)
        )
        | (
            old5.valid
            & (old5.state == 1)
            & (old5.deadline > epoch)
            & (old5.item.route == 2)
        )
        | (
            old6.valid
            & (old6.state == 1)
            & (old6.deadline > epoch)
            & (old6.item.route == 2)
        )
        | (
            old7.valid
            & (old7.state == 1)
            & (old7.deadline > epoch)
            & (old7.item.route == 2)
        )
    )
    issue_index2, issue_valid2 = entries.argmin(
        where=lambda row: row.valid
        & (row.state == 0)
        & (row.item.route == 2)
        & (
            (row.item.waits_for == 255)
            | (
                old0.valid
                & (old0.state == 2)
                & (old0.item.sequence_id == row.item.waits_for)
            )
            | (
                old1.valid
                & (old1.state == 2)
                & (old1.item.sequence_id == row.item.waits_for)
            )
            | (
                old2.valid
                & (old2.state == 2)
                & (old2.item.sequence_id == row.item.waits_for)
            )
            | (
                old3.valid
                & (old3.state == 2)
                & (old3.item.sequence_id == row.item.waits_for)
            )
            | (
                old4.valid
                & (old4.state == 2)
                & (old4.item.sequence_id == row.item.waits_for)
            )
            | (
                old5.valid
                & (old5.state == 2)
                & (old5.item.sequence_id == row.item.waits_for)
            )
            | (
                old6.valid
                & (old6.state == 2)
                & (old6.item.sequence_id == row.item.waits_for)
            )
            | (
                old7.valid
                & (old7.state == 2)
                & (old7.item.sequence_id == row.item.waits_for)
            )
        ),
        key=lambda row: row.item.sequence_id,
    )
    issue2 = issue_valid2 & ~busy2
    deadline_result2 = execution_deadline(epoch, entries[issue_index2].item.cycles)
    deadline2 = deadline_result2.value
    busy3 = (
        (
            old0.valid
            & (old0.state == 1)
            & (old0.deadline > epoch)
            & (old0.item.route == 3)
        )
        | (
            old1.valid
            & (old1.state == 1)
            & (old1.deadline > epoch)
            & (old1.item.route == 3)
        )
        | (
            old2.valid
            & (old2.state == 1)
            & (old2.deadline > epoch)
            & (old2.item.route == 3)
        )
        | (
            old3.valid
            & (old3.state == 1)
            & (old3.deadline > epoch)
            & (old3.item.route == 3)
        )
        | (
            old4.valid
            & (old4.state == 1)
            & (old4.deadline > epoch)
            & (old4.item.route == 3)
        )
        | (
            old5.valid
            & (old5.state == 1)
            & (old5.deadline > epoch)
            & (old5.item.route == 3)
        )
        | (
            old6.valid
            & (old6.state == 1)
            & (old6.deadline > epoch)
            & (old6.item.route == 3)
        )
        | (
            old7.valid
            & (old7.state == 1)
            & (old7.deadline > epoch)
            & (old7.item.route == 3)
        )
    )
    issue_index3, issue_valid3 = entries.argmin(
        where=lambda row: row.valid
        & (row.state == 0)
        & (row.item.route == 3)
        & (
            (row.item.waits_for == 255)
            | (
                old0.valid
                & (old0.state == 2)
                & (old0.item.sequence_id == row.item.waits_for)
            )
            | (
                old1.valid
                & (old1.state == 2)
                & (old1.item.sequence_id == row.item.waits_for)
            )
            | (
                old2.valid
                & (old2.state == 2)
                & (old2.item.sequence_id == row.item.waits_for)
            )
            | (
                old3.valid
                & (old3.state == 2)
                & (old3.item.sequence_id == row.item.waits_for)
            )
            | (
                old4.valid
                & (old4.state == 2)
                & (old4.item.sequence_id == row.item.waits_for)
            )
            | (
                old5.valid
                & (old5.state == 2)
                & (old5.item.sequence_id == row.item.waits_for)
            )
            | (
                old6.valid
                & (old6.state == 2)
                & (old6.item.sequence_id == row.item.waits_for)
            )
            | (
                old7.valid
                & (old7.state == 2)
                & (old7.item.sequence_id == row.item.waits_for)
            )
        ),
        key=lambda row: row.item.sequence_id,
    )
    issue3 = issue_valid3 & ~busy3
    deadline_result3 = execution_deadline(epoch, entries[issue_index3].item.cycles)
    deadline3 = deadline_result3.value
    complete0 = old0.valid & (old0.state == 1) & (old0.deadline <= epoch)
    complete1 = old1.valid & (old1.state == 1) & (old1.deadline <= epoch)
    complete2 = old2.valid & (old2.state == 1) & (old2.deadline <= epoch)
    complete3 = old3.valid & (old3.state == 1) & (old3.deadline <= epoch)
    complete4 = old4.valid & (old4.state == 1) & (old4.deadline <= epoch)
    complete5 = old5.valid & (old5.state == 1) & (old5.deadline <= epoch)
    complete6 = old6.valid & (old6.state == 1) & (old6.deadline <= epoch)
    complete7 = old7.valid & (old7.state == 1) & (old7.deadline <= epoch)
    done_index, done_valid = entries.argmin(
        where=lambda row: row.valid & (row.state == 2),
        key=lambda row: concat(row.deadline, row.item.sequence_id),
    )
    retiring_item = entries[done_index].item
    retire = done_valid & space
    advance(
        entries,
        epoch,
        head,
        admit,
        free_index,
        duplicate,
        issue0,
        issue_index0,
        deadline0,
        issue1,
        issue_index1,
        deadline1,
        issue2,
        issue_index2,
        deadline2,
        issue3,
        issue_index3,
        deadline3,
        retire,
        done_index,
        complete0,
        complete1,
        complete2,
        complete3,
        complete4,
        complete5,
        complete6,
        complete7,
    )
    return SchedulerMove(take=admit, push=retire, item=retiring_item)
