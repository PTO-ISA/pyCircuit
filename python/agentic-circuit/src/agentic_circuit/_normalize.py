"""Source-ordered assignment, call, and SSA normalization."""

from __future__ import annotations

import ast
from dataclasses import dataclass

from ._diagnostics import Diagnostic, DiagnosticBag, RelatedLocation, SourceSpan
from ._frontend import CapturedProgram
from ._naming import StableNameAllocator, StableNameError
from ._resolve import (
    ResolvedCall,
    ResolutionError,
    UnresolvedCall,
    ValueCategory,
    ValueVersion,
    resolve_call,
)
from ._schemas import ComponentSchema, signature_for
from ._static_eval import StaticEnvironment, StaticValue, evaluate_static


@dataclass(frozen=True, slots=True)
class NormalizedScopeRegion:
    key: str
    name: str
    parent: str | None
    call_keys: tuple[str, ...]
    value_names: tuple[str, ...]
    source: SourceSpan


@dataclass(frozen=True, slots=True)
class NormalizedProgram:
    definition: str
    arguments: tuple[ValueVersion, ...]
    values: tuple[ValueVersion, ...]
    calls: tuple[ResolvedCall, ...]
    returns: tuple[ValueVersion, ...]
    diagnostics: tuple[Diagnostic, ...]
    scopes: tuple[NormalizedScopeRegion, ...] = ()

    def value_names(self) -> tuple[str, ...]:
        return tuple(value.name for value in self.values)

    def return_names(self) -> tuple[str, ...]:
        return tuple(value.name for value in self.returns)


def _span(path: str, node: ast.AST) -> SourceSpan:
    return SourceSpan(
        path,
        node.lineno,
        node.col_offset + 1,
        node.end_lineno or node.lineno,
        (node.end_col_offset or node.col_offset) + 1,
    )


def _annotation_category(annotation: ast.expr | None) -> ValueCategory:
    value = annotation.value if isinstance(annotation, ast.Subscript) else annotation
    if isinstance(value, ast.Name):
        return {
            "Static": "static",
            "Flow": "flow",
            "Endpoint": "endpoint",
            "ResourceRef": "resource",
        }.get(value.id, "static")
    return "static"


def _result_category(type_key: str) -> ValueCategory:
    lowered = type_key.lower()
    if "flow" in lowered:
        return "flow"
    if "endpoint" in lowered:
        return "endpoint"
    if "resource" in lowered:
        return "resource"
    return "result"


class _Normalizer:
    def __init__(
        self,
        captured: CapturedProgram,
        definition: str,
        node: ast.FunctionDef | ast.AsyncFunctionDef,
    ) -> None:
        self._captured = captured
        self._definition = definition
        self._node = node
        self._diagnostics = DiagnosticBag()
        self._versions: dict[str, int] = {}
        self._current: dict[str, ValueVersion] = {}
        self._values: list[ValueVersion] = []
        self._calls: list[ResolvedCall] = []
        self._returns: tuple[ValueVersion, ...] = ()
        self._scopes: list[NormalizedScopeRegion | None] = []
        self._scope_stack: list[str] = []
        self._names = StableNameAllocator()
        self._static_values = dict(captured.static_arguments)
        arguments: list[ValueVersion] = []
        for argument in [
            *node.args.posonlyargs,
            *node.args.args,
            *node.args.kwonlyargs,
        ]:
            value = ValueVersion(
                source_name=argument.arg,
                version=0,
                category=_annotation_category(argument.annotation),
                type_key=(
                    ast.unparse(argument.annotation)
                    if argument.annotation
                    else "unknown"
                ),
                producer=None,
            )
            arguments.append(value)
            self._current[argument.arg] = value
        self._arguments = tuple(arguments)

    def _error(
        self,
        code: str,
        message: str,
        node: ast.AST,
        related: tuple[RelatedLocation, ...] = (),
    ) -> None:
        self._diagnostics.add(
            Diagnostic(
                stage="ssa-normalization",
                code=code,
                severity="error",
                message=message,
                source=_span(self._captured.source.path, node),
                related=related,
            )
        )

    def _new_value(
        self,
        source_name: str,
        category: ValueCategory,
        type_key: str,
        producer: str,
        ownership: str = "borrowed",
    ) -> ValueVersion:
        version = self._versions.get(source_name, 0)
        self._versions[source_name] = version + 1
        value = ValueVersion(
            source_name, version, category, type_key, producer, ownership
        )
        self._current[source_name] = value
        self._values.append(value)
        return value

    def _value(self, expression: ast.expr) -> ValueVersion:
        if not isinstance(expression, ast.Name) or expression.id not in self._current:
            raise ResolutionError("ACPY-SYMBOL-001: expression is not a bound value")
        return self._current[expression.id]

    def _static(self, expression: ast.expr | object) -> StaticValue:
        if isinstance(expression, ast.expr):
            return evaluate_static(expression, StaticEnvironment(self._static_values))
        return expression

    def _bound_candidate(self, node: ast.Call, schema: ComponentSchema) -> tuple[
        tuple[tuple[str, ValueVersion], ...],
        tuple[tuple[str, StaticValue], ...],
        str | None,
    ]:
        if any(keyword.arg is None for keyword in node.keywords):
            raise ResolutionError("keyword unpacking is not supported")
        signature = signature_for(schema)
        keyword_values = {
            keyword.arg: keyword.value
            for keyword in node.keywords
            if keyword.arg is not None
        }
        try:
            bound = signature.bind(*node.args, **keyword_values)
        except TypeError as error:
            raise ResolutionError(str(error)) from error
        bound.apply_defaults()
        ports = tuple(
            (port.name, self._value(bound.arguments[port.name]))
            for port in schema.ports
        )
        static_arguments = tuple(
            (parameter.name, self._static(bound.arguments[parameter.name]))
            for parameter in schema.parameters
        )
        instance_name = self._static(bound.arguments["name"])
        if instance_name is not None and type(instance_name) is not str:
            raise ResolutionError("instance name must be a static string")
        return ports, static_arguments, instance_name

    def _target_names(self, target: ast.expr) -> tuple[str, ...]:
        if isinstance(target, ast.Name):
            return (target.id,)
        if isinstance(target, (ast.Tuple, ast.List)) and all(
            isinstance(item, ast.Name) for item in target.elts
        ):
            return tuple(item.id for item in target.elts)
        raise ResolutionError("assignment target must be a name or name tuple")

    def _call(self, target: ast.expr | None, node: ast.Call) -> None:
        if not isinstance(node.func, ast.Name):
            self._error("ACPY-CALL-001", "call target must be a registered name", node)
            return
        candidates = self._captured.registry.candidates(node.func.id)
        viable: list[
            tuple[
                ComponentSchema,
                tuple[tuple[str, ValueVersion], ...],
                tuple[tuple[str, StaticValue], ...],
                str | None,
            ]
        ] = []
        attempts: list[RelatedLocation] = []
        source = _span(self._captured.source.path, node)
        for schema in candidates:
            try:
                ports, static_arguments, explicit_name = self._bound_candidate(
                    node, schema
                )
            except (ResolutionError, ValueError) as error:
                attempts.append(
                    RelatedLocation(
                        f"attempted {schema.identity}: {error}", source, None
                    )
                )
            else:
                viable.append((schema, ports, static_arguments, explicit_name))
                attempts.append(
                    RelatedLocation(
                        f"attempted {schema.identity}: viable", source, None
                    )
                )
        if len(viable) != 1:
            code = "ACPY-CALL-006" if len(viable) > 1 else "ACPY-CALL-003"
            message = (
                f"call {node.func.id!r} is ambiguous"
                if len(viable) > 1
                else f"call {node.func.id!r} has no exact binding"
            )
            self._error(code, message, node, tuple(attempts))
            return

        schema, ports, static_arguments, explicit_name = viable[0]
        if target is None:
            target_names = ()
        else:
            try:
                target_names = self._target_names(target)
            except ResolutionError as error:
                self._error("ACPY-CALL-005", str(error), target)
                return
        if len(target_names) != len(schema.results):
            self._error(
                "ACPY-CALL-005",
                f"{schema.identity} returns {len(schema.results)} values, not {len(target_names)}",
                target,
            )
            return
        entity_key = f"call:{node.lineno}:{node.col_offset + 1}"
        assignment_name = target_names[0] if len(target_names) == 1 else None
        try:
            instance_name = self._names.allocate(
                schema.identity,
                assignment_name if schema.effect_kind == "stateful" else None,
                explicit_name,
                (node.lineno, node.col_offset + 1),
            )
        except StableNameError as error:
            self._error("ACPY-NAME-002", str(error), node)
            return
        result_values = tuple(
            self._new_value(
                target_name,
                _result_category(result.acir_type),
                result.acir_type,
                entity_key,
                result.ownership,
            )
            for target_name, result in zip(target_names, schema.results, strict=True)
        )
        unresolved = UnresolvedCall(
            entity_key=entity_key,
            instance_name=instance_name,
            static_arguments=static_arguments,
            port_values=ports,
            result_values=result_values,
            source=source,
        )
        try:
            self._calls.append(resolve_call(unresolved, schema))
        except ResolutionError as error:
            self._error("ACPY-CALL-004", str(error), node)

    def _assignment(self, statement: ast.Assign) -> None:
        if len(statement.targets) != 1:
            self._error(
                "ACPY-SYNTAX-001", "chained assignment is not supported", statement
            )
            return
        if isinstance(statement.value, ast.Call):
            self._call(statement.targets[0], statement.value)
            return
        try:
            source = self._value(statement.value)
            targets = self._target_names(statement.targets[0])
        except ResolutionError as error:
            self._error("ACPY-SYMBOL-001", str(error), statement)
            return
        if len(targets) != 1:
            self._error(
                "ACPY-SYNTAX-001", "value shape does not match target", statement
            )
            return
        self._new_value(
            targets[0], source.category, source.type_key, source.producer or "bind"
        )

    def _return(self, statement: ast.Return) -> None:
        if statement.value is None:
            self._returns = ()
            return
        expressions = (
            tuple(statement.value.elts)
            if isinstance(statement.value, (ast.Tuple, ast.List))
            else (statement.value,)
        )
        try:
            self._returns = tuple(self._value(expression) for expression in expressions)
        except ResolutionError as error:
            self._error("ACPY-SYMBOL-001", str(error), statement)

    def _with_scope(self, statement: ast.With) -> None:
        valid = (
            len(statement.items) == 1
            and statement.items[0].optional_vars is None
            and isinstance(statement.items[0].context_expr, ast.Call)
            and isinstance(statement.items[0].context_expr.func, ast.Name)
            and statement.items[0].context_expr.func.id == "scope"
            and len(statement.items[0].context_expr.args) == 1
            and not statement.items[0].context_expr.keywords
        )
        if not valid:
            self._error(
                "ACPY-SCOPE-001", "only with scope(static_name) is supported", statement
            )
            return
        context = statement.items[0].context_expr
        assert isinstance(context, ast.Call)
        try:
            name = self._static(context.args[0])
        except ValueError as error:
            self._error("ACPY-SCOPE-001", str(error), context.args[0])
            return
        if type(name) is not str or not name:
            self._error(
                "ACPY-SCOPE-001",
                "scope name must be a non-empty static string",
                context,
            )
            return
        key = f"scope:{statement.lineno}:{statement.col_offset + 1}"
        parent = self._scope_stack[-1] if self._scope_stack else None
        index = len(self._scopes)
        self._scopes.append(None)
        call_start = len(self._calls)
        value_start = len(self._values)
        self._scope_stack.append(key)
        for nested in statement.body:
            self._statement(nested)
        self._scope_stack.pop()
        self._scopes[index] = NormalizedScopeRegion(
            key=key,
            name=name,
            parent=parent,
            call_keys=tuple(call.entity_key for call in self._calls[call_start:]),
            value_names=tuple(value.name for value in self._values[value_start:]),
            source=_span(self._captured.source.path, statement),
        )

    def _statement(self, statement: ast.stmt) -> None:
        if isinstance(statement, ast.Assign):
            self._assignment(statement)
        elif isinstance(statement, ast.Return):
            self._return(statement)
        elif isinstance(statement, ast.With):
            self._with_scope(statement)
        elif isinstance(statement, ast.Expr) and isinstance(statement.value, ast.Call):
            self._call(None, statement.value)
        elif isinstance(statement, ast.Pass):
            return
        else:
            self._error(
                "ACPY-SYNTAX-001",
                f"{type(statement).__name__} normalization is not supported yet",
                statement,
            )

    def run(self) -> NormalizedProgram:
        for statement in self._node.body:
            self._statement(statement)
        return NormalizedProgram(
            definition=self._definition,
            arguments=self._arguments,
            values=tuple(self._values),
            calls=tuple(self._calls),
            returns=self._returns,
            diagnostics=self._diagnostics.freeze(),
            scopes=tuple(scope for scope in self._scopes if scope is not None),
        )


def normalize_program(
    captured: CapturedProgram, *, definition: str | None = None
) -> NormalizedProgram:
    qualified_name = definition or (
        captured.selected_system.qualified_name
        if captured.selected_system is not None
        else ""
    )
    sites = {
        site.qualified_name: site
        for site in captured.source.definitions
        if isinstance(site.node, (ast.FunctionDef, ast.AsyncFunctionDef))
    }
    site = sites.get(qualified_name)
    if site is None or not isinstance(
        site.node, (ast.FunctionDef, ast.AsyncFunctionDef)
    ):
        diagnostic = Diagnostic(
            stage="ssa-normalization",
            code="ACPY-SYMBOL-DEFINITION",
            severity="error",
            message=f"definition {qualified_name!r} is not captured",
        )
        return NormalizedProgram(qualified_name, (), (), (), (), (diagnostic,))
    return _Normalizer(captured, qualified_name, site.node).run()
