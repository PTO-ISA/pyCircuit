"""Real public source-unit closure; DUT/runner execution belongs to the next packet."""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
for name in ("repo", "source-compiler", "linker", "emitter", "scratch"):
    parser.add_argument("--" + name, required=True)
args = parser.parse_args()
repo = Path(args.repo).resolve()
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
private = tempfile.TemporaryDirectory(prefix="source-bindings-", dir=scratch)
root = Path(private.name)
source = root / "source"
source.mkdir()
fixtures = Path(__file__).resolve().parent
for name in ("child.py", "parent.py", "bad_width.py"):
    shutil.copyfile(fixtures / name, source / name)
env = dict(os.environ, PYTHONPATH=str(repo / "python/pycircuit/src"),
           PYTHONDONTWRITEBYTECODE="1", PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
           PYCIRCUIT_LINKER=args.linker, PYCIRCUIT_EMITTER=args.emitter)
commands = []


def run(arguments, accepted=True):
    command = [sys.executable, "-m", "pycircuit.cli", *map(str, arguments)]
    result = subprocess.run(command, env=env, cwd=repo, capture_output=True,
                            text=True, timeout=20)
    record = {"command": command, "exit_status": result.returncode,
              "stdout": result.stdout, "stderr": result.stderr}
    commands.append(record)
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.stdout == "", record
    assert "Assertion failed" not in result.stderr, record
    assert "Traceback" not in result.stderr, record
    if accepted:
        assert result.returncode == 0, record
    else:
        assert result.returncode == 1, record
    return result


def compile_unit(name, target, imports=(), replace=False):
    arguments = ["compile", "-c", source / name, "--source-root", source,
                 "--package-prefix", "bindings", "-o", target]
    for unit in imports:
        arguments.extend(("-I", unit))
    if replace:
        arguments.append("--replace")
    return arguments


def snapshot(target):
    return {p.relative_to(target).as_posix(): p.read_bytes()
            for p in target.rglob("*") if p.is_file()}


def constants(text):
    # Resolve printed MLIR attribute aliases, then inspect only real constant
    # operations. Width literals or source metadata alone are not evidence.
    aliases = dict(re.findall(r"^(#[A-Za-z_][A-Za-z_0-9]*) = (.*)$", text, re.M))
    def expand(line):
        for _ in range(len(aliases) + 1):
            updated = re.sub(r"#[A-Za-z_][A-Za-z_0-9]*\b",
                             lambda m: aliases.get(m[0], m[0]), line)
            if updated == line:
                return line
            line = updated
        raise AssertionError("attribute alias expansion contains a cycle")
    values = []
    for line in text.splitlines():
        if '"ac.bits.constant"' not in line:
            continue
        operation = expand(line.split(" : () ->", 1)[0])
        literal = re.findall(r'kind = "integer", value = #ac\.math_int<(-?\d+)>',
                             operation)
        assert len(literal) == 1, operation
        values.append(int(literal[0]))
    return values


units = root / "units"
units.mkdir()
child, parent = units / "child", units / "parent"
run(compile_unit("child.py", child))
assert {p.name for p in child.iterdir()} == {
    "child.ac", "child.interface.ac", "child.d", "unit.json"}
assert 0 in constants((child / "child.ac").read_text())
assert 1 in constants((child / "child.ac").read_text())

# Child source, body and depfile cannot be inputs to parent compile. Restore
# only the two published artifacts before link, which needs the full closure.
hidden = root / "hidden"
hidden.mkdir()
for path in (source / "child.py", child / "child.ac", child / "child.d"):
    path.rename(hidden / path.name)
run(compile_unit("parent.py", parent, [child]))
depfile = (parent / "parent.d").read_text()
assert str(child / "child.interface.ac") in depfile
assert str(child / "unit.json") in depfile
assert str(source / "child.py") not in depfile
assert str(child / "child.ac") not in depfile
assert str(child / "child.d") not in depfile
for name in ("child.ac", "child.d"):
    (hidden / name).rename(child / name)

final = root / "program.ac"
run(["link", child, parent, "--top", "bindings.parent.Top", "-o", final])
final_hash = hashlib.sha256(final.read_bytes()).hexdigest()
for target in ("cpp", "verilog"):
    output = root / target
    run(["emit", final, "--target", target, "-o", output])
    assert (output / "generated.json").is_file()
    assert hashlib.sha256(final.read_bytes()).hexdigest() == final_hash

bad = units / "bad_width"
rejected = run(compile_unit("bad_width.py", bad, [child]), accepted=False)
assert "boundary requires known integer source and destination kinds" in rejected.stderr.lower(), rejected.stderr
assert not bad.exists()
# Failed replacement must preserve the previous same-owner publication.
before = snapshot(parent)
(source / "parent.py").write_text((source / "bad_width.py").read_text())
run(compile_unit("parent.py", parent, [child], replace=True), accepted=False)
assert snapshot(parent) == before

# Link cannot obtain a missing child's body through a source fallback.
(child / "child.ac").rename(hidden / "child.ac")
missing = root / "missing.ac"
run(["link", child, parent, "--top", "bindings.parent.Top", "-o", missing], accepted=False)
assert not missing.exists()
sys.stdout.write("source module bindings: header-only compile, full closure, bool constants, dual emit, type rejection and publication protection passed\n")
