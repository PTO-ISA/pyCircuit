"""Public immutable local binding, capture fact, four-state and publication oracles."""
import argparse
import ast
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
for name in ("repo", "source-compiler", "linker", "emitter", "cxx", "scratch"):
    parser.add_argument("--" + name, required=True)
parser.add_argument("--four-state", action="store_true")
for name in ("verilator", "iverilog", "vvp"):
    parser.add_argument("--" + name)
args = parser.parse_args()
assert (args.iverilog and args.vvp) if args.four_state else args.verilator
repo = Path(args.repo).resolve()
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
private = tempfile.TemporaryDirectory(prefix="local-", dir=scratch)
root = Path(private.name)
source = root / "source"
source.mkdir()
fixtures = Path(__file__).resolve().parent
for fixture in fixtures.glob("local_binding_*.py"):
    shutil.copyfile(fixture, source / fixture.name)
env = dict(os.environ, PYTHONPATH=str(repo / "python/pycircuit/src"),
           PYTHONDONTWRITEBYTECODE="1", PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
           PYCIRCUIT_LINKER=args.linker, PYCIRCUIT_EMITTER=args.emitter)
commands = []


def run(command, accepted=True):
    result = subprocess.run(list(map(str, command)), env=env, cwd=repo,
                            capture_output=True, text=True, timeout=180)
    record = {"command": list(map(str, command)), "exit_status": result.returncode,
              "stdout": result.stdout, "stderr": result.stderr}
    commands.append(record)
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert "Assertion failed" not in result.stderr and "Traceback" not in result.stderr, record
    assert result.returncode in (0, 1) if accepted is None else result.returncode == (0 if accepted else 1), record
    return result


def cli(*arguments, accepted=True):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], accepted)


def compile_unit(name, target, imports=(), replace=False, accepted=True):
    arguments = ["compile", "-c", source / name, "--source-root", source,
                 "--package-prefix", "locals", "-o", target]
    for unit in imports:
        arguments.extend(("-I", unit))
    if replace:
        arguments.append("--replace")
    return cli(*arguments, accepted=accepted)


def snapshot(path):
    return {p.relative_to(path).as_posix(): p.read_bytes()
            for p in path.rglob("*") if p.is_file()}


toolroot = Path(args.source_compiler).resolve().parent.parent
runtime = next((p for p in (toolroot / "simulator/gfsim/libpyc6_runtime.a",
                           toolroot / "lib/libpyc6_runtime.a") if p.is_file()), None)
assert runtime is not None, "Runtime archive missing from compiler build/install"


def execute(units, top, stem, frames):
    output = root / stem
    output.mkdir()
    final = output / "locals.ac"
    cli("link", *units, "--top", top, "-o", final)
    for target in ("cpp", "verilog"):
        cli("emit", final, "--target", target, "-o", output / target)
    receipt = json.loads((output / "cpp/generated.json").read_text())
    cpp = [output / "cpp" / row["path"] for row in receipt["files"]
           if row["path"].endswith(".cpp")]
    executable = output / "runner"
    defines = ["-DLOCAL_BINDING_FOUR_STATE"] if args.four_state else []
    run([args.cxx, "-std=c++20", "-pthread", *defines, "-I" + str(repo / "include"),
         "-I" + str(output / "cpp"), fixtures / (stem + ".cpp"), *cpp, runtime,
         "-o", executable])
    traces = [[line for line in run([executable, str(workers)]).stdout.splitlines()
               if line.startswith("WORK ")] for workers in (1, 2)]
    assert len(traces[0]) == frames and traces[0] == traces[1]
    receipt = json.loads((output / "verilog/generated.json").read_text())
    rtl = [output / "verilog" / row["path"] for row in receipt["files"] if row["role"] == "rtl"]
    rtl.sort(key=lambda p: (p.name != "design_top.sv", str(p)))
    bench_stem = stem.removesuffix("_outputs")
    if args.four_state:
        rtl_executable = output / "rtl-runner"
        run([args.iverilog, "-g2012", "-s", "tb", "-o", rtl_executable, repo / "include/verilog/dff.v", *rtl,
             fixtures / (bench_stem + "_four_state.sv")])
        rtl_command = [args.vvp, rtl_executable]
    else:
        rtl_build = output / "rtl-build"
        run([args.verilator, "--binary", "--timing", "--top-module", "tb", "--prefix", "Vlocal",
             "--Mdir", rtl_build, "-Wno-fatal", repo / "include/verilog/dff.v", *rtl, fixtures / (bench_stem + "_known.sv")])
        rtl_command = [rtl_build / "Vlocal"]
    rtl_trace = [line for line in run(rtl_command).stdout.splitlines() if line.startswith("WORK ")]
    assert rtl_trace == traces[0]
    return final, output


child, unit = root / "child-unit", root / "local-unit"
compile_unit("local_binding_child.py", child)
compile_unit("local_binding_root.py", unit, [child])
# Check producer sharing through original SSA and ordinary boundary resizes.
body = unit / json.loads((unit / "unit.json").read_text())["files"]["body"]
ir = body.read_text()
ast_root = ast.parse((source / "local_binding_root.py").read_text()).body[-1]
line_number = next(node.value.lineno for node in ast_root.body
                   if isinstance(node, ast.Assign) and node.targets[0].id == "candidate")
locations = set(re.findall(r'^#(loc\d+) = loc\("[^"\n]+":' + str(line_number) + r':\d+\)', ir, re.M))
producer_lines = [line for line in ir.splitlines()
                  if '"ac.bits.binary"' in line and 'opcode = "and"' in line
                  and any('loc(#' + location + ')' in line for location in locations)]
assert len(producer_lines) == 1, producer_lines
producer = re.match(r'\s*(%\w+) =', producer_lines[0]).group(1)
rule_line = next(line for line in ir.splitlines() if '"ac.rule"(' in line)
assert producer in re.search(r'"ac.rule"\(([^)]*)\)', rule_line).group(1).split(', ')
outer_yield = next(line for line in ir.splitlines() if line.startswith('    "ac.yield"('))
result = re.search(r'"ac.yield"\(([^)]*)\)', outer_yield).group(1).split(', ')[0]
resizes = {match.group(1): match.group(2) for match in
           re.finditer(r'^\s*(%\w+) = "ac.bits.resize"\((%\w+)\)', ir, re.M)}
seen = set()
while result in resizes and result not in seen:
    seen.add(result)
    result = resizes[result]
assert result == producer, (result, producer)
(scratch / "sharing.json").write_text(json.dumps({"producer": producer,
    "same_producer_feeds_root_output_and_rule_capture": True,
    "body_sha256": hashlib.sha256(body.read_bytes()).hexdigest()}, indent=2) + "\n")
final, products = execute((child, unit), "locals.local_binding_root.Bindings",
                          "local_binding_outputs", 11 if args.four_state else 14)
if not args.four_state:
    before = snapshot(unit)
    final_before = final.read_bytes()
    emitted_before = {target: snapshot(products / target) for target in ("cpp", "verilog")}
    cases = sorted(fixture.stem.removeprefix("local_binding_bad_")
                   for fixture in source.glob("local_binding_bad_*.py")
                   if fixture.stem != "local_binding_bad_cycle")
    scope = ("shadow", "bind", "binding", "name", "collision", "reserved", "rebind", "duplicate")
    for name in cases:
        fixture = "local_binding_bad_" + name + ".py"
        absent = root / ("bad-" + name)
        rejected = compile_unit(fixture, absent, [child], accepted=False)
        if any(word in name for word in ("shadow", "intrinsic", "import_", "constructor", "definition", "instance_then", "local_then", "rule_then", "rebind")):
            words = (*scope, "integer", "kind", "interval", "unsupported", "call", "type")
        elif name in ("self", "forward", "post_return_reference"):
            words = ("unknown", "forward", "wire", "return", "declar", "bind")
        elif name == "post_return":
            words = ("return", "declar", "after", "bind")
        elif any(word in name for word in ("boundary", "kind", "math", "provenance", "shift", "instrument", "mask")):
            words = ("kind", "integer", "boolean", "bound", "range", "interval", "mathematical", "constant", "mask", "type", "unsupported")
        else:
            words = ("assign", "target", "unsupported", "name", "call", "rule")
        assert any(word in rejected.stderr.lower() for word in words), rejected.stderr
        assert not absent.exists()
        (source / "local_binding_root.py").write_text((source / fixture).read_text())
        compile_unit("local_binding_root.py", unit, [child], replace=True, accepted=False)
        assert snapshot(unit) == before
        assert final.read_bytes() == final_before
        assert all(snapshot(products / target) == emitted_before[target] for target in emitted_before)
    cycle = root / "cycle-unit"
    rejected = compile_unit("local_binding_bad_cycle.py", cycle, accepted=None)
    if rejected.returncode:
        assert not cycle.exists()
        cycle_stage = "compile"
        (source / "local_binding_root.py").write_text((source / "local_binding_bad_cycle.py").read_text())
        compile_unit("local_binding_root.py", unit, replace=True, accepted=False)
        assert snapshot(unit) == before
    else:
        cycle_stage = "link; source unit publication allowed before dependency verification"
        cycle_final = root / "cycle.ac"
        rejected = cli("link", cycle, "--top", "locals.local_binding_bad_cycle.Bad",
                       "-o", cycle_final, accepted=False)
        assert not cycle_final.exists()
        cli("link", cycle, "--top", "locals.local_binding_bad_cycle.Bad", "-o", final,
            "--replace", accepted=False)
    assert any(word in rejected.stderr.lower() for word in ("cycle", "cyclic", "dependency", "loop")), rejected.stderr
    assert final.read_bytes() == final_before
    assert all(snapshot(products / target) == emitted_before[target] for target in emitted_before)
    instrument_stages = []
    for entrance in ("only", "mixed"):
        instrument = root / ("instrument-" + entrance)
        compile_unit("local_binding_instrument_" + entrance + ".py", instrument)
        instrument_final = root / ("instrument-" + entrance + ".ac")
        cli("link", instrument, "--top", "locals.local_binding_instrument_" + entrance + ".Instrument", "-o", instrument_final)
        for target in ("cpp", "verilog"):
            absent = root / ("instrument-" + entrance + "-" + target)
            rejected = cli("emit", instrument_final, "--target", target, "-o", absent, accepted=False)
            assert any(word in rejected.stderr.lower() for word in ("instrument", "observation", "observe", "event", "report", "unsupported")), rejected.stderr
            assert not absent.exists()
            cli("emit", instrument_final, "--target", target, "-o", products / target,
                "--replace", accepted=False)
            assert snapshot(products / target) == emitted_before[target]
        instrument_stages.append({"entrance": entrance, "compile_link": "accepted", "emission": "rejected/protected"})
    (scratch / "admission.json").write_text(json.dumps({"compile_rejection_cases": cases,
        "compile_protected_rejections": 2 * len(cases), "cycle_owner": cycle_stage,
        "instrumentation": instrument_stages}, indent=2) + "\n")
evidence_files = [*fixtures.glob("local_binding_*"), fixtures / "source_local_bindings.py",
                  *root.rglob("generated.json"), *root.rglob("unit.json"), *root.rglob("*.ac")]
(scratch / "files.json").write_text(json.dumps({str(path): hashlib.sha256(path.read_bytes()).hexdigest()
    for path in evidence_files if path.is_file()}, indent=2) + "\n")
sys.stdout.write("local binding source gate passed: " +
    ("11 complete X/Z/old-Q C++ workers1/2 + full typed-DFF Icarus frames" if args.four_state else
     "14 known C++ workers1/2 + Verilator frames, protected lexical/semantic/cycle rejections and2 compile-only capture entrances with refused emission") + "\n")
