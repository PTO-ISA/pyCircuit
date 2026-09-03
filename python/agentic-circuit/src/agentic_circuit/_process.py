"""Immutable process CFG construction from the portable Python subset."""

from __future__ import annotations

import ast
from dataclasses import dataclass, field
from types import MappingProxyType
from typing import Literal, TypeAlias

from ._diagnostics import SourceSpan
from ._resolve import ValueVersion
from ._source import DefinitionSite
from ._validate import validate_process_site


EffectKind: TypeAlias = Literal[
    "pure",
    "queue",
    "resource",
    "storage",
    "event",
    "trace",
    "statistics",
    "suspension",
]
EdgeKind: TypeAlias = Literal["branch", "jump", "suspend", "return"]


class ProcessConstructionError(ValueError):
    def __init__(self, code: str, message: str, source: SourceSpan) -> None:
        self.code = code
        self.message = message
        self.source = source
        super().__init__(f"{code}: {message}")


@dataclass(frozen=True, slots=True)
class EffectDeclaration:
    operation: str
    kind: EffectKind
    suspension: bool = False
    linear_arguments: tuple[int, ...] = ()

    def __post_init__(self) -> None:
        if not self.operation:
            raise ValueError("effect operation must be non-empty")
        if self.kind == "suspension" and not self.suspension:
            raise ValueError("suspension effects must declare suspension=True")
        if self.kind != "suspension" and self.suspension:
            raise ValueError("only suspension effects may suspend")
        if any(
            type(index) is not int or index < 0 for index in self.linear_arguments
        ) or len(self.linear_arguments) != len(set(self.linear_arguments)):
            raise ValueError(
                "linear effect arguments must be unique non-negative indices"
            )


class EffectRegistry:
    def __init__(self, declarations: tuple[EffectDeclaration, ...]) -> None:
        indexed: dict[str, EffectDeclaration] = {}
        for declaration in declarations:
            if declaration.operation in indexed:
                raise ValueError(f"duplicate effect {declaration.operation!r}")
            indexed[declaration.operation] = declaration
        self._declarations = tuple(declarations)
        self._indexed = MappingProxyType(indexed)

    @property
    def declarations(self) -> tuple[EffectDeclaration, ...]:
        return self._declarations

    def find(self, operation: str) -> EffectDeclaration | None:
        return self._indexed.get(operation)


@dataclass(frozen=True, slots=True)
class ProcessAction:
    operation: str
    arguments: tuple[str, ...]
    result: str | None
    effect_kind: EffectKind | None
    source: SourceSpan


@dataclass(frozen=True, slots=True)
class ProcessEdge:
    kind: EdgeKind
    targets: tuple[str, ...]
    condition: str | None = None
    operation: str | None = None
    arguments: tuple[str, ...] = ()


@dataclass(frozen=True, slots=True)
class ProcessEffect:
    operation: str
    kind: EffectKind
    source: SourceSpan


@dataclass(frozen=True, slots=True)
class ProcessBlock:
    name: str
    arguments: tuple[ValueVersion, ...]
    actions: tuple[ProcessAction, ...]
    edge: ProcessEdge


@dataclass(frozen=True, slots=True)
class ProcessProgram:
    name: str
    entry: str
    captures: tuple[ValueVersion, ...]
    blocks: tuple[ProcessBlock, ...]
    effects: tuple[ProcessEffect, ...]


@dataclass(slots=True)
class _MutableBlock:
    name: str
    actions: list[ProcessAction]
    edge: ProcessEdge | None = None
    uses: set[str] = field(default_factory=set)
    definitions: set[str] = field(default_factory=set)


def _span(path: str, node: ast.AST) -> SourceSpan:
    line = getattr(node, "lineno", 1)
    column = getattr(node, "col_offset", 0)
    end_line = getattr(node, "end_lineno", line)
    end_column = getattr(node, "end_col_offset", column)
    return SourceSpan(path, line, column + 1, end_line, end_column + 1)


def _annotation_category(annotation: ast.expr | None) -> str:
    value = annotation.value if isinstance(annotation, ast.Subscript) else annotation
    if isinstance(value, ast.Name):
        return {
            "Static": "static",
            "Flow": "flow",
            "Endpoint": "endpoint",
            "ResourceRef": "resource",
        }.get(value.id, "result")
    return "result"


class _ProcessBuilder:
    def __init__(self, site: DefinitionSite, effects: EffectRegistry) -> None:
        node = site.node
        assert isinstance(node, ast.FunctionDef)
        self._site = site
        self._path = site.span.file
        self._node = node
        self._registry = effects
        self._blocks: list[_MutableBlock] = []
        self._block_names: set[str] = set()
        self._effects: list[ProcessEffect] = []
        self._loop_stack: list[tuple[str, str]] = []
        self._local_order: list[str] = []
        arguments = [
            *node.args.posonlyargs,
            *node.args.args,
            *node.args.kwonlyargs,
        ]
        self._versions = {argument.arg: 0 for argument in arguments}
        self._consumed_linear: set[tuple[str, int]] = set()
        self._current = self._new_block("entry")

    def _error(self, code: str, message: str, node: ast.AST) -> None:
        raise ProcessConstructionError(code, message, _span(self._path, node))

    def _new_block(self, base: str) -> _MutableBlock:
        name = base
        suffix = 2
        while name in self._block_names:
            name = f"{base}_{suffix}"
            suffix += 1
        block = _MutableBlock(name, [])
        self._block_names.add(name)
        self._blocks.append(block)
        return block

    def _finish(self, block: _MutableBlock, edge: ProcessEdge) -> None:
        if block.edge is not None:
            self._error(
                "ACPY-PROCESS-003", "process block already has a terminator", self._node
            )
        block.edge = edge

    @staticmethod
    def _names(nodes: tuple[ast.AST, ...]) -> tuple[str, ...]:
        found = {
            candidate.id
            for node in nodes
            for candidate in ast.walk(node)
            if isinstance(candidate, ast.Name) and isinstance(candidate.ctx, ast.Load)
        }
        return tuple(sorted(found))

    def _record_uses(self, block: _MutableBlock, nodes: tuple[ast.AST, ...]) -> None:
        block.uses.update(
            name for name in self._names(nodes) if name not in block.definitions
        )

    def _record_definition(self, block: _MutableBlock, name: str) -> None:
        block.definitions.add(name)
        if name not in self._local_order:
            self._local_order.append(name)
        self._versions[name] = self._versions.get(name, -1) + 1

    def _call(self, node: ast.Call, result: str | None) -> None:
        if not isinstance(node.func, ast.Name):
            self._error(
                "ACPY-EFFECT-003", "process call target must be a declared name", node
            )
        declaration = self._registry.find(node.func.id)
        if declaration is None:
            self._error(
                "ACPY-EFFECT-003",
                f"operation {node.func.id!r} has no declared process effect",
                node,
            )
        if any(keyword.arg is None for keyword in node.keywords):
            self._error(
                "ACPY-PROCESS-003", "process calls cannot unpack keywords", node
            )
        for index in declaration.linear_arguments:
            if index >= len(node.args) or not isinstance(node.args[index], ast.Name):
                self._error(
                    "ACPY-PROCESS-007",
                    f"linear argument {index} of {node.func.id!r} must be one SSA name",
                    node,
                )
            argument = node.args[index]
            key = (argument.id, self._versions.get(argument.id, 0))
            if key in self._consumed_linear:
                self._error(
                    "ACPY-PROCESS-007",
                    f"linear value {argument.id!r} is consumed more than once",
                    argument,
                )
            self._consumed_linear.add(key)
        self._record_uses(
            self._current,
            (*node.args, *(keyword.value for keyword in node.keywords)),
        )
        arguments = tuple(ast.unparse(argument) for argument in node.args) + tuple(
            f"{keyword.arg}={ast.unparse(keyword.value)}" for keyword in node.keywords
        )
        source = _span(self._path, node)
        self._effects.append(ProcessEffect(node.func.id, declaration.kind, source))
        if declaration.suspension:
            if result is not None:
                self._error(
                    "ACPY-PROCESS-003",
                    "a suspension point cannot produce a Python assignment result",
                    node,
                )
            if node.func.id == "yield_sim":
                target = "entry"
            else:
                target = "resume" if "resume" not in self._block_names else "done"
                continuation = self._new_block(target)
                target = continuation.name
            self._finish(
                self._current,
                ProcessEdge(
                    "suspend",
                    (target,),
                    operation=node.func.id,
                    arguments=arguments,
                ),
            )
            if node.func.id != "yield_sim":
                self._current = continuation
            return
        self._current.actions.append(
            ProcessAction(node.func.id, arguments, result, declaration.kind, source)
        )
        if result is not None:
            self._record_definition(self._current, result)

    def _can_reach_backedge_without_suspension(
        self, statements: list[ast.stmt]
    ) -> bool:
        can_fall_through = True
        for statement in statements:
            if not can_fall_through:
                return False
            if isinstance(statement, ast.Expr) and isinstance(
                statement.value, ast.Call
            ):
                function = statement.value.func
                declaration = (
                    self._registry.find(function.id)
                    if isinstance(function, ast.Name)
                    else None
                )
                if declaration is not None and declaration.suspension:
                    can_fall_through = False
            elif isinstance(statement, ast.If):
                can_fall_through = self._can_reach_backedge_without_suspension(
                    statement.body
                ) or self._can_reach_backedge_without_suspension(statement.orelse)
            elif isinstance(statement, (ast.Return, ast.Break)):
                can_fall_through = False
            elif isinstance(statement, ast.Continue):
                return True
        return can_fall_through

    def _assignment(self, statement: ast.Assign | ast.AnnAssign) -> None:
        targets = (
            statement.targets
            if isinstance(statement, ast.Assign)
            else [statement.target]
        )
        if len(targets) != 1 or not isinstance(targets[0], ast.Name):
            self._error(
                "ACPY-PROCESS-003",
                "process assignment target must be one name",
                statement,
            )
        value = statement.value
        if value is None:
            return
        if isinstance(value, ast.Call):
            self._call(value, targets[0].id)
            return
        self._record_uses(self._current, (value,))
        self._current.actions.append(
            ProcessAction(
                "assign",
                (ast.unparse(value),),
                targets[0].id,
                None,
                _span(self._path, statement),
            )
        )
        self._record_definition(self._current, targets[0].id)

    def _if(self, statement: ast.If) -> None:
        then_block = self._new_block("then")
        else_block = self._new_block("else")
        self._record_uses(self._current, (statement.test,))
        self._finish(
            self._current,
            ProcessEdge(
                "branch",
                (then_block.name, else_block.name),
                condition=ast.unparse(statement.test),
            ),
        )

        self._current = then_block
        self._statements(statement.body)
        then_end = self._current

        self._current = else_block
        self._statements(statement.orelse)
        else_end = self._current

        open_blocks = tuple(
            block for block in (then_end, else_end) if block.edge is None
        )
        if not open_blocks:
            self._current = else_end
            return
        join_block = self._new_block("resume")
        for block in open_blocks:
            self._finish(block, ProcessEdge("jump", (join_block.name,)))
        self._current = join_block

    def _while(self, statement: ast.While) -> None:
        if self._can_reach_backedge_without_suspension(statement.body):
            self._error(
                "ACPY-PROCESS-006",
                "runtime while loop must make progress through suspension",
                statement,
            )
        loop = self._new_block("loop")
        body = self._new_block("loop_body")
        after = self._new_block("after_loop")
        self._finish(self._current, ProcessEdge("jump", (loop.name,)))
        self._record_uses(loop, (statement.test,))
        self._finish(
            loop,
            ProcessEdge(
                "branch",
                (body.name, after.name),
                condition=ast.unparse(statement.test),
            ),
        )
        self._loop_stack.append((loop.name, after.name))
        self._current = body
        self._statements(statement.body)
        if self._current.edge is None:
            self._finish(self._current, ProcessEdge("jump", (loop.name,)))
        self._loop_stack.pop()
        self._current = after
        self._statements(statement.orelse)

    @staticmethod
    def _integer(node: ast.expr) -> int | None:
        if isinstance(node, ast.Constant) and type(node.value) is int:
            return node.value
        if (
            isinstance(node, ast.UnaryOp)
            and isinstance(node.op, ast.USub)
            and isinstance(node.operand, ast.Constant)
            and type(node.operand.value) is int
        ):
            return -node.operand.value
        return None

    def _range_trip_count(self, node: ast.expr) -> int | None:
        if not (
            isinstance(node, ast.Call)
            and isinstance(node.func, ast.Name)
            and node.func.id == "range"
            and not node.keywords
            and 1 <= len(node.args) <= 3
        ):
            return None
        values = [self._integer(argument) for argument in node.args]
        if any(value is None for value in values):
            return None
        exact = [value for value in values if value is not None]
        start, stop, step = (
            (0, exact[0], 1)
            if len(exact) == 1
            else (
                (exact[0], exact[1], 1)
                if len(exact) == 2
                else (exact[0], exact[1], exact[2])
            )
        )
        if step <= 0:
            return None
        return len(range(start, stop, step))

    def _for(self, statement: ast.For) -> None:
        if not isinstance(statement.target, ast.Name):
            self._error(
                "ACPY-PROCESS-003", "runtime for target must be one name", statement
            )
        trip_count = self._range_trip_count(statement.iter)
        if trip_count is None or trip_count > 1_000_000:
            self._error(
                "ACPY-PROCESS-006",
                "runtime for loop requires a finite positive static range",
                statement,
            )
        loop = self._new_block("for_loop")
        body = self._new_block("for_body")
        after = self._new_block("after_for")
        self._finish(self._current, ProcessEdge("jump", (loop.name,)))
        self._record_uses(loop, (statement.iter,))
        self._record_definition(loop, statement.target.id)
        self._finish(
            loop,
            ProcessEdge(
                "branch",
                (body.name, after.name),
                condition=(f"{statement.target.id} in {ast.unparse(statement.iter)}"),
            ),
        )
        self._loop_stack.append((loop.name, after.name))
        self._current = body
        self._statements(statement.body)
        if self._current.edge is None:
            self._finish(self._current, ProcessEdge("jump", (loop.name,)))
        self._loop_stack.pop()
        self._current = after
        self._statements(statement.orelse)

    def _statement(self, statement: ast.stmt) -> None:
        if self._current.edge is not None:
            self._error(
                "ACPY-PROCESS-003", "statement follows a process terminator", statement
            )
        if isinstance(statement, (ast.Assign, ast.AnnAssign)):
            self._assignment(statement)
        elif isinstance(statement, ast.Expr) and isinstance(statement.value, ast.Call):
            self._call(statement.value, None)
        elif isinstance(statement, ast.If):
            self._if(statement)
        elif isinstance(statement, ast.While):
            self._while(statement)
        elif isinstance(statement, ast.For):
            self._for(statement)
        elif isinstance(statement, ast.Return):
            if statement.value is not None and not (
                isinstance(statement.value, ast.Constant)
                and statement.value.value is None
            ):
                self._error(
                    "ACPY-PROCESS-003", "process return cannot carry a value", statement
                )
            self._finish(self._current, ProcessEdge("return", ()))
        elif isinstance(statement, ast.Break):
            if not self._loop_stack:
                self._error("ACPY-PROCESS-003", "break is outside a loop", statement)
            self._finish(self._current, ProcessEdge("jump", (self._loop_stack[-1][1],)))
        elif isinstance(statement, ast.Continue):
            if not self._loop_stack:
                self._error("ACPY-PROCESS-003", "continue is outside a loop", statement)
            self._finish(self._current, ProcessEdge("jump", (self._loop_stack[-1][0],)))
        elif isinstance(statement, ast.Pass):
            return
        else:
            self._error(
                "ACPY-PROCESS-003",
                f"{type(statement).__name__} is not supported in a process",
                statement,
            )

    def _statements(self, statements: list[ast.stmt]) -> None:
        for statement in statements:
            self._statement(statement)

    def build(self) -> ProcessProgram:
        self._statements(self._node.body)
        if self._current.edge is None:
            self._finish(self._current, ProcessEdge("return", ()))
        live_in: dict[str, set[str]] = {block.name: set() for block in self._blocks}
        changed = True
        while changed:
            changed = False
            for block in reversed(self._blocks):
                assert block.edge is not None
                live_out = set().union(
                    *(live_in[target] for target in block.edge.targets)
                )
                updated = block.uses | (live_out - block.definitions)
                if updated != live_in[block.name]:
                    live_in[block.name] = updated
                    changed = True
        local_values = {
            name: ValueVersion(name, 0, "result", "unknown", "process-local")
            for name in self._local_order
        }
        blocks = tuple(
            ProcessBlock(
                block.name,
                tuple(
                    local_values[name]
                    for name in self._local_order
                    if name in live_in[block.name]
                ),
                tuple(block.actions),
                block.edge if block.edge is not None else ProcessEdge("return", ()),
            )
            for block in self._blocks
        )
        names = {block.name for block in blocks}
        if any(
            target not in names for block in blocks for target in block.edge.targets
        ):
            self._error(
                "ACPY-PROCESS-003", "process CFG has an unresolved edge", self._node
            )
        arguments = [
            *self._node.args.posonlyargs,
            *self._node.args.args,
            *self._node.args.kwonlyargs,
        ]
        captures = tuple(
            ValueVersion(
                argument.arg,
                0,
                _annotation_category(argument.annotation),
                (
                    ast.unparse(argument.annotation)
                    if argument.annotation is not None
                    else "unknown"
                ),
                None,
            )
            for argument in arguments
        )
        return ProcessProgram(
            self._site.name, "entry", captures, blocks, tuple(self._effects)
        )


def construct_process(
    definition: DefinitionSite, effects: EffectRegistry
) -> ProcessProgram:
    if "process" not in definition.decorator_names:
        raise ProcessConstructionError(
            "ACPY-PROCESS-001",
            f"definition {definition.qualified_name!r} is not decorated with @process",
            definition.span,
        )
    issues = validate_process_site(definition)
    if issues:
        issue = issues[0]
        raise ProcessConstructionError(
            issue.code, issue.message, _span(definition.span.file, issue.node)
        )
    return _ProcessBuilder(definition, effects).build()
