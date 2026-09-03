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
        pattern=_rx(r"\.(?:eq|lt|select|trunc|zext|sext)\s*\("),
        message="removed method-style wire API",
        hint="use operators/slicing/inference (`==`, `<`, `a if c else b`, slicing) instead",
    ),
    # PYC416 (ban on mux/cond) intentionally omitted:
    # eager=True / V6 cycle-aware authoring still requires mux(); JIT prefers
    # `a if c else b`, but both styles coexist so hygiene must not forbid mux(.
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
        pattern=_rx(r"\.as_unsigned\s*\("),
        message="removed cast helper `.as_unsigned(...)`",
        hint="use signed intent + assignment coercion",
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
