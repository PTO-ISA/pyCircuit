from __future__ import annotations

import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path

import agentic_circuit as ac
from agentic_circuit._jit import _lower_acir_to_cpp

from designs.davincioo.spe.iex.wba import wba_system

root = Path.cwd().resolve()
out = root / ".pycircuit_out/issue63-opt05-table-borrow"
out.mkdir(parents=True, exist_ok=True)
source = _lower_acir_to_cpp(ac.jit(wba_system, workspace=root).lower_acir())

table_pattern = re.compile(
    r"^(\s*)const auto &(\w+) = " r"(table_\w+->at\(static_cast<size_t>.*)$",
    re.MULTILINE,
)
projection_pattern = re.compile(r"^(\s*)const auto &(\w+) = (\w+\.\w+;)$", re.MULTILINE)
table_borrows = len(table_pattern.findall(source))
projection_borrows = len(projection_pattern.findall(source))
before_like = table_pattern.sub(r"\1auto \2 = \3", source)
before_like = projection_pattern.sub(r"\1auto \2 = \3", before_like)

harness = """
int main() {
  ac_generated::WbaSystem model;
  auto rows = model.dispatch_rows();
  for (unsigned tick = 1; tick != 5; ++tick) {
    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows) row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }
  return 0;
}
"""
source_paths = {
    "before_like": out / "wba_before_like.cpp",
    "after": out / "wba_after.cpp",
}
source_paths["before_like"].write_text(before_like + harness)
source_paths["after"].write_text(source + harness)
compiler = os.environ.get("CXX", "c++")
executables: dict[str, Path] = {}
for name, path in source_paths.items():
    executable = out / f"wba_{name}"
    subprocess.run(
        [
            compiler,
            "-O3",
            "-std=c++20",
            "-I",
            str(root / "simulator/gfsim/include"),
            str(path),
            "-o",
            str(executable),
        ],
        check=True,
    )
    subprocess.run([str(executable)], check=True)
    executables[name] = executable

size_output = subprocess.check_output(
    ["size", str(executables["before_like"]), str(executables["after"])],
    text=True,
)
text_sizes: dict[str, int] = {}
instruction_counts: dict[str, int] = {}
for name, executable in executables.items():
    size_line = next(
        line for line in size_output.splitlines() if line.endswith(str(executable))
    )
    text_sizes[name] = int(size_line.split()[0])
    disassembly = subprocess.check_output(["otool", "-tvV", str(executable)], text=True)
    instruction_counts[name] = sum(
        bool(re.match(r"^[0-9a-f]+\t", line)) for line in disassembly.splitlines()
    )

base_revision = "318a9275f98b9e4437b29196f4baeb3aee58cccf"
diff = subprocess.check_output(
    [
        "git",
        "diff",
        "--binary",
        base_revision,
        "--",
        "compiler/acir/lib/CodeGen/QueueGraphGenerator.cpp",
        "tests/cpp/agentic-circuit/CodeGen/QueueGraphPlanTest.cpp",
        "designs/davincioo/tests/spe/iex/test_wba.py",
    ]
)
result = {
    "base_revision": base_revision,
    "implementation_diff_sha256": hashlib.sha256(diff).hexdigest(),
    "wba_generated": {
        "before_like_bytes": len(before_like.encode()),
        "after_bytes": len(source.encode()),
        "aggregate_table_get_borrows": table_borrows,
        "aggregate_projection_borrows": projection_borrows,
        "by_value_sites_removed": table_borrows + projection_borrows,
        "note": (
            "before_like mechanically restores the exact pre-change auto bindings "
            "in the same generated source; it is not a separate base-worktree build"
        ),
    },
    "apple_clang_o3_smoke": {
        "both_executed": True,
        "text_bytes": text_sizes,
        "disassembled_instruction_count": instruction_counts,
        "note": "Diagnostic comparison only; no runtime speedup is claimed.",
    },
}
sys.stdout.write(json.dumps(result, indent=2, sort_keys=True) + "\n")
