"""Memory declaration, selection, and request statement handlers."""

from __future__ import annotations

import ast

from .._source_map import source_frame
from .errors import QueueFrontendError
from .model import (
    MemoryInstanceBinding,
    MemoryRequestBinding,
    MergeBinding,
    QueueBinding,
    RouteBinding,
    SelectedMemoryBinding,
    StaticMemoryArrayBinding,
)
from .parser_context import (
    HANDLED,
    UNHANDLED,
    _ParserEnvironment,
    _ParserState,
    _StatementContext,
    _StatementResult,
)
from .statement_common import (
    _call_name,
    _lambda,
    _memory_request_parameters,
    _positive_int,
    _queue_reference,
)
from .static_types import (
    _integer_width,
    _nonnegative_int_value,
    _payload,
    _positive_int_value,
    _static_int_value,
)


def handle_memory_array_select(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
) -> _StatementResult:
    statement = context.statement
    scope = context.scope
    aliases = context.aliases
    current_order = context.current_order
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


def _memory_instance_binding(
    environment: _ParserEnvironment,
    name: str,
    call: ast.Call,
    scope: tuple[str, ...],
    current_order: int,
    static_values: dict[str, int] | None = None,
) -> MemoryInstanceBinding:
    if _call_name(call) != "memory" or len(call.args) != 1:
        raise QueueFrontendError("ACPY-QUEUE-015: memory requires one data type")
    if any(
        keyword.arg is None or keyword.arg not in {"entries", "init", "latency"}
        for keyword in call.keywords
    ):
        raise QueueFrontendError(
            "ACPY-QUEUE-015: memory instance has an unsupported keyword"
        )
    data_type = _payload(
        call.args[0],
        environment.payload_map,
        environment.enum_map,
        static_values=environment.type_static_values,
    )
    if _integer_width(data_type) is None:
        raise QueueFrontendError("ACPY-QUEUE-015: memory data type must be an integer")
    values = environment.static_values if static_values is None else static_values
    entries = _positive_int_value(call, "entries", 16, values)
    init = _nonnegative_int_value(call, "init", 0, values)
    latency = _positive_int_value(call, "latency", 1, values)
    if init != 0:
        raise QueueFrontendError("ACPY-QUEUE-015: memory init must be zero")
    return MemoryInstanceBinding(
        name,
        data_type,
        entries,
        init,
        latency,
        scope,
        current_order,
        source_frame(call),
    )


def handle_memory_declaration(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
) -> _StatementResult:
    statement = context.statement
    if not (
        isinstance(statement, ast.Assign)
        and len(statement.targets) == 1
        and isinstance(statement.targets[0], ast.Name)
        and isinstance(statement.value, ast.Call)
        and _call_name(statement.value) == "memory"
        and len(statement.value.args) == 1
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
            "ACPY-QUEUE-015: memory instance requires one fresh name"
        )
    instance = _memory_instance_binding(
        environment, name, statement.value, context.scope, context.current_order
    )
    state.memory_instances.append(instance)
    state.memory_by_name[name] = instance
    return HANDLED


def handle_memory_array_declaration(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
) -> _StatementResult:
    """Expand an array generator whose leaves are memory declarations."""
    statement = context.statement
    if not (
        isinstance(statement, ast.Assign)
        and len(statement.targets) == 1
        and isinstance(statement.targets[0], ast.Name)
        and isinstance(statement.value, ast.Call)
        and _call_name(statement.value) == "array"
        and len(statement.value.args) == 2
    ):
        return UNHANDLED
    call = statement.value
    argument, generator = _lambda(environment, call.args[1])
    if not isinstance(generator, ast.Call) or _call_name(generator) != "memory":
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
            "ACPY-QUEUE-005: collection assignment requires one fresh name"
        )
    extent = _static_int_value(call.args[0], {})
    if extent is None or extent <= 0:
        raise QueueFrontendError(
            "ACPY-QUEUE-015: memory array requires a positive compile-time extent"
        )
    pending: list[MemoryInstanceBinding] = []
    for index in range(extent):
        member_name = f"{name}__{index}"
        if (
            member_name in state.by_name
            or member_name in state.collections
            or member_name in state.memory_by_name
            or member_name in state.memory_arrays
            or member_name in state.selected_memories
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory array element name collides with an "
                "existing binding"
            )
        pending.append(
            _memory_instance_binding(
                environment,
                member_name,
                generator,
                context.scope,
                context.current_order,
                {argument: index},
            )
        )
    configurations = {
        (item.data_type, item.entries, item.init, item.latency) for item in pending
    }
    if len(configurations) != 1:
        raise QueueFrontendError(
            "ACPY-QUEUE-015: memory array elements must be homogeneous"
        )
    for instance in pending:
        state.memory_instances.append(instance)
        state.memory_by_name[instance.name] = instance
    first = pending[0]
    state.memory_arrays[name] = StaticMemoryArrayBinding(
        name,
        tuple(instance.name for instance in pending),
        first.data_type,
        first.entries,
        first.init,
        first.latency,
        context.scope,
        context.current_order,
    )
    return HANDLED


def handle_memory_request(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
) -> _StatementResult:
    statement = context.statement
    scope = context.scope
    aliases = context.aliases
    current_order = context.current_order
    if not (
        isinstance(statement, ast.Assign)
        and len(statement.targets) == 1
        and isinstance(statement.value, ast.Call)
        and isinstance(statement.value.func, ast.Attribute)
    ):
        return UNHANDLED
    call = statement.value
    target = statement.targets[0]
    operation = call.func.attr
    if not isinstance(target, ast.Name):
        if operation == "memory":
            raise QueueFrontendError(
                "ACPY-QUEUE-015: Queue.memory was removed; declare ac.memory(...) "
                "and connect it with instance.request(...)"
            )
        return UNHANDLED
    name = target.id
    receiver = call.func.value
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
        merge_policies = [k.value for k in call.keywords if k.arg == "merge_policy"]
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
        merge_policy = merge_policies[0].value if merge_policies else "priority"
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
    if (
        operation == "request"
        and isinstance(receiver, ast.Name)
        and len(call.args) == 1
    ):
        if name in state.by_name or name in state.collections:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory request output requires one fresh name"
            )
        instance = state.memory_by_name.get(receiver.id)
        if instance is None:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory request instance is unbound"
            )
        if (
            len(scope) < len(instance.scope)
            or scope[: len(instance.scope)] != instance.scope
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory instance is only visible in its "
                "declaration scope and descendants"
            )
        incoming_name = _queue_reference(state, call.args[0], aliases)
        incoming = state.by_name.get(incoming_name)
        if incoming is None:
            raise QueueFrontendError("ACPY-QUEUE-015: memory request input is unbound")
        argument, address, write, data, result_field, depth = (
            _memory_request_parameters(environment, call, incoming, instance.data_type)
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
