"""Queue graph statement handlers."""

from __future__ import annotations

import ast

from .errors import QueueFrontendError
from .model import (
    BarrierBinding,
    CreditBinding,
    DependencyBinding,
    ForkBinding,
    MergeBinding,
    QueueBinding,
    ReorderBinding,
    RouteBinding,
    SelectBinding,
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
    _field_expression,
    _is_queue_reference_syntax,
    _keyword_value,
    _lambda,
    _nonnegative_int,
    _positive_int,
    _queue_reference,
)
from .static_types import _types_equal_in_epoch_05


def _handle_merge(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
    call: ast.Call,
    name: str,
    receiver: ast.expr,
) -> _StatementResult:
    scope = context.scope
    aliases = context.aliases
    current_order = context.current_order
    operation = "merge"
    if operation == "merge" and _is_queue_reference_syntax(state, receiver, aliases):
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
            not _types_equal_in_epoch_05(state.by_name[input_name].payload, payload)
            for input_name in inputs
        ):
            raise QueueFrontendError("ACPY-QUEUE-008: merge Queue payloads must match")
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
    return UNHANDLED


def _handle_reorder(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
    call: ast.Call,
    name: str,
    receiver: ast.expr,
) -> _StatementResult:
    scope = context.scope
    current_order = context.current_order
    operation = "reorder"
    if operation == "reorder" and isinstance(receiver, ast.Name) and not call.args:
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
            raise QueueFrontendError("ACPY-QUEUE-013: reorder requires one key lambda")
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
    return UNHANDLED


def _handle_depend(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
    call: ast.Call,
    name: str,
    receiver: ast.expr,
) -> _StatementResult:
    scope = context.scope
    current_order = context.current_order
    operation = "depend"
    if operation == "depend" and isinstance(receiver, ast.Name) and not call.args:
        if name in state.by_name or name in state.collections:
            raise QueueFrontendError(
                "ACPY-QUEUE-014: dependency output requires one fresh name"
            )
        incoming = state.by_name.get(receiver.id)
        if incoming is None:
            raise QueueFrontendError("ACPY-QUEUE-014: dependency input is unbound")
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
        if len({key_argument, waits_argument, resource_argument, cost_argument}) != 1:
            raise QueueFrontendError(
                "ACPY-QUEUE-014: dependency lambdas require one argument name"
            )
        capacity = _positive_int(environment, call, "capacity", 16)
        resources = _positive_int(environment, call, "resources", 1)
        no_dependency = _nonnegative_int(environment, call, "no_dependency", 255)
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
    return UNHANDLED


def _handle_select(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
    call: ast.Call,
    name: str,
    receiver: ast.expr,
) -> _StatementResult:
    scope = context.scope
    aliases = context.aliases
    current_order = context.current_order
    operation = "select"
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
            not _types_equal_in_epoch_05(state.by_name[input_name].payload, payload)
            for input_name in inputs
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-018: select data Queue payloads must match"
            )
        keys = [k.value for k in call.keywords if k.arg == "key"]
        if len(keys) != 1:
            raise QueueFrontendError("ACPY-QUEUE-018: select requires one key lambda")
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
    return UNHANDLED


def _handle_credit(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
    call: ast.Call,
    name: str,
    receiver: ast.expr,
) -> _StatementResult:
    scope = context.scope
    current_order = context.current_order
    operation = "credit"
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
            raise QueueFrontendError("ACPY-QUEUE-016: credit requires one cost lambda")
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
    return UNHANDLED


def handle_queue_graph_operation(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
) -> _StatementResult:
    """Dispatch one single-output Queue graph operation to its leaf handler."""
    statement = context.statement
    if not (
        isinstance(statement, ast.Assign)
        and len(statement.targets) == 1
        and isinstance(statement.targets[0], ast.Name)
        and isinstance(statement.value, ast.Call)
        and isinstance(statement.value.func, ast.Attribute)
    ):
        return UNHANDLED
    operation = statement.value.func.attr
    handler = {
        "merge": _handle_merge,
        "reorder": _handle_reorder,
        "depend": _handle_depend,
        "select": _handle_select,
        "credit": _handle_credit,
    }.get(operation)
    if handler is None:
        return UNHANDLED
    return handler(
        environment,
        state,
        context,
        statement.value,
        statement.targets[0].id,
        statement.value.func.value,
    )


def _handle_barrier(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
    call: ast.Call,
    names: tuple[str, ...],
) -> _StatementResult:
    scope = context.scope
    aliases = context.aliases
    current_order = context.current_order
    operation = "barrier"
    if operation == "barrier":
        if any(
            keyword.arg is None or keyword.arg not in {"depth", "latency"}
            for keyword in call.keywords
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-017: barrier has an unsupported keyword"
            )
        method_style = (
            isinstance(call.func, ast.Attribute)
            and isinstance(call.func.value, ast.Name)
            and call.func.value.id in state.by_name
        )
        operands = [call.func.value, *call.args] if method_style else list(call.args)
        inputs = tuple(
            _queue_reference(state, operand, aliases) for operand in operands
        )
        if len(inputs) < 2 or len(names) != len(inputs):
            raise QueueFrontendError(
                "ACPY-QUEUE-017: barrier requires matching input/output arity"
            )
        if len(set(inputs)) != len(inputs):
            raise QueueFrontendError(
                "ACPY-QUEUE-017: barrier inputs must be unique Queues"
            )
        if len(set(names)) != len(names) or any(
            output in state.by_name or output in state.collections for output in names
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-017: barrier outputs require fresh tuple names"
            )
        depth = _positive_int(environment, call, "depth", 1)
        latency = _positive_int(environment, call, "latency", 1)
        for input_name, output_name in zip(inputs, names, strict=True):
            output = QueueBinding(
                output_name,
                state.by_name[input_name].payload,
                depth,
                latency,
                None,
                scope=scope,
                order=current_order,
                barrier_output=True,
            )
            state.queues.append(output)
            state.by_name[output_name] = output
        state.barriers.append(
            BarrierBinding(inputs, names, depth, latency, scope, current_order)
        )
        return HANDLED
    return UNHANDLED


def _handle_route(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
    call: ast.Call,
    names: tuple[str, ...],
) -> _StatementResult:
    scope = context.scope
    aliases = context.aliases
    current_order = context.current_order
    operation = "route"
    method_style = (
        isinstance(call.func, ast.Attribute)
        and isinstance(call.func.value, ast.Name)
        and call.func.value.id in state.by_name
    )
    if method_style:
        assert isinstance(call.func, ast.Attribute)
        assert isinstance(call.func.value, ast.Name)
        if call.args:
            if operation == "route":
                raise QueueFrontendError(
                    "ACPY-QUEUE-006: method route takes no positional arguments"
                )
            raise QueueFrontendError(
                "ACPY-QUEUE-012: method fork takes no positional arguments"
            )
        input_name = call.func.value.id
    else:
        if len(call.args) != 1:
            raise QueueFrontendError(
                f"ACPY-QUEUE-024: {operation} requires one input Queue"
            )
        input_name = _queue_reference(state, call.args[0], aliases)
    incoming = state.by_name.get(input_name)
    if incoming is None:
        if operation == "route":
            raise QueueFrontendError(
                f"ACPY-QUEUE-001: input queue {input_name!r} is unbound"
            )
        raise QueueFrontendError("ACPY-QUEUE-012: fork input is unbound")
    output_count = _positive_int(environment, call, "outputs", 0)
    if operation == "route":
        if output_count != len(names) or len(set(names)) != len(names):
            raise QueueFrontendError(
                "ACPY-QUEUE-006: route outputs must match fresh tuple names"
            )
        if method_style:
            keys = [k.value for k in call.keywords if k.arg == "key"]
            if len(keys) != 1:
                raise QueueFrontendError(
                    "ACPY-QUEUE-006: route requires one key lambda"
                )
            argument, selector = _lambda(environment, keys[0])
        else:
            argument = "item"
            selector = _field_expression(
                environment,
                _keyword_value(call, "by"),
                incoming,
                argument,
            )
    elif output_count != len(names) or len(names) < 2:
        raise QueueFrontendError("ACPY-QUEUE-012: fork outputs must match tuple arity")
    depth = _positive_int(environment, call, "depth", 1)
    latency = _positive_int(environment, call, "latency", 1)
    for name in names:
        if name in state.by_name:
            if operation == "route":
                raise QueueFrontendError(
                    "ACPY-QUEUE-006: route output name is already bound"
                )
            raise QueueFrontendError(
                "ACPY-QUEUE-012: fork output name is already bound"
            )
        output = QueueBinding(
            name,
            incoming.payload,
            depth,
            latency,
            None,
            scope=scope,
            order=current_order,
            route_output=True,
        )
        state.queues.append(output)
        state.by_name[name] = output
    if operation == "route":
        state.routes.append(
            RouteBinding(
                incoming.name,
                names,
                argument,
                selector,
                depth,
                latency,
                scope,
                current_order,
            )
        )
    else:
        state.forks.append(
            ForkBinding(incoming.name, names, depth, latency, scope, current_order)
        )
    return HANDLED
    return UNHANDLED


def _handle_fork(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
    call: ast.Call,
    names: tuple[str, ...],
) -> _StatementResult:
    scope = context.scope
    aliases = context.aliases
    current_order = context.current_order
    operation = "fork"
    method_style = (
        isinstance(call.func, ast.Attribute)
        and isinstance(call.func.value, ast.Name)
        and call.func.value.id in state.by_name
    )
    if method_style:
        assert isinstance(call.func, ast.Attribute)
        assert isinstance(call.func.value, ast.Name)
        if call.args:
            if operation == "route":
                raise QueueFrontendError(
                    "ACPY-QUEUE-006: method route takes no positional arguments"
                )
            raise QueueFrontendError(
                "ACPY-QUEUE-012: method fork takes no positional arguments"
            )
        input_name = call.func.value.id
    else:
        if len(call.args) != 1:
            raise QueueFrontendError(
                f"ACPY-QUEUE-024: {operation} requires one input Queue"
            )
        input_name = _queue_reference(state, call.args[0], aliases)
    incoming = state.by_name.get(input_name)
    if incoming is None:
        if operation == "route":
            raise QueueFrontendError(
                f"ACPY-QUEUE-001: input queue {input_name!r} is unbound"
            )
        raise QueueFrontendError("ACPY-QUEUE-012: fork input is unbound")
    output_count = _positive_int(environment, call, "outputs", 0)
    if operation == "route":
        if output_count != len(names) or len(set(names)) != len(names):
            raise QueueFrontendError(
                "ACPY-QUEUE-006: route outputs must match fresh tuple names"
            )
        if method_style:
            keys = [k.value for k in call.keywords if k.arg == "key"]
            if len(keys) != 1:
                raise QueueFrontendError(
                    "ACPY-QUEUE-006: route requires one key lambda"
                )
            argument, selector = _lambda(environment, keys[0])
        else:
            argument = "item"
            selector = _field_expression(
                environment,
                _keyword_value(call, "by"),
                incoming,
                argument,
            )
    elif output_count != len(names) or len(names) < 2:
        raise QueueFrontendError("ACPY-QUEUE-012: fork outputs must match tuple arity")
    depth = _positive_int(environment, call, "depth", 1)
    latency = _positive_int(environment, call, "latency", 1)
    for name in names:
        if name in state.by_name:
            if operation == "route":
                raise QueueFrontendError(
                    "ACPY-QUEUE-006: route output name is already bound"
                )
            raise QueueFrontendError(
                "ACPY-QUEUE-012: fork output name is already bound"
            )
        output = QueueBinding(
            name,
            incoming.payload,
            depth,
            latency,
            None,
            scope=scope,
            order=current_order,
            route_output=True,
        )
        state.queues.append(output)
        state.by_name[name] = output
    if operation == "route":
        state.routes.append(
            RouteBinding(
                incoming.name,
                names,
                argument,
                selector,
                depth,
                latency,
                scope,
                current_order,
            )
        )
    else:
        state.forks.append(
            ForkBinding(incoming.name, names, depth, latency, scope, current_order)
        )
    return HANDLED
    return UNHANDLED


def handle_multi_output_operation(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
) -> _StatementResult:
    """Dispatch a tuple-producing graph operation to its leaf handler."""
    statement = context.statement
    if not (
        isinstance(statement, ast.Assign)
        and len(statement.targets) == 1
        and isinstance(statement.targets[0], ast.Tuple | ast.List)
        and isinstance(statement.value, ast.Call)
        and all(isinstance(item, ast.Name) for item in statement.targets[0].elts)
    ):
        return UNHANDLED
    operation = _call_name(statement.value)
    handler = {
        "barrier": _handle_barrier,
        "route": _handle_route,
        "fork": _handle_fork,
    }.get(operation)
    if handler is None:
        return UNHANDLED
    names = tuple(
        item.id for item in statement.targets[0].elts if isinstance(item, ast.Name)
    )
    return handler(environment, state, context, statement.value, names)
