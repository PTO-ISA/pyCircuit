from __future__ import annotations

import ast
import hashlib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import agentic_circuit as ac
from agentic_circuit._jit import _lower_acir_to_cpp
from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

from designs.davincioo.gpe.ipf.xbar import gpe_ipf_xbar_system
from designs.davincioo.mem.noc.xbar import mem_noc_xbar_system
from designs.davincioo.tmu.bgf.xbar import bgf_xbar_system

root = Path.cwd().resolve()
specs = (
    (
        "tmu_bgf",
        root / "designs/davincioo/tmu/bgf/xbar.py",
        bgf_xbar_system,
        (1, 2, 4, 7),
    ),
    (
        "gpe_ipf",
        root / "designs/davincioo/gpe/ipf/xbar.py",
        gpe_ipf_xbar_system,
        (1, 2, 3, 4),
    ),
    (
        "mem_noc",
        root / "designs/davincioo/mem/noc/xbar.py",
        mem_noc_xbar_system,
        (1, 2, 3, 5),
    ),
)


out = root / ".pycircuit_out/issue63-opt06-davincioo-xbars"
out.mkdir(parents=True, exist_ok=True)
optimizer = root / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"
pycgen = optimizer.with_name("acir-queue-pycgen")
pycc = root / ".pycircuit_out/toolchain/install/bin/pycc"
metadata = (
    root / ".pycircuit_out/toolchain/install/share/pycircuit/toolchain-metadata.json"
)
verilator = shutil.which("verilator")
required = (optimizer, pycgen, pycc, metadata)
if not all(path.is_file() for path in required) or verilator is None:
    raise SystemExit("PYC C++/Verilog build toolchain is required for OPT-06 metrics")

baseline_loc = {"tmu_bgf": 90, "gpe_ipf": 81, "mem_noc": 106}
results: dict[str, object] = {}
compiler = os.environ.get("CXX", "c++")
for name, path, system, latencies in specs:
    raw = ac.jit(system, workspace=root).lower_acir()
    generated_cpp = _lower_acir_to_cpp(raw)
    design_out = out / name
    shutil.rmtree(design_out, ignore_errors=True)
    design_out.mkdir(parents=True, exist_ok=True)
    raw_path = design_out / "raw.mlir"
    frozen_path = design_out / "frozen.mlir"
    raw_path.write_text(raw, encoding="utf-8")
    frozen = subprocess.run(
        (
            str(optimizer),
            f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
            str(raw_path),
        ),
        text=True,
        capture_output=True,
        check=True,
    )
    frozen_path.write_text(frozen.stdout, encoding="utf-8")
    pyc_out = design_out / "pyc"
    subprocess.run(
        (
            str(root / "compiler/acir/tools/ac-queue-pyc-build.py"),
            str(frozen_path),
            "--pycgen-tool",
            str(pycgen),
            "--pycc",
            str(pycc),
            "--toolchain-lock",
            str(root / "toolchains/agentic-circuit/pyc.lock.json"),
            "--toolchain-metadata",
            str(metadata),
            "--cxx",
            compiler,
            "--verilator",
            verilator,
            "--pyc-output",
            str(pyc_out / "model.pyc"),
            "--cpp-output-dir",
            str(pyc_out / "cpp"),
            "--verilog-output-dir",
            str(pyc_out / "verilog"),
            "--manifest",
            str(pyc_out / "manifest.json"),
        ),
        cwd=root,
        text=True,
        capture_output=True,
        check=True,
    )
    source = path.read_text(encoding="utf-8")
    tree = ast.parse(source)
    apply_calls = sum(
        isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and node.func.attr == "apply"
        for node in ast.walk(tree)
    )
    array_calls = sum(
        isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and isinstance(node.func.value, ast.Name)
        and node.func.value.id == "ac"
        and node.func.attr == "array"
        for node in ast.walk(tree)
    )
    operation_lines = tuple(
        line.strip() for line in raw.splitlines() if " = ac." in line
    )
    cpp_files = tuple((pyc_out / "cpp").glob("*.cpp"))
    verilog_files = tuple((pyc_out / "verilog").glob("*.v"))
    manifest = pyc_out / "manifest.json"
    results[name] = {
        "source": {
            "external_revision_loc": baseline_loc[name],
            "static_template_loc": len(source.splitlines()),
            "ac_array_calls": array_calls,
            "apply_calls": apply_calls,
        },
        "latencies": latencies,
        "expanded_queue_operations": {
            "merge": sum(" = ac.merge " in line for line in operation_lines),
            "route": sum(" = ac.route " in line for line in operation_lines),
            "transform": sum(" = ac.transform " in line for line in operation_lines),
            "transform_specializations": len(latencies),
        },
        "generated_bytes": {
            "gfsim_cpp": len(generated_cpp.encode()),
            "pyc": (pyc_out / "model.pyc").stat().st_size,
            "pyc_cpp": sum(path.stat().st_size for path in cpp_files),
            "verilog": sum(path.stat().st_size for path in verilog_files),
        },
        "generated_files": {
            "pyc_cpp": len(cpp_files),
            "verilog": len(verilog_files),
        },
        "manifest_sha256": hashlib.sha256(manifest.read_bytes()).hexdigest(),
    }

old_total = sum(baseline_loc.values())
new_total = sum(item["source"]["static_template_loc"] for item in results.values())
report = {
    "framework_revision": subprocess.check_output(
        ("git", "rev-parse", "HEAD"), text=True
    ).strip(),
    "external_source_revision": "b81ecfc2b634d41886b01fd8724905eeb5bb6551",
    "source_loc": {
        "external_total": old_total,
        "static_template_total": new_total,
        "removed": old_total - new_total,
    },
    "designs": results,
}
sys.stdout.write(json.dumps(report, indent=2, sort_keys=True) + "\n")
