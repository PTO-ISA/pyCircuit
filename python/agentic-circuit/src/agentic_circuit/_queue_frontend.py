"""Stable Queue frontend facade and source-lowering orchestration."""

# ruff: noqa: F401

from __future__ import annotations

from collections.abc import Mapping

from ._queue_compiler.acir_text import (
    _abi_layout,
    _align,
    _enum_layout_entry,
    _payload_layout_entry,
    _render_bitfield,
    _render_dense_i64,
    _render_enum,
    _render_interface_display_attributes,
    _render_queue_type,
    _render_static_mlir_dictionary,
    _render_static_mlir_value,
    _render_static_type_attributes,
    _render_table_domain_attributes,
    _render_table_init_value,
    _table_schema_id,
)
from ._queue_compiler.definitions import (
    _extract_conditional_effect_guard,
    _helper_type,
    _invariant_definitions,
    _is_none_return,
    _lambda_value,
    _pure_helper_definitions,
)
from ._queue_compiler.errors import QueueFrontendError
from ._queue_compiler.expressions import (
    _ExpressionEmitter,
    _ExpressionFact,
    _resolve_invariant_call,
)
from ._queue_compiler.lower_acir import _ModuleRenderSpec, lower_queue_program
from ._queue_compiler.model import (
    BarrierBinding,
    BitfieldBinding,
    CandidateSetBinding,
    CollectionBinding,
    CreditBinding,
    DependencyBinding,
    EntryViewBinding,
    EnumBinding,
    ExpectBinding,
    FeedbackBinding,
    ForkBinding,
    InvariantDefinition,
    MaskedEntryViewBinding,
    MaskedTableWriteBinding,
    MemoryBinding,
    MemoryInstanceBinding,
    MemoryRequestBinding,
    MergeBinding,
    ObservationBinding,
    Payload,
    ProjectedTableViewBinding,
    PureHelperDefinition,
    QueueBinding,
    QueueProgram,
    RecursiveQueueHelper,
    ReorderBinding,
    RouteBinding,
    RuleDefinition,
    RuleFindBinding,
    RuleFindDefinition,
    RuleLocalBinding,
    RuleLocalDefinition,
    RuleSlotOwnerBinding,
    RuleSlotReleaseBinding,
    RuleSlotReleaseDefinition,
    RuleStateOwnerBinding,
    RuleStateReadBinding,
    RuleStateReadDefinition,
    RuleStateWriteBinding,
    RuleStateWriteDefinition,
    ScopeBinding,
    SelectBinding,
    SelectedMemoryBinding,
    SelectionBinding,
    SinkBinding,
    SlotBinding,
    SlotReleaseBinding,
    StaticConfigBinding,
    StaticMemoryArrayBinding,
    StaticQueueCollection,
    StaticTypeCheck,
    TableBinding,
    TableReadBinding,
    TableWriteBinding,
    VarStateBinding,
)
from ._queue_compiler.modules import _lower_simple_module_source
from ._queue_compiler.normalize import (
    _constantize_expression,
    _desugar_nested_rule_captures,
    _normalize_rule_field_assignments,
    _strip_static_assertions,
)
from ._queue_compiler.parser import RULE_LOWERING_PIPELINE, parse_queue_program
from ._queue_compiler.provenance import build_queue_acpy
from ._queue_compiler.source import (
    _DEFAULT_QUEUE_SOURCE_PATH,
    _normalize_queue_source_path,
    _render_callsite_location,
    _render_fused_source_locations,
    _render_source_frame_location,
)
from ._queue_compiler.static_types import (
    MAX_PACKED_VALUE_WIDTH,
    StaticParameterAlias,
    _bitfields,
    _bounded_annotation_static_checks,
    _candidate_mask_type,
    _config_schema_document,
    _config_type_names,
    _constant_integer,
    _contains_declared_range,
    _dependent_static_type_expression,
    _enums,
    _epoch_05_integer_width,
    _is_epoch_05_bool_compatible,
    _module_static_values,
    _nonnegative_int_value,
    _payload,
    _payloads,
    _positive_int_value,
    _primitive_integer_width,
    _product,
    _project_static_config_value,
    _proven_integer_in,
    _scalar_annotation_static_check,
    _scalar_reset_init,
    _scalar_type_descriptor,
    _static_config_bindings_for_checks,
    _static_config_expression_type,
    _static_constraint,
    _static_int_value,
    _static_parameter_aliases,
    _static_parameter_value,
    _static_type_bindings_for_checks,
    _table_axis_width,
    _type_static_values,
    _types_equal_in_epoch_05,
    _validate_static_config_roots,
)
from ._queue_compiler.syntax import _decorator_name
from ._queue_compiler.type_rendering import _render_type
from ._source_map import SourceNodeLocations
from ._static_eval import StaticValue


def lower_queue_source(
    text: str,
    system: str,
    static_arguments: Mapping[str, StaticValue] | None = None,
    specialization_fingerprint: str | None = None,
    *,
    host_results: bool = False,
    source_path: str | None = None,
    definition_locations: Mapping[str, tuple[str, int, int]] | None = None,
    static_assert_locations: (
        Mapping[str, tuple[tuple[str, int, int], ...]] | None
    ) = None,
    source_node_locations: SourceNodeLocations | None = None,
) -> str:
    if lowered := _lower_simple_module_source(
        text,
        system,
        static_arguments=static_arguments,
        specialization_fingerprint=specialization_fingerprint,
        host_results=host_results,
        source_path=source_path,
        definition_locations=definition_locations,
        static_assert_locations=static_assert_locations,
        source_node_locations=source_node_locations,
    ):
        return lowered
    if host_results:
        raise QueueFrontendError(
            "ACPY-MODULE-006: host result boundaries require structured modules"
        )
    return lower_queue_program(
        parse_queue_program(
            text,
            system,
            static_arguments=static_arguments,
            specialization_fingerprint=specialization_fingerprint,
            source_path=source_path,
            definition_locations=definition_locations,
            static_assert_locations=static_assert_locations,
            source_node_locations=source_node_locations,
        )
    )
