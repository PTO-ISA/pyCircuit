#!/usr/bin/env python3
"""Universal build tool for PyCircuit V6 designs.

Usage:
    python tools/build_design.py <module_path> <build_fn> <name> [--kwargs KEY=VAL ...]
                                 [--out-dir DIR] [--logic-depth N]

Example:
    python tools/build_design.py examples.pycircuit.counter.counter build counter \\
        --kwargs width=8 --out-dir examples/pycircuit/counter/build

    python tools/build_design.py designs.blocks.RegisterFile.regfile build regfile \\
        --kwargs ptag_count=32 const_count=8 nr=4 nw=2 \\
        --out-dir designs/blocks/RegisterFile/build
"""

from __future__ import annotations

import argparse
import importlib
import os
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO_ROOT / "python" / "pycircuit" / "src"))
sys.path.insert(0, str(REPO_ROOT))


def find_pycc() -> Path:
    env = os.environ.get("PYCC")
    if env:
        p = Path(env)
        if p.is_file() and os.access(p, os.X_OK):
            return p
    candidates = [
        REPO_ROOT / "build" / "bin" / "pycc",
        REPO_ROOT / "compiler" / "mlir" / "build" / "bin" / "pycc",
        REPO_ROOT / "compiler" / "mlir" / "build2" / "bin" / "pycc",
    ]
    for p in candidates:
        if p.is_file() and os.access(p, os.X_OK):
            return p
    raise SystemExit("pycc not found. Set PYCC=<path> or build the toolchain first.")


def compile_and_build(
    module_path: str,
    fn_name: str,
    design_name: str,
    kwargs: dict,
    out_dir: Path,
    logic_depth: int = 256,
    hierarchical: bool = True,
) -> bool:
    from pycircuit import build_cycle_aware

    mod = importlib.import_module(module_path)
    build_fn = getattr(mod, fn_name)

    t0 = time.time()
    circuit = build_cycle_aware(
        build_fn,
        name=design_name,
        hierarchical=hierarchical,
        **kwargs,
    )
    mlir = circuit.emit_mlir()
    time.time() - t0

    mlir_dir = out_dir / "mlir"
    mlir_dir.mkdir(parents=True, exist_ok=True)
    mlir_path = mlir_dir / f"{design_name}.pyc"
    mlir_path.write_text(mlir, encoding="utf-8")

    pycc = find_pycc()
    verilog_dir = out_dir / "verilog"
    verilog_dir.mkdir(parents=True, exist_ok=True)

    cmd = [
        str(pycc),
        str(mlir_path),
        "--emit=verilog",
        f"--out-dir={verilog_dir}",
        f"--logic-depth={logic_depth}",
    ]
    if hierarchical:
        cmd.append("--hierarchical")

    t0 = time.time()
    result = subprocess.run(cmd, capture_output=True, text=True)
    time.time() - t0

    if result.returncode != 0:
        return False

    v_files = list(verilog_dir.glob("*.v"))
    sum(1 for f in v_files for _ in f.open())
    if result.stdout.strip():
        pass
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="Build a PyCircuit V6 design")
    parser.add_argument(
        "module_path",
        help="Python module path (e.g. examples.pycircuit.counter.counter)",
    )
    parser.add_argument("fn_name", help="Build function name")
    parser.add_argument("name", help="Design name")
    parser.add_argument("--kwargs", nargs="*", default=[], help="KEY=VAL pairs")
    parser.add_argument("--out-dir", required=True, help="Output directory")
    parser.add_argument("--logic-depth", type=int, default=256)
    parser.add_argument(
        "--flat", action="store_true", help="Use flat mode instead of hierarchical"
    )
    args = parser.parse_args()

    kwargs = {}
    for kv in args.kwargs:
        k, v = kv.split("=", 1)
        try:
            kwargs[k] = int(v)
        except ValueError:
            try:
                kwargs[k] = float(v)
            except ValueError:
                kwargs[k] = v

    out_dir = Path(args.out_dir)
    ok = compile_and_build(
        args.module_path,
        args.fn_name,
        args.name,
        kwargs,
        out_dir,
        args.logic_depth,
        hierarchical=not args.flat,
    )
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
