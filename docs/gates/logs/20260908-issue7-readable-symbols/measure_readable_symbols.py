from __future__ import annotations

import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

import agentic_circuit as ac
from agentic_circuit._jit import _lower_acir_to_cpp
from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

from designs.davincioo.spe.iex.i2 import i2_system

root = Path.cwd().resolve()
out = root / ".pycircuit_out/issue7-readable-symbols"
shutil.rmtree(out, ignore_errors=True)
out.mkdir(parents=True)
optimizer = root / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"
queue_plan = optimizer.with_name("acir-queue-plan")


def run(*arguments: str) -> str:
    return subprocess.run(
        arguments,
        cwd=root,
        text=True,
        capture_output=True,
        check=True,
    ).stdout


raw = ac.jit(i2_system, workspace=root).lower_acir()
raw_path = out / "i2.raw.mlir"
frozen_path = out / "i2.frozen.mlir"
raw_path.write_text(raw, encoding="utf-8")
frozen = run(
    str(optimizer),
    f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
    str(raw_path),
)
frozen_path.write_text(frozen, encoding="utf-8")
plan = json.loads(run(str(queue_plan), str(frozen_path)))
i2_specialization = next(
    item for item in plan["module_specializations"] if item["definition"] == "i2"
)
digest = i2_specialization["specialization"].removeprefix("sha256:")
readable_i2 = _lower_acir_to_cpp(raw)
before_like_i2 = readable_i2.replace("Module_I2", "I2_" + digest)

direct_source = root / "tests/mlir/agentic-circuit/CodeGen/emit-cxx-current.mlir"
direct_frozen = out / "direct.frozen.mlir"
direct_acsim = out / "direct.acsim.mlir"
direct_output = out / "direct"
run(
    str(optimizer),
    "--verify-each=false",
    "--pass-pipeline=builtin.module(ac-freeze-topology)",
    str(direct_source),
    "-o",
    str(direct_frozen),
)
run(
    str(optimizer),
    "--ac-lower-to-acsim",
    "--ac-binding-profile=fast",
    "--ac-binding-target=x86_64-linux-gnu",
    str(direct_frozen),
    "-o",
    str(direct_acsim),
)
run(
    str(optimizer),
    "--ac-lower-to-acsim",
    "--ac-binding-profile=fast",
    "--ac-binding-target=x86_64-linux-gnu",
    f"--acsim-output-dir={direct_output}",
    str(direct_frozen),
    "-o",
    "/dev/null",
)
acsim = direct_acsim.read_text(encoding="utf-8")
module_digest = re.search(
    r'acsim\.module @Top .*specialization "sha256:([0-9a-f]{64})"', acsim
)
process_digest = re.search(
    r'acsim\.process @workload .*specialization "sha256:([0-9a-f]{64})"',
    acsim,
)
assert module_digest is not None and process_digest is not None
direct_header = (direct_output / "include/generated/model.h").read_text(
    encoding="utf-8"
)
direct_cpp = (direct_output / "src/generated/model.cpp").read_text(encoding="utf-8")
direct_generated = direct_header + direct_cpp
old_process = (
    "acsim_generated::Top::s"
    + module_digest.group(1)
    + "::workload::p"
    + process_digest.group(1)
)
before_like_direct = direct_generated.replace(
    "acsim_generated::module_Top::process_workload", old_process
).replace(
    "acsim_generated::module_Top",
    "acsim_generated::Top::s" + module_digest.group(1),
)
manifest = json.loads((direct_output / "build-manifest.json").read_text())
process_record = next(
    record
    for record in manifest["component_specializations"]
    if record["canonical_name"] == "Top::workload"
)
schema_set = re.search(r'schema_set = "(sha256:[0-9a-f]{64})"', acsim)
assert schema_set is not None


def full_fingerprints(value: object) -> int:
    if isinstance(value, str):
        return int(bool(re.fullmatch(r"sha256:[0-9a-f]{64}", value)))
    if isinstance(value, list):
        return sum(full_fingerprints(item) for item in value)
    if isinstance(value, dict):
        return sum(full_fingerprints(item) for item in value.values())
    return 0


report = {
    "base_revision": "b69cbd7d0a67aa3d9352e0b012b41d482006d5c6",
    "queuegraph_i2": {
        "before_like_bytes": len(before_like_i2.encode()),
        "readable_bytes": len(readable_i2.encode()),
        "bytes_removed": len(before_like_i2.encode()) - len(readable_i2.encode()),
        "routine_full_digest_class_names": len(
            re.findall(r"class [A-Za-z_][A-Za-z0-9_]*_[0-9a-f]{64}\b", readable_i2)
        ),
        "readable_class": "Module_I2",
        "full_specialization_fingerprint_retained": i2_specialization["specialization"],
        "note": (
            "before_like mechanically restores the base-revision full-digest "
            "class spelling in the same generated source"
        ),
    },
    "direct_acsim": {
        "before_like_bytes": len(before_like_direct.encode()),
        "readable_bytes": len(direct_generated.encode()),
        "before_like_max_line": max(map(len, before_like_direct.splitlines())),
        "readable_max_line": max(map(len, direct_generated.splitlines())),
        "old_full_digest_namespace_segments": len(
            re.findall(r"::[sp][0-9a-f]{64}::", direct_generated)
        ),
        "readable_process_namespace": ("acsim_generated::module_Top::process_workload"),
        "readable_thunk_references": {
            entry: direct_generated.count(
                "acsim_generated::module_Top::process_workload::Process::thunk"
                + entry.capitalize()
            )
            for entry in ("work", "xfer", "reset", "validate")
        },
        "manifest_full_sha256_values": full_fingerprints(manifest),
        "process_manifest_record": process_record,
        "process_schema_matches_model_schema_set": (
            process_record["schema_fingerprint"] == schema_set.group(1)
        ),
    },
}
sys.stdout.write(json.dumps(report, indent=2, sort_keys=True) + "\n")
