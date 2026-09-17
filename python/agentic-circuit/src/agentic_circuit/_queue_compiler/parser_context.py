"""Shared context and mutable state for Queue statement handlers."""

from __future__ import annotations

import ast
from collections.abc import Mapping
from dataclasses import dataclass, field
from enum import Enum, auto
from typing import Any

from _pycircuit_semantics import ValueType

from .._source_map import SourceFrame
from .._static_eval import StaticValue
from .model import (
    BarrierBinding,
    CandidateSetBinding,
    CollectionBinding,
    CreditBinding,
    DependencyBinding,
    EntryViewBinding,
    ExpectBinding,
    FeedbackBinding,
    ForkBinding,
    MaskedEntryViewBinding,
    MaskedTableWriteBinding,
    MemoryBinding,
    MemoryInstanceBinding,
    MemoryRequestBinding,
    MergeBinding,
    ObservationBinding,
    Payload,
    ProjectedTableViewBinding,
    QueueBinding,
    ReorderBinding,
    RouteBinding,
    RuleDefinition,
    ScopeBinding,
    SelectBinding,
    SelectedMemoryBinding,
    SelectionBinding,
    SinkBinding,
    SlotBinding,
    SlotReleaseBinding,
    StaticMemoryArrayBinding,
    StaticQueueCollection,
    TableBinding,
    TableReadBinding,
    TableWriteBinding,
    VarStateBinding,
)


class _StatementResult(Enum):
    """Result of one ordered statement-handler attempt."""

    HANDLED = auto()
    UNHANDLED = auto()


HANDLED = _StatementResult.HANDLED
UNHANDLED = _StatementResult.UNHANDLED


@dataclass(frozen=True, slots=True)
class _ParserEnvironment:
    """Read-only inputs shared by Queue statement handlers."""

    static_values: Mapping[str, StaticValue]
    payloads: tuple[Payload, ...]
    result_payloads: tuple[ValueType, ...] | None
    rule_definitions: Mapping[str, RuleDefinition]
    payload_map: Mapping[str, Payload] = field(default_factory=dict)
    enum_map: Mapping[str, ValueType] = field(default_factory=dict)
    type_static_values: Mapping[str, StaticValue] = field(default_factory=dict)
    entry_kind: str = "system"
    syntax_tree: ast.Module | None = None
    helper_map: Mapping[str, Any] = field(default_factory=dict)


@dataclass(slots=True)
class _ParserState:
    """Mutable bindings and source order owned by one parser invocation."""

    queues: list[QueueBinding] = field(default_factory=list)
    effect_rules: list[QueueBinding] = field(default_factory=list)
    scopes: list[ScopeBinding] = field(default_factory=list)
    routes: list[RouteBinding] = field(default_factory=list)
    forks: list[ForkBinding] = field(default_factory=list)
    feedbacks: list[FeedbackBinding] = field(default_factory=list)
    merges: list[MergeBinding] = field(default_factory=list)
    reorders: list[ReorderBinding] = field(default_factory=list)
    dependencies: list[DependencyBinding] = field(default_factory=list)
    credits: list[CreditBinding] = field(default_factory=list)
    barriers: list[BarrierBinding] = field(default_factory=list)
    selects: list[SelectBinding] = field(default_factory=list)
    memory_instances: list[MemoryInstanceBinding] = field(default_factory=list)
    memory_requests: list[MemoryRequestBinding] = field(default_factory=list)
    memories: list[MemoryBinding] = field(default_factory=list)
    variables: list[VarStateBinding] = field(default_factory=list)
    tables: list[TableBinding] = field(default_factory=list)
    table_reads: list[TableReadBinding] = field(default_factory=list)
    table_writes: list[TableWriteBinding] = field(default_factory=list)
    masked_table_writes: list[MaskedTableWriteBinding] = field(default_factory=list)
    slots: list[SlotBinding] = field(default_factory=list)
    slot_releases: list[SlotReleaseBinding] = field(default_factory=list)
    candidates: list[CandidateSetBinding] = field(default_factory=list)
    selections: list[SelectionBinding] = field(default_factory=list)
    sinks: list[SinkBinding] = field(default_factory=list)
    observations: list[ObservationBinding] = field(default_factory=list)
    expectations: list[ExpectBinding] = field(default_factory=list)
    collections: dict[str, StaticQueueCollection] = field(default_factory=dict)
    collection_bindings: list[CollectionBinding] = field(default_factory=list)
    by_name: dict[str, QueueBinding] = field(default_factory=dict)
    table_by_name: dict[str, TableBinding] = field(default_factory=dict)
    variable_by_name: dict[str, VarStateBinding] = field(default_factory=dict)
    entry_views: dict[
        str,
        EntryViewBinding | MaskedEntryViewBinding | ProjectedTableViewBinding,
    ] = field(default_factory=dict)
    slot_by_name: dict[str, SlotBinding] = field(default_factory=dict)
    candidate_by_name: dict[str, CandidateSetBinding] = field(default_factory=dict)
    selection_by_name: dict[str, SelectionBinding] = field(default_factory=dict)
    selection_tuple_aliases: dict[str, tuple[str, ...]] = field(default_factory=dict)
    selection_lane_ordinals: dict[str, int] = field(default_factory=dict)
    memory_by_name: dict[str, MemoryInstanceBinding] = field(default_factory=dict)
    memory_arrays: dict[str, StaticMemoryArrayBinding] = field(default_factory=dict)
    selected_memories: dict[str, SelectedMemoryBinding] = field(default_factory=dict)
    consumed_selected_memories: set[str] = field(default_factory=set)
    arbitration_descriptors: dict[str, int] = field(default_factory=dict)
    arbitration_owners: dict[str, str] = field(default_factory=dict)
    statement_sources: dict[int, SourceFrame] = field(default_factory=dict)
    order: int = 0


_Aliases = dict[str, str | StaticQueueCollection]


@dataclass(slots=True)
class _StatementContext:
    """Per-statement data shared by ordered handlers.

    ``aliases`` is intentionally mutable: TableChoice tuple unpacking updates
    the lexical aliases consumed by later handlers in the same visit.
    """

    statement: ast.stmt
    scope: tuple[str, ...]
    aliases: _Aliases
    current_order: int
