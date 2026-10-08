"""Four-input round-robin selection; cursor advances only on committed transfer."""

# ruff: noqa: F841 -- hardware state proposals.
from history_routed.records import WorkItem
from pycircuit import module, rule, struct, u1, u2


@struct
class MergeMove:
    take0: u1
    take1: u1
    take2: u1
    take3: u1
    push: u1
    item: WorkItem


@rule
def choose(
    cursor,
    avail0,
    item0: WorkItem,
    avail1,
    item1: WorkItem,
    avail2,
    item2: WorkItem,
    avail3,
    item3: WorkItem,
    space,
) -> MergeMove:
    selected: u2 = cursor
    selected_valid: u1 = 0
    selected_item = WorkItem(
        sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
    )
    if cursor == 0:
        if avail0:
            selected = 0
            selected_valid = 1
            selected_item = item0
        elif avail1:
            selected = 1
            selected_valid = 1
            selected_item = item1
        elif avail2:
            selected = 2
            selected_valid = 1
            selected_item = item2
        elif avail3:
            selected = 3
            selected_valid = 1
            selected_item = item3
    if cursor == 1:
        if avail1:
            selected = 1
            selected_valid = 1
            selected_item = item1
        elif avail2:
            selected = 2
            selected_valid = 1
            selected_item = item2
        elif avail3:
            selected = 3
            selected_valid = 1
            selected_item = item3
        elif avail0:
            selected = 0
            selected_valid = 1
            selected_item = item0
    if cursor == 2:
        if avail2:
            selected = 2
            selected_valid = 1
            selected_item = item2
        elif avail3:
            selected = 3
            selected_valid = 1
            selected_item = item3
        elif avail0:
            selected = 0
            selected_valid = 1
            selected_item = item0
        elif avail1:
            selected = 1
            selected_valid = 1
            selected_item = item1
    if cursor == 3:
        if avail3:
            selected = 3
            selected_valid = 1
            selected_item = item3
        elif avail0:
            selected = 0
            selected_valid = 1
            selected_item = item0
        elif avail1:
            selected = 1
            selected_valid = 1
            selected_item = item1
        elif avail2:
            selected = 2
            selected_valid = 1
            selected_item = item2
    fire = selected_valid & space
    if fire:
        cursor = selected + 1
    return MergeMove(
        take0=fire & (selected == 0),
        take1=fire & (selected == 1),
        take2=fire & (selected == 2),
        take3=fire & (selected == 3),
        push=fire,
        item=selected_item,
    )


@module
def merge(
    avail0: u1,
    item0: WorkItem,
    avail1: u1,
    item1: WorkItem,
    avail2: u1,
    item2: WorkItem,
    avail3: u1,
    item3: WorkItem,
    space: u1,
) -> MergeMove:
    cursor: u2 = 0
    return choose(
        cursor, avail0, item0, avail1, item1, avail2, item2, avail3, item3, space
    )
