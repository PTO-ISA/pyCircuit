"""Immutable data model for the Queue frontend."""

from __future__ import annotations

import ast
from dataclasses import dataclass

from _pycircuit_semantics import BitfieldLayout, StructType, ValueType

from .._diagnostics import Diagnostic
from .._source_map import SourceFrame
from .._static_eval import StaticValue
from .source import _DEFAULT_QUEUE_SOURCE_PATH
from .type_rendering import _render_type


@dataclass(frozen=True, slots=True)
class StaticTypeCheck:
    target: str
    program: tuple[str, ...]
    result: int
    concrete_type: ValueType | None = None


@dataclass(frozen=True, slots=True)
class StaticConfigBinding:
    root: str
    type_name: str
    schema: str
    value: str


@dataclass(frozen=True, slots=True)
class Payload:
    descriptor: StructType
    resolved_type_checks: tuple[StaticTypeCheck, ...] = ()

    @property
    def name(self) -> str:
        return self.descriptor.name

    @property
    def field_descriptors(self) -> tuple[tuple[str, ValueType], ...]:
        return tuple((field.name, field.type) for field in self.descriptor.fields)

    @property
    def fields(self) -> tuple[tuple[str, str], ...]:
        return tuple(
            (name, _render_type(descriptor))
            for name, descriptor in self.field_descriptors
        )

    @property
    def acir_type(self) -> str:
        return _render_type(self.descriptor)


@dataclass(frozen=True, slots=True)
class BitfieldBinding:
    name: str
    layout: BitfieldLayout


@dataclass(frozen=True, slots=True)
class EnumBinding:
    name: str
    descriptor: ValueType


@dataclass(frozen=True, slots=True)
class RuleStateWriteDefinition:
    argument: str
    index: ast.expr | None
    value: ast.expr
    guard: ast.expr | None = None
    guard_negated: bool = False


@dataclass(frozen=True, slots=True)
class RuleStateReadDefinition:
    name: str
    argument: str
    index: ast.expr | None


@dataclass(frozen=True, slots=True)
class RuleLocalDefinition:
    name: str
    value: ast.expr
    guard: ast.expr | None = None
    guard_negated: bool = False
    prior_name: str | None = None


@dataclass(frozen=True, slots=True)
class RuleFindDefinition:
    name: str
    argument: str
    predicate_argument: str
    predicate: ast.expr
    key_argument: str | None
    key: ast.expr | None
    row: ast.expr | None = None


@dataclass(frozen=True, slots=True)
class RuleStateWriteBinding:
    variable: str
    argument: str
    value_type: ValueType
    entries: int
    index: ast.expr | None
    value: ast.expr
    guard: ast.expr | None = None
    guard_negated: bool = False
    owner_kind: str = "var"
    shape: tuple[int, ...] = ()
    mode: str = "replace"
    write_fields: tuple[str, ...] = ()


@dataclass(frozen=True, slots=True)
class RuleStateReadBinding:
    name: str
    variable: str
    argument: str
    value_type: ValueType
    entries: int
    index: ast.expr | None
    owner_kind: str = "var"
    shape: tuple[int, ...] = ()


@dataclass(frozen=True, slots=True)
class RuleLocalBinding:
    name: str
    value: ast.expr
    guard: ast.expr | None = None
    guard_negated: bool = False
    prior_name: str | None = None


@dataclass(frozen=True, slots=True)
class RuleFindBinding:
    name: str
    variable: str
    argument: str
    value_type: ValueType
    entries: int
    predicate_argument: str
    predicate: ast.expr
    key_argument: str | None
    key: ast.expr | None
    shape: tuple[int, ...] = ()
    row: ast.expr | None = None
    owner_kind: str = "var"


@dataclass(frozen=True, slots=True)
class RuleStateOwnerBinding:
    variable: str
    argument: str
    value_type: ValueType
    entries: int
    owner_kind: str = "var"
    shape: tuple[int, ...] = ()


@dataclass(frozen=True, slots=True)
class RuleSlotReleaseDefinition:
    argument: str
    guard: ast.expr | None = None
    guard_negated: bool = False


@dataclass(frozen=True, slots=True)
class RuleSlotOwnerBinding:
    slot: str
    argument: str
    payload: ValueType


@dataclass(frozen=True, slots=True)
class RuleSlotReleaseBinding:
    slot: str
    argument: str
    guard: ast.expr | None = None
    guard_negated: bool = False


@dataclass(frozen=True, slots=True)
class QueueBinding:
    name: str
    payload: ValueType
    depth: int
    latency: int
    input_name: str | None
    argument: str | None = None
    expression: ast.expr | None = None
    scope: tuple[str, ...] = ()
    order: int = 0
    route_output: bool = False
    feedback_output: bool = False
    merge_output: bool = False
    reorder_output: bool = False
    dependency_output: bool = False
    credit_output: bool = False
    memory_output: bool = False
    table_read_output: bool = False
    barrier_output: bool = False
    select_output: bool = False
    instance_output: bool = False
    provider: str = "transform"
    rate: int = 1
    lanes: int = 1
    rule_name: str | None = None
    rule_source_line: int | None = None
    rule_source_column: int | None = None
    rule_source_path: str | None = None
    rule_table: str | None = None
    rule_table_index: ast.expr | None = None
    rule_table_value: ast.expr | None = None
    rule_write_mode: str = "replace"
    rule_write_fields: tuple[str, ...] = ()
    rule_table_read_name: str | None = None
    rule_table_read_index: ast.expr | None = None
    rule_input_names: tuple[str, ...] = ()
    rule_arguments: tuple[str, ...] = ()
    rule_payloads: tuple[ValueType, ...] = ()
    rule_var: str | None = None
    rule_var_argument: str | None = None
    rule_var_value: ast.expr | None = None
    rule_var_index: ast.expr | None = None
    rule_var_read_name: str | None = None
    rule_var_read_index: ast.expr | None = None
    rule_has_output: bool = True
    rule_guard: ast.expr | None = None
    rule_effect_guard: ast.expr | None = None
    rule_output_guard: ast.expr | None = None
    rule_state_writes: tuple[RuleStateWriteBinding, ...] = ()
    rule_state_reads: tuple[RuleStateReadBinding, ...] = ()
    rule_locals: tuple[RuleLocalBinding, ...] = ()
    rule_finds: tuple[RuleFindBinding, ...] = ()
    rule_state_owners: tuple[RuleStateOwnerBinding, ...] = ()
    rule_slot_owners: tuple[RuleSlotOwnerBinding, ...] = ()
    rule_slot_releases: tuple[RuleSlotReleaseBinding, ...] = ()
    rule_output_names: tuple[str, ...] = ()
    rule_output_payloads: tuple[ValueType, ...] = ()
    rule_output_expressions: tuple[ast.expr, ...] = ()
    rule_output_guards: tuple[ast.expr, ...] = ()
    source: SourceFrame | None = None


@dataclass(frozen=True, slots=True)
class ScopeBinding:
    name: str
    path: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class SinkBinding:
    queue: str
    scope: tuple[str, ...]
    order: int
    source: SourceFrame | None = None


@dataclass(frozen=True, slots=True)
class ObservationBinding:
    queue: str
    name: str
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class ExpectBinding:
    queue: str
    argument: str
    predicate: ast.expr
    message: str
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class RouteBinding:
    input_name: str
    outputs: tuple[str, ...]
    argument: str
    selector: ast.expr
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int
    boolean_selector: bool = False


@dataclass(frozen=True, slots=True)
class ForkBinding:
    input_name: str
    outputs: tuple[str, ...]
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class FeedbackBinding:
    input_name: str
    output_name: str
    argument: str
    condition: ast.expr
    update: ast.expr
    depth: int
    latency: int
    max_iterations: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class MergeBinding:
    inputs: tuple[str, ...]
    output: str
    policy: str
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class ReorderBinding:
    input_name: str
    output_name: str
    argument: str
    key: ast.expr
    capacity: int
    start: int
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class DependencyBinding:
    input_name: str
    output_name: str
    argument: str
    key: ast.expr
    waits_for: ast.expr
    resource: ast.expr
    cost: ast.expr
    capacity: int
    resources: int
    no_dependency: int
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int
    provider: str = "dependency"


@dataclass(frozen=True, slots=True)
class CreditBinding:
    input_name: str
    output_name: str
    argument: str
    cost: ast.expr
    credits: int
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int
    provider: str = "credit"


@dataclass(frozen=True, slots=True)
class BarrierBinding:
    inputs: tuple[str, ...]
    outputs: tuple[str, ...]
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class SelectBinding:
    control: str
    inputs: tuple[str, ...]
    output: str
    argument: str
    selector: ast.expr
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class MemoryInstanceBinding:
    name: str
    data_type: ValueType
    entries: int
    init: int
    latency: int
    scope: tuple[str, ...]
    order: int
    source: SourceFrame | None = None


@dataclass(frozen=True, slots=True)
class MemoryRequestBinding:
    instance: str
    input_name: str
    output_name: str
    argument: str
    address: ast.expr
    write: ast.expr
    data: ast.expr
    result_field: str
    depth: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class MemoryBinding:
    input_name: str
    output_name: str
    argument: str
    address: ast.expr
    write: ast.expr
    data: ast.expr
    data_type: ValueType
    entries: int
    init: int
    result_field: str
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class TableBinding:
    name: str
    entry_type: ValueType
    entries: int
    shape: tuple[int, ...]
    init_image: tuple[object, ...] | None
    scope: tuple[str, ...]
    order: int
    source: SourceFrame | None = None


@dataclass(frozen=True, slots=True)
class VarStateBinding:
    name: str
    value_type: ValueType
    init: int | bool
    scope: tuple[str, ...]
    order: int
    entries: int = 1
    shape: tuple[int, ...] = ()
    source: SourceFrame | None = None


@dataclass(frozen=True, slots=True)
class EntryViewBinding:
    name: str
    table: str
    argument: str | None
    address: ast.expr
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class MaskedEntryViewBinding:
    name: str
    table: str
    candidates: str
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class ProjectedTableViewBinding:
    name: str
    table: str
    prefix: tuple[ast.expr, ...]
    domain_axes: tuple[int, ...]
    domain_shape: tuple[int, ...]
    domain_strides: tuple[int, ...]
    domain_offset: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class TableReadBinding:
    table: str
    input_name: str | None
    output_name: str
    argument: str | None
    address: ast.expr
    when: ast.expr
    view_alias: str | None
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class TableWriteBinding:
    table: str
    input_name: str | None
    argument: str | None
    address: ast.expr
    enable: ast.expr
    value: ast.expr | None
    patch_fields: tuple[tuple[str, ast.expr], ...]
    write_fields: tuple[str, ...]
    write_mode: str
    arbitration_rank: int | None
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class MaskedTableWriteBinding:
    table: str
    candidates: str
    enable: ast.expr
    value: ast.expr | None
    patch_fields: tuple[tuple[str, ast.expr], ...]
    write_fields: tuple[str, ...]
    write_mode: str
    arbitration_rank: int | None
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class SlotBinding:
    name: str
    input_name: str
    payload: ValueType
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class SlotReleaseBinding:
    slot: str
    when: ast.expr
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class CandidateSetBinding:
    name: str
    table: str
    entries: int
    domain_axes: tuple[int, ...]
    domain_shape: tuple[int, ...]
    domain_strides: tuple[int, ...]
    domain_offset: int
    argument: str
    predicate: ast.expr
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class SelectionBinding:
    name: str
    aliases: tuple[str, ...]
    table: str
    candidates: str
    count: int
    policy: str
    key_ordering: str | None
    stable_id: str
    initial_cursor: int
    argument: str | None
    key: ast.expr | None
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class StaticMemoryArrayBinding:
    name: str
    members: tuple[str, ...]
    data_type: ValueType
    entries: int
    init: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class SelectedMemoryBinding:
    name: str
    array: str
    input_name: str
    routed_inputs: tuple[str, ...]
    argument: str
    selector: ast.expr
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int
    provider: str = "memory"


@dataclass(frozen=True, slots=True)
class StaticQueueCollection:
    kind: str
    members: tuple[tuple[str | int | bool, str | StaticQueueCollection], ...]


@dataclass(frozen=True, slots=True)
class RecursiveQueueHelper:
    queue_parameter: str
    count_parameter: str
    argument: str
    expression: ast.expr
    apply_call: ast.Call


@dataclass(frozen=True, slots=True)
class RuleDefinition:
    name: str
    arguments: tuple[str, ...]
    expression: ast.expr | None
    source_line: int
    source_column: int
    table_argument: str | None = None
    table_index: ast.expr | None = None
    table_value: ast.expr | None = None
    table_read_name: str | None = None
    table_read_index: ast.expr | None = None
    var_argument: str | None = None
    var_value: ast.expr | None = None
    guard: ast.expr | None = None
    effect_guard: ast.expr | None = None
    output_guard: ast.expr | None = None
    state_arguments: tuple[str, ...] = ()
    slot_arguments: tuple[str, ...] = ()
    slot_releases: tuple[RuleSlotReleaseDefinition, ...] = ()
    state_writes: tuple[RuleStateWriteDefinition, ...] = ()
    state_reads: tuple[RuleStateReadDefinition, ...] = ()
    locals: tuple[RuleLocalDefinition, ...] = ()
    finds: tuple[RuleFindDefinition, ...] = ()
    output_types: tuple[ValueType, ...] = ()
    output_expressions: tuple[ast.expr, ...] = ()
    output_guards: tuple[ast.expr, ...] = ()
    source_path: str | None = None


@dataclass(frozen=True, slots=True)
class CollectionBinding:
    name: str
    value: StaticQueueCollection
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class InvariantDefinition:
    function_name: str
    qualified_name: str
    argument: str
    payload: StructType
    expression: ast.expr


@dataclass(frozen=True, slots=True)
class PureHelperDefinition:
    """One closed, typed, zero-delay helper retained in raw ACIR."""

    name: str
    arguments: tuple[tuple[str, ValueType], ...]
    result: ValueType
    expression: ast.expr
    inline: bool
    source: SourceFrame | None = None


@dataclass(frozen=True, slots=True)
class ChildInstanceBinding:
    """One child module instantiated inside a rule-backed module body.

    The child consumes parent-owned Queues named by ``input_names`` and
    produces parent-owned Queues named by ``output_names``. ``symbol`` is the
    resolved definition symbol, which may differ from ``module_name`` when the
    child is a parameterized family case. ``order`` is the parent body statement
    order that anchors the instance between two emitted segments.
    """

    name: str
    module_name: str
    symbol: str
    input_names: tuple[str, ...]
    output_names: tuple[str, ...]
    static_arguments: tuple[tuple[str, StaticValue], ...]
    order: int
    source: SourceFrame | None = None


@dataclass(frozen=True, slots=True)
class QueueProgram:
    system: str
    payloads: tuple[Payload, ...]
    enums: tuple[EnumBinding, ...]
    bitfields: tuple[BitfieldBinding, ...]
    invariants: tuple[InvariantDefinition, ...]
    helpers: tuple[PureHelperDefinition, ...]
    queues: tuple[QueueBinding, ...]
    effect_rules: tuple[QueueBinding, ...]
    scopes: tuple[ScopeBinding, ...]
    routes: tuple[RouteBinding, ...]
    forks: tuple[ForkBinding, ...]
    feedbacks: tuple[FeedbackBinding, ...]
    merges: tuple[MergeBinding, ...]
    reorders: tuple[ReorderBinding, ...]
    dependencies: tuple[DependencyBinding, ...]
    credits: tuple[CreditBinding, ...]
    barriers: tuple[BarrierBinding, ...]
    selects: tuple[SelectBinding, ...]
    memory_instances: tuple[MemoryInstanceBinding, ...]
    memory_requests: tuple[MemoryRequestBinding, ...]
    memories: tuple[MemoryBinding, ...]
    variables: tuple[VarStateBinding, ...]
    tables: tuple[TableBinding, ...]
    table_reads: tuple[TableReadBinding, ...]
    table_writes: tuple[TableWriteBinding, ...]
    masked_table_writes: tuple[MaskedTableWriteBinding, ...]
    slots: tuple[SlotBinding, ...]
    slot_releases: tuple[SlotReleaseBinding, ...]
    candidates: tuple[CandidateSetBinding, ...]
    selections: tuple[SelectionBinding, ...]
    collections: tuple[CollectionBinding, ...]
    observations: tuple[ObservationBinding, ...]
    expectations: tuple[ExpectBinding, ...]
    sinks: tuple[SinkBinding, ...]
    children: tuple[ChildInstanceBinding, ...] = ()
    resolved_type_bindings: tuple[tuple[str, int], ...] = ()
    resolved_type_checks: tuple[StaticTypeCheck, ...] = ()
    resolved_config_values: tuple[StaticConfigBinding, ...] = ()
    diagnostics: tuple[Diagnostic, ...] = ()
    source_path: str = _DEFAULT_QUEUE_SOURCE_PATH
    system_source: SourceFrame | None = None
    statement_sources: tuple[tuple[int, SourceFrame], ...] = ()
