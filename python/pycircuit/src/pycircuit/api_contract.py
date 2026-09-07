from __future__ import annotations

import ast
import re
from collections.abc import Iterable
from dataclasses import dataclass
from pathlib import Path

from .diagnostics import (
    Diagnostic,
    make_diagnostic,
    snippet_from_file,
    snippet_from_text,
)

# Frontend/backend contract marker stamped into emitted `.pyc` MLIR modules.
#
# This is intentionally versionless: we enforce a single in-repo contract and do
# not support multi-epoch frontend/backend compatibility.
FRONTEND_CONTRACT = "pycircuit"


_REMOVED_CALL_HINTS: dict[str, str] = {
    "eq": "use `lhs == rhs`",
    "lt": "use `lhs < rhs`",
    "cond": "use Python control flow (`if` / `a if cond else b`)",
    "select": "use `true_val if cond else false_val`",
    "trunc": "remove explicit cast and use slicing only when required",
    "zext": "remove explicit cast and rely on width inference",
    "sext": "remove explicit cast and rely on signed inference",
    "compile_design": "use `compile(...)`",
    "template": "use `const`",
    "instance_bind": "use `new(...)`",
    "instance_many": "use `array(...)`",
    "instance_list": "use `array(...)`",
    "instance_vector": "use `array(...)`",
    "instance_map": "use `array(...)`",
    "instance_dict": "use `array(...)`",
    "io_in": "use `inputs(...)`",
    "io_out": "use `outputs(...)`",
    "io_struct_in": "use `inputs(...)`",
    "io_struct_out": "use `outputs(...)`",
    "state_regs": "use `state(...)`",
    "state_struct_regs": "use `state(...)`",
    "pipe_regs": "use `pipe(...)`",
    "declare_inputs": "use `inputs(...)`",
    "declare_outputs": "use `outputs(...)`",
    "declare_state_regs": "use `state(...)`",
    "declare_struct_inputs": "use `inputs(...)`",
    "declare_struct_outputs": "use `outputs(...)`",
    "declare_struct_state_regs": "use `state(...)`",
    "bind_instance_ports": "use `ports(...)`",
    "connect_like": "use `connect(...)`",
    "connect_struct": "use `connect(...)`",
    "jit_inline": "use `function` for inline hardware helpers",
    "as_connector": "remove explicit connector wrapping and pass values directly",
    "debug": "define a standalone `@probe(target=...)` and consume it from the testbench",
    "debug_bundle": "define a standalone `@probe(target=...)` and use `ProbeBuilder.emit(...)`",
    "debug_probe": "define a standalone `@probe(target=...)` and use `ProbeBuilder.emit(...)`",
    "debug_occ": "define a standalone `@probe(target=...)` and use `ProbeBuilder.emit(...)`",
    "probe": "define a standalone `@probe(target=...)` and use `ProbeBuilder.emit(...)`",
}


def removed_call_hint(name: str) -> str | None:
    return _REMOVED_CALL_HINTS.get(str(name))


@dataclass(frozen=True)
class TextRule:
    code: str
    pattern: re.Pattern[str]
    message: str
    hint: str | None = None


def _rx(pat: str) -> re.Pattern[str]:
    return re.compile(pat)


TEXT_RULES: tuple[TextRule, ...] = (
    TextRule(
        code="PYC401",
        pattern=_rx(r"\bfrom\s+pycircuit\s+import[^\n]*\bcompile_design\b"),
        message="removed API `compile_design` is not allowed in pyCircuit",
        hint="import and call `compile(...)`",
    ),
    TextRule(
        code="PYC402",
        pattern=_rx(r"\bpycircuit\.compile_design\b"),
        message="removed API `pycircuit.compile_design` is not allowed in pyCircuit",
        hint="use `pycircuit.compile(...)`",
    ),
    TextRule(
        code="PYC403",
        pattern=_rx(r"\bfrom\s+pycircuit\s+import[^\n]*\btemplate\b"),
        message="removed API `template` is not allowed in pyCircuit",
        hint="use `const`",
    ),
    TextRule(
        code="PYC404",
        pattern=_rx(r"@\s*template\b"),
        message="removed decorator `@template` is not allowed in pyCircuit",
        hint="use `@const`",
    ),
    TextRule(
        code="PYC405",
        pattern=_rx(r"\bjit_inline\b"),
        message="removed API `jit_inline` is not allowed in pyCircuit",
        hint="use `@function`",
    ),
    TextRule(
        code="PYC410",
        pattern=_rx(r"\.instance_bind\s*\("),
        message="removed Circuit API `instance_bind`",
        hint="use `new(...)`",
    ),
    TextRule(
        code="PYC411",
        pattern=_rx(r"\.instance_(?:many|list|vector|map|dict)\s*\("),
        message="removed Circuit collection instance API",
        hint="use `array(...)`",
    ),
    TextRule(
        code="PYC412",
        pattern=_rx(r"\.io_(?:in|out|struct_in|struct_out)\s*\("),
        message="removed Circuit IO API",
        hint="use `inputs(...)`/`outputs(...)`",
    ),
    TextRule(
        code="PYC413",
        pattern=_rx(r"\.state_(?:regs|struct_regs)\s*\("),
        message="removed Circuit state API",
        hint="use `state(...)`",
    ),
    TextRule(
        code="PYC414",
        pattern=_rx(r"\.pipe_regs\s*\("),
        message="removed Circuit pipe API",
        hint="use `pipe(...)`",
    ),
    TextRule(
        code="PYC415",
        pattern=_rx(r"(?!x)x"),
        message="removed method-style Wire API",
    ),
    # PYC416 (ban on mux/cond) intentionally omitted:
    # Direct CycleAware elaboration requires mux(); JIT also accepts it, so
    # hygiene must not forbid mux(.
    # TextRule(
    #     code="PYC416",
    #     pattern=_rx(r"\b(?:mux|cond)\s*\("),
    #     message="removed helper API",
    #     hint="use Python control flow and ternary expressions",
    # ),
    TextRule(
        code="PYC417",
        pattern=_rx(r"\bm\.const\s*\("),
        message="removed explicit const helper call",
        hint="use literals or `u(width, value)` / `s(width, value)`",
    ),
    TextRule(
        code="PYC418",
        pattern=_rx(r"(?!x)x"),
        message="removed Wire cast helper",
    ),
    TextRule(
        code="PYC420",
        pattern=_rx(
            r"\b(?:meta\.)?declare_(?:inputs|outputs|state_regs|struct_inputs|struct_outputs|struct_state_regs)\s*\("
        ),
        message="removed meta.connect declaration API",
        hint="use `inputs(...)`, `outputs(...)`, `state(...)`",
    ),
    TextRule(
        code="PYC421",
        pattern=_rx(r"\b(?:meta\.)?bind_instance_ports\s*\("),
        message="removed meta.connect API `bind_instance_ports`",
        hint="use `ports(...)`",
    ),
    TextRule(
        code="PYC422",
        pattern=_rx(r"\b(?:meta\.)?connect_(?:like|struct)\s*\("),
        message="removed meta.connect compatibility APIs",
        hint="use `connect(...)`",
    ),
    TextRule(
        code="PYC423",
        pattern=_rx(r"\.as_connector\s*\("),
        message="removed explicit connector wrapper `.as_connector(...)`",
        hint="pass Wire/Reg/Signal/int/literal values directly; coercion is implicit at call boundaries",
    ),
    TextRule(
        code="PYC424",
        pattern=_rx(r"\.(?:debug|debug_bundle|debug_probe|debug_occ|probe)\s*\("),
        message="removed Circuit probe/debug API",
        hint="use standalone `@probe(target=...)` definitions",
    ),
)


_REMOVED_WIRE_METHODS = {
    "eq": "PYC415",
    "lt": "PYC415",
    "select": "PYC415",
    "trunc": "PYC415",
    "zext": "PYC415",
    "sext": "PYC415",
    "as_unsigned": "PYC418",
}


_PYC_IMPORT_MODULES = {"pycircuit", "pycircuit.hw", "pycircuit.v6"}


def _symbol_kind(symbol: str | None) -> str:
    leaf = str(symbol or "").rsplit(".", 1)[-1]
    if leaf in {"CycleAwareSignal", "ForwardSignal", "StateSignal"}:
        return "CAS"
    if leaf == "CycleAwareDomain":
        return "DOMAIN"
    if leaf in {"Circuit", "CycleAwareCircuit"}:
        return "CIRCUIT"
    if leaf in {"Reg", "Wire"}:
        return "WIRE"
    return "UNKNOWN"


class _TypedWireMethodVisitor(ast.NodeVisitor):
    def __init__(self, *, path: Path, text: str, stage: str) -> None:
        self.path = path
        self.lines = text.splitlines()
        self.stage = stage
        self.scopes: list[dict[str, str]] = [{}]
        self.canonical_scopes: list[dict[str, str | None]] = [{}]
        self.diagnostics: list[Diagnostic] = []

    def _canonical(self, node: ast.expr) -> str | None:
        if isinstance(node, ast.Name):
            for scope in reversed(self.canonical_scopes):
                if node.id in scope:
                    return scope[node.id]
            return None
        if isinstance(node, ast.Attribute):
            base = self._canonical(node.value)
            if base is not None and base.startswith("pycircuit"):
                return f"{base}.{node.attr}"
        return None

    def visit_Import(self, node: ast.Import) -> None:  # noqa: N802
        for alias in node.names:
            binding = alias.asname or alias.name.split(".", 1)[0]
            canonical = alias.name if alias.asname else alias.name.split(".", 1)[0]
            self.canonical_scopes[-1][binding] = (
                canonical
                if alias.name in _PYC_IMPORT_MODULES
                or alias.name == "pycircuit.structural"
                else None
            )

    def visit_ImportFrom(self, node: ast.ImportFrom) -> None:  # noqa: N802
        for alias in node.names:
            if alias.name == "*":
                continue
            binding = alias.asname or alias.name
            if node.module in _PYC_IMPORT_MODULES:
                self.canonical_scopes[-1][binding] = f"pycircuit.{alias.name}"
            else:
                self.canonical_scopes[-1][binding] = None

    def _annotation_kind(self, annotation: ast.expr | None) -> str:
        if annotation is None:
            return "UNKNOWN"
        if isinstance(annotation, ast.Subscript):
            outer = self._annotation_kind(annotation.value)
            return (
                outer if outer != "UNKNOWN" else self._annotation_kind(annotation.slice)
            )
        if isinstance(annotation, ast.BinOp) and isinstance(annotation.op, ast.BitOr):
            kinds = {
                self._annotation_kind(annotation.left),
                self._annotation_kind(annotation.right),
            }
            kinds.discard("UNKNOWN")
            return kinds.pop() if len(kinds) == 1 else "UNKNOWN"
        if isinstance(annotation, ast.Constant) and isinstance(annotation.value, str):
            value = annotation.value.replace(" ", "").split("[", 1)[0]
            if value.startswith("pycircuit."):
                return _symbol_kind(value)
            for scope in reversed(self.canonical_scopes):
                if value in scope:
                    return _symbol_kind(scope[value])
            return "UNKNOWN"
        return _symbol_kind(self._canonical(annotation))

    def visit_FunctionDef(self, node: ast.FunctionDef) -> None:  # noqa: N802
        self.canonical_scopes[-1][node.name] = None
        names = {
            arg.arg: self._annotation_kind(arg.annotation)
            for arg in (*node.args.posonlyargs, *node.args.args, *node.args.kwonlyargs)
        }
        self.scopes.append(names)
        self.canonical_scopes.append(dict.fromkeys(names))
        for statement in node.body:
            self.visit(statement)
        self.scopes.pop()
        self.canonical_scopes.pop()

    def visit_AsyncFunctionDef(self, node: ast.AsyncFunctionDef) -> None:  # noqa: N802
        self.canonical_scopes[-1][node.name] = None
        names = {
            arg.arg: self._annotation_kind(arg.annotation)
            for arg in (*node.args.posonlyargs, *node.args.args, *node.args.kwonlyargs)
        }
        self.scopes.append(names)
        self.canonical_scopes.append(dict.fromkeys(names))
        for statement in node.body:
            self.visit(statement)
        self.scopes.pop()
        self.canonical_scopes.pop()

    def visit_ClassDef(self, node: ast.ClassDef) -> None:  # noqa: N802
        self.canonical_scopes[-1][node.name] = None
        self.scopes.append({})
        self.canonical_scopes.append({})
        for statement in node.body:
            self.visit(statement)
        self.scopes.pop()
        self.canonical_scopes.pop()

    def visit_AnnAssign(self, node: ast.AnnAssign) -> None:  # noqa: N802
        if isinstance(node.target, ast.Name):
            kind = self._annotation_kind(node.annotation)
            self.scopes[-1][node.target.id] = (
                kind if kind != "UNKNOWN" else self._infer(node.value)
            )
            self.canonical_scopes[-1][node.target.id] = None
        self.generic_visit(node)

    def visit_Assign(self, node: ast.Assign) -> None:  # noqa: N802
        kind = self._infer(node.value)
        for target in node.targets:
            if isinstance(target, ast.Name):
                self.scopes[-1][target.id] = kind
                self.canonical_scopes[-1][target.id] = None
        self.generic_visit(node)

    def _lookup(self, name: str) -> str:
        for scope in reversed(self.scopes):
            if name in scope:
                return scope[name]
        return "UNKNOWN"

    def _bind_target(self, target: ast.expr, kind: str = "UNKNOWN") -> None:
        if isinstance(target, ast.Name):
            self.scopes[-1][target.id] = kind
            self.canonical_scopes[-1][target.id] = None
        elif isinstance(target, ast.Tuple | ast.List):
            for element in target.elts:
                self._bind_target(element, kind)

    def _infer(self, node: ast.expr | None) -> str:
        if node is None:
            return "UNKNOWN"
        if isinstance(node, ast.Name):
            return self._lookup(node.id)
        if isinstance(node, ast.Subscript | ast.UnaryOp):
            return self._infer(
                node.value if isinstance(node, ast.Subscript) else node.operand
            )
        if isinstance(node, ast.BinOp):
            kinds = {self._infer(node.left), self._infer(node.right)}
            return "CAS" if "CAS" in kinds else "WIRE" if "WIRE" in kinds else "UNKNOWN"
        if isinstance(node, ast.Compare):
            return self._infer(node.left)
        if isinstance(node, ast.IfExp):
            left = self._infer(node.body)
            right = self._infer(node.orelse)
            return left if left == right else "UNKNOWN"
        if isinstance(node, ast.Attribute):
            if node.attr == "q":
                return "WIRE"
            if node.attr in _REMOVED_WIRE_METHODS:
                return "UNKNOWN"
            return self._infer(node.value)
        if not isinstance(node, ast.Call):
            return "UNKNOWN"
        canonical = self._canonical(node.func)
        leaf = str(canonical or "").rsplit(".", 1)[-1]
        if leaf in {"cas", "mux", "CycleAwareSignal"}:
            return "CAS"
        if leaf in {"wire_of", "Wire", "Reg"}:
            return "WIRE"
        if isinstance(node.func, ast.Attribute):
            receiver = self._infer(node.func.value)
            if (
                node.func.attr == "as_cas"
                and _symbol_kind(self._canonical(node.func.value)) == "CAS"
            ):
                return "CAS"
            if (
                node.func.attr
                in {"create_const", "create_reset", "create_signal", "signal"}
                and receiver == "DOMAIN"
            ):
                return "CAS"
            if node.func.attr in {"input", "const"} and receiver == "CIRCUIT":
                return "WIRE"
            if self._canonical(node.func) == "pycircuit.structural.mux":
                return "WIRE"
            if receiver in {"CAS", "WIRE"}:
                return receiver
        return "UNKNOWN"

    @staticmethod
    def _join(left: dict[str, str], right: dict[str, str]) -> dict[str, str]:
        return {
            name: left.get(name) if left.get(name) == right.get(name) else "UNKNOWN"
            for name in set(left) | set(right)
        }

    @staticmethod
    def _join_canonical(
        left: dict[str, str | None], right: dict[str, str | None]
    ) -> dict[str, str | None]:
        return {
            name: left.get(name) if left.get(name) == right.get(name) else None
            for name in set(left) | set(right)
        }

    def _visit_branch(
        self,
        statements: list[ast.stmt],
        entry: dict[str, str],
        canonical_entry: dict[str, str | None],
    ) -> tuple[dict[str, str], dict[str, str | None]]:
        self.scopes[-1] = dict(entry)
        self.canonical_scopes[-1] = dict(canonical_entry)
        for statement in statements:
            self.visit(statement)
        return dict(self.scopes[-1]), dict(self.canonical_scopes[-1])

    def visit_If(self, node: ast.If) -> None:  # noqa: N802
        self.visit(node.test)
        entry = dict(self.scopes[-1])
        canonical_entry = dict(self.canonical_scopes[-1])
        body, body_canonical = self._visit_branch(node.body, entry, canonical_entry)
        other, other_canonical = self._visit_branch(node.orelse, entry, canonical_entry)
        self.scopes[-1] = self._join(body, other)
        self.canonical_scopes[-1] = self._join_canonical(
            body_canonical, other_canonical
        )

    def _visit_loop(
        self,
        body: list[ast.stmt],
        orelse: list[ast.stmt],
        target: ast.expr | None = None,
    ) -> None:
        entry = dict(self.scopes[-1])
        canonical_entry = dict(self.canonical_scopes[-1])
        self.scopes[-1] = dict(entry)
        self.canonical_scopes[-1] = dict(canonical_entry)
        if target is not None:
            self._bind_target(target)
        body_env, body_canonical = self._visit_branch(
            body, dict(self.scopes[-1]), dict(self.canonical_scopes[-1])
        )
        joined = self._join(entry, body_env)
        joined_canonical = self._join_canonical(canonical_entry, body_canonical)
        else_env, else_canonical = self._visit_branch(orelse, joined, joined_canonical)
        self.scopes[-1] = self._join(joined, else_env)
        self.canonical_scopes[-1] = self._join_canonical(
            joined_canonical, else_canonical
        )

    def visit_Try(self, node: ast.Try) -> None:  # noqa: N802
        entry = dict(self.scopes[-1])
        canonical_entry = dict(self.canonical_scopes[-1])
        body, body_canonical = self._visit_branch(node.body, entry, canonical_entry)
        normal, normal_canonical = self._visit_branch(node.orelse, body, body_canonical)
        states = [(normal, normal_canonical)]
        for handler in node.handlers:
            self.scopes[-1] = dict(entry)
            self.canonical_scopes[-1] = dict(canonical_entry)
            if handler.name:
                self._bind_target(ast.Name(id=handler.name))
            states.append(
                self._visit_branch(
                    handler.body,
                    dict(self.scopes[-1]),
                    dict(self.canonical_scopes[-1]),
                )
            )
        joined, joined_canonical = states[0]
        for state, canonical_state in states[1:]:
            joined = self._join(joined, state)
            joined_canonical = self._join_canonical(joined_canonical, canonical_state)
        self.scopes[-1], self.canonical_scopes[-1] = self._visit_branch(
            node.finalbody, joined, joined_canonical
        )

    def visit_Match(self, node: ast.Match) -> None:  # noqa: N802
        self.visit(node.subject)
        entry = dict(self.scopes[-1])
        canonical_entry = dict(self.canonical_scopes[-1])
        states = [(entry, canonical_entry)]
        for case in node.cases:
            self.scopes[-1] = dict(entry)
            self.canonical_scopes[-1] = dict(canonical_entry)
            for pattern_node in ast.walk(case.pattern):
                name = getattr(pattern_node, "name", None)
                if isinstance(name, str):
                    self._bind_target(ast.Name(id=name))
                rest = getattr(pattern_node, "rest", None)
                if isinstance(rest, str):
                    self._bind_target(ast.Name(id=rest))
            if case.guard is not None:
                self.visit(case.guard)
            states.append(
                self._visit_branch(
                    case.body,
                    dict(self.scopes[-1]),
                    dict(self.canonical_scopes[-1]),
                )
            )
        joined, joined_canonical = states[0]
        for state, canonical_state in states[1:]:
            joined = self._join(joined, state)
            joined_canonical = self._join_canonical(joined_canonical, canonical_state)
        self.scopes[-1] = joined
        self.canonical_scopes[-1] = joined_canonical

    def visit_For(self, node: ast.For) -> None:  # noqa: N802
        self.visit(node.iter)
        self._visit_loop(node.body, node.orelse, node.target)

    def visit_AsyncFor(self, node: ast.AsyncFor) -> None:  # noqa: N802
        self.visit(node.iter)
        self._visit_loop(node.body, node.orelse, node.target)

    def visit_While(self, node: ast.While) -> None:  # noqa: N802
        self.visit(node.test)
        self._visit_loop(node.body, node.orelse)

    def _diagnose_method(self, func: ast.Attribute) -> None:
        self._diagnose_receiver(func.value, func.attr, func)

    def _diagnose_getattr(self, value: ast.expr | None) -> None:
        if (
            isinstance(value, ast.Call)
            and isinstance(value.func, ast.Name)
            and value.func.id == "getattr"
            and len(value.args) >= 2
            and isinstance(value.args[1], ast.Constant)
            and isinstance(value.args[1].value, str)
            and value.args[1].value in _REMOVED_WIRE_METHODS
        ):
            self._diagnose_receiver(value.args[0], value.args[1].value, value)

    def _diagnose_receiver(
        self, receiver: ast.expr, attr: str, location: ast.expr
    ) -> None:
        receiver_kind = self._infer(receiver)
        if receiver_kind == "CAS":
            return
        code = _REMOVED_WIRE_METHODS[attr]
        self.diagnostics.append(
            make_diagnostic(
                code=code,
                stage=self.stage,
                path=str(self.path),
                line=location.lineno,
                col=location.col_offset + 1,
                message=f"removed Wire method `.{attr}()` on {receiver_kind.lower()} receiver",
                hint=removed_call_hint(attr),
                snippet=self.lines[location.lineno - 1],
            )
        )

    def visit_Call(self, node: ast.Call) -> None:  # noqa: N802
        self._diagnose_getattr(node)
        self.generic_visit(node)

    def visit_With(self, node: ast.With) -> None:  # noqa: N802
        for item in node.items:
            self.visit(item.context_expr)
            if item.optional_vars is not None:
                self._bind_target(item.optional_vars)
        for statement in node.body:
            self.visit(statement)

    def visit_AsyncWith(self, node: ast.AsyncWith) -> None:  # noqa: N802
        self.visit_With(node)

    def _visit_comprehension(
        self, generators: list[ast.comprehension], values: list[ast.expr]
    ) -> None:
        self.scopes.append({})
        self.canonical_scopes.append({})
        for generator in generators:
            self.visit(generator.iter)
            self._bind_target(generator.target)
            for condition in generator.ifs:
                self.visit(condition)
        for value in values:
            self.visit(value)
        self.scopes.pop()
        self.canonical_scopes.pop()

    def visit_ListComp(self, node: ast.ListComp) -> None:  # noqa: N802
        self._visit_comprehension(node.generators, [node.elt])

    def visit_SetComp(self, node: ast.SetComp) -> None:  # noqa: N802
        self._visit_comprehension(node.generators, [node.elt])

    def visit_GeneratorExp(self, node: ast.GeneratorExp) -> None:  # noqa: N802
        self._visit_comprehension(node.generators, [node.elt])

    def visit_DictComp(self, node: ast.DictComp) -> None:  # noqa: N802
        self._visit_comprehension(node.generators, [node.key, node.value])

    def visit_Attribute(self, node: ast.Attribute) -> None:  # noqa: N802
        if node.attr in _REMOVED_WIRE_METHODS:
            self._diagnose_method(node)
        self.generic_visit(node)


def _scan_typed_wire_methods(
    *, path: Path, text: str, stage: str, enabled_codes: set[str]
) -> list[Diagnostic]:
    if not enabled_codes & {"PYC415", "PYC418"} or path.suffix != ".py":
        return []
    try:
        tree = ast.parse(text, filename=str(path))
    except SyntaxError:
        return []
    visitor = _TypedWireMethodVisitor(path=path, text=text, stage=stage)
    visitor.visit(tree)
    return [d for d in visitor.diagnostics if d.code in enabled_codes]


@dataclass(frozen=True)
class ScanViolation:
    diagnostic: Diagnostic


def scan_text(
    *,
    path: Path,
    text: str,
    stage: str = "api-contract",
    rules: Iterable[TextRule] = TEXT_RULES,
) -> list[Diagnostic]:
    out: list[Diagnostic] = []
    for line_no, line in enumerate(text.splitlines(), start=1):
        for rule in rules:
            for m in rule.pattern.finditer(line):
                out.append(
                    make_diagnostic(
                        code=rule.code,
                        stage=stage,
                        path=str(path),
                        line=line_no,
                        col=m.start() + 1,
                        message=rule.message,
                        hint=rule.hint,
                        snippet=line.rstrip("\n"),
                    )
                )
    out.extend(
        _scan_typed_wire_methods(
            path=path,
            text=text,
            stage=stage,
            enabled_codes={rule.code for rule in rules},
        )
    )
    return out


def scan_file(
    path: Path, *, stage: str = "api-contract", rules: Iterable[TextRule] = TEXT_RULES
) -> list[Diagnostic]:
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return []
    return scan_text(path=path, text=text, stage=stage, rules=rules)


def nearest_project_root(start: Path) -> Path:
    cur = start.resolve()
    if cur.is_file():
        cur = cur.parent
    for d in [cur, *cur.parents]:
        if (d / ".git").exists() or (d / "pyproject.toml").exists():
            return d
    return cur


def _resolve_relative_import(
    from_file: Path, module: str | None, level: int
) -> Path | None:
    base = from_file.parent
    steps = max(0, int(level) - 1)
    for _ in range(steps):
        base = base.parent
    if module:
        base = base / module.replace(".", "/")
    py = base.with_suffix(".py")
    if py.is_file():
        return py.resolve()
    init = base / "__init__.py"
    if init.is_file():
        return init.resolve()
    return None


def _resolve_absolute_import(project_root: Path, module: str) -> Path | None:
    base = project_root / module.replace(".", "/")
    py = base.with_suffix(".py")
    if py.is_file():
        return py.resolve()
    init = base / "__init__.py"
    if init.is_file():
        return init.resolve()
    return None


def _import_targets(path: Path, *, project_root: Path) -> list[Path]:
    try:
        tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    except Exception:
        return []

    out: list[Path] = []
    for n in ast.walk(tree):
        if isinstance(n, ast.Import):
            for a in n.names:
                p = _resolve_absolute_import(project_root, a.name)
                if p is not None:
                    out.append(p)
            continue
        if isinstance(n, ast.ImportFrom):
            module = n.module
            level = int(n.level or 0)
            if level > 0:
                p = _resolve_relative_import(path, module, level)
                if p is not None:
                    out.append(p)
                continue
            if module:
                p = _resolve_absolute_import(project_root, module)
                if p is not None:
                    out.append(p)
    return out


def collect_local_python_graph(entry: Path, *, project_root: Path) -> list[Path]:
    root = project_root.resolve()
    start = entry.resolve()
    seen: set[Path] = set()
    stack: list[Path] = [start]
    out: list[Path] = []

    while stack:
        cur = stack.pop()
        if cur in seen:
            continue
        seen.add(cur)
        if not cur.is_file() or cur.suffix != ".py":
            continue
        if root not in cur.parents and cur != root:
            continue
        out.append(cur)
        for dep in _import_targets(cur, project_root=root):
            if dep not in seen:
                stack.append(dep)

    return sorted(out)


def removed_call_diagnostic(
    *,
    attr: str,
    path: str | None,
    line: int | None,
    col: int | None,
    source_text: str | None,
    stage: str = "jit",
) -> Diagnostic | None:
    hint = removed_call_hint(attr)
    if hint is None:
        return None
    snippet = None
    if source_text is not None and line is not None:
        snippet = snippet_from_text(source_text, line)
    elif path is not None and line is not None:
        snippet = snippet_from_file(Path(path), line)
    return make_diagnostic(
        code="PYC430",
        stage=stage,
        path=path,
        line=line,
        col=col,
        message=f"removed API `{attr}` is not supported",
        hint=hint,
        snippet=snippet,
    )
