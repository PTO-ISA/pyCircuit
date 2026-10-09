"""Protect checked output publication against a genuine missing expect."""

import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

parser = argparse.ArgumentParser()
for name in ("repo", "emitter", "scratch"):
    parser.add_argument("--" + name, required=True)
args = parser.parse_args()
repo = Path(args.repo).resolve()
root = Path(args.scratch).resolve()
completed = list(root.glob("multi-rule-writes-*/candidate.json"))
assert completed, "complete multi-rule hardware gate must finish first"
gate = max(completed, key=lambda path: path.stat().st_mtime_ns).parent
source = gate / "address-checked.ac"
text = source.read_text()
expect = next(line for line in text.splitlines(keepends=True) if '"ac.expect"(' in line)
invalid = gate / "address-checked-missing-expect.ac"
mutated = text.replace(expect, "", 1)
assert mutated != text and "ac.required_checks" in mutated
invalid.write_text(mutated)
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYCIRCUIT_EMITTER=args.emitter,
)
commands = []


def snapshot(directory):
    return {
        str(path.relative_to(directory)): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in directory.rglob("*")
        if path.is_file()
    }


for target in ("cpp", "verilog"):
    existing = gate / ("checked-fresh-" + target)
    before = snapshot(existing)
    assert before
    for replace in (False, True):
        output = existing if replace else gate / ("missing-expect-fresh-" + target)
        assert replace or not output.exists()
        command = [
            sys.executable,
            "-m",
            "pycircuit.cli",
            "emit",
            str(invalid),
            "--target",
            target,
            "-o",
            str(output),
        ]
        if replace:
            command.append("--replace")
        result = subprocess.run(
            command, env=env, cwd=repo, capture_output=True, text=True, timeout=240
        )
        commands.append(
            {
                "command": command,
                "exit_status": result.returncode,
                "stdout": result.stdout,
                "stderr": result.stderr,
                "target": target,
                "replace": replace,
                "protected_before": before,
                "protected_after": snapshot(existing),
            }
        )
        assert result.returncode == 1, commands[-1]
        assert "RequiredCheck has no matching expect" in result.stderr, result.stderr
        assert (
            "Traceback" not in result.stderr and "Assertion failed" not in result.stderr
        )
        assert (
            snapshot(existing) == before
        ), "invalid checked IR changed published output"
        assert (
            replace or not output.exists()
        ), "invalid checked IR published fresh output"
(gate / "checked-publication-protection.json").write_text(
    json.dumps(
        {
            "commands": commands,
            "mutant": str(invalid),
            "mutant_sha256": hashlib.sha256(invalid.read_bytes()).hexdigest(),
            "original_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
            "complete_output_hashes_preserved": True,
        },
        indent=2,
    )
    + "\n"
)
print(
    "PASS: genuine missing-expect rejects fresh and same-owner replacement for CPP/RTL"
)
