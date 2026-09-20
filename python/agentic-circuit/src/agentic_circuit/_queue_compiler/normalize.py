"""AST normalization passes for the Queue frontend."""

from __future__ import annotations

import ast
import copy
from collections.abc import Mapping

from .._canonical_json import canonical_json_bytes
from .._diagnostics import SourceSpan
from .._static_eval import (
    StaticEnvironment,
    StaticValue,
    evaluate_static,
    static_json_value,
)
from .errors import QueueFrontendError
from .syntax import _decorator_name


def _constantize_expression(
    node: ast.expr,
    argument: str,
    values: Mapping[str, StaticValue],
) -> ast.expr:
    class Constantizer(ast.NodeTransformer):
        def _constant(self, candidate: ast.expr) -> ast.expr | None:
            try:
                value = evaluate_static(candidate, StaticEnvironment(values))
            except ValueError:
                return None
            if value is None or type(value) in {bool, int, float, str}:
                return ast.copy_location(ast.Constant(value=value), candidate)
            return None

        def visit_Name(self, candidate: ast.Name) -> ast.expr:
            if candidate.id == argument:
                return candidate
            return self._constant(candidate) or candidate

        def visit_Attribute(self, candidate: ast.Attribute) -> ast.expr:
            return self._constant(candidate) or self.generic_visit(candidate)

        def visit_BinOp(self, candidate: ast.BinOp) -> ast.expr:
            rewritten = self.generic_visit(candidate)
            assert isinstance(rewritten, ast.expr)
            return self._constant(rewritten) or rewritten

        def visit_UnaryOp(self, candidate: ast.UnaryOp) -> ast.expr:
            rewritten = self.generic_visit(candidate)
            assert isinstance(rewritten, ast.expr)
            return self._constant(rewritten) or rewritten

    result = Constantizer().visit(copy.deepcopy(node))
    assert isinstance(result, ast.expr)
    return ast.fix_missing_locations(result)


def _desugar_nested_rule_captures(
    tree: ast.Module, system: str, entry_kind: str
) -> ast.Module:
    candidates = [
        node
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and node.name == system
        and any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == entry_kind
            for decorator in node.decorator_list
        )
    ]
    if len(candidates) != 1:
        return tree
    function = candidates[0]

    def declared_slot_name(statement: ast.stmt) -> str | None:
        if not (
            isinstance(statement, ast.Assign)
            and len(statement.targets) == 1
            and isinstance(statement.targets[0], ast.Name)
            and isinstance(statement.value, ast.Call)
            and _decorator_name(statement.value.func).rsplit(".", 1)[-1] == "slot"
        ):
            return None
        return statement.targets[0].id

    def declared_table_name(statement: ast.stmt) -> str | None:
        if not (
            isinstance(statement, ast.Assign)
            and len(statement.targets) == 1
            and isinstance(statement.targets[0], ast.Name)
            and isinstance(statement.value, ast.Call)
            and isinstance(statement.value.func, ast.Subscript)
            and _decorator_name(statement.value.func.value).rsplit(".", 1)[-1]
            == "table"
        ):
            return None
        return statement.targets[0].id

    state_order = tuple(
        name
        for statement in function.body
        for name in (
            (
                statement.target.id
                if isinstance(statement, ast.AnnAssign)
                and isinstance(statement.target, ast.Name)
                else declared_slot_name(statement) or declared_table_name(statement)
            ),
        )
        if name is not None
    )
    state_names = set(state_order)
    state_lines = {
        name: statement.lineno
        for statement in function.body
        for name in (
            (
                statement.target.id
                if isinstance(statement, ast.AnnAssign)
                and isinstance(statement.target, ast.Name)
                else declared_slot_name(statement) or declared_table_name(statement)
            ),
        )
        if name is not None
    }
    untyped_module_names = {
        target.id
        for statement in function.body
        if isinstance(statement, ast.Assign)
        for target in statement.targets
        if isinstance(target, ast.Name)
    } - state_names
    module_parameter_names = {argument.arg for argument in function.args.args}
    nested_rules = {
        statement.name: statement
        for statement in function.body
        if isinstance(statement, ast.FunctionDef)
        and any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == "rule"
            for decorator in statement.decorator_list
        )
    }
    if not nested_rules:
        return tree
    if len(nested_rules) != sum(
        isinstance(statement, ast.FunctionDef)
        and any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == "rule"
            for decorator in statement.decorator_list
        )
        for statement in function.body
    ):
        raise QueueFrontendError(
            "ACPY-RULE-015: nested rule names must be unique within one module"
        )

    transformed: list[ast.FunctionDef] = []
    captures_by_rule: dict[str, tuple[str, ...]] = {}
    qualified_names = {name: f"compiler_nested_{system}_{name}" for name in nested_rules}
    existing_names = {
        node.name
        for node in tree.body
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef))
    }
    collisions = sorted(set(qualified_names.values()) & existing_names)
    if collisions:
        raise QueueFrontendError(
            "ACPY-RULE-015: generated nested rule identity collides with "
            f"existing definition {collisions[0]!r}"
        )
    for name, nested in nested_rules.items():
        nonlocals = [
            statement
            for statement in nested.body
            if isinstance(statement, ast.Nonlocal)
        ]
        nested_nonlocals = [
            statement
            for statement in ast.walk(nested)
            if isinstance(statement, ast.Nonlocal) and statement not in nonlocals
        ]
        if nested_nonlocals:
            raise QueueFrontendError(
                "ACPY-RULE-015: nested rule nonlocal declarations must be direct "
                "body statements"
            )
        requested = {
            captured for statement in nonlocals for captured in statement.names
        }
        unknown = sorted(requested - state_names)
        if unknown:
            raise QueueFrontendError(
                "ACPY-RULE-015: nested rule capture must name typed module "
                f"state; unknown capture {unknown[0]!r}"
            )
        parameter_names = {argument.arg for argument in nested.args.args}
        referenced_names = {
            candidate.id
            for candidate in ast.walk(nested)
            if isinstance(candidate, ast.Name)
        }
        referenced_state = referenced_names & state_names
        overlap = sorted(referenced_state & parameter_names)
        if overlap:
            raise QueueFrontendError(
                "ACPY-RULE-015: nested rule state capture cannot shadow parameter "
                f"{overlap[0]!r}"
            )
        referenced_untyped = sorted(referenced_names & untyped_module_names)
        if referenced_untyped:
            raise QueueFrontendError(
                "ACPY-RULE-015: nested rule capture must name typed module "
                f"state; untyped reference {referenced_untyped[0]!r}"
            )
        referenced_inputs = sorted(
            (referenced_names & module_parameter_names) - parameter_names
        )
        if referenced_inputs:
            raise QueueFrontendError(
                "ACPY-RULE-015: nested rule cannot capture module input "
                f"{referenced_inputs[0]!r}"
            )
        if nonlocals and requested != referenced_state:
            mismatch = sorted(requested ^ referenced_state)
            raise QueueFrontendError(
                "ACPY-RULE-015: explicit nonlocal captures must match inferred "
                f"module-state references; mismatch {mismatch[0]!r}"
            )
        captured_state = requested if nonlocals else referenced_state
        late = sorted(
            captured
            for captured in captured_state
            if state_lines[captured] >= nested.lineno
        )
        if late:
            raise QueueFrontendError(
                "ACPY-RULE-015: captured module state must be declared before "
                f"the nested rule; late capture {late[0]!r}"
            )
        captures = tuple(state for state in state_order if state in captured_state)
        for candidate in ast.walk(nested):
            if (
                isinstance(candidate, ast.Call)
                and isinstance(candidate.func, ast.Name)
                and candidate.func.id in nested_rules
            ):
                raise QueueFrontendError(
                    "ACPY-RULE-015: nested rules cannot call or recurse through "
                    "another nested rule"
                )
        lowered = copy.deepcopy(nested)
        lowered.name = qualified_names[name]
        lowered.body = [
            statement
            for statement in lowered.body
            if not isinstance(statement, ast.Nonlocal)
        ]
        lowered.args.args = [
            *(ast.arg(arg=capture, annotation=None) for capture in captures),
            *lowered.args.args,
        ]
        transformed.append(lowered)
        captures_by_rule[name] = captures

    class ValidateNestedRuleUses(ast.NodeVisitor):
        def __init__(self) -> None:
            self.direct_calls: set[str] = set()

        def visit_FunctionDef(self, node: ast.FunctionDef) -> None:
            return None

        def visit_Call(self, node: ast.Call) -> None:
            if isinstance(node.func, ast.Name) and node.func.id in nested_rules:
                self.direct_calls.add(node.func.id)
                for argument in node.args:
                    self.visit(argument)
                for keyword in node.keywords:
                    self.visit(keyword.value)
                return
            self.generic_visit(node)

        def visit_Name(self, node: ast.Name) -> None:
            if node.id in nested_rules:
                raise QueueFrontendError(
                    "ACPY-RULE-015: nested rule identity cannot escape its "
                    f"direct call; invalid reference {node.id!r}"
                )

    uses = ValidateNestedRuleUses()
    for statement in function.body:
        if not (
            isinstance(statement, ast.FunctionDef) and statement.name in nested_rules
        ):
            uses.visit(statement)
    unused = sorted(set(nested_rules) - uses.direct_calls)
    if unused:
        raise QueueFrontendError(
            "ACPY-RULE-015: nested rule must have one or more direct module "
            f"calls; unused rule {unused[0]!r}"
        )

    class RewriteCalls(ast.NodeTransformer):
        def visit_FunctionDef(self, node: ast.FunctionDef) -> ast.AST:
            return node

        def visit_Call(self, node: ast.Call) -> ast.AST:
            rewritten = self.generic_visit(node)
            assert isinstance(rewritten, ast.Call)
            if not isinstance(rewritten.func, ast.Name):
                return rewritten
            captures = captures_by_rule.get(rewritten.func.id)
            if captures is None:
                return rewritten
            rewritten.func.id = qualified_names[rewritten.func.id]
            rewritten.args = [
                *(ast.Name(id=capture, ctx=ast.Load()) for capture in captures),
                *rewritten.args,
            ]
            return rewritten

    rewritten_function = copy.deepcopy(function)
    rewritten_function.body = [
        statement
        for statement in rewritten_function.body
        if not (
            isinstance(statement, ast.FunctionDef) and statement.name in nested_rules
        )
    ]
    rewriter = RewriteCalls()
    rewritten_function.body = [
        rewriter.visit(statement) for statement in rewritten_function.body
    ]
    tree.body = [rewritten_function if node is function else node for node in tree.body]
    tree.body.extend(transformed)
    return ast.fix_missing_locations(tree)


def _normalize_rule_field_assignments(
    statements: list[ast.stmt], *, reserved_names: set[str]
) -> list[ast.stmt]:
    next_index = 0

    def fresh_index_name() -> str:
        nonlocal next_index
        while True:
            name = f"compiler_field_index_{next_index}"
            next_index += 1
            if name not in reserved_names:
                reserved_names.add(name)
                return name

    def normalize(items: list[ast.stmt]) -> list[ast.stmt]:
        def indexed_field_key(statement: ast.stmt) -> tuple[str, str] | None:
            if not (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Attribute)
                and isinstance(statement.targets[0].value, ast.Subscript)
                and isinstance(statement.targets[0].value.value, ast.Name)
                and not isinstance(statement.targets[0].value.slice, ast.Slice)
            ):
                return None
            target = statement.targets[0].value
            return (
                target.value.id,
                ast.dump(target.slice, include_attributes=False),
            )

        direct_keys = {
            key for statement in items if (key := indexed_field_key(statement))
        }
        branch_keys: set[tuple[str, str]] = set()
        for statement in items:
            if not isinstance(statement, ast.If):
                continue
            for candidate in ast.walk(statement):
                key = indexed_field_key(candidate)
                if key is not None:
                    branch_keys.add(key)
        if direct_keys & branch_keys:
            raise QueueFrontendError(
                "ACPY-RULE-011: field updates cannot cross a branch boundary "
                "for the same indexed target"
            )

        normalized: list[ast.stmt] = []
        active_key: tuple[str, str] | None = None
        active_assignment: ast.Assign | None = None
        for statement in items:
            if isinstance(statement, ast.If):
                rewritten = copy.deepcopy(statement)
                rewritten.body = normalize(rewritten.body)
                rewritten.orelse = normalize(rewritten.orelse)
                normalized.append(ast.fix_missing_locations(rewritten))
                active_key = None
                active_assignment = None
                continue
            if isinstance(statement, ast.For):
                rewritten = copy.deepcopy(statement)
                rewritten.body = normalize(rewritten.body)
                rewritten.orelse = normalize(rewritten.orelse)
                normalized.append(ast.fix_missing_locations(rewritten))
                active_key = None
                active_assignment = None
                continue
            if isinstance(statement, ast.AugAssign) and isinstance(
                statement.target, (ast.Attribute, ast.Subscript)
            ):
                raise QueueFrontendError(
                    "ACPY-RULE-002: field and indexed state updates do not "
                    "support augmented assignment"
                )
            if not (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Attribute)
            ):
                normalized.append(statement)
                active_key = None
                active_assignment = None
                continue
            target = statement.targets[0]
            if isinstance(target.value, ast.Name):
                base_name = target.value.id
                updated = ast.Call(
                    func=ast.Attribute(
                        value=ast.Name(id=base_name, ctx=ast.Load()),
                        attr="with_fields",
                        ctx=ast.Load(),
                    ),
                    args=[],
                    keywords=[ast.keyword(arg=target.attr, value=statement.value)],
                )
                rewritten = ast.Assign(
                    targets=[ast.Name(id=base_name, ctx=ast.Store())],
                    value=updated,
                )
                normalized.append(
                    ast.fix_missing_locations(ast.copy_location(rewritten, statement))
                )
                active_key = None
                active_assignment = None
                continue
            if isinstance(target.value, ast.Subscript) and isinstance(
                target.value.value, ast.Name
            ):
                if isinstance(target.value.slice, ast.Slice):
                    raise QueueFrontendError(
                        "ACPY-RULE-002: field assignment does not support a "
                        "slice target"
                    )
                owner = target.value.value.id
                key = (
                    owner,
                    ast.dump(target.value.slice, include_attributes=False),
                )
                if key == active_key and active_assignment is not None:
                    active_assignment.value = ast.fix_missing_locations(
                        ast.copy_location(
                            ast.Call(
                                func=ast.Attribute(
                                    value=active_assignment.value,
                                    attr="with_fields",
                                    ctx=ast.Load(),
                                ),
                                args=[],
                                keywords=[
                                    ast.keyword(
                                        arg=target.attr,
                                        value=copy.deepcopy(statement.value),
                                    )
                                ],
                            ),
                            statement,
                        )
                    )
                    continue
                index_name = fresh_index_name()
                index_assign = ast.Assign(
                    targets=[ast.Name(id=index_name, ctx=ast.Store())],
                    value=copy.deepcopy(target.value.slice),
                )
                indexed_value = ast.Subscript(
                    value=ast.Name(id=owner, ctx=ast.Load()),
                    slice=ast.Name(id=index_name, ctx=ast.Load()),
                    ctx=ast.Load(),
                )
                updated = ast.Call(
                    func=ast.Attribute(
                        value=copy.deepcopy(indexed_value),
                        attr="with_fields",
                        ctx=ast.Load(),
                    ),
                    args=[],
                    keywords=[ast.keyword(arg=target.attr, value=statement.value)],
                )
                state_assign = ast.Assign(
                    targets=[
                        ast.Subscript(
                            value=ast.Name(id=owner, ctx=ast.Load()),
                            slice=ast.Name(id=index_name, ctx=ast.Load()),
                            ctx=ast.Store(),
                        )
                    ],
                    value=updated,
                )
                normalized.extend(
                    ast.fix_missing_locations(ast.copy_location(item, statement))
                    for item in (index_assign, state_assign)
                )
                active_key = key
                active_assignment = state_assign
                continue
            raise QueueFrontendError(
                "ACPY-RULE-002: field assignment target must be one local/state "
                "record or one directly indexed persistent record"
            )
        return normalized

    return normalize(statements)


def _strip_static_assertions(
    function: ast.FunctionDef,
    values: Mapping[str, StaticValue],
    source_path: str,
    definition_locations: Mapping[str, tuple[str, int, int]] | None = None,
    static_assert_locations: (
        Mapping[str, tuple[tuple[str, int, int], ...]] | None
    ) = None,
) -> None:
    """Evaluate direct ``ac.static_assert`` statements and erase them.

    The assertion is an elaboration contract: it is checked only after the
    entry's ``ac.const`` bindings are closed and never reaches verified ACIR.
    """

    definition_location = (definition_locations or {}).get(function.name)
    assertion_locations = (static_assert_locations or {}).get(function.name, ())

    def location(statement: ast.stmt, ordinal: int) -> tuple[str, int, int]:
        if ordinal < len(assertion_locations):
            return assertion_locations[ordinal]
        if definition_location is None:
            return source_path, statement.lineno, statement.col_offset + 1
        path, original_line, _ = definition_location
        return (
            path,
            original_line + statement.lineno - function.lineno,
            statement.col_offset + 1,
        )

    def assertion_error(
        statement: ast.stmt,
        ordinal: int,
        condition_node: ast.expr,
        message: str,
    ) -> QueueFrontendError:
        path, line, column = location(statement, ordinal)
        referenced = sorted(
            {
                item.id
                for item in ast.walk(condition_node)
                if isinstance(item, ast.Name) and item.id in values
            }
        )
        bindings = {name: static_json_value(values[name]) for name in referenced}
        rendered_bindings = canonical_json_bytes(bindings).decode("utf-8")
        expression = ast.unparse(condition_node)
        detail = (
            f"{path}:{line}:{column}: {message}; "
            f"expression={expression!r}; bindings={rendered_bindings}"
        )
        return QueueFrontendError(
            "ACPY-STATIC-003",
            detail,
            SourceSpan(path, line, column, line, column),
        )

    body: list[ast.stmt] = []
    assertion_ordinal = 0
    for statement in function.body:
        call = (
            statement.value
            if isinstance(statement, ast.Expr) and isinstance(statement.value, ast.Call)
            else None
        )
        if (
            call is None
            or _decorator_name(call.func).rsplit(".", 1)[-1] != "static_assert"
        ):
            body.append(statement)
            continue
        current_ordinal = assertion_ordinal
        assertion_ordinal += 1
        if (
            not 1 <= len(call.args) <= 2
            or any(keyword.arg != "message" for keyword in call.keywords)
            or len(call.keywords) > 1
            or (len(call.args) == 2 and call.keywords)
        ):
            raise QueueFrontendError(
                "ACPY-STATIC-003: static_assert requires a condition and optional message"
            )
        message_node = (
            call.args[1]
            if len(call.args) == 2
            else (
                call.keywords[0].value
                if call.keywords
                else ast.Constant("static assertion failed")
            )
        )
        try:
            message = evaluate_static(message_node, StaticEnvironment(values))
        except ValueError as error:
            raise QueueFrontendError(
                "ACPY-STATIC-003: static_assert message must be a static string"
            ) from error
        if type(message) is not str:
            raise QueueFrontendError(
                "ACPY-STATIC-003: static_assert message must be a static string"
            )
        try:
            condition = evaluate_static(call.args[0], StaticEnvironment(values))
        except ValueError as error:
            raise assertion_error(
                statement,
                current_ordinal,
                call.args[0],
                "static_assert condition is not closed",
            ) from error
        if type(condition) is not bool:
            raise assertion_error(
                statement,
                current_ordinal,
                call.args[0],
                "static_assert condition must be bool",
            )
        if not condition:
            raise assertion_error(
                statement,
                current_ordinal,
                call.args[0],
                message,
            )
    function.body = body
    if any(
        isinstance(candidate, ast.Call)
        and _decorator_name(candidate.func).rsplit(".", 1)[-1] == "static_assert"
        for statement in body
        for candidate in ast.walk(statement)
    ):
        raise QueueFrontendError(
            "ACPY-STATIC-003: static_assert must be a direct entry-body statement"
        )
