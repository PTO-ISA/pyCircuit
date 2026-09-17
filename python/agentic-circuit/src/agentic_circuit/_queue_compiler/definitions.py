"""Queue frontend definition and conditional-effect analysis."""

from __future__ import annotations

import ast
import copy
from collections.abc import Mapping

from _pycircuit_semantics import (
    ArrayType,
    BitfieldLayout,
    BoolType,
    EnumType,
    StructType,
    TupleType,
    ValueType,
)

from .._source_map import (
    source_frame,
)
from .errors import QueueFrontendError
from .expressions import (
    _ExpressionEmitter,
    _resolve_invariant_call,
)
from .model import (
    InvariantDefinition,
    Payload,
    PureHelperDefinition,
)
from .static_types import (
    _constant_integer,
    _is_epoch_05_bool_compatible,
    _payload,
)
from .syntax import _decorator_name


def _invariant_definitions(
    tree: ast.Module,
    payloads: dict[str, Payload],
    bitfields: Mapping[str, BitfieldLayout],
    enum_types: Mapping[str, EnumType],
) -> tuple[InvariantDefinition, ...]:
    agentic_module_aliases = {
        alias.asname or alias.name
        for statement in tree.body
        if isinstance(statement, ast.Import)
        for alias in statement.names
        if alias.name == "agentic_circuit"
    }
    agentic_bare_imports = {
        alias.name
        for statement in tree.body
        if isinstance(statement, ast.ImportFrom)
        and statement.level == 0
        and statement.module == "agentic_circuit"
        for alias in statement.names
        if alias.asname is None
    }
    definitions: list[InvariantDefinition] = []
    for node in tree.body:
        if not isinstance(node, ast.FunctionDef) or not any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == "invariant"
            for decorator in node.decorator_list
        ):
            continue
        if any(isinstance(decorator, ast.Call) for decorator in node.decorator_list):
            raise QueueFrontendError(
                "ACPY-INVARIANT-001: invariant decorators do not accept options"
            )
        if (
            len(node.args.args) != 1
            or node.args.posonlyargs
            or node.args.kwonlyargs
            or node.args.vararg is not None
            or node.args.kwarg is not None
            or node.args.defaults
            or node.args.kw_defaults
        ):
            raise QueueFrontendError(
                f"ACPY-INVARIANT-001: invariant {node.name!r} requires exactly "
                "one typed payload parameter"
            )
        parameter = node.args.args[0]
        if parameter.annotation is None:
            raise QueueFrontendError(
                f"ACPY-INVARIANT-001: invariant {node.name!r} requires an exact "
                "nominal payload annotation"
            )
        payload = _payload(parameter.annotation, payloads, enum_types)
        if not isinstance(payload, StructType):
            raise QueueFrontendError(
                f"ACPY-INVARIANT-001: invariant {node.name!r} payload must be "
                "a nominal struct"
            )
        if node.returns is None or _decorator_name(node.returns) != "bool":
            raise QueueFrontendError(
                f"ACPY-INVARIANT-001: invariant {payload.name}.{node.name} "
                "must return bool"
            )
        body = list(node.body)
        if (
            body
            and isinstance(body[0], ast.Expr)
            and isinstance(body[0].value, ast.Constant)
            and isinstance(body[0].value.value, str)
        ):
            body.pop(0)
        if (
            len(body) != 1
            or not isinstance(body[0], ast.Return)
            or body[0].value is None
        ):
            raise QueueFrontendError(
                f"ACPY-INVARIANT-002: invariant {payload.name}.{node.name} "
                "requires one pure return expression"
            )
        if any(
            isinstance(candidate, (ast.Lambda, ast.NamedExpr, ast.Await, ast.Yield))
            for candidate in ast.walk(body[0].value)
        ):
            raise QueueFrontendError(
                f"ACPY-INVARIANT-002: invariant {payload.name}.{node.name} uses "
                "an unsupported expression"
            )
        definitions.append(
            InvariantDefinition(
                node.name,
                f"{payload.name}.{node.name}",
                parameter.arg,
                payload,
                copy.deepcopy(body[0].value),
            )
        )
    names = [definition.function_name for definition in definitions]
    if len(set(names)) != len(names):
        raise QueueFrontendError(
            "ACPY-INVARIANT-001: invariant function names must be unique in a closure"
        )

    by_name = {definition.function_name: definition for definition in definitions}
    call_graph: dict[str, tuple[str, ...]] = {}
    for definition in definitions:
        shadowed = {definition.argument}
        for candidate in ast.walk(definition.expression):
            if not isinstance(candidate, ast.Call):
                continue
            if isinstance(candidate.func, ast.Name):
                name = candidate.func.id
                if name not in shadowed and (
                    _resolve_invariant_call(candidate, by_name, shadowed) is not None
                    or name in payloads
                    or name in bitfields
                    or name in agentic_bare_imports
                ):
                    continue
                raise QueueFrontendError(
                    f"ACPY-INVARIANT-002: invariant {definition.qualified_name} "
                    "uses unsupported call target "
                    f"{ast.unparse(candidate.func)!r}"
                )
            bitfield_view = (
                isinstance(candidate.func, ast.Attribute)
                and candidate.func.attr == "view"
                and isinstance(candidate.func.value, ast.Name)
                and candidate.func.value.id not in shadowed
                and candidate.func.value.id in bitfields
            )
            agentic_intrinsic = (
                isinstance(candidate.func, ast.Attribute)
                and isinstance(candidate.func.value, ast.Name)
                and candidate.func.value.id not in shadowed
                and candidate.func.value.id in agentic_module_aliases
            )
            if not bitfield_view and not agentic_intrinsic:
                raise QueueFrontendError(
                    f"ACPY-INVARIANT-002: invariant {definition.qualified_name} "
                    "uses unsupported call target "
                    f"{ast.unparse(candidate.func)!r}"
                )
        call_graph[definition.function_name] = tuple(
            dict.fromkeys(
                resolved.function_name
                for candidate in ast.walk(definition.expression)
                if isinstance(candidate, ast.Call)
                and (resolved := _resolve_invariant_call(candidate, by_name, shadowed))
                is not None
            )
        )

    visited: set[str] = set()
    active: list[str] = []

    def visit(function_name: str) -> None:
        if function_name in active:
            cycle = active[active.index(function_name) :] + [function_name]
            rendered = " -> ".join(by_name[name].qualified_name for name in cycle)
            raise QueueFrontendError(
                "ACPY-INVARIANT-002: recursive invariant call graph: " + rendered
            )
        if function_name in visited:
            return
        active.append(function_name)
        for callee in call_graph[function_name]:
            visit(callee)
        active.pop()
        visited.add(function_name)

    for definition in definitions:
        visit(definition.function_name)

    for definition in definitions:
        validator = _ExpressionEmitter(
            payloads,
            definition.argument,
            definition.payload,
            root_name="value",
            enum_types=enum_types,
            bitfields=bitfields,
            invariants=by_name,
        )
        try:
            _, result_type = validator.emit(definition.expression, BoolType())
        except QueueFrontendError as error:
            raise QueueFrontendError(
                f"ACPY-INVARIANT-002: invariant {definition.qualified_name} "
                f"for payload {definition.payload.name}: {error}"
            ) from error
        if not _is_epoch_05_bool_compatible(result_type):
            raise QueueFrontendError(
                f"ACPY-INVARIANT-002: invariant {definition.qualified_name} "
                f"for payload {definition.payload.name} must produce bool"
            )
    return tuple(definitions)


def _helper_type(
    node: ast.expr, payloads: dict[str, Payload], enums: Mapping[str, ValueType]
) -> ValueType:
    kind = (
        _decorator_name(node.value).rsplit(".", 1)[-1]
        if isinstance(node, ast.Subscript)
        else ""
    )
    if isinstance(node, ast.Subscript) and kind in {"tuple", "Tuple"}:
        elements = (
            tuple(node.slice.elts)
            if isinstance(node.slice, ast.Tuple)
            else (node.slice,)
        )
        if not elements:
            raise QueueFrontendError("ACPY-HELPER-002: tuple result cannot be empty")
        return TupleType(
            tuple(_helper_type(item, payloads, enums) for item in elements)
        )
    if isinstance(node, ast.Subscript) and kind == "array":
        if not isinstance(node.slice, ast.Tuple) or len(node.slice.elts) != 2:
            raise QueueFrontendError(
                "ACPY-HELPER-002: array type requires static [length, element]"
            )
        length = _constant_integer(node.slice.elts[0])
        if length is None or length <= 0:
            raise QueueFrontendError(
                "ACPY-HELPER-002: array type length must be positive and static"
            )
        return ArrayType(
            length,
            _helper_type(node.slice.elts[1], payloads, enums),
        )
    return _payload(node, payloads, enums)


def _pure_helper_definitions(
    tree: ast.Module,
    payloads: dict[str, Payload],
    enums: Mapping[str, ValueType],
    *,
    entry: str | None = None,
    reachable_only: bool = False,
    defer_unbound_unreachable: bool = False,
) -> tuple[PureHelperDefinition, ...]:
    """Capture typed, state-free helpers as closed SSA-like expressions."""

    architecture = {"system", "module", "extern_module", "process", "rule", "invariant"}
    architecture_nodes = {
        node.name: node
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and {_decorator_name(item).rsplit(".", 1)[-1] for item in node.decorator_list}
        & architecture
    }
    helper_nodes = {
        node.name: node
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and not (
            {_decorator_name(item).rsplit(".", 1)[-1] for item in node.decorator_list}
            & architecture
        )
    }
    declared_payload_names = {
        node.name
        for node in tree.body
        if isinstance(node, ast.ClassDef)
        and any(
            _decorator_name(item).rsplit(".", 1)[-1]
            in {"struct", "packet", "transaction"}
            for item in node.decorator_list
        )
    }
    unavailable_payload_names = declared_payload_names - set(payloads)
    reachable_helpers: set[str] | None = None
    if reachable_only or defer_unbound_unreachable:
        if entry is None or entry not in architecture_nodes:
            raise QueueFrontendError(
                "ACPY-HELPER-001: reachable helper filtering requires an entry"
            )
        reachable_architecture: set[str] = set()
        reachable_helpers = set()
        pending_architecture = [entry]
        while pending_architecture:
            name = pending_architecture.pop()
            if name in reachable_architecture:
                continue
            reachable_architecture.add(name)
            for item in ast.walk(architecture_nodes[name]):
                if not isinstance(item, ast.Name) or not isinstance(item.ctx, ast.Load):
                    continue
                if item.id in architecture_nodes:
                    pending_architecture.append(item.id)
                if item.id in helper_nodes:
                    reachable_helpers.add(item.id)
        pending_helpers = list(reachable_helpers)
        while pending_helpers:
            name = pending_helpers.pop()
            for item in ast.walk(helper_nodes[name]):
                if (
                    isinstance(item, ast.Name)
                    and isinstance(item.ctx, ast.Load)
                    and item.id in helper_nodes
                    and item.id not in reachable_helpers
                ):
                    reachable_helpers.add(item.id)
                    pending_helpers.append(item.id)
    nodes: dict[str, ast.FunctionDef] = {}
    signatures: dict[
        str, tuple[tuple[tuple[str, ValueType], ...], ValueType, bool]
    ] = {}
    for node in tree.body:
        if not isinstance(node, ast.FunctionDef):
            continue
        if (
            reachable_only
            and reachable_helpers is not None
            and node.name not in reachable_helpers
        ):
            continue
        decorators = {
            _decorator_name(item).rsplit(".", 1)[-1] for item in node.decorator_list
        }
        if decorators & architecture:
            continue
        if (
            defer_unbound_unreachable
            and reachable_helpers is not None
            and node.name not in reachable_helpers
            and any(
                isinstance(item, ast.Name)
                and isinstance(item.ctx, ast.Load)
                and item.id in unavailable_payload_names
                for item in ast.walk(node)
            )
        ):
            continue
        inline_requested = "inline" in decorators
        fully_typed = node.returns is not None and all(
            argument.annotation is not None for argument in node.args.args
        )
        if not fully_typed and not inline_requested:
            continue
        if not fully_typed or decorators - {"inline"}:
            raise QueueFrontendError(
                f"ACPY-HELPER-002: helper {node.name!r} requires only @ac.inline "
                "and exact parameter/result annotations"
            )
        if (
            node.args.posonlyargs
            or node.args.kwonlyargs
            or node.args.vararg
            or node.args.kwarg
            or node.args.defaults
            or node.args.kw_defaults
        ):
            raise QueueFrontendError(
                f"ACPY-HELPER-002: helper {node.name!r} requires fixed positional parameters"
            )
        if node.name in nodes:
            raise QueueFrontendError(f"ACPY-HELPER-001: duplicate helper {node.name!r}")
        assert node.returns is not None
        nodes[node.name] = node
        try:
            arguments = tuple(
                (argument.arg, _helper_type(argument.annotation, payloads, enums))
                for argument in node.args.args
                if argument.annotation is not None
            )
            result = _helper_type(node.returns, payloads, enums)
        except QueueFrontendError as error:
            if (
                defer_unbound_unreachable
                and reachable_helpers is not None
                and node.name not in reachable_helpers
            ):
                continue
            raise QueueFrontendError(
                f"{error}; helper {node.name!r} has an unavailable annotation"
            ) from error
        signatures[node.name] = (arguments, result, inline_requested)

    graph: dict[str, set[str]] = {name: set() for name in nodes}
    pure_intrinsics = {
        "concat",
        "literal",
        "zero",
        "zext",
        "sext",
        "truncate",
        "insert",
        "matches",
        "popcount",
        "count_leading_zeros",
        "count_trailing_zeros",
        "priority_encode",
        "wrap",
        "saturate",
        "checked",
        "refine",
        "onehot_enum",
        "match_enum",
    }

    class Rewrite(ast.NodeTransformer):
        def __init__(self, values: Mapping[str, ast.expr]) -> None:
            self.values = values

        def visit_Name(self, node: ast.Name) -> ast.expr:
            if isinstance(node.ctx, ast.Load) and node.id in self.values:
                return ast.copy_location(copy.deepcopy(self.values[node.id]), node)
            return node

        def visit_Call(self, node: ast.Call) -> ast.Call:
            node.args = [self.visit(item) for item in node.args]
            node.keywords = [
                ast.keyword(arg=item.arg, value=self.visit(item.value))
                for item in node.keywords
            ]
            if isinstance(node.func, ast.Attribute):
                node.func.value = self.visit(node.func.value)
            return node

    def rewrite(value: ast.expr, environment: Mapping[str, ast.expr]) -> ast.expr:
        result = Rewrite(environment).visit(copy.deepcopy(value))
        assert isinstance(result, ast.expr)
        return ast.fix_missing_locations(result)

    def lower(node: ast.FunctionDef) -> ast.expr:
        body = list(node.body)
        if (
            body
            and isinstance(body[0], ast.Expr)
            and isinstance(body[0].value, ast.Constant)
            and type(body[0].value.value) is str
        ):
            body.pop(0)
        returns = [item for item in ast.walk(node) if isinstance(item, ast.Return)]
        if not body or not isinstance(body[-1], ast.Return) or len(returns) != 1:
            raise QueueFrontendError(
                f"ACPY-HELPER-003: helper {node.name!r} requires one final return"
            )
        returned = body.pop()
        assert isinstance(returned, ast.Return)
        if returned.value is None:
            raise QueueFrontendError(
                f"ACPY-HELPER-003: helper {node.name!r} must return a value"
            )
        environment: dict[str, ast.expr] = {
            item.arg: ast.Name(id=item.arg, ctx=ast.Load()) for item in node.args.args
        }

        def validate_expression(
            expression: ast.expr, values: Mapping[str, ast.expr]
        ) -> None:
            allowed_names = (
                set(nodes)
                | pure_intrinsics
                | set(payloads)
                | set(enums)
                | set(values)
                | {"ac", "agentic_circuit"}
            )
            for item in ast.walk(expression):
                if isinstance(item, ast.Call):
                    call = _decorator_name(item.func).rsplit(".", 1)[-1]
                    if call in nodes:
                        graph[node.name].add(call)
                    elif (
                        call not in pure_intrinsics
                        and call not in payloads
                        and not (
                            isinstance(item.func, ast.Attribute)
                            and item.func.attr
                            in {
                                "all",
                                "any",
                                "count",
                                "fold",
                                "is_one_of",
                                "project",
                                "with_fields",
                                "with_element",
                                "view",
                                "update",
                            }
                        )
                    ):
                        raise QueueFrontendError(
                            f"ACPY-HELPER-005: helper {node.name!r} calls "
                            f"unknown or effectful function {call!r}"
                        )
                if (
                    isinstance(item, ast.Name)
                    and isinstance(item.ctx, ast.Load)
                    and item.id not in allowed_names
                ):
                    raise QueueFrontendError(
                        f"ACPY-HELPER-004: helper {node.name!r} reads undefined "
                        f"or captured name {item.id!r}"
                    )

        def bind(
            target: ast.expr, value: ast.expr, values: dict[str, ast.expr]
        ) -> None:
            if isinstance(target, ast.Name):
                values[target.id] = value
            elif isinstance(target, (ast.Tuple, ast.List)) and all(
                isinstance(item, ast.Name) for item in target.elts
            ):
                for index, item in enumerate(target.elts):
                    assert isinstance(item, ast.Name)
                    values[item.id] = ast.Subscript(
                        value=copy.deepcopy(value),
                        slice=ast.Constant(index),
                        ctx=ast.Load(),
                    )
            else:
                raise QueueFrontendError(
                    f"ACPY-HELPER-003: helper {node.name!r} has unsupported assignment"
                )

        def walk(statements: list[ast.stmt], values: dict[str, ast.expr]) -> None:
            for statement in statements:
                if isinstance(statement, ast.Assign) and len(statement.targets) == 1:
                    value = rewrite(statement.value, values)
                    validate_expression(value, values)
                    bind(statement.targets[0], value, values)
                elif (
                    isinstance(statement, ast.AnnAssign) and statement.value is not None
                ):
                    value = rewrite(statement.value, values)
                    validate_expression(value, values)
                    bind(statement.target, value, values)
                elif isinstance(statement, ast.If):
                    condition = rewrite(statement.test, values)
                    validate_expression(condition, values)
                    true_values, false_values = dict(values), dict(values)
                    walk(statement.body, true_values)
                    walk(statement.orelse, false_values)
                    for name in set(true_values) | set(false_values):
                        if name not in true_values or name not in false_values:
                            raise QueueFrontendError(
                                f"ACPY-HELPER-004: helper {node.name!r} local {name!r} is not defined on every path"
                            )
                        left, right = true_values[name], false_values[name]
                        values[name] = (
                            left
                            if ast.dump(left) == ast.dump(right)
                            else ast.IfExp(
                                test=copy.deepcopy(condition), body=left, orelse=right
                            )
                        )
                else:
                    raise QueueFrontendError(
                        f"ACPY-HELPER-003: helper {node.name!r} supports only assignments and finite if/elif/else"
                    )

        walk(body, environment)
        expression = rewrite(returned.value, environment)
        validate_expression(expression, environment)
        return ast.fix_missing_locations(expression)

    expressions = {name: lower(node) for name, node in nodes.items()}
    active: set[str] = set()
    done: set[str] = set()

    def visit(name: str) -> None:
        if name in active:
            raise QueueFrontendError(
                f"ACPY-HELPER-006: recursive helper involving {name!r} is forbidden"
            )
        if name in done:
            return
        active.add(name)
        for callee in graph[name]:
            visit(callee)
        active.remove(name)
        done.add(name)

    for name in nodes:
        visit(name)
    return tuple(
        PureHelperDefinition(
            name,
            signatures[name][0],
            signatures[name][1],
            expressions[name],
            signatures[name][2],
            source_frame(nodes[name]),
        )
        for name in nodes
    )


def _lambda_value(node: ast.expr) -> tuple[str, ast.expr]:
    if not isinstance(node, ast.Lambda) or len(node.args.args) != 1:
        raise QueueFrontendError("ACPY-QUEUE-003: apply requires a one-argument lambda")
    return node.args.args[0].arg, node.body


def _is_none_return(statement: ast.stmt) -> bool:
    return isinstance(statement, ast.Return) and (
        statement.value is None
        or (isinstance(statement.value, ast.Constant) and statement.value.value is None)
    )


def _extract_conditional_effect_guard(
    body: list[ast.stmt],
    parameter_names: tuple[str, ...],
    has_value_return: bool,
    *,
    capture_conditions: bool = False,
) -> tuple[list[ast.stmt], ast.expr | None]:
    early_returns = [
        (index, statement)
        for index, statement in enumerate(body)
        if isinstance(statement, ast.If)
        and not statement.orelse
        and len(statement.body) == 1
        and _is_none_return(statement.body[0])
    ]
    if not early_returns:
        return body, None
    if has_value_return:
        raise QueueFrontendError(
            "ACPY-RULE-010: conditional-effect early return is currently outputless"
        )
    indices = [index for index, _ in early_returns]
    early_return_indices = set(indices)
    for index, statement in enumerate(body[: indices[-1] + 1]):
        if index in early_return_indices:
            continue
        if not isinstance(statement, ast.Assign) or len(statement.targets) != 1:
            raise QueueFrontendError(
                "ACPY-RULE-010: only pure local bindings may appear between "
                "early-return guards"
            )
        target = statement.targets[0]
        if isinstance(target, ast.Subscript) or (
            isinstance(target, ast.Name) and target.id in parameter_names
        ):
            raise QueueFrontendError(
                "ACPY-RULE-010: early-return guards must precede state effects"
            )

    def continuing_condition(statement: ast.If) -> ast.expr:
        if isinstance(statement.test, ast.UnaryOp) and isinstance(
            statement.test.op, ast.Not
        ):
            return copy.deepcopy(statement.test.operand)
        return ast.UnaryOp(op=ast.Not(), operand=copy.deepcopy(statement.test))

    conditions: list[ast.expr] = []
    used_names = {
        node.id
        for statement in body
        for node in ast.walk(statement)
        if isinstance(node, ast.Name)
    } | set(parameter_names)
    for ordinal, (index, statement) in enumerate(early_returns):
        assert isinstance(statement, ast.If)
        condition = continuing_condition(statement)
        if not capture_conditions:
            conditions.append(condition)
            continue
        name = f"__ac_effect_guard_{ordinal}"
        while name in used_names:
            name += "_"
        used_names.add(name)
        body[index] = ast.copy_location(
            ast.Assign(
                targets=[ast.Name(id=name, ctx=ast.Store())],
                value=condition,
            ),
            statement,
        )
        conditions.append(ast.Name(id=name, ctx=ast.Load()))
    guard = (
        conditions[0]
        if len(conditions) == 1
        else ast.BoolOp(op=ast.And(), values=conditions)
    )
    if not capture_conditions:
        for index in reversed(indices):
            body.pop(index)
    return body, ast.fix_missing_locations(guard)
