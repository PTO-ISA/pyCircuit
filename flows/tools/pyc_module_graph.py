#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import subprocess
from collections.abc import Iterable, Mapping
from dataclasses import dataclass
from pathlib import Path
from typing import Any

_TOP_RE = re.compile(r"\bpyc\.top\s*=\s*@([A-Za-z_][A-Za-z0-9_\$]*)\b")
_FUNC_RE = re.compile(r"\bfunc\.func\s+@([A-Za-z_][A-Za-z0-9_\$]*)\b")
_SSA_RE = re.compile(r"%[A-Za-z0-9_]+")

_INSTANCE_CALLEE_RE = re.compile(r"\bcallee\s*=\s*@([A-Za-z_][A-Za-z0-9_\$]*)\b")
_INSTANCE_NAME_RE = re.compile(r'\bname\s*=\s*"((?:\\.|[^"\\])*)"')

_ALIAS_RE = re.compile(r"^\s*(%[A-Za-z0-9_]+)\s*=\s*pyc\.alias\s+(%[A-Za-z0-9_]+)\b")


@dataclass(frozen=True)
class ModulePorts:
    arg_names: tuple[str, ...]
    result_names: tuple[str, ...]


@dataclass(frozen=True)
class InstanceRec:
    name: str
    callee: str
    operands: tuple[str, ...]
    results: tuple[str, ...]
    index: int


@dataclass(frozen=True)
class FuncParsed:
    sym: str
    arg_ssa: tuple[str, ...]
    ports: ModulePorts | None
    instances: tuple[InstanceRec, ...]
    alias_of: dict[str, str]
    drivers: dict[str, set[str]]


@dataclass(frozen=True)
class NodeInfo:
    path: tuple[str, ...]
    name: str
    callee: str
    level: int
    expanded: bool


def _ensure_dot() -> None:
    try:
        subprocess.run(["dot", "-V"], check=False, capture_output=True)
    except FileNotFoundError as e:
        raise SystemExit(
            "error: missing Graphviz `dot` executable in PATH.\n"
            "Install (macOS):\n"
            "  brew install graphviz\n"
        ) from e


def _brace_delta(s: str) -> int:
    """Count '{'/'}' outside of quoted strings."""
    delta = 0
    in_str = False
    esc = False
    for ch in s:
        if esc:
            esc = False
            continue
        if in_str and ch == "\\":
            esc = True
            continue
        if ch == '"':
            in_str = not in_str
            continue
        if in_str:
            continue
        if ch == "{":
            delta += 1
        elif ch == "}":
            delta -= 1
    return delta


def _find_matching(s: str, start: int, open_ch: str, close_ch: str) -> int:
    """Return index of matching close_ch for the open_ch at s[start]."""
    if start < 0 or start >= len(s) or s[start] != open_ch:
        raise ValueError("start must point to an opening delimiter")
    depth = 0
    in_str = False
    esc = False
    for i in range(start, len(s)):
        ch = s[i]
        if esc:
            esc = False
            continue
        if in_str and ch == "\\":
            esc = True
            continue
        if ch == '"':
            in_str = not in_str
            continue
        if in_str:
            continue
        if ch == open_ch:
            depth += 1
        elif ch == close_ch:
            depth -= 1
            if depth == 0:
                return i
    raise ValueError("unmatched delimiter")


def _extract_json_array(line: str, key: str) -> list[str] | None:
    i = line.find(key)
    if i < 0:
        return None
    i = line.find("[", i)
    if i < 0:
        return None
    try:
        j = _find_matching(line, i, "[", "]")
    except ValueError:
        return None
    lit = line[i : j + 1]
    try:
        v = json.loads(lit)
    except Exception:  # noqa: BLE001
        return None
    if not isinstance(v, list) or not all(isinstance(x, str) for x in v):
        return None
    return [str(x) for x in v]


def _parse_func_arg_ssa(line: str, sym: str) -> tuple[str, ...]:
    at = line.find(f"@{sym}")
    if at < 0:
        return tuple()
    lpar = line.find("(", at)
    if lpar < 0:
        return tuple()
    try:
        rpar = _find_matching(line, lpar, "(", ")")
    except ValueError:
        return tuple()
    return tuple(_SSA_RE.findall(line[lpar : rpar + 1]))


def _parse_instance_line(line: str, *, index: int) -> InstanceRec | None:
    if "pyc.instance" not in line:
        return None

    before, after = line.split("pyc.instance", 1)

    results: list[str] = []
    if "=" in before:
        lhs = before.split("=", 1)[0]
        results = _SSA_RE.findall(lhs)

    attr_start = after.find("{")
    if attr_start < 0:
        return None

    operands_part = after[:attr_start]
    operands = _SSA_RE.findall(operands_part)

    try:
        attr_end = _find_matching(after, attr_start, "{", "}")
    except ValueError:
        return None
    attrs = after[attr_start + 1 : attr_end]

    m_callee = _INSTANCE_CALLEE_RE.search(attrs)
    if not m_callee:
        return None
    callee = str(m_callee.group(1))

    m_name = _INSTANCE_NAME_RE.search(attrs)
    if m_name:
        try:
            name = json.loads('"' + m_name.group(1) + '"')
        except Exception:  # noqa: BLE001
            name = str(m_name.group(1))
    else:
        name = callee

    return InstanceRec(
        name=str(name),
        callee=callee,
        operands=tuple(operands),
        results=tuple(results),
        index=int(index),
    )


def _sanitize_dot_id(name: str) -> str:
    s = re.sub(r"[^A-Za-z0-9_]", "_", str(name))
    if not s:
        return "n"
    if s[0].isdigit():
        s = "n_" + s
    return s


def _dot_escape(s: str) -> str:
    # Used for label="...". Escape quotes only; keep backslash escapes (e.g. \n) intact.
    return str(s).replace('"', '\\"')


class _DotWriter:
    def __init__(self) -> None:
        self._lines: list[str] = []
        self._indent = 0

    def writeln(self, line: str = "") -> None:
        self._lines.append(("\t" * self._indent) + line)

    def begin(self, line: str) -> None:
        self.writeln(f"{line} {{")
        self._indent += 1

    def end(self) -> None:
        self._indent -= 1
        self.writeln("}")

    def text(self) -> str:
        return "\n".join(self._lines) + "\n"


def _tarjan_scc(nodes: Iterable[str], adj: dict[str, set[str]]) -> list[list[str]]:
    index = 0
    stack: list[str] = []
    on_stack: set[str] = set()
    indices: dict[str, int] = {}
    lowlink: dict[str, int] = {}
    out: list[list[str]] = []

    def strongconnect(v: str) -> None:
        nonlocal index
        indices[v] = index
        lowlink[v] = index
        index += 1
        stack.append(v)
        on_stack.add(v)

        for w in sorted(adj.get(v, set())):
            if w not in indices:
                strongconnect(w)
                lowlink[v] = min(lowlink[v], lowlink[w])
            elif w in on_stack:
                lowlink[v] = min(lowlink[v], indices[w])

        if lowlink[v] == indices[v]:
            comp: list[str] = []
            while True:
                w = stack.pop()
                on_stack.remove(w)
                comp.append(w)
                if w == v:
                    break
            out.append(comp)

    for v in sorted(nodes):
        if v not in indices:
            strongconnect(v)
    return out


def _resolve_alias(v: str, alias_of: Mapping[str, str]) -> str:
    cur = str(v)
    seen: set[str] = set()
    while cur in alias_of and cur not in seen:
        seen.add(cur)
        cur = alias_of[cur]
    return cur


def _resolve_origins(
    v: str,
    *,
    alias_of: Mapping[str, str],
    drivers: Mapping[str, set[str]],
    origin: Mapping[str, tuple[str, int]],
    stack: set[str] | None = None,
) -> set[tuple[str, int]]:
    stack = set() if stack is None else stack
    cur = _resolve_alias(v, alias_of)
    if cur in origin:
        return {origin[cur]}
    if cur in stack:
        return set()
    stack.add(cur)
    out: set[tuple[str, int]] = set()
    for src in sorted(drivers.get(cur, set())):
        out |= _resolve_origins(
            src, alias_of=alias_of, drivers=drivers, origin=origin, stack=stack
        )
    stack.remove(cur)
    return out


def _dedup_instance_names(instances: list[InstanceRec]) -> list[InstanceRec]:
    inst_by_name: dict[str, InstanceRec] = {}
    name_counts: dict[str, int] = {}
    for inst in instances:
        n = inst.name
        name_counts[n] = name_counts.get(n, 0) + 1
        if n in inst_by_name:
            suffix = name_counts[n] - 1
            n2 = f"{n}__{suffix}"
            inst_by_name[n2] = InstanceRec(
                name=n2,
                callee=inst.callee,
                operands=inst.operands,
                results=inst.results,
                index=inst.index,
            )
        else:
            inst_by_name[n] = inst
    return sorted(inst_by_name.values(), key=lambda x: x.index)


def _compute_edges_for_func(
    func: FuncParsed,
    *,
    module_ports: Mapping[str, ModulePorts],
) -> dict[tuple[str, str], dict[str, Any]]:
    origin: dict[str, tuple[str, int]] = {}
    for inst in func.instances:
        for out_idx, ssa in enumerate(inst.results):
            origin[str(ssa)] = (inst.name, int(out_idx))

    inst_by_name = {i.name: i for i in func.instances}

    edges: dict[tuple[str, str], dict[str, Any]] = {}
    for dst in func.instances:
        dst_ports = module_ports.get(dst.callee)
        for in_idx, op in enumerate(dst.operands):
            srcs = _resolve_origins(
                op, alias_of=func.alias_of, drivers=func.drivers, origin=origin
            )
            if not srcs:
                continue
            for src_name, src_out_idx in sorted(srcs):
                if src_name not in inst_by_name:
                    continue
                src_inst = inst_by_name[src_name]

                src_ports = module_ports.get(src_inst.callee)
                src_port = (
                    src_ports.result_names[src_out_idx]
                    if src_ports and 0 <= src_out_idx < len(src_ports.result_names)
                    else f"out{src_out_idx}"
                )
                dst_port = (
                    dst_ports.arg_names[in_idx]
                    if dst_ports and 0 <= in_idx < len(dst_ports.arg_names)
                    else f"in{in_idx}"
                )

                key = (src_name, dst.name)
                ent = edges.setdefault(key, {"count": 0, "maps": set()})
                ent["count"] += 1
                ent["maps"].add(
                    (str(dst_port), str(src_port), int(in_idx), int(src_out_idx))
                )
    return edges


def _parse_all_funcs(
    pyc_path: Path,
) -> tuple[str | None, dict[str, FuncParsed], dict[str, ModulePorts]]:
    top_sym: str | None = None
    module_ports: dict[str, ModulePorts] = {}
    funcs: dict[str, FuncParsed] = {}

    cur_sym: str | None = None
    cur_arg_ssa: tuple[str, ...] = tuple()
    cur_ports: ModulePorts | None = None
    cur_instances: list[InstanceRec] = []
    cur_alias: dict[str, str] = {}
    cur_drivers: dict[str, set[str]] = {}
    depth = 0

    with pyc_path.open("r", encoding="utf-8", errors="replace") as f:
        for raw_line in f:
            line = raw_line.rstrip("\n")

            if top_sym is None:
                mtop = _TOP_RE.search(line)
                if mtop:
                    top_sym = str(mtop.group(1))

            mfunc = _FUNC_RE.search(line)
            if mfunc:
                # Finish previous function if it was unterminated (shouldn't happen in valid MLIR).
                if cur_sym is not None and depth > 0:
                    funcs[cur_sym] = FuncParsed(
                        sym=cur_sym,
                        arg_ssa=cur_arg_ssa,
                        ports=cur_ports,
                        instances=tuple(_dedup_instance_names(cur_instances)),
                        alias_of=cur_alias,
                        drivers=cur_drivers,
                    )

                sym = str(mfunc.group(1))
                arg_names = _extract_json_array(line, "arg_names")
                result_names = _extract_json_array(line, "result_names")
                if arg_names is not None and result_names is not None:
                    cur_ports = ModulePorts(tuple(arg_names), tuple(result_names))
                    module_ports[sym] = cur_ports
                else:
                    cur_ports = None

                cur_sym = sym
                cur_arg_ssa = _parse_func_arg_ssa(line, sym)
                cur_instances = []
                cur_alias = {}
                cur_drivers = {}
                depth = _brace_delta(line)
                continue

            if cur_sym is None:
                continue

            if rec := _parse_instance_line(line, index=len(cur_instances)):
                cur_instances.append(rec)

            if m_alias := _ALIAS_RE.match(line):
                dst, src = str(m_alias.group(1)), str(m_alias.group(2))
                cur_alias[dst] = src

            if line.lstrip().startswith("pyc.assign"):
                ssa = _SSA_RE.findall(line)
                if len(ssa) >= 2:
                    dst, src = ssa[0], ssa[1]
                    cur_drivers.setdefault(dst, set()).add(src)

            depth += _brace_delta(line)
            if depth <= 0 and cur_sym is not None:
                funcs[cur_sym] = FuncParsed(
                    sym=cur_sym,
                    arg_ssa=cur_arg_ssa,
                    ports=cur_ports,
                    instances=tuple(_dedup_instance_names(cur_instances)),
                    alias_of=cur_alias,
                    drivers=cur_drivers,
                )
                cur_sym = None

    return top_sym, funcs, module_ports


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Generate a module-instance connectivity graph from a textual .pyc (MLIR)."
    )
    ap.add_argument("--pyc", required=True, help="Path to textual .pyc file (MLIR)")
    ap.add_argument(
        "--module",
        default="",
        help="Target module symbol (default: module attribute pyc.top)",
    )
    ap.add_argument(
        "--out", required=True, help="Output path (.dot or a Graphviz render like .svg)"
    )

    ap.add_argument(
        "--hierarchical",
        action="store_true",
        help="Show selected submodules as nested clusters",
    )
    ap.add_argument(
        "--expand",
        action="append",
        default=[],
        help="Expand by instance name, instance path, or callee symbol. Repeatable. Example: --expand janus_iex or --expand JanusBccIexTop",
    )
    ap.add_argument(
        "--expand-depth",
        type=int,
        default=1,
        help="Max expansion depth from the target module (hierarchical only)",
    )
    ap.add_argument(
        "--auto-expand-max-instances",
        type=int,
        default=16,
        help="Auto-expand callees with <= N instances (hierarchical only; 0 disables)",
    )
    ap.add_argument(
        "--expand-all",
        action="store_true",
        help="Expand all instances up to --expand-depth (can be huge)",
    )

    ap.add_argument(
        "--edge-label-mode", choices=["ports", "count", "none"], default="ports"
    )
    ap.add_argument("--edge-label-limit", type=int, default=4)
    ap.add_argument("--max-nodes", type=int, default=500)
    ap.add_argument("--max-edges", type=int, default=2000)
    args = ap.parse_args()

    pyc_path = Path(args.pyc).resolve()
    if not pyc_path.is_file():
        raise SystemExit(f"missing --pyc file: {pyc_path}")

    out_svg = Path(args.out).resolve()
    out_svg.parent.mkdir(parents=True, exist_ok=True)
    dot_path = out_svg.with_suffix(".dot")
    json_path = out_svg.with_suffix(".json")

    edge_label_limit = max(0, int(args.edge_label_limit))

    top_sym, funcs, module_ports = _parse_all_funcs(pyc_path)
    target_sym = str(args.module).strip() or (top_sym or "")
    if not target_sym:
        raise SystemExit(
            "error: cannot determine target module (missing --module and pyc.top not found)"
        )
    if target_sym not in funcs:
        raise SystemExit(f"error: target module not found in file: {target_sym}")

    func_edges: dict[str, dict[tuple[str, str], dict[str, Any]]] = {}

    def get_edges(sym: str) -> dict[tuple[str, str], dict[str, Any]]:
        if sym not in func_edges:
            func_edges[sym] = _compute_edges_for_func(
                funcs[sym], module_ports=module_ports
            )
        return func_edges[sym]

    hierarchical = bool(args.hierarchical)
    expand_depth = max(0, int(args.expand_depth)) if hierarchical else 0
    expand_terms = [str(x).strip() for x in (args.expand or []) if str(x).strip()]
    auto_expand_max = max(0, int(args.auto_expand_max_instances)) if hierarchical else 0

    nodes: dict[tuple[str, ...], NodeInfo] = {}
    expanded: set[tuple[str, ...]] = set()
    edges_global: dict[tuple[tuple[str, ...], tuple[str, ...]], dict[str, Any]] = {}

    def should_expand(
        prefix: tuple[str, ...], inst: InstanceRec, *, cur_level: int
    ) -> bool:
        if not hierarchical:
            return False
        if cur_level >= expand_depth:
            return False
        if inst.callee not in funcs:
            return False
        full_path = "/".join(prefix + (inst.name,))
        forced = any(t in (inst.name, inst.callee, full_path) for t in expand_terms)
        if forced:
            return True
        callee_n = len(funcs[inst.callee].instances)
        # Avoid generating empty clusters for leaf modules in recursive mode.
        if callee_n == 0:
            return False
        if args.expand_all:
            return True
        if auto_expand_max <= 0:
            return False
        return 0 < callee_n <= auto_expand_max

    def walk_context(
        sym: str, prefix: tuple[str, ...], *, level: int, mod_stack: tuple[str, ...]
    ) -> None:
        if sym not in funcs:
            return
        if sym in mod_stack:
            return
        f = funcs[sym]

        for inst in f.instances:
            p = prefix + (inst.name,)
            do_expand = should_expand(prefix, inst, cur_level=level)
            nodes[p] = NodeInfo(
                path=p,
                name=inst.name,
                callee=inst.callee,
                level=level + 1,
                expanded=do_expand,
            )
            if do_expand:
                expanded.add(p)
                walk_context(
                    inst.callee, p, level=level + 1, mod_stack=mod_stack + (sym,)
                )

        for (src, dst), ent in get_edges(sym).items():
            sp = prefix + (src,)
            dp = prefix + (dst,)
            if sp not in nodes or dp not in nodes:
                continue
            key = (sp, dp)
            g = edges_global.setdefault(key, {"count": 0, "maps": set()})
            g["count"] += int(ent["count"])
            g["maps"].update(ent["maps"])

    walk_context(target_sym, tuple(), level=0, mod_stack=tuple())

    # Some leaf modules have no sub-instances. Still emit a valid empty graph + JSON
    # so callers (e.g. pycircuit.cli emit) can use a single codepath.

    if len(nodes) > int(args.max_nodes):
        raise SystemExit(
            f"error: instance nodes {len(nodes)} exceeds --max-nodes={args.max_nodes}\n"
            "Hint: lower --auto-expand-max-instances, reduce --expand-depth, or explicitly --expand only the module(s) you care about.\n"
        )

    if len(edges_global) > int(args.max_edges):
        raise SystemExit(
            f"error: edges {len(edges_global)} exceeds --max-edges={args.max_edges}\n"
            "Hint: lower --auto-expand-max-instances, reduce --expand-depth, or explicitly --expand only the module(s) you care about.\n"
        )

    node_keys = sorted(nodes.keys(), key=lambda p: (len(p), "/".join(p)))
    node_ids = ["/".join(p) for p in node_keys]

    # SCCs on the displayed graph.
    adj: dict[str, set[str]] = {n: set() for n in node_ids}
    for sp, dp in edges_global.keys():
        s = "/".join(sp)
        d = "/".join(dp)
        if s in adj and d in adj:
            adj[s].add(d)
    sccs = _tarjan_scc(node_ids, adj)
    multi_sccs = [sorted(c) for c in sccs if len(c) > 1]
    scc_members: set[str] = set()
    for c in multi_sccs:
        scc_members.update(c)
    largest_scc = max((len(c) for c in sccs), default=0)

    # Port usage summary (record nodes stay readable).
    in_ports_used: dict[str, set[str]] = {n: set() for n in node_ids}
    out_ports_used: dict[str, set[str]] = {n: set() for n in node_ids}
    for (sp, dp), ent in edges_global.items():
        s = "/".join(sp)
        d = "/".join(dp)
        for dst_port, src_port, _in_idx, _out_idx in ent["maps"]:
            out_ports_used.setdefault(s, set()).add(str(src_port))
            in_ports_used.setdefault(d, set()).add(str(dst_port))

    def edge_label(ent: dict[str, Any]) -> str:
        cnt = int(ent["count"])
        if args.edge_label_mode == "none":
            return ""
        if args.edge_label_mode == "count":
            return f"n={cnt}"
        maps = sorted(ent["maps"], key=lambda x: (x[0], x[1], x[2], x[3]))
        maps = maps[:edge_label_limit] if edge_label_limit else []
        lines = [f"n={cnt}"]
        for dst_port, src_port, _in_idx, _out_idx in maps:
            lines.append(f"{dst_port} <= {src_port}")
        return "\\n".join(lines)

    # Emit DOT + SVG (Graphviz required).
    dot_id: dict[str, str] = {}
    used: set[str] = set()

    def node_id_for(path: tuple[str, ...]) -> str:
        base = _sanitize_dot_id("__".join(path))
        nid = base
        k = 0
        while nid in used:
            k += 1
            nid = f"{base}__{k}"
        used.add(nid)
        return nid

    w = _DotWriter()
    graph_name = _sanitize_dot_id(f"{target_sym}_module_graph")
    w.begin(f"digraph {graph_name}")
    w.writeln(
        'graph [bgcolor="white" fontname="Helvetica" fontsize="10" '
        f'label="{_dot_escape(f"{target_sym} module graph (pyc.instance connectivity)")}" '
        'labelloc="t" labeljust="c" pad="0.25" rankdir="LR" ranksep="0.55" nodesep="0.2" '
        'splines="polyline" newrank="true"];'
    )
    w.writeln(
        'node [fillcolor="#F7F7F7" fontname="Helvetica" fontsize="8" shape="record" style="filled"];'
    )
    w.writeln('edge [arrowsize="0.6" color="#9E9E9E" fontsize="7"];')

    def emit_node(p: tuple[str, ...]) -> None:
        key = "/".join(p)
        if key not in dot_id:
            dot_id[key] = node_id_for(p)
        nid = dot_id[key]

        n = nodes[p]
        border = "#111111"
        pen = "1.1"
        if key in scc_members:
            border = "#D32F2F"
            pen = "2.0"

        in_cnt = len(in_ports_used.get(key, set()))
        out_cnt = len(out_ports_used.get(key, set()))
        label_mid = _dot_escape(f"{n.name}\\n{n.callee}")
        label = (
            "{<in> "
            + f"in({in_cnt})"
            + "|"
            + label_mid
            + "|<out> "
            + f"out({out_cnt})"
            + "}"
        )
        w.writeln(f'{nid} [label="{label}" color="{border}" penwidth="{pen}"];')

    def cluster_name_for(path: tuple[str, ...]) -> str:
        return "cluster_" + _sanitize_dot_id(target_sym + "__" + "__".join(path))

    def begin_cluster(cluster_name: str, label: str, *, font_size: str) -> None:
        w.begin(f"subgraph {cluster_name}")
        w.writeln(
            f'color="#BDBDBD" style="rounded" fontname="Helvetica" fontsize="{font_size}" '
            f'label="{_dot_escape(label)}" labelloc="t" labeljust="l";'
        )

    def emit_context(sym: str, prefix: tuple[str, ...], *, level: int) -> None:
        f = funcs[sym]
        for inst in f.instances:
            p = prefix + (inst.name,)
            if p not in nodes:
                continue
            if p in expanded:
                begin_cluster(
                    cluster_name_for(p), f"{inst.name}\\n{inst.callee}", font_size="10"
                )
                emit_node(p)
                if level < expand_depth and inst.callee in funcs:
                    emit_context(inst.callee, p, level=level + 1)
                w.end()
            else:
                emit_node(p)

    begin_cluster(
        "cluster_" + _sanitize_dot_id(target_sym), f"{target_sym}", font_size="11"
    )
    emit_context(target_sym, tuple(), level=0)
    w.end()

    for (sp, dp), ent in sorted(
        edges_global.items(), key=lambda x: ("/".join(x[0][0]), "/".join(x[0][1]))
    ):
        src_key = "/".join(sp)
        dst_key = "/".join(dp)
        if src_key not in dot_id or dst_key not in dot_id:
            continue
        lbl = edge_label(ent)
        if lbl:
            w.writeln(
                f'{dot_id[src_key]}:out -> {dot_id[dst_key]}:in [label="{_dot_escape(lbl)}"];'
            )
        else:
            w.writeln(f"{dot_id[src_key]}:out -> {dot_id[dst_key]}:in;")

    w.end()
    dot_path.write_text(w.text(), encoding="utf-8")

    # Render if requested output isn't dot.
    out_ext = out_svg.suffix.lower().lstrip(".")
    if out_ext != "dot":
        _ensure_dot()
        try:
            subprocess.run(
                ["dot", f"-T{out_ext}", str(dot_path), "-o", str(out_svg)],
                check=True,
            )
        except subprocess.CalledProcessError as e:
            raise SystemExit(
                f"error: dot render failed (dot -T{out_ext} ...): {e}"
            ) from e

    nodes_json = []
    for p, n in sorted(nodes.items(), key=lambda x: (x[1].level, "/".join(x[0]))):
        mp = module_ports.get(n.callee)
        nodes_json.append(
            {
                "path": "/".join(p),
                "parent": "/".join(p[:-1]),
                "name": n.name,
                "callee": n.callee,
                "level": int(n.level),
                "expanded": bool(n.expanded),
                "arg_names": list(mp.arg_names) if mp else [],
                "result_names": list(mp.result_names) if mp else [],
            }
        )

    edges_json = []
    for (sp, dp), ent in sorted(
        edges_global.items(), key=lambda x: ("/".join(x[0][0]), "/".join(x[0][1]))
    ):
        maps = sorted(ent["maps"], key=lambda x: (x[0], x[1], x[2], x[3]))
        maps_json = [
            {"dst_port": dp, "src_port": sp, "dst_idx": di, "src_idx": si}
            for dp, sp, di, si in maps[:edge_label_limit]
            if edge_label_limit
        ]
        edges_json.append(
            {
                "src": "/".join(sp),
                "dst": "/".join(dp),
                "count": int(ent["count"]),
                "mappings": maps_json,
            }
        )

    payload = {
        "version": 2,
        "input_pyc": str(pyc_path),
        "module": target_sym,
        "hierarchical": bool(hierarchical),
        "expand_depth": int(expand_depth),
        "expand_terms": expand_terms,
        "auto_expand_max_instances": int(auto_expand_max),
        "nodes": nodes_json,
        "edges": edges_json,
        "summary": {
            "instance_count": len(nodes),
            "edge_count": len(edges_global),
            "scc_count": len(sccs),
            "largest_scc": int(largest_scc),
            "multi_node_scc_count": len(multi_sccs),
        },
        "sccs": [{"size": len(c), "nodes": sorted(c)} for c in multi_sccs],
    }
    json_path.write_text(
        json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8"
    )

    print(str(out_svg))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
