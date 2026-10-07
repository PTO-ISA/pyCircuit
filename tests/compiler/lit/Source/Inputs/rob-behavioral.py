"""Independent table/scalar behavior, frontend rejection and output protection."""
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
for name in ("repo", "source-compiler", "linker", "emitter", "cxx", "verilator", "scratch"):
    parser.add_argument("--" + name, required=True)
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
env = dict(os.environ, PYTHONPATH=str(repo / "python/pycircuit/src"),
           PYTHONDONTWRITEBYTECODE="1", PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
           PYCIRCUIT_LINKER=args.linker, PYCIRCUIT_EMITTER=args.emitter)
commands = []


def run(command, accepted=True, cwd=repo):
    command = list(map(str, command))
    result = subprocess.run(command, env=env, cwd=cwd, capture_output=True,
                            text=True, timeout=240)
    commands.append({"command": command, "exit_status": result.returncode,
                     "stdout": result.stdout, "stderr": result.stderr})
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == (0 if accepted else 1), commands[-1]
    assert "Assertion failed" not in result.stderr and "Traceback" not in result.stderr, commands[-1]
    return result


def cli(*arguments, accepted=True):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], accepted)


def snapshot(directory):
    return {path.relative_to(directory).as_posix(): path.read_bytes()
            for path in directory.rglob("*") if path.is_file()}


toolroot = Path(args.source_compiler).resolve().parent.parent
runtime = next((path for path in (toolroot / "simulator/gfsim/libpyc6_runtime.a",
                                 toolroot / "lib/libpyc6_runtime.a") if path.is_file()), None)
assert runtime is not None, "Runtime archive missing from this build/install"
with tempfile.TemporaryDirectory(prefix="rob-behavioral-", dir=scratch) as temporary:
    build = Path(temporary)
    source = build / "source"
    source.mkdir()
    generic = source / "scatter.py"
    shutil.copyfile(fixtures / "rob-behavioral-generic.py", generic)
    unit = build / "unit"

    def compile_source(path, output, accepted=True, replace=False):
        arguments = ["compile", "-c", path, "--source-root", source,
                     "--package-prefix", "table_probe", "-o", output]
        if replace:
            arguments.append("--replace")
        return cli(*arguments, accepted=accepted)

    compile_source(generic, unit)
    final = build / "scatter.ac"
    cli("link", unit, "--top", "table_probe.scatter.Scatter", "-o", final)
    for target in ("cpp", "verilog"):
        cli("emit", final, "--target", target, "-o", build / target)
    receipt = json.loads((build / "cpp/generated.json").read_text())
    cpp = [build / "cpp" / row["path"] for row in receipt["files"] if row["path"].endswith(".cpp")]
    assert cpp, "Generated source-owned C++ translation unit missing"
    runner = build / "runner"
    run([args.cxx, "-std=c++20", "-pthread", "-I" + str(repo / "include"),
         "-I" + str(build / "cpp"), fixtures / "rob-behavioral-generic.cpp", *cpp, runtime,
         "-o", runner])
    traces = []
    for workers in (1, 2):
        trace = run([runner, str(workers)]).stdout
        (scratch / f"workers-{workers}.stdout").write_text(trace)
        traces.append([row for row in trace.splitlines() if row.startswith("WORK ")])
    assert len(traces[0]) == 262 and traces[0] == traces[1]
    receipt = json.loads((build / "verilog/generated.json").read_text())
    rtl = [build / "verilog" / row["path"] for row in receipt["files"] if row["role"] == "rtl"]
    rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
    with tempfile.TemporaryDirectory(prefix="rob-table-rtl-") as rtl_temporary:
        rtl_build = Path(rtl_temporary)
        run([args.verilator, "--binary", "--timing", "--top-module", "tb", "--prefix", "Vtable",
             "--Mdir", rtl_build, "-j", "2", "-Wno-fatal",
             *sorted((repo / "include/verilog").glob("*.v")), *rtl,
             fixtures / "rob-behavioral-generic.sv"])
        rtl_result = run([rtl_build / "Vtable"])
        (scratch / "rtl.stdout").write_text(rtl_result.stdout)
        rtl_trace = [row for row in rtl_result.stdout.splitlines() if row.startswith("WORK ")]
        assert rtl_trace == traces[0]

    alias = """import pycircuit as ac
@ac.rule
def increment(a, b):
    a = a + 1
    return {"out": b}
@ac.module
def Alias() -> {"out": ac.u3}:
    state: ac.u3 = 0
    return increment(state, state)
"""
    readonly = alias.replace("    a = a + 1\n", "").replace('return {"out": b}', 'return {"out": a ^ b}')
    readonly_path = source / "readonly.py"
    readonly_path.write_text(readonly)
    compile_source(readonly_path, build / "readonly-unit")
    boolean_join = """import pycircuit as ac
@ac.rule
def select(enable):
    flag = False
    if enable:
        flag = True
    return {"out": flag}
@ac.module
def BooleanJoin(enable: ac.u1) -> {"out": ac.u1}:
    return select(enable)
"""
    boolean_path = source / "boolean_join.py"
    boolean_path.write_text(boolean_join)
    compile_source(boolean_path, build / "boolean-unit")
    text = generic.read_text()
    nominal = text.replace("@ac.rule\ndef revise", "@ac.struct\nclass Other:\n    active: ac.u1\n    payload: ac.u7\n\n@ac.rule\ndef revise")
    nominal = nominal.replace("= Parcel(active=1, payload=value)", "= Other(active=1, payload=value)")
    negative_table = """import pycircuit as ac
@ac.rule
def update(state, index):
    state[index] = (0 - 1)
    return {"out": state[index]}
@ac.module
def Underflow(index: ac.u1) -> {"out": ac.u1}:
    state = ac.table[2, ac.u1](init=0)
    return update(state, index)
"""
    negative_branch = """import pycircuit as ac
@ac.rule
def choose(enable):
    local = (0 - 1)
    if enable:
        local = 0
    return {"out": local}
@ac.module
def Unproven(enable: ac.u1) -> {"out": ac.u1}:
    return choose(enable)
"""
    negative_invert = """import pycircuit as ac
@ac.rule
def invert():
    local = ~0
    return {"out": local}
@ac.module
def Unbounded() -> {"out": ac.u1}:
    return invert()
"""
    negative_index_read = """import pycircuit as ac
@ac.rule
def inspect(state):
    return {"out": state[(0 - 1)]}
@ac.module
def NegativeIndex() -> {"out": ac.u1}:
    state = ac.table[2, ac.u1](init=0)
    return inspect(state)
"""
    negative_index_write = """import pycircuit as ac
@ac.rule
def update(state):
    state[(0 - 1)] = 1
    return {"out": state[0]}
@ac.module
def NegativeIndex() -> {"out": ac.u1}:
    state = ac.table[2, ac.u1](init=0)
    return update(state)
"""
    cases = {
        "alias-writable-readonly": (alias, ("alias", "overlap", "owner", "writ")),
        "alias-two-writers": (alias.replace("    a = a + 1\n", "    a = a + 1\n    b = b + 1\n"),
                              ("alias", "overlap", "owner", "writ")),
        "index": (text.replace("index: ac.u3", "index: ac.u4"), ("index", "bound", "range", "extent")),
        "narrowing-input-to-payload": (text.replace("value: ac.u7", "value: ac.u8"), ("narrow",)),
        "nominal": (nominal, ("struct", "type", "nominal", "record")),
        "clock": (text.replace("def Scatter(enable:", "def Scatter(pyc_clk: ac.u1, enable:"), ("reserved", "pyc_clk", "collision")),
        "reset": (text.replace("def Scatter(enable:", "def Scatter(pyc_rst: ac.u1, enable:"), ("reserved", "pyc_rst", "collision")),
        "input-initializer": (text.replace("cursor: ac.u3 = 3", "cursor: ac.u3 = index"),
                              ("initializer", "static")),
        "owner-initializer": (text.replace("    saved = cursor", "    copied: ac.u3 = cursor\n    saved = cursor"),
                              ("initializer", "static")),
        "recursive-struct": (text.replace("payload: ac.bits[7]", "payload: Parcel"),
                             ("cycle", "recursive", "packed", "struct")),
        "boolean-field": (text.replace("active: ac.u1", "active: bool"),
                          ("unsigned", "struct", "bits")),
        "annotated-field": (text.replace("import pycircuit as ac", "from typing import Annotated\nimport pycircuit as ac")
                            .replace("payload: ac.bits[7]", "payload: Annotated[int, range(128)]"),
                            ("unsigned", "struct", "bits")),
        "shadowed-constructor": (text.replace("shadow, enable, index, value):", "shadow, enable, index, value, Parcel):")
                                 .replace("saved, enable, index, value)", "saved, enable, index, value, enable)"),
                                 ("shadow", "declaration", "namespace")),
        "rule-return-annotation": (text.replace("shadow, enable, index, value):", "shadow, enable, index, value) -> ac.u7:"),
                                   ("annotation", "return", "mapping")),
        "negative-struct-field": (text.replace("Parcel(active=1, payload=value)",
                                               "Parcel(active=(0 - 1), payload=value)"),
                                  ("unsigned", "integer", "negative", "represent", "range")),
        "negative-rule-output": (text.replace('"active": active, "cursor": cursor',
                                              '"active": (0 - 1), "cursor": cursor'),
                                 ("unsigned", "integer", "negative", "represent", "range")),
        "negative-table-element": (negative_table,
                                   ("unsigned", "integer", "negative", "represent", "range")),
        "negative-branch-local": (negative_branch,
                                  ("unsigned", "integer", "negative", "represent", "range", "proven")),
        "negative-branch-boolean-output": (negative_branch.replace('-> {"out": ac.u1}:',
                                                                   '-> {"out": bool}:'),
                                           ("unsigned", "integer", "negative", "represent", "range", "proven")),
        "negative-inverted-integer": (negative_invert,
                                      ("unsigned", "integer", "negative", "represent", "range", "proven")),
        "negative-index-read": (negative_index_read,
                                ("index", "range", "unsigned", "integer", "proven")),
        "negative-index-write": (negative_index_write,
                                 ("index", "range", "unsigned", "integer", "proven")),
    }
    unit_before = snapshot(unit)
    # Replacing the valid owning source with each invalid variant must preserve
    # its complete previously published unit byte-for-byte.
    for name, (invalid, diagnostic) in cases.items():
        generic.write_text(invalid)
        result = compile_source(generic, unit, accepted=False, replace=True)
        assert any(word in result.stderr.lower() for word in diagnostic), commands[-1]
        if name.startswith("negative-"):
            assert "scalar region yield types do not match results" not in result.stderr, commands[-1]
        assert snapshot(unit) == unit_before, name
        absent = build / ("invalid-" + name)
        compile_source(generic, absent, accepted=False)
        assert not absent.exists(), name
    generic.write_text(text)
    inputs = [fixtures / ("rob-behavioral-generic." + extension) for extension in ("py", "cpp", "sv")]
    (scratch / "candidate.json").write_text(json.dumps({
        "fixture_sha256": {str(path.relative_to(repo)): hashlib.sha256(path.read_bytes()).hexdigest()
                           for path in inputs},
        "verified_final_sha256": hashlib.sha256(final.read_bytes()).hexdigest(),
        "successful_work_samples": 262, "workers": [1, 2], "rtl": "verilator",
        "rejected_cases": sorted(cases), "readonly_owner_alias_accepted": True,
        "boolean_branch_join_to_u1_accepted": True,
        "rewritten_saved_snapshot_preserves_owner": True,
        "failed_compile_preserved_unit": True,
    }, indent=2) + "\n")
