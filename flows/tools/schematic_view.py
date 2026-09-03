#!/usr/bin/env python3
"""
schematic_view.py — Generate a PDF schematic from a pyCircuit-generated Verilog file.

Usage:
    python schematic_view.py <verilog_file> [-o output.pdf] [--collapse] [--no-constants]

Layout:
    - Input ports on the left
    - Combinational logic levels laid out left-to-right by data dependency
    - Registers (pyc_reg) shown as flip-flop boxes
    - Output ports on the right
"""

from __future__ import annotations

import argparse
import re
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path

import graphviz

# ---------------------------------------------------------------------------
# 1. Verilog parser  (targeted at pyCircuit-generated Verilog)
# ---------------------------------------------------------------------------


@dataclass
class Port:
    name: str
    direction: str  # "input" | "output"
    width: int  # 1 for scalar


@dataclass
class WireDecl:
    name: str
    width: int
    comment: str = ""  # e.g. pyc.name="..."


@dataclass
class Assign:
    dst: str
    expr: str  # raw RHS string
    sources: list[str]  # signal names referenced on RHS


@dataclass
class Instance:
    module_type: str  # "pyc_reg", "pyc_byte_mem", etc.
    inst_name: str
    params: dict[str, str]  # e.g. {"WIDTH": "8"}
    connections: dict[str, str]  # port -> signal


@dataclass
class VerilogModule:
    name: str
    ports: list[Port]
    wires: dict[str, WireDecl]
    assigns: list[Assign]
    instances: list[Instance]


def _parse_width(spec: str) -> int:
    """Parse '[63:0]' → 64, '' → 1."""
    if not spec:
        return 1
    m = re.match(r"\[(\d+):(\d+)\]", spec.strip())
    if m:
        return int(m.group(1)) - int(m.group(2)) + 1
    return 1


# Regex to extract signal identifiers from a Verilog expression.
# Matches valid Verilog identifiers but excludes numeric literals.
_IDENT_RE = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\b")
_NUM_LIT_RE = re.compile(r"^\d+'[bdho]")  # e.g. 64'd0


def _extract_sources(expr: str, known_signals: set[str]) -> list[str]:
    """Extract signal names from an expression, filtering to known signals."""
    srcs = []
    for m in _IDENT_RE.finditer(expr):
        name = m.group(1)
        if name in known_signals and name not in srcs:
            srcs.append(name)
    return srcs


def parse_verilog(path: str) -> VerilogModule:
    """Parse a pyCircuit-generated Verilog file into a VerilogModule."""
    text = Path(path).read_text()

    # --- Module header ---
    mod_m = re.search(r"module\s+(\w+)\s*\(", text)
    mod_name = mod_m.group(1) if mod_m else "unknown"

    # --- Ports ---
    # Find module port block
    port_block_m = re.search(r"module\s+\w+\s*\((.*?)\);", text, re.DOTALL)
    ports: list[Port] = []
    if port_block_m:
        block = port_block_m.group(1)
        for line in block.split("\n"):
            line = line.strip().rstrip(",")
            pm = re.match(r"(input|output)\s*(\[[\d:]+\])?\s*(\w+)", line)
            if pm:
                direction = pm.group(1)
                width = _parse_width(pm.group(2) or "")
                name = pm.group(3)
                ports.append(Port(name=name, direction=direction, width=width))

    # --- Wire declarations ---
    wires: dict[str, WireDecl] = {}
    for wm in re.finditer(r"wire\s+(\[[\d:]+\])?\s*(\w+)\s*;(.*)", text):
        width_str = wm.group(1) or ""
        name = wm.group(2)
        comment = wm.group(3).strip()
        wires[name] = WireDecl(
            name=name, width=_parse_width(width_str), comment=comment
        )

    # Build known signal set (ports + wires)
    known: set[str] = set(wires.keys())
    for p in ports:
        known.add(p.name)

    # --- Assign statements ---
    assigns: list[Assign] = []
    for am in re.finditer(r"assign\s+(\w+)\s*=\s*(.+?);", text):
        dst = am.group(1)
        expr = am.group(2).strip()
        srcs = _extract_sources(expr, known)
        assigns.append(Assign(dst=dst, expr=expr, sources=srcs))

    # --- Module instances ---
    instances: list[Instance] = []
    # Match: module_type #(.P(V), ...) inst_name ( .port(sig), ... );
    # Use nested-paren aware pattern: allow one level of (...) inside #(...)
    inst_re = re.compile(
        r"(\w+)\s+#\(((?:[^()]*|\([^()]*\))*)\)\s+(\w+)\s*\(((?:[^()]*|\([^()]*\))*)\)\s*;",
        re.DOTALL,
    )
    for im in inst_re.finditer(text):
        mod_type = im.group(1)
        params_str = im.group(2)
        inst_name = im.group(3)
        conns_str = im.group(4)

        params = {}
        for pp in re.finditer(r"\.(\w+)\(([^)]*)\)", params_str):
            params[pp.group(1)] = pp.group(2).strip()

        conns = {}
        for cp in re.finditer(r"\.(\w+)\(([^)]*)\)", conns_str):
            conns[cp.group(1)] = cp.group(2).strip()

        instances.append(
            Instance(
                module_type=mod_type,
                inst_name=inst_name,
                params=params,
                connections=conns,
            )
        )

    return VerilogModule(
        name=mod_name,
        ports=ports,
        wires=wires,
        assigns=assigns,
        instances=instances,
    )


# ---------------------------------------------------------------------------
# 2. Build dependency graph & compute topological levels
# ---------------------------------------------------------------------------


@dataclass
class Node:
    name: str
    kind: str  # "input", "output", "const", "comb", "reg", "mem", "passthru"
    label: str  # Display label
    op: str = ""  # e.g. "add", "mux", "eq", "and", "or"
    width: int = 1
    level: int = 0  # topological level (0 = inputs)
    group: str = ""  # pipeline stage group (IF/ID/EX/MEM/WB)
    sources: list[str] = field(default_factory=list)
    targets: list[str] = field(default_factory=list)


def _classify_signal(name: str, expr: str) -> tuple[str, str]:
    """Classify a signal by its assignment expression. Returns (kind, op)."""
    # Constants
    if re.match(r"^\d+'[bdho]\d+$", expr):
        return "const", "const"
    # Operations from comments or name patterns
    for prefix, op in [
        ("pyc_add_", "add"),
        ("pyc_sub_", "sub"),
        ("pyc_mul_", "mul"),
        ("pyc_mux_", "mux"),
        ("pyc_eq_", "eq"),
        ("pyc_ne_", "ne"),
        ("pyc_lt_", "lt"),
        ("pyc_gt_", "gt"),
        ("pyc_le_", "le"),
        ("pyc_ge_", "ge"),
        ("pyc_and_", "and"),
        ("pyc_or_", "or"),
        ("pyc_xor_", "xor"),
        ("pyc_not_", "not"),
        ("pyc_sext_", "sext"),
        ("pyc_zext_", "zext"),
        ("pyc_trunc_", "trunc"),
        ("pyc_extract_", "extract"),
        ("pyc_shli_", "shl"),
        ("pyc_shri_", "shr"),
        ("arith_select_", "mux"),
        ("pyc_comb_", "wire"),
        ("pyc_constant_", "const"),
    ]:
        if name.startswith(prefix):
            return "comb", op
    return "comb", ""


def _infer_group(name: str) -> str:
    """Infer pipeline stage group from signal name prefix."""
    for prefix in ["IF__", "ID__", "EX__", "MEM__", "WB__"]:
        if name.startswith(prefix):
            return prefix.rstrip("_")
    # From pyc.name comments or register names
    for stage in ["ifid__", "idex__", "exmem__", "memwb__"]:
        if name.startswith(stage):
            return stage.rstrip("_").upper()
    if name.startswith("gpr__"):
        return "WB"
    if name.startswith("state__"):
        return "WB"
    return ""


def _short_label(name: str, kind: str, op: str, expr: str = "") -> str:
    """Create a short label for display."""
    if kind == "const":
        # Show the constant value
        m = re.match(r".*?'[bdho](.+)", expr)
        if m:
            return m.group(1)
        return expr if expr else name
    if op in ("add", "sub", "mul"):
        return {"add": "+", "sub": "-", "mul": "*"}.get(op, op)
    if op in ("and", "or", "xor", "not"):
        return {"and": "&", "or": "|", "xor": "^", "not": "~"}.get(op, op)
    if op in ("eq", "ne", "lt", "gt", "le", "ge"):
        return {
            "eq": "==",
            "ne": "!=",
            "lt": "<",
            "gt": ">",
            "le": "<=",
            "ge": ">=",
        }.get(op, op)
    if op == "mux":
        return "MUX"
    if op in ("sext", "zext", "trunc", "extract"):
        return op
    if op in ("shl", "shr"):
        return {"shl": "<<", "shr": ">>"}.get(op, op)
    if op == "wire":
        return "="
    # Named signals — use a shortened version
    # Strip stage prefix and common suffixes for readability
    short = name
    for prefix in ["IF__", "ID__", "EX__", "MEM__", "WB__"]:
        if short.startswith(prefix):
            short = short[len(prefix) :]
            break
    # Truncate long names
    if len(short) > 30:
        short = short[:27] + "..."
    return short


def build_graph(
    mod: VerilogModule,
    collapse_passthru: bool = True,
    no_constants: bool = True,
) -> dict[str, Node]:
    """Build a dependency graph from the parsed Verilog module."""
    nodes: dict[str, Node] = {}

    # Input ports
    for p in mod.ports:
        if p.direction == "input":
            nodes[p.name] = Node(
                name=p.name,
                kind="input",
                label=p.name,
                width=p.width,
                sources=[],
                targets=[],
            )

    # Output ports — we'll add them later connected to their drivers

    # All wire signals
    all_signals: set[str] = set(mod.wires.keys())
    for p in mod.ports:
        all_signals.add(p.name)

    # Process assigns to build nodes
    assign_map: dict[str, Assign] = {}
    for a in mod.assigns:
        assign_map[a.dst] = a

    # Identify pass-through assigns (dst = src, single source, no operation)
    passthru: dict[str, str] = {}  # maps dst -> ultimate source
    if collapse_passthru:
        for a in mod.assigns:
            # Check if this is a simple identity: dst = src (single ident, no op)
            expr = a.expr.strip()
            if re.match(r"^[A-Za-z_]\w*$", expr) and expr in all_signals:
                passthru[a.dst] = expr

        # Resolve chains: a=b, b=c → a→c
        def _resolve(name: str, visited: set[str] | None = None) -> str:
            if visited is None:
                visited = set()
            if name in visited:
                return name
            visited.add(name)
            if name in passthru:
                return _resolve(passthru[name], visited)
            return name

        resolved_passthru: dict[str, str] = {}
        for dst in passthru:
            resolved_passthru[dst] = _resolve(dst)
        passthru = resolved_passthru

    # Helper to resolve a signal through pass-through chains
    def resolve(name: str) -> str:
        return passthru.get(name, name)

    # Build nodes for non-passthru assigns
    for a in mod.assigns:
        dst = a.dst
        if dst in passthru:
            continue

        kind, op = _classify_signal(dst, a.expr)

        # Skip constants if requested
        if no_constants and kind == "const":
            continue

        resolved_srcs = []
        for s in a.sources:
            rs = resolve(s)
            if no_constants and rs in assign_map:
                sk, _ = _classify_signal(rs, assign_map[rs].expr)
                if sk == "const":
                    continue
            if rs not in resolved_srcs:
                resolved_srcs.append(rs)

        group = _infer_group(dst)
        label = _short_label(dst, kind, op, a.expr)

        nodes[dst] = Node(
            name=dst,
            kind=kind,
            label=label,
            op=op,
            width=mod.wires[dst].width if dst in mod.wires else 1,
            group=group,
            sources=resolved_srcs,
            targets=[],
        )

    # Build nodes for register instances
    for inst in mod.instances:
        if inst.module_type == "pyc_reg":
            q_sig = inst.connections.get("q", "")
            d_sig = resolve(inst.connections.get("d", ""))
            en_sig = resolve(inst.connections.get("en", ""))
            width = inst.params.get("WIDTH", "1")

            # Find what the Q output drives (its resolved name)
            rq = resolve(q_sig) if q_sig in passthru else q_sig
            label = rq
            for prefix in ["pyc_reg_"]:
                if label.startswith(prefix):
                    # Try to find a named alias
                    for dst, src in passthru.items():
                        if src == q_sig and not dst.startswith("pyc_"):
                            label = dst
                            break
                    break
            # Shorten
            for prefix in ["IF__", "ID__", "EX__", "MEM__", "WB__"]:
                if label.startswith(prefix):
                    label = label[len(prefix) :]
                    break
            if len(label) > 25:
                label = label[:22] + "..."

            srcs = []
            if d_sig:
                srcs.append(d_sig)
            if en_sig and en_sig != d_sig:
                srcs.append(en_sig)

            node_name = q_sig
            nodes[node_name] = Node(
                name=node_name,
                kind="reg",
                label=f"REG\\n{label}\\nW={width}",
                op="reg",
                width=int(width),
                group=_infer_group(q_sig) or _infer_group(label),
                sources=srcs,
                targets=[],
            )

        elif inst.module_type == "pyc_byte_mem":
            rdata = inst.connections.get("rdata", "")
            raddr = resolve(inst.connections.get("raddr", ""))
            waddr = resolve(inst.connections.get("waddr", ""))
            wdata = resolve(inst.connections.get("wdata", ""))
            wvalid = resolve(inst.connections.get("wvalid", ""))
            depth = inst.params.get("DEPTH", "?")

            srcs = [s for s in [raddr, waddr, wdata, wvalid] if s]
            nodes[rdata] = Node(
                name=rdata,
                kind="mem",
                label=f"MEM\\n{inst.inst_name}\\nD={depth}",
                op="mem",
                width=64,
                group="MEM",
                sources=srcs,
                targets=[],
            )

    # Output port nodes
    for p in mod.ports:
        if p.direction == "output":
            # Find what drives this output
            driver = resolve(p.name)
            if driver == p.name and p.name in assign_map:
                a = assign_map[p.name]
                srcs = [resolve(s) for s in a.sources]
            else:
                srcs = [driver] if driver != p.name else []

            nodes[f"out_{p.name}"] = Node(
                name=f"out_{p.name}",
                kind="output",
                label=p.name,
                width=p.width,
                sources=srcs,
                targets=[],
            )

    # --- Compute topological levels ---
    # Registers and memories are *sources* in the combinational graph.
    # Their D inputs are "feedback" edges that should NOT affect level.
    # We compute levels only over the combinational forward edges.

    # Separate forward sources (for level computation) from feedback sources
    # (reg/mem D-input edges, shown as dashed in the schematic).
    fwd_sources: dict[str, list[str]] = {}  # name -> sources for level computation
    fb_edges: list[tuple[str, str]] = []  # (src, dst) feedback edges

    for n in nodes.values():
        if n.kind in ("reg", "mem"):
            # Reg/mem nodes are sources — D/write inputs are feedback
            fwd_sources[n.name] = []
            for src in n.sources:
                if src in nodes:
                    fb_edges.append((src, n.name))
        else:
            fwd_sources[n.name] = [s for s in n.sources if s in nodes]

    # Build forward targets
    fwd_targets: dict[str, list[str]] = defaultdict(list)
    for name, srcs in fwd_sources.items():
        for src in srcs:
            fwd_targets[src].append(name)

    # Also store all targets (including feedback) for edge drawing
    for n in nodes.values():
        n.targets = []
    for n in nodes.values():
        for src in n.sources:
            if src in nodes:
                nodes[src].targets.append(n.name)

    # BFS topological sort on forward edges only
    in_degree: dict[str, int] = {name: len(srcs) for name, srcs in fwd_sources.items()}

    queue: list[str] = [name for name, deg in in_degree.items() if deg == 0]
    while queue:
        next_queue: list[str] = []
        for name in queue:
            node = nodes[name]
            for tgt in fwd_targets.get(name, []):
                if tgt in nodes:
                    nodes[tgt].level = max(nodes[tgt].level, node.level + 1)
                    in_degree[tgt] -= 1
                    if in_degree[tgt] == 0:
                        next_queue.append(tgt)
        queue = next_queue

    # Force outputs to max level + 1
    max_level = max((n.level for n in nodes.values()), default=0)
    for n in nodes.values():
        if n.kind == "output":
            n.level = max_level + 1

    return nodes


# ---------------------------------------------------------------------------
# 3. Render to PDF via graphviz
# ---------------------------------------------------------------------------

# Color scheme for node kinds
_COLORS = {
    "input": ("#E3F2FD", "#1565C0"),  # light blue bg, dark blue border
    "output": ("#FFF3E0", "#E65100"),  # light orange bg, dark orange border
    "reg": ("#E8F5E9", "#2E7D32"),  # light green bg, dark green border
    "mem": ("#F3E5F5", "#6A1B9A"),  # light purple bg, dark purple border
    "comb": ("#FAFAFA", "#616161"),  # light gray bg, dark gray border
    "const": ("#ECEFF1", "#90A4AE"),  # blue gray
}

# Per-operation color overrides (fill, border)
_OP_COLORS = {
    # Arithmetic — blue
    "add": ("#BBDEFB", "#1565C0"),
    "sub": ("#BBDEFB", "#1565C0"),
    "mul": ("#BBDEFB", "#0D47A1"),
    # Logic gates — teal
    "and": ("#B2DFDB", "#00695C"),
    "or": ("#B2DFDB", "#00695C"),
    "xor": ("#B2DFDB", "#00695C"),
    "not": ("#B2DFDB", "#004D40"),
    # Comparators — pink
    "eq": ("#F8BBD0", "#AD1457"),
    "ne": ("#F8BBD0", "#AD1457"),
    "lt": ("#F8BBD0", "#880E4F"),
    "gt": ("#F8BBD0", "#880E4F"),
    "le": ("#F8BBD0", "#880E4F"),
    "ge": ("#F8BBD0", "#880E4F"),
    # MUX / select — yellow
    "mux": ("#FFF9C4", "#F57F17"),
    # Bit manipulation — light purple
    "sext": ("#E1BEE7", "#7B1FA2"),
    "zext": ("#E1BEE7", "#7B1FA2"),
    "trunc": ("#E1BEE7", "#7B1FA2"),
    "extract": ("#E1BEE7", "#7B1FA2"),
    "shl": ("#E1BEE7", "#6A1B9A"),
    "shr": ("#E1BEE7", "#6A1B9A"),
    # Wire (passthru that survived collapse) — pale gray
    "wire": ("#F5F5F5", "#BDBDBD"),
}

# Shape map
_SHAPES = {
    "input": "house",
    "output": "invhouse",
    "reg": "box3d",
    "mem": "cylinder",
    "comb": "box",
    "const": "plain",
}

# Per-operation shape overrides
_OP_SHAPES = {
    "mux": "diamond",
    "and": "invtrapezium",
    "or": "invtrapezium",
    "xor": "invtrapezium",
    "not": "invtriangle",
    "eq": "hexagon",
    "ne": "hexagon",
    "lt": "hexagon",
    "gt": "hexagon",
    "le": "hexagon",
    "ge": "hexagon",
    "add": "oval",
    "sub": "oval",
    "mul": "oval",
}


def render_schematic(
    nodes: dict[str, Node],
    output_path: str,
    title: str = "",
    max_nodes: int = 2000,
    fmt: str = "pdf",
) -> str:
    """Render the graph to a file (pdf/svg/png). Returns the output path."""
    if len(nodes) > max_nodes:
        print(
            f"ERROR: {len(nodes)} nodes exceeds max_nodes={max_nodes}. "
            f"Rendering aborted — graphviz would be very slow.\n"
            f"Options:\n"
            f"  --max-nodes {len(nodes) + 100}   Override limit\n"
            f"  --collapse --no-constants         Reduce node count (default: on)",
            file=sys.stderr,
        )
        sys.exit(1)

    dot = graphviz.Digraph(
        name=title or "schematic",
        format=fmt,
        engine="dot",
    )
    dot.attr(
        rankdir="LR",
        fontname="Helvetica",
        fontsize="10",
        nodesep="0.15",
        ranksep="0.4",
        splines="polyline",
        bgcolor="white",
        label=title,
        labelloc="t",
        labeljust="c",
        pad="0.3",
    )
    dot.attr("node", fontname="Helvetica", fontsize="8", style="filled")
    dot.attr("edge", fontsize="7", color="#9E9E9E", arrowsize="0.5")

    # Group nodes by level for rank constraints
    levels: dict[int, list[str]] = defaultdict(list)
    for n in nodes.values():
        levels[n.level].append(n.name)

    # Group nodes by pipeline stage
    groups: dict[str, list[str]] = defaultdict(list)
    for n in nodes.values():
        if n.group:
            groups[n.group].append(n.name)

    # Add nodes
    for n in nodes.values():
        # Determine color: op-specific > kind-level > default
        if n.op in _OP_COLORS:
            fill, border = _OP_COLORS[n.op]
        else:
            fill, border = _COLORS.get(n.kind, _COLORS["comb"])

        # Determine shape: op-specific > kind-level > default
        if n.op in _OP_SHAPES:
            shape = _OP_SHAPES[n.op]
        else:
            shape = _SHAPES.get(n.kind, "box")

        dot.node(
            n.name,
            label=n.label,
            shape=shape,
            fillcolor=fill,
            color=border,
            penwidth="1.2",
        )

    # Collect feedback edges as a set for fast lookup
    fb_set: set[tuple[str, str]] = set()
    for n in nodes.values():
        if n.kind in ("reg", "mem"):
            for src in n.sources:
                if src in nodes:
                    fb_set.add((src, n.name))

    # Add edges
    for n in nodes.values():
        for src in n.sources:
            if src in nodes:
                is_fb = (src, n.name) in fb_set
                # Color edge by source kind
                edge_color = "#9E9E9E"
                if nodes[src].kind == "reg":
                    edge_color = "#2E7D32"
                elif nodes[src].kind == "input":
                    edge_color = "#1565C0"
                attrs: dict[str, str] = {"color": edge_color}
                if is_fb:
                    attrs["style"] = "dashed"
                    attrs["color"] = "#D32F2F"  # red for feedback
                    attrs["constraint"] = "false"  # don't affect layout ranking
                dot.edge(src, n.name, **attrs)

    # Add rank constraints: same level → same rank
    for level, names in sorted(levels.items()):
        with dot.subgraph() as s:
            s.attr(rank="same")
            for name in names:
                s.node(name)

    # Render
    out = Path(output_path)
    stem = str(out.with_suffix(""))  # graphviz appends .pdf
    dot.render(stem, cleanup=True)
    return str(out)


# ---------------------------------------------------------------------------
# 4. Statistics summary
# ---------------------------------------------------------------------------


def print_stats(mod: VerilogModule, nodes: dict[str, Node]) -> None:
    """Print a summary of the design."""
    inputs = [p for p in mod.ports if p.direction == "input"]
    outputs = [p for p in mod.ports if p.direction == "output"]
    regs = [i for i in mod.instances if i.module_type == "pyc_reg"]
    mems = [i for i in mod.instances if i.module_type == "pyc_byte_mem"]

    kind_counts = defaultdict(int)
    for n in nodes.values():
        kind_counts[n.kind] += 1

    max_level = max((n.level for n in nodes.values()), default=0)

    print(f"Module: {mod.name}")
    print(f"  Inputs:     {len(inputs)}")
    print(f"  Outputs:    {len(outputs)}")
    print(f"  Wires:      {len(mod.wires)}")
    print(f"  Assigns:    {len(mod.assigns)}")
    print(f"  Registers:  {len(regs)}")
    print(f"  Memories:   {len(mems)}")
    print(f"  Graph nodes: {len(nodes)}")
    print(f"  Logic levels: {max_level}")
    for kind, count in sorted(kind_counts.items()):
        print(f"    {kind}: {count}")


# ---------------------------------------------------------------------------
# 5. Main
# ---------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(
        description="Generate a PDF schematic from pyCircuit-generated Verilog."
    )
    parser.add_argument("verilog", help="Path to the Verilog file")
    parser.add_argument(
        "-o", "--output", default=None, help="Output PDF path (default: <module>.pdf)"
    )
    parser.add_argument(
        "--collapse",
        action="store_true",
        default=True,
        help="Collapse pass-through assigns (a=b) [default: on]",
    )
    parser.add_argument(
        "--no-collapse",
        action="store_true",
        help="Disable collapsing pass-through assigns",
    )
    parser.add_argument(
        "--no-constants",
        action="store_true",
        default=True,
        help="Hide constant nodes [default: on]",
    )
    parser.add_argument(
        "--show-constants", action="store_true", help="Show constant nodes"
    )
    parser.add_argument(
        "--format",
        choices=["pdf", "svg", "png"],
        default="pdf",
        help="Output format (default: pdf)",
    )
    parser.add_argument(
        "--max-nodes",
        type=int,
        default=2000,
        help="Max nodes before aborting (default: 2000)",
    )
    parser.add_argument("--stats", action="store_true", help="Print design statistics")

    args = parser.parse_args()

    collapse = not args.no_collapse
    no_constants = not args.show_constants

    print(f"Parsing {args.verilog} ...")
    mod = parse_verilog(args.verilog)

    print(
        f"Building dependency graph (collapse={collapse}, no_constants={no_constants}) ..."
    )
    nodes = build_graph(mod, collapse_passthru=collapse, no_constants=no_constants)

    if args.stats:
        print_stats(mod, nodes)

    output = args.output or f"{mod.name}.{args.format}"
    print(f"Rendering {len(nodes)} nodes to {output} ...")
    render_schematic(
        nodes, output, title=mod.name, max_nodes=args.max_nodes, fmt=args.format
    )
    print(f"Done: {output}")


if __name__ == "__main__":
    main()
