"""Common AST and Queue-reference helpers for statement handlers."""

from __future__ import annotations

import ast

from _pycircuit_semantics import ValueType

from .definitions import _lambda_value
from .errors import QueueFrontendError
from .model import QueueBinding, StaticQueueCollection
from .normalize import _constantize_expression
from .parser_context import _Aliases, _ParserEnvironment, _ParserState
from .static_types import (
    _nonnegative_int_value,
    _positive_int_value,
    _types_compatible,
)
from .syntax import _decorator_name


def _lambda(environment: _ParserEnvironment, node: ast.expr) -> tuple[str, ast.expr]:
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


def _queue_reference(state: _ParserState, node: ast.expr, aliases: _Aliases) -> str:
    value = _static_reference(state, node, aliases)
    if isinstance(value, str):
        return value
    raise QueueFrontendError("ACPY-QUEUE-005: a collection cannot be used as one Queue")


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


def _call_name(call: ast.Call) -> str:
    return _decorator_name(call.func).rsplit(".", 1)[-1]


def _keyword_value(call: ast.Call, name: str) -> ast.expr:
    matches = [keyword.value for keyword in call.keywords if keyword.arg == name]
    if len(matches) != 1:
        raise QueueFrontendError(
            f"ACPY-QUEUE-024: high-level block requires one {name!r} parameter"
        )
    return matches[0]


def _field_expression(
    environment: _ParserEnvironment,
    node: ast.expr,
    queue: QueueBinding,
    argument: str = "item",
) -> ast.expr:
    if not isinstance(node, ast.Attribute) or not isinstance(node.value, ast.Name):
        raise QueueFrontendError(
            "ACPY-QUEUE-024: high-level block requires a typed field descriptor"
        )
    payload = next(
        (item for item in environment.payloads if item.descriptor == queue.payload),
        None,
    )
    if payload is None or node.value.id != payload.name:
        raise QueueFrontendError(
            "ACPY-QUEUE-024: field descriptor payload does not match Queue"
        )
    if node.attr not in {field.name for field in payload.descriptor.fields}:
        raise QueueFrontendError(f"ACPY-QUEUE-024: payload has no field {node.attr!r}")
    return ast.copy_location(
        ast.Attribute(
            value=ast.Name(id=argument, ctx=ast.Load()),
            attr=node.attr,
            ctx=ast.Load(),
        ),
        node,
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
        values = [keyword.value for keyword in call.keywords if keyword.arg == policy]
        if len(values) != 1:
            raise QueueFrontendError(
                f"ACPY-QUEUE-015: memory request requires one {policy} lambda"
            )
        policies[policy] = values[0]
    arguments_and_values = [_lambda(environment, policies[item]) for item in policies]
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
    if not _types_compatible(field_types[result_field], data_type):
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
