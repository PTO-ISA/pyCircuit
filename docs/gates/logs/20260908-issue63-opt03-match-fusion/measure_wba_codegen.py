from __future__ import annotations

import hashlib
import json
import statistics
import subprocess
import sys
from pathlib import Path

import agentic_circuit as ac
from agentic_circuit._jit import _lower_acir_to_cpp

from designs.davincioo.spe.iex.wba import wba_system

root = Path.cwd().resolve()
source = _lower_acir_to_cpp(ac.jit(wba_system, workspace=root).lower_acir())
base_revision = "6c9bf7eabab70a30bed7e4081bde5a4ae5f7433c"
before = {
    "bytes": 237663,
    "entries_scan_loops": 14,
    "fused_match_markers": 0,
    "sha256": "27f986786f0d09d4e0d4b18c2a161e6d1036bcfbffd7cce5dfc6271e6fdb6732",
    "note": "Generated from the exact base revision before the implementation edit.",
}
needle = "for (std::size_t index = 0; index < table_entries->size(); ++index)"
after = {
    "bytes": len(source.encode()),
    "entries_scan_loops": source.count(needle),
    "fused_match_markers": source.count("fused_match_"),
    "sha256": hashlib.sha256(source.encode()).hexdigest(),
}


def samples(name: str) -> list[float]:
    path = (
        root
        / f"docs/gates/logs/20260908-issue63-opt03-match-fusion/{name}_benchmark.txt"
    )
    return [float(line.split()[0]) for line in path.read_text().splitlines()]


separate_samples = samples("separate")
fused_samples = samples("fused")
diff = subprocess.check_output(
    [
        "git",
        "diff",
        "--binary",
        base_revision,
        "--",
        "compiler/acir/include/acir/CodeGen/QueueGraphPlan.h",
        "compiler/acir/lib/CodeGen/QueueGraphGenerator.cpp",
        "compiler/acir/lib/CodeGen/QueueGraphPlan.cpp",
        "tests/cpp/agentic-circuit/CodeGen/QueueGraphPlanTest.cpp",
        "designs/davincioo/tests/spe/iex/test_wba.py",
    ]
)
result = {
    "base_revision": base_revision,
    "implementation_diff_sha256": hashlib.sha256(diff).hexdigest(),
    "wba_generated": {"before": before, "after": after},
    "local_apple_clang_o3_ns_per_iteration": {
        "separate_samples": separate_samples,
        "fused_samples": fused_samples,
        "separate_median": statistics.median(separate_samples),
        "fused_median": statistics.median(fused_samples),
        "checksum": 1275000000,
        "note": "Diagnostic eight-entry predicate benchmark; not a semantic threshold.",
    },
}
sys.stdout.write(json.dumps(result, indent=2, sort_keys=True) + "\n")
