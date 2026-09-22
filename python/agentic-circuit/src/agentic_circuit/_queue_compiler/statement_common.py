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
    _constant_integer,
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


def _unresolved_reference_error(node: ast.expr) -> QueueFrontendError:
    """Explain why an expression is not a usable Queue reference.

    ``ac.sink(keep(q))`` used to report "collection reference must be statically
    resolvable", which sends the reader looking for an ``ac.array``/``ac.map``
    mistake. A Queue reference is a name or a static collection member, so the
    real fix for a call result is to bind it to a name first. Both the parser
    closure and the statement handlers funnel their final rejection through here
    so the two copies cannot drift.
    """

    if isinstance(node, ast.Call):
        expression = ast.unparse(node)
        callee = ast.unparse(node.func)
        if len(expression) > 60:
            expression = f"{expression[:57]}..."
        return QueueFrontendError(
            "ACPY-QUEUE-005: a call result cannot be used directly where a Queue "
            f"reference is required: {expression}. Queue references are names or "
            "static collection members; bind the call to a name first, for "
            f"example `result = {callee}(...)`, then use `result`"
        )
    return QueueFrontendError(
        "ACPY-QUEUE-005: collection reference must be statically resolvable: "
        f"{ast.unparse(node)!r}"
    )


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
    if isinstance(node, ast.Subscript) and isinstance(node.slice, ast.Slice):
        collection = _static_reference(state, node.value, aliases)
        if not isinstance(collection, StaticQueueCollection):
            raise QueueFrontendError(
                "ACPY-QUEUE-005: static slicing requires a collection"
            )
        return _slice_collection(collection, node.slice)
    if isinstance(node, ast.Subscript):
        # A static index may be spelled through a static length, as in
        # `lanes[len(lanes) - 1]`. Only `len(<static collection>)` is rewritten,
        # so any other expression stays unresolved and reports the same
        # diagnostic as before.
        key_node = _substitute_static_lengths(state, node.slice, aliases)
        if isinstance(key_node, ast.Constant) and type(key_node.value) in {
            str,
            int,
            bool,
        }:
            key: str | int | bool | None = key_node.value
        else:
            key = _constant_integer(key_node)
        if key is None:
            raise _unresolved_reference_error(node)
        collection = _static_reference(state, node.value, aliases)
        if not isinstance(collection, StaticQueueCollection):
            raise QueueFrontendError(
                "ACPY-QUEUE-005: static indexing requires a collection"
            )
        for member_key, value in collection.members:
            if type(member_key) is type(key) and member_key == key:
                return value
        raise QueueFrontendError(f"ACPY-QUEUE-005: collection has no key {key!r}")
    raise _unresolved_reference_error(node)


def _static_slice_bounds(
    node: ast.Slice,
) -> tuple[int | None, int | None, int | None]:
    """Read compile-time slice bounds, failing closed on anything dynamic."""

    bounds: list[int | None] = []
    for part in (node.lower, node.upper, node.step):
        if part is None:
            bounds.append(None)
        elif isinstance(part, ast.Constant) and type(part.value) is int:
            bounds.append(part.value)
        elif (
            isinstance(part, ast.UnaryOp)
            and isinstance(part.op, ast.USub)
            and isinstance(part.operand, ast.Constant)
            and type(part.operand.value) is int
        ):
            # A negative bound is an unfolded constant, and slice.indices
            # already implements Python's negative-bound rule.
            bounds.append(-part.operand.value)
        else:
            raise QueueFrontendError(
                "ACPY-QUEUE-005: static slice bounds must be compile-time integers"
            )
    return bounds[0], bounds[1], bounds[2]


def _slice_collection(
    source: StaticQueueCollection, node: ast.Slice
) -> StaticQueueCollection:
    """Return the sub-collection a Python slice selects.

    A static collection is fully known during elaboration, so a slice is a new
    static collection with the selected members re-keyed from zero.
    ``slice.indices`` supplies Python's clamping and negative-bound rules rather
    than a reimplementation of them.
    """

    if source.kind == "map":
        raise QueueFrontendError(
            "ACPY-QUEUE-005: a keyed collection cannot be sliced; slice an "
            "ordered collection"
        )
    lower, upper, step = _static_slice_bounds(node)
    try:
        selected = range(*slice(lower, upper, step).indices(len(source.members)))
    except ValueError:
        raise QueueFrontendError(
            "ACPY-QUEUE-005: static slice step must not be zero"
        ) from None
    members = tuple(
        (position, source.members[index][1])
        for position, index in enumerate(selected)
    )
    if not members:
        raise QueueFrontendError("ACPY-QUEUE-005: static slice selects no members")
    return StaticQueueCollection(source.kind, members)


def _static_collection_length(
    state: _ParserState, node: ast.expr, aliases: _Aliases
) -> int | None:
    """Resolve ``len(<static collection>)`` to its member count.

    A static collection is fully known during elaboration, so its length is a
    compile-time integer. Without this, a design that expands ``K`` queues has to
    repeat ``K`` in its loop bound, which is the duplication the ``ac.list``
    consolidation removes.
    """

    if not (
        isinstance(node, ast.Call)
        and isinstance(node.func, ast.Name)
        and node.func.id == "len"
        and len(node.args) == 1
        and not node.keywords
    ):
        return None
    try:
        value = _static_reference(state, node.args[0], aliases)
    except QueueFrontendError:
        return None
    if not isinstance(value, StaticQueueCollection):
        return None
    return len(value.members)


def _substitute_static_lengths(
    state: _ParserState, node: ast.expr, aliases: _Aliases
) -> ast.expr:
    """Rewrite ``len(<static collection>)`` to its constant member count.

    The rewrite runs before static evaluation so the length participates in
    ordinary compile-time integer arithmetic (``range(len(lanes) - 1)``). A
    ``len`` that does not resolve is left alone, so the caller keeps its
    existing compile-time-integer diagnostic.
    """

    class LengthSubstituter(ast.NodeTransformer):
        def visit_Call(self, call: ast.Call) -> ast.expr:
            self.generic_visit(call)
            length = _static_collection_length(state, call, aliases)
            if length is None:
                return call
            return ast.copy_location(ast.Constant(length), call)

    return LengthSubstituter().visit(node)


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
