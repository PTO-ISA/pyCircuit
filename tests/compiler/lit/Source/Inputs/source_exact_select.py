"""Public exact-Integer select and preserved pure-IfExp independent oracles."""
import argparse
import json
import os
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
private = tempfile.TemporaryDirectory(prefix="select-", dir=scratch)
root = Path(private.name)
source = root / "source"
source.mkdir()
fixtures = Path(__file__).resolve().parent
for fixture in fixtures.glob("select_*.py"):
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
    assert result.returncode == (0 if accepted else 1), record
    return result


def cli(*arguments, accepted=True):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], accepted)


def compile_unit(name, target, imports=(), replace=False, accepted=True):
    arguments = ["compile", "-c", source / name, "--source-root", source,
                 "--package-prefix", "selects", "-o", target]
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
    final = output / "select.ac"
    cli("link", *units, "--top", top, "-o", final)
    for target in ("cpp", "verilog"):
        cli("emit", final, "--target", target, "-o", output / target)
    receipt = json.loads((output / "cpp/generated.json").read_text())
    cpp = [output / "cpp" / row["path"] for row in receipt["files"]
           if row["path"].endswith(".cpp")]
    executable = output / "runner"
    defines = ["-DSELECT_FOUR_STATE"] if args.four_state else []
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
        run([args.iverilog, "-g2012", "-s", "tb", "-o", rtl_executable, *rtl,
             fixtures / (bench_stem + "_four_state.sv")])
        rtl_command = [args.vvp, rtl_executable]
    else:
        rtl_build = output / "rtl-build"
        run([args.verilator, "--binary", "--timing", "--top-module", "tb", "--prefix", "Vselect",
             "--Mdir", rtl_build, "-Wno-fatal", *rtl, fixtures / (bench_stem + "_known.sv")])
        rtl_command = [rtl_build / "Vselect"]
    rtl_trace = [line for line in run(rtl_command).stdout.splitlines() if line.startswith("WORK ")]
    assert rtl_trace == traces[0]
    return final, output


unit = root / "select-unit"
compile_unit("select_root.py", unit)
final, products = execute((unit,), "selects.select_root.Select", "select_outputs",
                          6 if args.four_state else 8)
parameter, parameter_top = root / "parameter-unit", root / "parameter-top-unit"
compile_unit("select_parameter.py", parameter)
compile_unit("select_parameter_top.py", parameter_top, [parameter])
execute((parameter, parameter_top), "selects.select_parameter_top.Parameters",
        "select_parameter_outputs", 4 if args.four_state else 6)
child, unknown = root / "child-unit", root / "unknown-unit"
compile_unit("select_child.py", child)
compile_unit("select_unknown_pure.py", unknown, [child])
execute((child, unknown), "selects.select_unknown_pure.UnknownPure",
        "select_unknown_outputs", 4)
if not args.four_state:
    before = snapshot(unit)
    final_before = final.read_bytes()
    emitted_before = {target: snapshot(products / target) for target in ("cpp", "verilog")}
    cases = [("condition_import", ("boolean", "kind")),
             ("branch_import", ("integer", "interval", "kind")),
             ("integer_condition", ("boolean", "kind", "condition")),
             ("boolean_branch", ("integer", "boolean", "kind")),
             ("boolean_destination", ("integer", "boolean", "kind")),
             ("symbolic", ("static", "symbolic", "width", "interval")),
             ("overflow", ("range", "bound", "interval")),
             ("negative", ("range", "bound", "interval")),
             ("checked", ("integer", "arithmetic", "unsupported", "zero")),
             ("literal", ("range", "bound", "interval")),
             ("or", ("type", "width", "equal")),
             ("compare", ("type", "width", "equal")),
             ("parameter_math", ("integer", "interval", "parameter", "proof"))]
    for name, words in cases:
        fixture = "select_bad_" + name + ".py"
        absent = root / ("bad-" + name)
        rejected = compile_unit(fixture, absent, [child], accepted=False)
        assert any(word in rejected.stderr.lower() for word in words), rejected.stderr
        assert not absent.exists()
        (source / "select_root.py").write_text((source / fixture).read_text())
        compile_unit("select_root.py", unit, [child], replace=True, accepted=False)
        assert snapshot(unit) == before
        assert final.read_bytes() == final_before
        assert all(snapshot(products / target) == emitted_before[target] for target in emitted_before)
sys.stdout.write("exact select source gate passed: " +
                 ("6 select +4 parameter +4 opaque X/Z C++/Icarus frames" if args.four_state else
                  "8 select +6 parameter +4 opaque C++/RTL frames and26 protected rejections") + "\n")
