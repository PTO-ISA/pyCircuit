"""Full 64-entry reorder bookkeeping, with unsigned 64-bit expected key."""

# ruff: noqa: F841 -- hardware state proposals.
from history_routed.records import WorkItem
from pycircuit import module, rule, struct, table, u1, u64


@struct
class ReorderEntry:
    valid: u1
    key: u64
    item: WorkItem


@struct
class ReorderMove:
    take: u1
    push: u1
    item: WorkItem


@rule
def advance(entries, next_key, available, head: WorkItem, space) -> ReorderMove:
    incoming_key: u64 = head.sequence_id
    match_index, match_valid = entries.first(
        where=lambda row: row.valid & (row.key == next_key)
    )
    free_index, free_valid = entries.first(where=lambda row: not row.valid)
    duplicate_index, duplicate = entries.first(
        where=lambda row: row.valid & (row.key == incoming_key)
    )
    if available & free_valid:
        assert incoming_key >= next_key, "reorder_stale_key"
        assert not duplicate, "reorder_duplicate_key"
    admit = available & free_valid
    retire = match_valid & space
    retiring_item = entries[match_index].item
    if retire:
        entries[match_index].valid = 0
        next_key = next_key + 1
    if admit:
        entries[free_index].valid = 1
        entries[free_index].key = incoming_key
        entries[free_index].item = head
    return ReorderMove(take=admit, push=retire, item=retiring_item)


@module
def reorder(available: u1, head: WorkItem, space: u1) -> ReorderMove:
    entries = table[64, ReorderEntry](init=0)
    next_key: u64 = 0
    return advance(entries, next_key, available, head, space)
