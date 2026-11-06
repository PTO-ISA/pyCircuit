"""Public ACIR export survives loss of Python and lowers in a fresh process."""

import argparse
import ast
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser()
for option in ("repo", "source-compiler", "linker", "optimizer", "scratch"):
    parser.add_argument("--" + option, required=True)
args = parser.parse_args()
repo = Path(args.repo).resolve()
evidence = Path(args.scratch).resolve()
evidence.mkdir(parents=True, exist_ok=True)
scratch = Path(tempfile.mkdtemp(prefix="run-", dir=evidence))
source = scratch / "source"
source.mkdir(exist_ok=True)
fixtures = runpy.run_path(
    str(Path(__file__).with_name("pure-scalar-helpers-oracles.py"))
)
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
)
commands = []


def run(command, success=True, environment=None):
    result = subprocess.run(
        list(map(str, command)),
        cwd=scratch,
        env=environment or env,
        text=True,
        capture_output=True,
        timeout=120,
    )
    commands.append(
        {
            "command": list(map(str, command)),
            "exit_status": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
        }
    )
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert (result.returncode == 0) == success, commands[-1]
    assert "Assertion failed" not in result.stderr, commands[-1]
    return result


def compile_source(path, output, *options, success=True):
    return run(
        [
            sys.executable,
            "-m",
            "pycircuit.cli",
            "compile",
            "-c",
            path,
            "--source-root",
            source,
            "--package-prefix",
            "roundtrip",
            "-o",
            output,
            *options,
        ],
        success=success,
    )


def canonical(path, output):
    # Both artifacts are parsed independently. Strip printer-only locations by
    # using the optimizer's default printer rather than editing textual IR.
    run([args.optimizer, path, "-o", output])
    return output.read_text()


def check_nested_call_sites(text):
    # Derive call paths from original source, independently of lowered SSA.
    tree = ast.parse(fixtures["NAMED_MAP_DESIGN"])
    paths = {}

    def visit(node, path=()):
        paths[node] = path
        for field, value in ast.iter_fields(node):
            if isinstance(value, ast.AST):
                visit(value, path + (field,))
            elif isinstance(value, list):
                for index, child in enumerate(value):
                    if isinstance(child, ast.AST):
                        visit(child, path + (field, index))

    visit(tree)
    mix = next(
        node
        for node in tree.body
        if isinstance(node, ast.FunctionDef) and node.name == "mix"
    )
    bump = next(
        node
        for node in ast.walk(mix)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Name)
        and node.func.id == "bump"
    )
    inner_path = paths[bump]
    top = next(
        node
        for node in tree.body
        if isinstance(node, ast.FunctionDef) and node.name == "Top"
    )
    outer_paths = {
        paths[node]
        for node in ast.walk(top)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Name)
        and node.func.id == "mix"
    }
    checked = 0
    for expansion in re.findall(r"expansion = \[(.*?)\], site =", text):
        frames = []
        for callee, path, caller in re.findall(
            r'callee = @([\w.$-]+), kind = "call", site = \{ast_path = \[(.*?)\], definition = @([\w.$-]+)\}',
            expansion,
        ):
            coordinates = tuple(
                name or int(index)
                for name, index in re.findall(
                    r'name = "([^"]+)"|value = (\d+) : i64', path
                )
            )
            frames.append(
                (callee.rsplit(".", 1)[-1], coordinates, caller.rsplit(".", 1)[-1])
            )
        outer = next(
            (
                index
                for index, frame in enumerate(frames)
                if frame[0] == "mix" and frame[2] == "Top" and frame[1] in outer_paths
            ),
            None,
        )
        if outer is not None and any(
            frame[0] == "bump" and frame[2] == "mix" for frame in frames
        ):
            assert outer + 1 < len(frames), frames
            assert frames[outer + 1] == ("bump", inner_path, "mix"), frames
            checked += 1
    assert checked, "no nested Top -> mix -> bump source occurrence found"


cases = []
for name, fixture in (("count", "COUNT_DESIGN"), ("maps", "NAMED_MAP_DESIGN")):
    path = source / (name + ".py")
    path.write_text(fixtures[fixture])
    acir = scratch / (name + ".acir")
    compile_source(path, acir, "--emit", "acir")
    exported = acir.read_bytes()
    text = exported.decode()
    assert "func.func" in text and '"ac.call"' in text, text
    callees = re.findall(r'callee = @("[^\"]+"|[\w.$-]+)', text)
    assert callees, text
    repeated = max(set(callees), key=callees.count)
    assert callees.count(repeated) >= 3, callees
    assert (
        len(
            re.findall(r"func\.func(?: private)? @" + re.escape(repeated) + r"\(", text)
        )
        == 1
    ), text
    if name == "maps":
        assert re.search(r"ac\.query|ac\.table\.(?:query|callback)", text), text
        before = set(scratch.rglob("*"))
        stdout = compile_source(path, "-", "--emit", "acir")
        assert stdout.stdout.encode() == exported
        assert set(scratch.rglob("*")) == before, "stdout export published artifacts"
        compile_source(path, acir, "--emit", "acir", success=False)
        assert acir.read_bytes() == exported
        compile_source(path, acir, "--emit", "acir", "--replace")
        assert acir.read_bytes() == exported
    unit = scratch / (name + ".unit")
    compile_source(path, unit)
    receipt = json.loads((unit / "unit.json").read_text())
    body = unit / receipt["files"]["body"]
    cases.append((name, acir, body))

# These existing contracts require post-inline range reasoning. Export keeps
# the original carrier; only standalone lowering decides index admissibility.
admission = runpy.run_path(
    str(Path(__file__).with_name("pure-scalar-helpers-admission.py"))
)
deferred = []
for valid, rows, selected in (
    (
        True,
        admission["aggregate_fact_controls"](),
        "aggregate_scalar_nests_preserve_projected_kinds_and_ranges",
    ),
    (
        False,
        admission["aggregate_fact_admission_cases"](),
        "aggregate_return_does_not_narrow_unchecked_table_index",
    ),
):
    case = next(row for row in rows if row["name"] == selected)
    path = source / (selected + ".py")
    path.write_text(case["text"])
    acir = scratch / (selected + ".acir")
    compile_source(path, acir, "--emit", "acir")
    assert '"ac.index_check"' in acir.read_text()
    deferred.append((valid, acir))
    if not valid:
        protected_unit = scratch / "maps.unit"
        before = {
            p.relative_to(protected_unit): p.read_bytes()
            for p in protected_unit.rglob("*")
            if p.is_file()
        }
        compile_source(path, protected_unit, "--replace", success=False)
        assert before == {
            p.relative_to(protected_unit): p.read_bytes()
            for p in protected_unit.rglob("*")
            if p.is_file()
        }

# Two pure helpers provide one legacy-call purity boundary.
provider = source / "provider.py"
provider.write_text(
    """import pycircuit as ac
def bump(value: ac.u8) -> ac.u8:
    return value + 1

def chain(value: ac.u8) -> ac.u8:
    return bump(value)
"""
)
provider_acir = scratch / "provider.acir"
compile_source(provider, provider_acir, "--emit", "acir")
legacy = provider_acir.read_text().replace('"ac.call"', '"func.call"')
legacy_path = scratch / "legacy-helper.acir"
legacy_path.write_text(legacy)
run(
    [
        args.optimizer,
        legacy_path,
        "--ac-verify-acir",
        "-o",
        scratch / "legacy-verified.acir",
    ]
)
legacy_path.write_text(
    legacy.replace('ac.helper_kind = "value"', 'ac.helper_kind = "external"', 1)
)
run(
    [
        args.optimizer,
        legacy_path,
        "--ac-verify-acir",
        "-o",
        scratch / "legacy-rejected.acir",
    ],
    success=False,
)

# Invalid capture must preserve an existing ACIR destination during replacement.
invalid = source / "invalid.py"
invalid.write_text("def invalid(:\n")
protected = scratch / "protected.acir"
protected.write_bytes(b"existing destination\n")
compile_source(invalid, protected, "--emit", "acir", "--replace", success=False)
assert protected.read_bytes() == b"existing destination\n"

# The optimizer receives only text. Source no longer exists at the captured
# paths, and no Python module search path or compiler driver is supplied.
source.rename(scratch / "unavailable-python")
standalone_env = dict(os.environ)
standalone_env.pop("PYTHONPATH", None)
standalone_env.pop("PYCIRCUIT_SOURCE_COMPILER", None)
standalone_env.pop("PYCIRCUIT_LINKER", None)
for name, acir, body in cases:
    lowered = scratch / (name + ".lowered.ac")
    run(
        [
            args.optimizer,
            acir,
            "--pass-pipeline=builtin.module(acir-to-source,ac-analyze-source-method-summaries,ac-simplify-record-wires)",
            "-o",
            lowered,
        ],
        environment=standalone_env,
    )
    result = canonical(lowered, scratch / (name + ".canonical.ac"))
    expected = canonical(body, scratch / (name + ".expected.ac"))
    if name == "maps":
        check_nested_call_sites(result)
    assert result == expected, (name, lowered, body)
for valid, acir in deferred:
    lowered = scratch / (acir.stem + ".lowered.ac")
    run(
        [
            args.optimizer,
            acir,
            "--pass-pipeline=builtin.module(acir-to-source)",
            "-o",
            lowered,
        ],
        success=valid,
        environment=standalone_env,
    )
    if valid:
        assert '"ac.index_check"' not in lowered.read_text()
print("ACIR file/stdout publication and standalone source roundtrip passed")
