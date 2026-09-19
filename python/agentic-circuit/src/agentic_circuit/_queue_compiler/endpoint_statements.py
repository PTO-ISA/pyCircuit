"""Queue endpoint statement handlers."""

from __future__ import annotations

import ast

from .._source_map import source_frame
from .errors import QueueFrontendError
from .model import (
    ExpectBinding,
    ObservationBinding,
    SinkBinding,
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
    _queue_reference,
)
from .static_types import _types_compatible


def handle_expect(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
) -> _StatementResult:
    statement = context.statement
    scope = context.scope
    aliases = context.aliases
    current_order = context.current_order
    if not (
        isinstance(statement, ast.Expr)
        and isinstance(statement.value, ast.Call)
        and _call_name(statement.value) == "expect"
        and len(statement.value.args) == 1
    ):
        return UNHANDLED
    call = statement.value
    if any(
        keyword.arg is None or keyword.arg not in {"predicate", "message"}
        for keyword in call.keywords
    ):
        raise QueueFrontendError("ACPY-QUEUE-021: expect has an unsupported keyword")
    predicates = [k.value for k in call.keywords if k.arg == "predicate"]
    messages = [k.value for k in call.keywords if k.arg == "message"]
    if len(predicates) != 1 or len(messages) != 1:
        raise QueueFrontendError(
            "ACPY-QUEUE-021: expect requires predicate and message"
        )
    if (
        not isinstance(messages[0], ast.Constant)
        or type(messages[0].value) is not str
        or not messages[0].value
    ):
        raise QueueFrontendError(
            "ACPY-QUEUE-021: expect message must be a static string"
        )
    argument, predicate = _lambda(environment, predicates[0])
    state.expectations.append(
        ExpectBinding(
            _queue_reference(state, call.args[0], aliases),
            argument,
            predicate,
            messages[0].value,
            scope,
            current_order,
        )
    )
    return HANDLED


def handle_observe(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
) -> _StatementResult:
    statement = context.statement
    scope = context.scope
    aliases = context.aliases
    current_order = context.current_order
    del environment
    if not (
        isinstance(statement, ast.Expr)
        and isinstance(statement.value, ast.Call)
        and _call_name(statement.value) == "observe"
        and len(statement.value.args) == 1
    ):
        return UNHANDLED
    name = _queue_reference(state, statement.value.args[0], aliases)
    state.observations.append(
        ObservationBinding(name, f"observe_{current_order}", scope, current_order)
    )
    return HANDLED


def handle_sink(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
) -> _StatementResult:
    statement = context.statement
    scope = context.scope
    aliases = context.aliases
    current_order = context.current_order
    del environment
    if not (
        isinstance(statement, ast.Expr)
        and isinstance(statement.value, ast.Call)
        and _call_name(statement.value) == "sink"
        and len(statement.value.args) == 1
    ):
        return UNHANDLED
    name = _queue_reference(state, statement.value.args[0], aliases)
    state.sinks.append(SinkBinding(name, scope, current_order, source_frame(statement)))
    return HANDLED


def handle_return(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
) -> _StatementResult:
    statement = context.statement
    scope = context.scope
    aliases = context.aliases
    current_order = context.current_order
    if not isinstance(statement, ast.Return):
        return UNHANDLED
    if statement.value is None:
        if environment.result_payloads not in {None, ()}:
            raise QueueFrontendError(
                "ACPY-QUEUE-026: typed system results must be returned"
            )
        return HANDLED
    values = (
        tuple(statement.value.elts)
        if isinstance(statement.value, ast.Tuple | ast.List)
        else (statement.value,)
    )
    returned = tuple(_queue_reference(state, value, aliases) for value in values)
    if environment.result_payloads is not None:
        if len(returned) != len(environment.result_payloads):
            raise QueueFrontendError(
                "ACPY-QUEUE-026: system return arity does not match its annotation"
            )
        for index, (queue_name, expected_payload) in enumerate(
            zip(returned, environment.result_payloads, strict=True)
        ):
            if not _types_compatible(
                state.by_name[queue_name].payload, expected_payload
            ):
                raise QueueFrontendError(
                    "ACPY-QUEUE-026: system return "
                    f"{index} payload does not match its annotation"
                )
    for index, queue_name in enumerate(returned):
        state.sinks.append(
            SinkBinding(
                queue_name,
                scope,
                current_order + index,
                source_frame(statement),
            )
        )
    state.order += len(returned) - 1
    return HANDLED
