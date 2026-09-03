"""Context-sensitive validation for the portable Python subset."""

from __future__ import annotations

import ast
from dataclasses import dataclass

from ._definitions import DefinitionKind
from ._diagnostics import Diagnostic, DiagnosticBag, SourceSpan
from ._source import DefinitionSite


@dataclass(frozen=True, slots=True)
class ValidationContext:
    definition_kind: DefinitionKind
    static_names: frozenset[str]
    symbolic_names: frozenset[str]
    approved_helpers: frozenset[str]


@dataclass(frozen=True, slots=True)
class ProcessValidationIssue:
    code: str
    message: str
    node: ast.AST


def _span(path: str, node: ast.AST) -> SourceSpan:
    line = getattr(node, "lineno", 1)
    column = getattr(node, "col_offset", 0)
    end_line = getattr(node, "end_lineno", line)
    end_column = getattr(node, "end_col_offset", column) + 1
    return SourceSpan(path, line, column + 1, end_line, end_column)


class _Validator:
    _forbidden_calls = {
        "eval",
        "exec",
        "open",
        "getattr",
        "setattr",
        "compile",
        "__import__",
    }
    _forbidden_expressions = (
        ast.Await,
        ast.Lambda,
        ast.NamedExpr,
        ast.Set,
        ast.SetComp,
        ast.Yield,
        ast.YieldFrom,
    )

    def __init__(
        self, path: str, context: ValidationContext, diagnostics: DiagnosticBag
    ) -> None:
        self._path = path
        self._context = context
        self._diagnostics = diagnostics

    def _error(self, code: str, message: str, node: ast.AST) -> None:
        self._diagnostics.add(
            Diagnostic(
                stage="python-validation",
                code=code,
                severity="error",
                message=message,
                source=_span(self._path, node),
            )
        )

    def _symbolic_operand(self, node: ast.AST) -> ast.Name | None:
        matches = [
            candidate
            for candidate in ast.walk(node)
            if isinstance(candidate, ast.Name)
            and candidate.id in self._context.symbolic_names
        ]
        return min(
            matches, key=lambda item: (item.lineno, item.col_offset), default=None
        )

    def _require_static_control(self, node: ast.expr) -> None:
        operand = self._symbolic_operand(node)
        if operand is not None:
            self._error(
                "ACPY-STATIC-002",
                f"symbolic value {operand.id!r} cannot control Python execution",
                operand,
            )
        self._validate_expression(node)

    def _validate_target(self, node: ast.expr) -> None:
        if isinstance(node, ast.Name):
            return
        if isinstance(node, (ast.Tuple, ast.List)):
            for item in node.elts:
                self._validate_target(item)
            return
        self._error("ACPY-SYNTAX-001", "mutation target is not portable", node)

    def _validate_expression(self, node: ast.AST) -> None:
        for candidate in ast.walk(node):
            if isinstance(candidate, self._forbidden_expressions):
                self._error(
                    "ACPY-SYNTAX-001",
                    f"{type(candidate).__name__} is not supported",
                    candidate,
                )
            elif isinstance(candidate, ast.Call):
                if isinstance(candidate.func, ast.Name):
                    if candidate.func.id in self._forbidden_calls:
                        self._error(
                            "ACPY-SYNTAX-001",
                            f"call to {candidate.func.id!r} is not portable",
                            candidate,
                        )
                elif isinstance(candidate.func, ast.Attribute):
                    self._error(
                        "ACPY-SYNTAX-001",
                        "qualified or reflective call target is not portable",
                        candidate,
                    )
                else:
                    self._error(
                        "ACPY-SYNTAX-001",
                        "dynamic call target is not portable",
                        candidate,
                    )

    def validate_statement(self, statement: ast.stmt) -> None:
        if isinstance(statement, (ast.Assign, ast.AnnAssign)):
            targets = (
                statement.targets
                if isinstance(statement, ast.Assign)
                else [statement.target]
            )
            for target in targets:
                self._validate_target(target)
            value = statement.value
            if value is not None:
                self._validate_expression(value)
            return
        if isinstance(statement, ast.Expr):
            self._validate_expression(statement.value)
            return
        if isinstance(statement, ast.Return):
            if statement.value is not None:
                self._validate_expression(statement.value)
            return
        if isinstance(statement, ast.If):
            self._require_static_control(statement.test)
            self.validate_statements(statement.body)
            self.validate_statements(statement.orelse)
            return
        if isinstance(statement, ast.For):
            self._validate_target(statement.target)
            self._require_static_control(statement.iter)
            self.validate_statements(statement.body)
            self.validate_statements(statement.orelse)
            return
        if isinstance(statement, ast.With):
            valid_scope = all(
                isinstance(item.context_expr, ast.Call)
                and isinstance(item.context_expr.func, ast.Name)
                and item.context_expr.func.id == "scope"
                and item.optional_vars is None
                for item in statement.items
            )
            if not valid_scope:
                self._error(
                    "ACPY-SYNTAX-001",
                    "only with scope(static_name) is supported",
                    statement,
                )
            for item in statement.items:
                self._require_static_control(item.context_expr)
            self.validate_statements(statement.body)
            return
        if isinstance(statement, ast.Assert):
            self._require_static_control(statement.test)
            if statement.msg is not None:
                self._validate_expression(statement.msg)
            return
        if isinstance(statement, ast.Pass):
            return
        self._error(
            "ACPY-SYNTAX-001",
            f"{type(statement).__name__} is not supported in portable definitions",
            statement,
        )

    def validate_statements(self, statements: list[ast.stmt]) -> None:
        for statement in statements:
            self.validate_statement(statement)


def validate_definition(
    site: DefinitionSite,
    context: ValidationContext,
    diagnostics: DiagnosticBag,
) -> None:
    if isinstance(site.node, ast.AsyncFunctionDef):
        diagnostics.add(
            Diagnostic(
                stage="python-validation",
                code="ACPY-SYNTAX-001",
                severity="error",
                message="async definitions are not portable",
                source=site.span,
            )
        )
        return
    if not isinstance(site.node, ast.FunctionDef):
        diagnostics.add(
            Diagnostic(
                stage="python-validation",
                code="ACPY-SYNTAX-001",
                severity="error",
                message="selected definition must be a function",
                source=site.span,
            )
        )
        return
    _Validator(site.span.file, context, diagnostics).validate_statements(site.node.body)


def validate_process_site(site: DefinitionSite) -> tuple[ProcessValidationIssue, ...]:
    """Validate syntax that can never be represented by a portable process CFG."""

    node = site.node
    if isinstance(node, ast.AsyncFunctionDef):
        return (
            ProcessValidationIssue(
                "ACPY-PROCESS-002",
                "Python coroutines are not a process runtime mechanism",
                node,
            ),
        )
    if not isinstance(node, ast.FunctionDef):
        return (
            ProcessValidationIssue(
                "ACPY-PROCESS-002", "a process must decorate a function", node
            ),
        )

    issues: list[ProcessValidationIssue] = []
    for candidate in ast.walk(node):
        if isinstance(candidate, (ast.Yield, ast.YieldFrom, ast.Await)):
            issues.append(
                ProcessValidationIssue(
                    "ACPY-PROCESS-002",
                    "Python generators and await are not process suspension points",
                    candidate,
                )
            )
        elif isinstance(candidate, (ast.Try, ast.Raise)):
            issues.append(
                ProcessValidationIssue(
                    "ACPY-PROCESS-004",
                    "exception-driven process control is not portable",
                    candidate,
                )
            )
    return tuple(
        sorted(
            issues,
            key=lambda issue: (
                getattr(issue.node, "lineno", 0),
                getattr(issue.node, "col_offset", 0),
                issue.code,
            ),
        )
    )
