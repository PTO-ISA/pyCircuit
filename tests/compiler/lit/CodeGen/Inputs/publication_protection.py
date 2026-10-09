"""Check failed emit preserves an existing published artifact byte for byte."""

import os
import subprocess
import sys
import tempfile
from pathlib import Path

source_root, emitter, valid, invalid, target, scratch = sys.argv[1:]
scratch_root = Path(scratch)
scratch_root.mkdir(parents=True, exist_ok=True)
private_run = tempfile.TemporaryDirectory(prefix="publication-", dir=scratch_root)
root = Path(private_run.name)
output = root / "published"
env = dict(
    os.environ,
    PYTHONPATH=str(Path(source_root) / "python"),
    PYCIRCUIT_EMITTER=emitter,
)


def invoke(source, replace=False):
    cmd = [
        sys.executable,
        "-m",
        "pycircuit.cli",
        "emit",
        source,
        "--target",
        target,
        "-o",
        str(output),
    ]
    if replace:
        cmd.append("--replace")
    return subprocess.run(cmd, env=env, text=True, capture_output=True)


def snapshot():
    return {
        str(path.relative_to(root)): (
            ("file", path.read_bytes()) if path.is_file() else ("directory", None)
        )
        for path in root.rglob("*")
    }


first = invoke(valid)
if first.returncode:
    raise AssertionError(f"valid emit failed: {first.stderr}")
before = snapshot()
assert before, "valid emitter published no files"
failed = invoke(invalid, replace=True)
assert failed.returncode != 0, "invalid hardware design was emitted"
assert snapshot() == before, "failed replacement mutated prior publication"
