#!/usr/bin/env python3
"""
visualize_cpp.py — Generate a PDF schematic from a pyCircuit-generated C++ .hpp file.

Usage:
    python visualize_cpp.py <hpp_file> [-o output.pdf] [--collapse] [--no-constants]

Layout follows the same style as schematic_view.py:
    - Input ports on the left
    - Combinational logic levels laid out left-to-right by data dependency
    - Registers (pyc_reg) shown as flip-flop boxes
    - Output ports on the right
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

# Reuse graph building and rendering from schematic_view
from schematic_view import (
    Assign,
    Instance,
    Port,
    VerilogModule,
    WireDecl,
    build_graph,
    print_stats,
    render_schematic,
)

# ---------------------------------------------------------------------------
# 1. C++ HPP parser  (targeted at pyCircuit-generated .hpp)
# ---------------------------------------------------------------------------


def _parse_wire_width(decl: str) -> int:
    """Extract width from 'pyc::cpp::Wire<N>'."""
    m = re.search(r"Wire<(\d+)>", decl)
    return int(m.group(1)) if m else 1


def _extract_cpp_sources(expr: str, known: set[str]) -> list[str]:
    """Extract signal names from a C++ expression, filtering to known signals."""
    # Remove C++ template noise: pyc::cpp::Wire<N>({...}), .toBool(), .template ...
    clean = re.sub(r"pyc::cpp::Wire<\d+>\(\{[^}]*\}\)", "", expr)
    clean = re.sub(r"\.toBool\(\)", "", clean)
    clean = re.sub(r"\.template\s+\w+<[^>]*>\(\)", "", clean)

    srcs: list[str] = []
    for m in re.finditer(r"\b([A-Za-z_][A-Za-z0-9_]*)\b", clean):
        name = m.group(1)
        if name in known and name not in srcs:
            srcs.append(name)
    return srcs


def parse_hpp(path: str) -> VerilogModule:
    """Parse a pyCircuit-generated C++ .hpp file into a VerilogModule structure."""
    text = Path(path).read_text()

    # --- Module name ---
    mod_m = re.search(r"struct\s+(\w+)\s*\{", text)
    mod_name = mod_m.group(1) if mod_m else "unknown"

    # --- Wire declarations (struct members) ---
    # Pattern: pyc::cpp::Wire<N> name{};
    wire_re = re.compile(r"pyc::cpp::Wire<(\d+)>\s+(\w+)\{?\};")
    all_wires: dict[str, WireDecl] = {}
    for wm in wire_re.finditer(text):
        width = int(wm.group(1))
        name = wm.group(2)
        all_wires[name] = WireDecl(name=name, width=width)

    # --- Register instances ---
    # Pattern: pyc::cpp::pyc_reg<N> name_inst;
    reg_inst_re = re.compile(r"pyc::cpp::pyc_reg<(\d+)>\s+(\w+);")
    reg_instances: dict[str, int] = {}  # inst_name -> width
    for rm in reg_inst_re.finditer(text):
        width = int(rm.group(1))
        inst_name = rm.group(2)
        reg_instances[inst_name] = width

    # --- Memory instances ---
    # Pattern: pyc::cpp::pyc_byte_mem<ADDR_W, DATA_W, DEPTH> name;
    mem_inst_re = re.compile(
        r"pyc::cpp::pyc_byte_mem<(\d+),\s*(\d+),\s*(\d+)>\s+(\w+);"
    )
    mem_instances: list[tuple[str, dict[str, str]]] = []
    for mm in mem_inst_re.finditer(text):
        mem_instances.append(
            (
                mm.group(4),
                {
                    "ADDR_WIDTH": mm.group(1),
                    "DATA_WIDTH": mm.group(2),
                    "DEPTH": mm.group(3),
                },
            )
        )

    # --- Register constructors (initializer list) ---
    # Pattern: inst_name(clk, rst, en, d, init, q)
    # Find the constructor initializer list
    instances: list[Instance] = []
    ctor_re = re.compile(
        r"(\w+_inst)\(clk,\s*rst,\s*(\w+),\s*(\w+),\s*(\w+),\s*(\w+)\)"
    )
    for cm in ctor_re.finditer(text):
        inst_name = cm.group(1)
        en = cm.group(2)
        d = cm.group(3)
        init = cm.group(4)
        q = cm.group(5)
        width = reg_instances.get(inst_name, 1)
        instances.append(
            Instance(
                module_type="pyc_reg",
                inst_name=inst_name,
                params={"WIDTH": str(width)},
                connections={
                    "clk": "clk",
                    "rst": "rst",
                    "en": en,
                    "d": d,
                    "init": init,
                    "q": q,
                },
            )
        )

    # --- Memory constructors ---
    # Pattern: mem(clk, rst, raddr, rdata, wvalid, waddr, wdata, wstrb)
    mem_ctor_re = re.compile(
        r"(\w+)\(clk,\s*rst,\s*(\w+),\s*(\w+),\s*(\w+),\s*(\w+),\s*(\w+),\s*(\w+)\)\s*\{"
    )
    for mm_ctor in mem_ctor_re.finditer(text):
        mem_name = mm_ctor.group(1)
        # Find matching mem_instance
        for mi_name, mi_params in mem_instances:
            if mi_name == mem_name:
                instances.append(
                    Instance(
                        module_type="pyc_byte_mem",
                        inst_name=mem_name,
                        params=mi_params,
                        connections={
                            "raddr": mm_ctor.group(2),
                            "rdata": mm_ctor.group(3),
                            "wvalid": mm_ctor.group(4),
                            "waddr": mm_ctor.group(5),
                            "wdata": mm_ctor.group(6),
                            "wstrb": mm_ctor.group(7),
                        },
                    )
                )
                break

    # --- Determine ports ---
    # In pyCircuit HPP, ports are the first Wire declarations in the struct,
    # separated from internal wires by a blank line.
    # clk/rst are always inputs; wires assigned directly in eval() (outside
    # eval_comb calls) are outputs; the rest are inputs.
    ports: list[Port] = []
    known_inputs = {"clk", "rst"}

    # Find the struct body
    struct_m = re.search(
        r"struct\s+\w+\s*\{(.*?)\n\s*pyc::cpp::pyc_reg", text, re.DOTALL
    )
    if not struct_m:
        struct_m = re.search(r"struct\s+\w+\s*\{(.*)", text, re.DOTALL)
    struct_body = struct_m.group(1) if struct_m else ""

    # Ports are Wire declarations before the first blank line in the struct
    port_candidates: list[str] = []
    for line in struct_body.split("\n"):
        stripped = line.strip()
        if not stripped and port_candidates:
            break  # blank line after at least one wire → end of ports
        wm = wire_re.search(stripped)
        if wm:
            port_candidates.append(wm.group(2))

    # Find output assignments in eval(): direct assignments (not eval_comb calls)
    eval_m = re.search(r"void eval\(\)\s*\{(.*?)\n\s*\}", text, re.DOTALL)
    eval_outputs: set[str] = set()
    if eval_m:
        for line in eval_m.group(1).strip().split("\n"):
            line = line.strip()
            am = re.match(r"^(\w+)\s*=\s*(.+);$", line)
            if am and not line.startswith("eval_comb"):
                dst = am.group(1)
                if dst in all_wires and dst not in known_inputs:
                    eval_outputs.add(dst)

    # Classify port candidates
    for name in port_candidates:
        if name in known_inputs:
            ports.append(
                Port(name=name, direction="input", width=all_wires[name].width)
            )
        elif name in eval_outputs:
            ports.append(
                Port(name=name, direction="output", width=all_wires[name].width)
            )
        else:
            ports.append(
                Port(name=name, direction="input", width=all_wires[name].width)
            )

    # --- Combinational assignments from eval()/eval_comb_*() ---
    known_signals: set[str] = set(all_wires.keys())
    assigns: list[Assign] = []
    seen_dsts: set[str] = set()

    # Parse all eval_comb_*() and eval() function bodies
    func_re = re.compile(
        r"(?:inline\s+)?void\s+(eval(?:_comb_\d+)?)\(\)\s*\{(.*?)\n\s*\}", re.DOTALL
    )
    for fm in func_re.finditer(text):
        body = fm.group(2)
        for line in body.strip().split("\n"):
            line = line.strip()
            # Skip function calls: eval_comb_N(), mem.eval(), etc.
            if re.match(r"^\w+(\.\w+)?\(", line):
                continue
            # Parse assignment: dst = expr;
            am = re.match(r"^(\w+)\s*=\s*(.+);$", line)
            if not am:
                continue
            dst = am.group(1)
            expr = am.group(2).strip()
            if dst not in known_signals:
                continue
            if dst in seen_dsts:
                continue  # first write wins (eval order)
            seen_dsts.add(dst)
            srcs = _extract_cpp_sources(expr, known_signals)
            assigns.append(Assign(dst=dst, expr=expr, sources=srcs))

    return VerilogModule(
        name=mod_name,
        ports=ports,
        wires=all_wires,
        assigns=assigns,
        instances=instances,
    )


# ---------------------------------------------------------------------------
# 2. Main
# ---------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(
        description="Generate a PDF schematic from pyCircuit-generated C++ .hpp."
    )
    parser.add_argument("hpp", help="Path to the .hpp file")
    parser.add_argument(
        "-o",
        "--output",
        default=None,
        help="Output file path (default: <module>.<format>)",
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

    print(f"Parsing {args.hpp} ...")
    mod = parse_hpp(args.hpp)

    print(
        f"Building dependency graph (collapse={collapse}, no_constants={no_constants}) ..."
    )
    nodes = build_graph(mod, collapse_passthru=collapse, no_constants=no_constants)

    if args.stats:
        print_stats(mod, nodes)

    output = args.output or f"cpp_{mod.name}.{args.format}"
    print(f"Rendering {len(nodes)} nodes to {output} ...")
    render_schematic(
        nodes,
        output,
        title=f"{mod.name} (C++)",
        max_nodes=args.max_nodes,
        fmt=args.format,
    )
    print(f"Done: {output}")


if __name__ == "__main__":
    main()
