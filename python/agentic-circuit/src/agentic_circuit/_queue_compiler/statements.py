"""Ordered statement handling for the Queue frontend parser."""

from __future__ import annotations

import ast
from collections.abc import Mapping
from dataclasses import dataclass, field
from enum import Enum, auto

from _pycircuit_semantics import ValueType

from .._source_map import SourceFrame
from .._static_eval import StaticValue
from .definitions import _lambda_value
from .errors import QueueFrontendError
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
from .normalize import _constantize_expression
from .static_types import (
    _nonnegative_int_value,
    _positive_int_value,
    _types_equal_in_epoch_05,
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


_Aliases = Mapping[str, str | StaticQueueCollection]


def _lambda(
    environment: _ParserEnvironment, node: ast.expr
) -> tuple[str, ast.expr]:
    argument, expression = _lambda_value(node)
    return argument, _constantize_expression(
        expression, argument, environment.static_values
    )


def _positive_int(
    environment: _ParserEnvironment, call: ast.Call, name: str, default: int
) -> int:
    return _positive_int_value(call, name, default, environment.static_values)


def _nonnegative_int(
    environment: _ParserEnvironment, call: ast.Call, name: str, default: int
) -> int:
    return _nonnegative_int_value(call, name, default, environment.static_values)


def _static_reference(
    state: _ParserState, node: ast.expr, aliases: _Aliases
) -> str | StaticQueueCollection:
    if isinstance(node, ast.Name):
        if node.id in aliases:
            return aliases[node.id]
        if node.id in state.by_name:
            return state.by_name[node.id].name
        if node.id in state.collections:
            return state.collections[node.id]
    if (
        isinstance(node, ast.Subscript)
        and isinstance(node.slice, ast.Constant)
        and type(node.slice.value) in {str, int, bool}
    ):
        collection = _static_reference(state, node.value, aliases)
        if not isinstance(collection, StaticQueueCollection):
            raise QueueFrontendError(
                "ACPY-QUEUE-005: static indexing requires a collection"
            )
        for key, value in collection.members:
            if type(key) is type(node.slice.value) and key == node.slice.value:
                return value
        raise QueueFrontendError(
            f"ACPY-QUEUE-005: collection has no key {node.slice.value!r}"
        )
    raise QueueFrontendError(
        "ACPY-QUEUE-005: collection reference must be statically resolvable"
    )


def _queue_reference(
    state: _ParserState, node: ast.expr, aliases: _Aliases
) -> str:
    value = _static_reference(state, node, aliases)
    if isinstance(value, str):
        return value
    raise QueueFrontendError(
        "ACPY-QUEUE-005: a collection cannot be used as one Queue"
    )


def _is_queue_reference_syntax(
    state: _ParserState, node: ast.expr, aliases: _Aliases
) -> bool:
    return isinstance(node, ast.Subscript) or (
        isinstance(node, ast.Name)
        and (
            node.id in state.by_name
            or node.id in state.collections
            or node.id in aliases
        )
    )


def _memory_request_parameters(
    environment: _ParserEnvironment,
    call: ast.Call,
    incoming: QueueBinding,
    data_type: ValueType,
    extra_keywords: set[str] | None = None,
) -> tuple[str, ast.expr, ast.expr, ast.expr, str, int]:
    allowed_keywords = {
        "address",
        "write",
        "data",
        "result_field",
        "depth",
        *(extra_keywords or set()),
    }
    if any(
        keyword.arg is None or keyword.arg not in allowed_keywords
        for keyword in call.keywords
    ):
        raise QueueFrontendError(
            "ACPY-QUEUE-015: memory request has an unsupported keyword"
        )
    policies: dict[str, ast.expr] = {}
    for policy in ("address", "write", "data"):
        values = [
            keyword.value for keyword in call.keywords if keyword.arg == policy
        ]
        if len(values) != 1:
            raise QueueFrontendError(
                f"ACPY-QUEUE-015: memory request requires one {policy} lambda"
            )
        policies[policy] = values[0]
    arguments_and_values = [
        _lambda(environment, policies[item]) for item in policies
    ]
    if len({argument for argument, _ in arguments_and_values}) != 1:
        raise QueueFrontendError(
            "ACPY-QUEUE-015: memory request lambdas require one argument name"
        )
    result_fields = [
        keyword.value for keyword in call.keywords if keyword.arg == "result_field"
    ]
    if (
        len(result_fields) != 1
        or not isinstance(result_fields[0], ast.Constant)
        or type(result_fields[0].value) is not str
        or not result_fields[0].value
    ):
        raise QueueFrontendError(
            "ACPY-QUEUE-015: memory request requires one static result_field"
        )
    payload = next(
        (
            declaration
            for declaration in environment.payloads
            if declaration.descriptor == incoming.payload
        ),
        None,
    )
    result_field = result_fields[0].value
    field_types = dict(payload.field_descriptors) if payload is not None else {}
    if result_field not in field_types:
        raise QueueFrontendError("ACPY-QUEUE-015: memory result_field is unknown")
    if not _types_equal_in_epoch_05(field_types[result_field], data_type):
        raise QueueFrontendError(
            "ACPY-QUEUE-015: memory result_field must match instance data type"
        )
    return (
        arguments_and_values[0][0],
        arguments_and_values[0][1],
        arguments_and_values[1][1],
        arguments_and_values[2][1],
        result_field,
        _positive_int(environment, call, "depth", 1),
    )


def handle_memory_array_select(
    environment: _ParserEnvironment,
    state: _ParserState,
    statement: ast.stmt,
    scope: tuple[str, ...],
    aliases: _Aliases,
    current_order: int,
) -> _StatementResult:
    """Handle selection of one statically declared memory bank."""
    if not (
        isinstance(statement, ast.Assign)
        and len(statement.targets) == 1
        and isinstance(statement.targets[0], ast.Name)
        and isinstance(statement.value, ast.Call)
        and isinstance(statement.value.func, ast.Attribute)
        and statement.value.func.attr == "select"
        and isinstance(statement.value.func.value, ast.Name)
        and statement.value.func.value.id in state.memory_arrays
    ):
        return UNHANDLED
    name = statement.targets[0].id
    if (
        name in state.by_name
        or name in state.collections
        or name in state.memory_by_name
        or name in state.memory_arrays
        or name in state.selected_memories
    ):
        raise QueueFrontendError(
            "ACPY-QUEUE-015: selected memory requires one fresh name"
        )
    call = statement.value
    if len(call.args) != 1 or any(
        keyword.arg is None or keyword.arg not in {"key", "depth", "latency"}
        for keyword in call.keywords
    ):
        raise QueueFrontendError(
            "ACPY-QUEUE-015: memory array select requires one request Queue"
        )
    keys = [keyword.value for keyword in call.keywords if keyword.arg == "key"]
    if len(keys) != 1:
        raise QueueFrontendError(
            "ACPY-QUEUE-015: memory array select requires one key lambda"
        )
    array = state.memory_arrays[call.func.value.id]
    if len(scope) < len(array.scope) or scope[: len(array.scope)] != array.scope:
        raise QueueFrontendError(
            "ACPY-QUEUE-015: memory array is only visible in its declaration "
            "scope and descendants"
        )
    input_name = _queue_reference(state, call.args[0], aliases)
    incoming = state.by_name[input_name]
    argument, selector = _lambda(environment, keys[0])
    depth = _positive_int(environment, call, "depth", 1)
    latency = _positive_int(environment, call, "latency", 1)
    routed_inputs = tuple(
        f"{name}__bank{index}_request" for index in range(len(array.members))
    )
    for routed in routed_inputs:
        if routed in state.by_name:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: selected memory synthetic Queue name collides "
                "with an existing binding"
            )
        output = QueueBinding(
            routed,
            incoming.payload,
            depth,
            latency,
            None,
            scope=scope,
            order=current_order,
            route_output=True,
        )
        state.queues.append(output)
        state.by_name[routed] = output
    state.routes.append(
        RouteBinding(
            input_name,
            routed_inputs,
            argument,
            selector,
            depth,
            latency,
            scope,
            current_order,
        )
    )
    state.selected_memories[name] = SelectedMemoryBinding(
        name,
        array.name,
        input_name,
        routed_inputs,
        argument,
        selector,
        depth,
        latency,
        scope,
        current_order,
    )
    return HANDLED


def handle_queue_graph_operation(
    environment: _ParserEnvironment,
    state: _ParserState,
    statement: ast.stmt,
    scope: tuple[str, ...],
    aliases: _Aliases,
    current_order: int,
) -> _StatementResult:
    """Handle the ordered single-output Queue graph and memory operations."""
    if not (
        isinstance(statement, ast.Assign)
        and len(statement.targets) == 1
        and isinstance(statement.value, ast.Call)
        and isinstance(statement.value.func, ast.Attribute)
    ):
        return UNHANDLED
    call = statement.value
    target = statement.targets[0]
    if not isinstance(target, ast.Name):
        if call.func.attr == "memory":
            raise QueueFrontendError(
                "ACPY-QUEUE-015: Queue.memory was removed; declare ac.memory(...) "
                "and connect it with instance.request(...)"
            )
        return UNHANDLED
    name = target.id
    receiver = call.func.value
    operation = call.func.attr

    if operation == "merge" and _is_queue_reference_syntax(
        state, receiver, aliases
    ):
        if name in state.by_name or name in state.collections:
            raise QueueFrontendError(
                "ACPY-QUEUE-008: merge output requires one fresh name"
            )
        inputs = tuple(
            _queue_reference(state, operand, aliases)
            for operand in (receiver, *call.args)
        )
        if len(inputs) < 2:
            raise QueueFrontendError(
                "ACPY-QUEUE-008: merge requires at least two Queues"
            )
        payload = state.by_name[inputs[0]].payload
        if any(
            not _types_equal_in_epoch_05(
                state.by_name[input_name].payload, payload
            )
            for input_name in inputs
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-008: merge Queue payloads must match"
            )
        policies = [
            keyword.value for keyword in call.keywords if keyword.arg == "policy"
        ]
        if len(policies) > 1 or (
            policies
            and (
                not isinstance(policies[0], ast.Constant)
                or policies[0].value not in {"round_robin", "priority"}
            )
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-008: merge policy must be round_robin or priority"
            )
        policy = policies[0].value if policies else "round_robin"
        depth = _positive_int(environment, call, "depth", 1)
        latency = _positive_int(environment, call, "latency", 1)
        output = QueueBinding(
            name,
            payload,
            depth,
            latency,
            None,
            scope=scope,
            order=current_order,
            merge_output=True,
        )
        state.queues.append(output)
        state.by_name[name] = output
        state.merges.append(
            MergeBinding(
                inputs,
                name,
                policy,
                depth,
                latency,
                scope,
                current_order,
            )
        )
        return HANDLED

    if (
        operation == "reorder"
        and isinstance(receiver, ast.Name)
        and not call.args
    ):
        if name in state.by_name or name in state.collections:
            raise QueueFrontendError(
                "ACPY-QUEUE-013: reorder output requires one fresh name"
            )
        incoming = state.by_name.get(receiver.id)
        if incoming is None:
            raise QueueFrontendError("ACPY-QUEUE-013: reorder input is unbound")
        allowed = {"key", "capacity", "start", "depth", "latency"}
        if any(k.arg is None or k.arg not in allowed for k in call.keywords):
            raise QueueFrontendError(
                "ACPY-QUEUE-013: reorder has an unsupported keyword"
            )
        keys = [k.value for k in call.keywords if k.arg == "key"]
        if len(keys) != 1:
            raise QueueFrontendError(
                "ACPY-QUEUE-013: reorder requires one key lambda"
            )
        argument, key = _lambda(environment, keys[0])
        capacity = _positive_int(environment, call, "capacity", 16)
        start = _nonnegative_int(environment, call, "start", 0)
        depth = _positive_int(environment, call, "depth", 1)
        latency = _positive_int(environment, call, "latency", 1)
        output = QueueBinding(
            name,
            incoming.payload,
            depth,
            latency,
            None,
            scope=scope,
            order=current_order,
            reorder_output=True,
        )
        state.queues.append(output)
        state.by_name[name] = output
        state.reorders.append(
            ReorderBinding(
                incoming.name,
                name,
                argument,
                key,
                capacity,
                start,
                depth,
                latency,
                scope,
                current_order,
            )
        )
        return HANDLED

    if operation == "depend" and isinstance(receiver, ast.Name) and not call.args:
        if name in state.by_name or name in state.collections:
            raise QueueFrontendError(
                "ACPY-QUEUE-014: dependency output requires one fresh name"
            )
        incoming = state.by_name.get(receiver.id)
        if incoming is None:
            raise QueueFrontendError(
                "ACPY-QUEUE-014: dependency input is unbound"
            )
        allowed = {
            "key",
            "waits_for",
            "resource",
            "cost",
            "capacity",
            "resources",
            "no_dependency",
            "depth",
            "latency",
        }
        if any(k.arg is None or k.arg not in allowed for k in call.keywords):
            raise QueueFrontendError(
                "ACPY-QUEUE-014: dependency has an unsupported keyword"
            )
        policies: dict[str, ast.expr] = {}
        for policy in ("key", "waits_for", "resource", "cost"):
            values = [k.value for k in call.keywords if k.arg == policy]
            if len(values) != 1:
                raise QueueFrontendError(
                    f"ACPY-QUEUE-014: dependency requires one {policy} lambda"
                )
            policies[policy] = values[0]
        key_argument, key = _lambda(environment, policies["key"])
        waits_argument, waits_for = _lambda(environment, policies["waits_for"])
        resource_argument, resource = _lambda(environment, policies["resource"])
        cost_argument, cost = _lambda(environment, policies["cost"])
        if len(
            {key_argument, waits_argument, resource_argument, cost_argument}
        ) != 1:
            raise QueueFrontendError(
                "ACPY-QUEUE-014: dependency lambdas require one argument name"
            )
        capacity = _positive_int(environment, call, "capacity", 16)
        resources = _positive_int(environment, call, "resources", 1)
        no_dependency = _nonnegative_int(
            environment, call, "no_dependency", 255
        )
        depth = _positive_int(environment, call, "depth", 1)
        latency = _positive_int(environment, call, "latency", 1)
        output = QueueBinding(
            name,
            incoming.payload,
            depth,
            latency,
            None,
            scope=scope,
            order=current_order,
            dependency_output=True,
        )
        state.queues.append(output)
        state.by_name[name] = output
        state.dependencies.append(
            DependencyBinding(
                incoming.name,
                name,
                key_argument,
                key,
                waits_for,
                resource,
                cost,
                capacity,
                resources,
                no_dependency,
                depth,
                latency,
                scope,
                current_order,
            )
        )
        return HANDLED

    if (
        operation == "select"
        and isinstance(receiver, ast.Name)
        and receiver.id in state.collections
    ):
        if name in state.by_name or name in state.collections:
            raise QueueFrontendError(
                "ACPY-QUEUE-018: select output requires one fresh name"
            )
        if len(call.args) != 1 or any(
            k.arg is None or k.arg not in {"key", "depth", "latency"}
            for k in call.keywords
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-018: select requires one control Queue"
            )
        control = _queue_reference(state, call.args[0], aliases)
        collection = state.collections[receiver.id]
        if any(not isinstance(member, str) for _, member in collection.members):
            raise QueueFrontendError(
                "ACPY-QUEUE-018: select requires a flat Queue collection"
            )
        inputs = tuple(
            member for _, member in collection.members if isinstance(member, str)
        )
        if len(inputs) < 2 or control in inputs:
            raise QueueFrontendError(
                "ACPY-QUEUE-018: select requires two unique data Queues"
            )
        payload = state.by_name[inputs[0]].payload
        if any(
            not _types_equal_in_epoch_05(
                state.by_name[input_name].payload, payload
            )
            for input_name in inputs
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-018: select data Queue payloads must match"
            )
        keys = [k.value for k in call.keywords if k.arg == "key"]
        if len(keys) != 1:
            raise QueueFrontendError(
                "ACPY-QUEUE-018: select requires one key lambda"
            )
        argument, selector = _lambda(environment, keys[0])
        depth = _positive_int(environment, call, "depth", 1)
        latency = _positive_int(environment, call, "latency", 1)
        output = QueueBinding(
            name,
            payload,
            depth,
            latency,
            None,
            scope=scope,
            order=current_order,
            select_output=True,
        )
        state.queues.append(output)
        state.by_name[name] = output
        state.selects.append(
            SelectBinding(
                control,
                inputs,
                name,
                argument,
                selector,
                depth,
                latency,
                scope,
                current_order,
            )
        )
        return HANDLED

    if operation == "credit" and isinstance(receiver, ast.Name) and not call.args:
        if name in state.by_name or name in state.collections:
            raise QueueFrontendError(
                "ACPY-QUEUE-016: credit output requires one fresh name"
            )
        incoming = state.by_name.get(receiver.id)
        if incoming is None:
            raise QueueFrontendError("ACPY-QUEUE-016: credit input is unbound")
        allowed = {"cost", "credits", "depth", "latency"}
        if any(k.arg is None or k.arg not in allowed for k in call.keywords):
            raise QueueFrontendError(
                "ACPY-QUEUE-016: credit has an unsupported keyword"
            )
        costs = [k.value for k in call.keywords if k.arg == "cost"]
        if len(costs) != 1:
            raise QueueFrontendError(
                "ACPY-QUEUE-016: credit requires one cost lambda"
            )
        argument, cost = _lambda(environment, costs[0])
        credit_count = _positive_int(environment, call, "credits", 16)
        depth = _positive_int(environment, call, "depth", 1)
        latency = _positive_int(environment, call, "latency", 1)
        output = QueueBinding(
            name,
            incoming.payload,
            depth,
            latency,
            None,
            scope=scope,
            order=current_order,
            credit_output=True,
        )
        state.queues.append(output)
        state.by_name[name] = output
        state.credits.append(
            CreditBinding(
                incoming.name,
                name,
                argument,
                cost,
                credit_count,
                depth,
                latency,
                scope,
                current_order,
            )
        )
        return HANDLED

    if (
        operation == "request"
        and isinstance(receiver, ast.Name)
        and receiver.id in state.selected_memories
        and not call.args
    ):
        if (
            name in state.by_name
            or name in state.collections
            or name in state.memory_by_name
            or name in state.memory_arrays
            or name in state.selected_memories
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory request output requires one fresh name"
            )
        selected_name = receiver.id
        selected = state.selected_memories[selected_name]
        if selected_name in state.consumed_selected_memories:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: selected memory may be requested only once"
            )
        if selected.scope != scope:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: selected memory must be requested in the same "
                "lexical scope"
            )
        incoming = state.by_name[selected.input_name]
        array = state.memory_arrays[selected.array]
        argument, address, write, data, result_field, depth = (
            _memory_request_parameters(
                environment,
                call,
                incoming,
                array.data_type,
                {"merge_policy", "merge_depth", "merge_latency"},
            )
        )
        merge_policies = [
            k.value for k in call.keywords if k.arg == "merge_policy"
        ]
        if len(merge_policies) > 1 or (
            merge_policies
            and (
                not isinstance(merge_policies[0], ast.Constant)
                or merge_policies[0].value not in {"priority", "round_robin"}
            )
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-015: merge_policy must be priority or round_robin"
            )
        merge_policy = (
            merge_policies[0].value if merge_policies else "priority"
        )
        merge_depth = _positive_int(environment, call, "merge_depth", 1)
        merge_latency = _positive_int(environment, call, "merge_latency", 1)
        response_names = tuple(
            f"{name}__bank{index}" for index in range(len(array.members))
        )
        for instance_name, input_name, output_name in zip(
            array.members, selected.routed_inputs, response_names, strict=True
        ):
            if output_name in state.by_name:
                raise QueueFrontendError(
                    "ACPY-QUEUE-015: selected memory response Queue name collides "
                    "with an existing binding"
                )
            output = QueueBinding(
                output_name,
                incoming.payload,
                depth,
                1,
                None,
                scope=scope,
                order=current_order,
                memory_output=True,
            )
            state.queues.append(output)
            state.by_name[output_name] = output
            state.memory_requests.append(
                MemoryRequestBinding(
                    instance_name,
                    input_name,
                    output_name,
                    argument,
                    address,
                    write,
                    data,
                    result_field,
                    depth,
                    scope,
                    current_order,
                )
            )
        merge_order = current_order + 1
        output = QueueBinding(
            name,
            incoming.payload,
            merge_depth,
            merge_latency,
            None,
            scope=scope,
            order=merge_order,
            merge_output=True,
        )
        state.queues.append(output)
        state.by_name[name] = output
        state.merges.append(
            MergeBinding(
                response_names,
                name,
                str(merge_policy),
                merge_depth,
                merge_latency,
                scope,
                merge_order,
            )
        )
        state.consumed_selected_memories.add(selected_name)
        state.order += 1
        return HANDLED

    if operation == "request" and isinstance(receiver, ast.Name) and len(call.args) == 1:
        if name in state.by_name or name in state.collections:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory request output requires one fresh name"
            )
        instance = state.memory_by_name.get(receiver.id)
        if instance is None:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory request instance is unbound"
            )
        if len(scope) < len(instance.scope) or scope[: len(instance.scope)] != instance.scope:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory instance is only visible in its "
                "declaration scope and descendants"
            )
        incoming_name = _queue_reference(state, call.args[0], aliases)
        incoming = state.by_name.get(incoming_name)
        if incoming is None:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory request input is unbound"
            )
        argument, address, write, data, result_field, depth = (
            _memory_request_parameters(
                environment, call, incoming, instance.data_type
            )
        )
        output = QueueBinding(
            name,
            incoming.payload,
            depth,
            1,
            None,
            scope=scope,
            order=current_order,
            memory_output=True,
        )
        state.queues.append(output)
        state.by_name[name] = output
        state.memory_requests.append(
            MemoryRequestBinding(
                instance.name,
                incoming.name,
                name,
                argument,
                address,
                write,
                data,
                result_field,
                depth,
                scope,
                current_order,
            )
        )
        return HANDLED

    if operation == "memory":
        raise QueueFrontendError(
            "ACPY-QUEUE-015: Queue.memory was removed; declare ac.memory(...) and "
            "connect it with instance.request(...)"
        )
    return UNHANDLED
