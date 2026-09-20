"""Stable Queue frontend facade and source-lowering orchestration."""

# ruff: noqa: F401

from __future__ import annotations

import ast
import copy
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
from ._queue_compiler.provenance import (
    DefinitionNdfMetadata,
    build_queue_acpy,
    extract_definition_ndf_metadata,
)
from ._queue_compiler.source import (
    _DEFAULT_QUEUE_SOURCE_PATH,
    _normalize_queue_source_path,
    _render_callsite_location,
    _render_fused_source_locations,
    _render_source_frame_location,
)
from ._queue_compiler.source_unit import (
    case_literal_bindings,
    first_family_case,
    specialize_annotation,
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
    _integer_width,
    _is_bool_like,
    _module_static_values,
    _nonnegative_int_value,
    _payload,
    _payloads,
    _positive_int_value,
    _primitive_integer_width,
    _product,
    _project_static_config_value,
    _proven_integer_in,
    _resolved_config_values_for_checks,
    _resolved_type_bindings_for_checks,
    _scalar_annotation_static_check,
    _scalar_reset_init,
    _scalar_type_descriptor,
    _static_config_expression_type,
    _static_constraint,
    _static_int_value,
    _static_parameter_aliases,
    _static_parameter_value,
    _table_axis_width,
    _type_static_values,
    _types_compatible,
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
    *,
    host_results: bool = False,
    source_path: str | None = None,
    definition_locations: Mapping[str, tuple[str, int, int]] | None = None,
    static_assert_locations: (
        Mapping[str, tuple[tuple[str, int, int], ...]] | None
    ) = None,
    source_node_locations: SourceNodeLocations | None = None,
    definition_ndf: DefinitionNdfMetadata | None = None,
) -> str:
    definition_ndf = definition_ndf or extract_definition_ndf_metadata(text)
    if lowered := _lower_simple_module_source(
        text,
        system,
        static_arguments=static_arguments,
        host_results=host_results,
        source_path=source_path,
        definition_locations=definition_locations,
        static_assert_locations=static_assert_locations,
        source_node_locations=source_node_locations,
        definition_ndf=definition_ndf,
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
            source_path=source_path,
            definition_locations=definition_locations,
            static_assert_locations=static_assert_locations,
            source_node_locations=source_node_locations,
        ),
        definition_locations=dict(definition_locations or {}),
        definition_ndf=definition_ndf,
    )


def lower_module_source(
    text: str,
    module: str,
    static_arguments: Mapping[str, StaticValue] | None = None,
    *,
    source_path: str | None = None,
    definition_locations: Mapping[str, tuple[str, int, int]] | None = None,
    static_assert_locations: (
        Mapping[str, tuple[tuple[str, int, int], ...]] | None
    ) = None,
    source_node_locations: SourceNodeLocations | None = None,
    definition_ndf: DefinitionNdfMetadata | None = None,
) -> str:
    """Lower one module through a synthetic typed root used only for linking."""

    tree = ast.parse(text, filename=source_path or _DEFAULT_QUEUE_SOURCE_PATH)
    candidates = [
        node
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and node.name == module
        and any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == "module"
            for decorator in node.decorator_list
        )
    ]
    if len(candidates) != 1:
        raise QueueFrontendError(
            f"ACPY-MODULE-001: module {module!r} is missing or ambiguous"
        )
    target = candidates[0]
    wrapper = "_acc_module_root"
    occupied = {
        node.name
        for node in tree.body
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef))
    }
    while wrapper in occupied:
        wrapper += "_"
    runtime_parameters = (*target.args.posonlyargs, *target.args.args)
    call = ast.Call(
        func=ast.Name(id=module, ctx=ast.Load()),
        args=[ast.Name(id=item.arg, ctx=ast.Load()) for item in runtime_parameters],
        keywords=[
            ast.keyword(
                arg=item.arg,
                value=ast.Name(id=item.arg, ctx=ast.Load()),
            )
            for item in target.args.kwonlyargs
        ],
    )
    outputs = (
        ()
        if target.returns is None
        or isinstance(target.returns, ast.Constant)
        and target.returns.value is None
        else (target.returns,)
    )
    wrapper_body: list[ast.stmt] = (
        [ast.Return(value=call)]
        if outputs
        else [ast.Expr(value=call), ast.Return(value=ast.Constant(value=None))]
    )
    wrapper_node = ast.FunctionDef(
        name=wrapper,
        args=copy.deepcopy(target.args),
        body=wrapper_body,
        decorator_list=[
            ast.Attribute(
                value=ast.Name(id="ac", ctx=ast.Load()),
                attr="system",
                ctx=ast.Load(),
            )
        ],
        returns=copy.deepcopy(target.returns),
        type_comment=None,
    )
    tree.body.append(ast.fix_missing_locations(wrapper_node))
    return lower_queue_source(
        ast.unparse(ast.fix_missing_locations(tree)),
        wrapper,
        static_arguments=static_arguments,
        host_results=True,
        source_path=source_path,
        definition_locations=definition_locations,
        static_assert_locations=static_assert_locations,
        source_node_locations=source_node_locations,
        definition_ndf=definition_ndf,
    )


def lower_source_unit(
    text: str,
    module_requests: tuple[
        tuple[str, tuple[tuple[str, StaticValue], ...]], ...
    ],
    *,
    source_path: str | None = None,
    definition_locations: Mapping[str, tuple[str, int, int]] | None = None,
    static_assert_locations: (
        Mapping[str, tuple[tuple[str, int, int], ...]] | None
    ) = None,
    source_node_locations: SourceNodeLocations | None = None,
    definition_ndf: DefinitionNdfMetadata | None = None,
) -> str:
    """Lower all requested module_requests owned by one Python source unit."""

    if not module_requests:
        raise QueueFrontendError("ACPY-MODULE-001: source unit is empty")
    tree = ast.parse(text, filename=source_path or _DEFAULT_QUEUE_SOURCE_PATH)
    modules = {
        node.name: node
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == "module"
            for decorator in node.decorator_list
        )
    }
    wrapper = "_acc_source_unit_root"
    occupied = {
        node.name
        for node in tree.body
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef))
    }
    while wrapper in occupied:
        wrapper += "_"

    wrapper_arguments: list[ast.arg] = []
    wrapper_keywords: list[ast.arg] = []
    wrapper_keyword_defaults: list[ast.expr | None] = []
    wrapper_static: dict[str, StaticValue] = {}
    body: list[ast.stmt] = []
    returned_values: list[ast.expr] = []
    returned_types: list[ast.expr] = []

    def is_static(annotation: ast.expr | None) -> bool:
        return (
            isinstance(annotation, ast.Subscript)
            and _decorator_name(annotation.value).rsplit(".", 1)[-1]
            in {"const", "Static"}
        )

    def output_types(annotation: ast.expr | None) -> tuple[ast.expr, ...]:
        if annotation is None or (
            isinstance(annotation, ast.Constant) and annotation.value is None
        ):
            return ()
        if (
            isinstance(annotation, ast.Subscript)
            and _decorator_name(annotation.value).rsplit(".", 1)[-1] == "tuple"
        ):
            return (
                tuple(annotation.slice.elts)
                if isinstance(annotation.slice, ast.Tuple)
                else (annotation.slice,)
            )
        return (annotation,)

    static_profiles: dict[str, list[tuple[StaticValue, str]]] = {}
    for module_name, frozen in module_requests:
        target = modules.get(module_name)
        if target is None:
            continue
        bindings = dict(frozen)
        for argument in target.args.kwonlyargs:
            if is_static(argument.annotation) and argument.arg in bindings:
                static_profiles.setdefault(argument.arg, []).append(
                    (
                        bindings[argument.arg],
                        ast.dump(argument.annotation, include_attributes=False),
                    )
                )
    shared_static_names = {
        name
        for name, profiles in static_profiles.items()
        if all(profile == profiles[0] for profile in profiles[1:])
    }
    declared_wrapper_keywords: set[str] = set()

    declarations = {
        node.name: node
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == "module_decl"
            for decorator in node.decorator_list
        )
    }

    for index, (module_name, frozen) in enumerate(module_requests):
        target = modules.get(module_name)
        if target is None:
            raise QueueFrontendError(
                f"ACPY-MODULE-001: source-unit module {module_name!r} is missing"
            )
        if target.args.vararg is not None or target.args.kwarg is not None:
            raise QueueFrontendError(
                "ACPY-MODULE-001: source-unit modules cannot use variadic arguments"
            )
        bindings = dict(frozen)
        family_case = (
            first_family_case(declarations, module_name) if not bindings else None
        )
        family_bindings = case_literal_bindings(family_case)
        call_arguments: list[ast.expr] = []
        call_keywords: list[ast.keyword] = []
        for argument in (*target.args.posonlyargs, *target.args.args):
            name = f"unit_{index}_{argument.arg}"
            wrapper_arguments.append(
                ast.arg(
                    arg=name,
                    annotation=specialize_annotation(
                        argument.annotation, family_bindings
                    ),
                )
            )
            call_arguments.append(ast.Name(id=name, ctx=ast.Load()))
        expected_static: set[str] = set()
        for argument in target.args.kwonlyargs:
            if is_static(argument.annotation):
                expected_static.add(argument.arg)
                if argument.arg not in bindings:
                    raise QueueFrontendError(
                        "ACPY-MODULE-001: source-unit family request for "
                        f"{module_name!r} is missing {argument.arg!r}"
                    )
                name = (
                    argument.arg
                    if argument.arg in shared_static_names
                    else f"unit_{index}_{argument.arg}"
                )
                wrapper_static[name] = bindings[argument.arg]
            else:
                name = f"unit_{index}_{argument.arg}"
            if name not in declared_wrapper_keywords:
                wrapper_keywords.append(
                    ast.arg(
                        arg=name,
                        annotation=copy.deepcopy(argument.annotation),
                    )
                )
                wrapper_keyword_defaults.append(None)
                declared_wrapper_keywords.add(name)
            call_keywords.append(
                ast.keyword(
                    arg=argument.arg,
                    value=ast.Name(id=name, ctx=ast.Load()),
                )
            )
        unknown = sorted(set(bindings) - expected_static)
        if unknown:
            raise QueueFrontendError(
                "ACPY-MODULE-001: source-unit family request for "
                f"{module_name!r} has unknown static argument {unknown[0]!r}"
            )
        call = ast.Call(
            func=ast.Name(id=module_name, ctx=ast.Load()),
            args=call_arguments,
            keywords=[
                *call_keywords,
                *(
                    [ast.keyword(arg="static", value=family_case)]
                    if family_case is not None
                    else []
                ),
            ],
        )
        outputs = output_types(target.returns)
        if not outputs:
            body.append(ast.Expr(value=call))
            continue
        names = [f"unit_{index}_result_{item}" for item in range(len(outputs))]
        assignment_target: ast.expr = (
            ast.Name(id=names[0], ctx=ast.Store())
            if len(names) == 1
            else ast.Tuple(
                elts=[ast.Name(id=name, ctx=ast.Store()) for name in names],
                ctx=ast.Store(),
            )
        )
        body.append(ast.Assign(targets=[assignment_target], value=call))
        returned_values.extend(ast.Name(id=name, ctx=ast.Load()) for name in names)
        returned_types.extend(
            specialize_annotation(item, family_bindings) for item in outputs
        )

    if not returned_values:
        returned = ast.Constant(value=None)
        return_annotation = ast.Constant(value=None)
    elif len(returned_values) == 1:
        returned: ast.expr = returned_values[0]
        return_annotation: ast.expr = returned_types[0]
    else:
        returned = ast.Tuple(elts=returned_values, ctx=ast.Load())
        return_annotation = ast.Subscript(
            value=ast.Name(id="tuple", ctx=ast.Load()),
            slice=ast.Tuple(elts=returned_types, ctx=ast.Load()),
            ctx=ast.Load(),
        )
    body.append(ast.Return(value=returned))
    wrapper_node = ast.FunctionDef(
        name=wrapper,
        args=ast.arguments(
            posonlyargs=[],
            args=wrapper_arguments,
            kwonlyargs=wrapper_keywords,
            kw_defaults=wrapper_keyword_defaults,
            defaults=[],
            vararg=None,
            kwarg=None,
        ),
        body=body,
        decorator_list=[
            ast.Attribute(
                value=ast.Name(id="ac", ctx=ast.Load()),
                attr="system",
                ctx=ast.Load(),
            )
        ],
        returns=return_annotation,
        type_comment=None,
    )
    tree.body.append(ast.fix_missing_locations(wrapper_node))
    return lower_queue_source(
        ast.unparse(ast.fix_missing_locations(tree)),
        wrapper,
        static_arguments=wrapper_static,
        host_results=True,
        source_path=source_path,
        definition_locations=definition_locations,
        static_assert_locations=static_assert_locations,
        source_node_locations=source_node_locations,
        definition_ndf=definition_ndf,
    )
