#!/usr/bin/env python3
"""Build ALL PyCircuit designs to Verilog via pycc --hierarchical.

Each design outputs to <design_dir>/build/ (mlir/ + verilog/).
"""
from __future__ import annotations

import importlib
import json
import os
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "python" / "pycircuit" / "src"))
sys.path.insert(0, str(REPO))


def find_pycc() -> Path:
    for p in [
        REPO / "build" / "bin" / "pycc",
        REPO / "compiler" / "mlir" / "build" / "bin" / "pycc",
    ]:
        if p.is_file() and os.access(p, os.X_OK):
            return p
    raise SystemExit("pycc not found")


def stamp_metadata(circuit, name: str, params_json: str = "{}") -> None:
    circuit.set_func_attr("pyc.kind", "module")
    circuit.set_func_attr("pyc.inline", "false")
    circuit.set_func_attr("pyc.params", params_json)
    circuit.set_func_attr("pyc.base", name)
    metrics = json.dumps(
        {
            "ast_node_count": 0,
            "collection_count": 0,
            "collection_instance_count": 0,
            "estimated_inline_cost": 0,
            "hardware_call_count": 0,
            "instance_count": 0,
            "loop_count": 0,
            "module_call_count": 0,
            "module_family_collection_count": 0,
            "repeat_pressure": 0,
            "repeated_body_clusters": [],
            "source_loc": 0,
            "state_alloc_count": 0,
            "state_call_count": 0,
        },
        sort_keys=True,
        separators=(",", ":"),
    )
    circuit.set_func_attr("pyc.struct.metrics", metrics)
    circuit.set_func_attr("pyc.struct.collections", "[]")
    circuit.set_func_attr_json("pyc.value_params", [])
    circuit.set_func_attr_json("pyc.value_param_types", [])


def wrap_module_attrs(mlir: str, top_name: str) -> str:
    return mlir.replace(
        "module {\n",
        f"module attributes {{pyc.top = @{top_name}, "
        f'pyc.frontend.contract = "pycircuit"}} {{\n',
        1,
    )


def build_one(spec: dict, pycc: Path, logic_depth: int = 256) -> tuple[str, bool, str]:
    from pycircuit import compile_cycle_aware

    name = spec["name"]
    mod_path = spec["module"]
    fn_name = spec["fn"]
    kwargs = spec.get("kwargs", {})
    out_dir = REPO / spec["out_dir"]
    hier = spec.get("hierarchical", False)

    try:
        mod = importlib.import_module(mod_path)
        fn = getattr(mod, fn_name)

        params_json = json.dumps(kwargs, sort_keys=True, separators=(",", ":"))
        circuit = compile_cycle_aware(
            fn, name=name, eager=True, hierarchical=hier, **kwargs
        )
        stamp_metadata(circuit, name, params_json)
        mlir = wrap_module_attrs(circuit.emit_mlir(), name)

        mlir_dir = out_dir / "mlir"
        mlir_dir.mkdir(parents=True, exist_ok=True)
        mlir_path = mlir_dir / f"{name}.pyc"
        mlir_path.write_text(mlir, encoding="utf-8")

        verilog_dir = out_dir / "verilog"
        verilog_dir.mkdir(parents=True, exist_ok=True)

        cmd = [
            str(pycc),
            str(mlir_path),
            "--emit=verilog",
            f"--out-dir={verilog_dir}",
            f"--logic-depth={logic_depth}",
            "--hierarchical",
        ]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            return name, False, result.stderr.strip()[:300]

        v_files = list(verilog_dir.glob("*.v"))
        total_lines = sum(1 for f in v_files for _ in f.open())
        return name, True, f"{len(v_files)} files, {total_lines:,} lines"

    except Exception as e:
        return name, False, str(e)[:300]


# ── Design Registry ──────────────────────────────────────────────────────


def all_designs() -> list[dict]:
    designs = []

    # ── Examples ──
    examples = [
        ("counter", "build", {"width": 8}),
        ("arith", "build", {"lanes": 8, "lane_width": 16}),
        (
            "digital_filter",
            "build",
            {"TAPS": 4, "DATA_W": 16, "COEFF_W": 16, "COEFFS": (1, 2, 3, 4)},
        ),
        ("hier_modules", "build", {"width": 8, "stages": 3}),
        ("boundary_value_ports", "build", {}),
        ("bundle_probe_expand", None, None),
        ("cache_params", "build", {}),
        ("calculator", "build", {}),
        ("decode_rules", "build", {}),
        ("digital_clock", "build", {}),
        ("fastfwd", "build", {}),
        ("fifo_loopback", "build", {}),
        ("fmac", None, None),
        ("fm16", None, None),
        ("huge_hierarchy_stress", None, None),
        ("instance_map", None, None),
        ("interface_wiring", None, None),
        ("issue_queue_2picker", "build", {}),
        ("jit_control_flow", "build", {}),
        ("jit_pipeline_vec", "build", {}),
        ("mem_rdw_olddata", "build", {}),
        ("module_collection", None, None),
        ("multiclock_regs", "build", {}),
        ("net_resolution_depth_smoke", "build", {}),
        ("obs_points", "build", {}),
        ("pipeline_builder", "build", {}),
        ("reset_invalidate_order_smoke", "build", {}),
        ("struct_transform", "build", {}),
        ("sync_mem_init_zero", "build", {}),
        ("trace_dsl_smoke", None, None),
        ("wire_ops", "build", {}),
        ("xz_value_model_smoke", "build", {}),
    ]

    for ename, fn, kwargs in examples:
        if fn is None:
            continue
        designs.append(
            {
                "name": ename,
                "module": f"examples.pycircuit.{ename}.{ename}",
                "fn": fn,
                "kwargs": kwargs or {},
                "out_dir": f"examples/pycircuit/{ename}/build",
            }
        )

    # ── RegisterFile ──
    designs.append(
        {
            "name": "regfile",
            "module": "designs.blocks.RegisterFile.regfile",
            "fn": "build",
            "kwargs": {"ptag_count": 32, "const_count": 8, "nr": 4, "nw": 2},
            "out_dir": "designs/blocks/RegisterFile/build",
        }
    )

    # ── BypassUnit ──
    designs.append(
        {
            "name": "bypass_unit",
            "module": "designs.blocks.BypassUnit.bypass_unit_v6",
            "fn": "bypass_unit",
            "kwargs": {
                "lanes": 4,
                "data_width": 32,
                "ptag_count": 64,
                "ptype_count": 4,
            },
            "out_dir": "designs/blocks/BypassUnit/build",
        }
    )

    return designs


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--filter", help="Only build designs matching this pattern")
    parser.add_argument("--logic-depth", type=int, default=256)
    parser.add_argument("--list", action="store_true")
    args = parser.parse_args()

    designs = all_designs()
    if args.filter:
        designs = [d for d in designs if args.filter in d["name"]]

    if args.list:
        for d in designs:
            print(f"  {d['name']:25s} {d['module']}::{d['fn']} -> {d['out_dir']}")
        print(f"\nTotal: {len(designs)} designs")
        return 0

    pycc = find_pycc()
    print(f"pycc: {pycc}")
    print(f"Designs: {len(designs)}")
    print()

    succeeded = []
    failed = []
    t0 = time.time()

    for i, spec in enumerate(designs, 1):
        name = spec["name"]
        print(f"[{i:2d}/{len(designs)}] {name:25s} ... ", end="", flush=True)
        t1 = time.time()
        name, ok, msg = build_one(spec, pycc, args.logic_depth)
        dt = time.time() - t1
        if ok:
            print(f"OK  ({msg}, {dt:.1f}s)")
            succeeded.append(name)
        else:
            print(f"FAIL ({dt:.1f}s)")
            print(f"       {msg}")
            failed.append((name, msg))

    total = time.time() - t0
    print(f"\n{'='*60}")
    print(f"Results: {len(succeeded)} succeeded, {len(failed)} failed ({total:.0f}s)")

    if failed:
        print("\nFailed designs:")
        for name, msg in failed:
            print(f"  {name}: {msg[:200]}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
