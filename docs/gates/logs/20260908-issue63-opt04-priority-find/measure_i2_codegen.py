from __future__ import annotations

import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

import agentic_circuit as ac
from agentic_circuit._jit import _lower_acir_to_cpp

from designs.davincioo.spe.iex.i2 import i2_system

root = Path.cwd().resolve()
source = _lower_acir_to_cpp(ac.jit(i2_system, workspace=root).lower_acir())
encoded = source.encode()
base_revision = os.environ.get(
    "PYC_METRIC_BASE", "d1c6f1e1c20fa61b5fa1b651f5cc5383e5d95d84"
)
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
        "simulator/gfsim/include/gfsim/priority_encode.h",
        "simulator/gfsim/include/gfsim/queue_blocks.h",
    ]
)
sys.stdout.write(
    json.dumps(
        {
            "base_revision": base_revision,
            "implementation_diff_sha256": hashlib.sha256(diff).hexdigest(),
            "bytes": len(encoded),
            "table_scan_loops": source.count(
                "for (std::size_t index = 0; index < table"
            ),
            "priority_encode_calls": source.count("gfsim::priorityEncode("),
            "sha256": hashlib.sha256(encoded).hexdigest(),
        },
        sort_keys=True,
    )
    + "\n"
)
