"""Independent exact-integer source, native DUT, RTL and publication gates."""
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
private = tempfile.TemporaryDirectory(prefix="numeric-", dir=scratch)
root = Path(private.name)
source = root / "source"
source.mkdir()
fixtures = Path(__file__).resolve().parent
for fixture in fixtures.glob("numeric_*.py"):
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
                 "--package-prefix", "numeric", "-o", target]
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
assert runtime is not None, "Runtime archive missing from the compiler build/install"


def execute(unit, top, stem, frames, four_state=False, primitives=()):
    output = root / stem
    output.mkdir()
    final = output / "numeric.ac"
    units = unit if isinstance(unit, tuple) else (unit,)
    cli("link", *units, "--top", top, "-o", final)
    for target in ("cpp", "verilog"):
        cli("emit", final, "--target", target, "-o", output / target)
    receipt = json.loads((output / "cpp/generated.json").read_text())
    cpp = [output / "cpp" / row["path"] for row in receipt["files"]
           if row["path"].endswith(".cpp")]
    executable = output / "runner"
    defines = ["-DNUMERIC_FOUR_STATE"] if four_state else []
    run([args.cxx, "-std=c++20", "-pthread", *defines, "-I" + str(repo / "include"),
         "-I" + str(output / "cpp"), fixtures / (stem + ".cpp"), *cpp, runtime,
         "-o", executable])
    traces = [[line for line in run([executable, str(workers)]).stdout.splitlines()
               if line.startswith("WORK ")] for workers in (1, 2)]
    assert len(traces[0]) == frames and traces[0] == traces[1]
    receipt = json.loads((output / "verilog/generated.json").read_text())
    rtl = [output / "verilog" / row["path"] for row in receipt["files"] if row["role"] == "rtl"]
    rtl.sort(key=lambda p: (p.name != "design_top.sv", str(p)))
    if four_state:
        rtl_executable = output / "rtl-runner"
        run([args.iverilog, "-g2012", "-s", "tb", "-o", rtl_executable, *rtl,
             fixtures / "numeric_four_state.sv"])
        rtl_command = [args.vvp, rtl_executable]
    else:
        rtl_build = output / "rtl-build"
        bench = "numeric_known.sv" if stem == "numeric_outputs" else stem + ".sv"
        run([args.verilator, "--binary", "--timing", "--top-module", "tb", "--prefix", "Vnumeric",
             "--Mdir", rtl_build, "-Wno-fatal", *primitives, *rtl, fixtures / bench])
        rtl_command = [rtl_build / "Vnumeric"]
    rtl_trace = [line for line in run(rtl_command).stdout.splitlines() if line.startswith("WORK ")]
    assert rtl_trace == traces[0]
    return final, output


numeric = root / "numeric-unit"
compile_unit("numeric_root.py", numeric)
final, products = execute(numeric, "numeric.numeric_root.Numeric", "numeric_outputs",
                          2 if args.four_state else 9, args.four_state)
if not args.four_state:
    capture = root / "capture-unit"
    compile_unit("numeric_capture.py", capture)
    execute(capture, "numeric.numeric_capture.Capture", "numeric_capture", 9,
            primitives=[repo / "include/verilog/dff.v", repo / "include/verilog/dffe.v"])
    mask, mask_top = root / "mask-unit", root / "mask-top-unit"
    compile_unit("numeric_mask.py", mask)
    compile_unit("numeric_mask_top.py", mask_top, [mask])
    execute((mask, mask_top), "numeric.numeric_mask_top.MaskTop", "numeric_masks", 6)
    child = root / "child-unit"
    compile_unit("numeric_child.py", child)
    before = snapshot(numeric)
    final_before = final.read_bytes()
    emitted_before = {target: snapshot(products / target) for target in ("cpp", "verilog")}
    cases = [("bool", ("integer", "boolean", "kind")),
             ("bool_literal", ("integer", "boolean", "kind")),
             ("integer_bool", ("boolean", "kind")),
             ("literal", ("boolean", "kind")),
             ("capture", ("boolean", "kind")),
             ("overflow", ("range", "bound", "overflow")),
             ("negative", ("range", "bound", "negative")),
             ("symbolic", ("static", "symbolic", "width", "range", "interval")),
             ("import", ("kind", "integer")),
             ("static_compare", ("mathematical", "arithmetic", "signed")),
             ("static_xor", ("mathematical", "arithmetic", "signed")),
             ("static_branch", ("mathematical", "arithmetic", "signed", "bound")),
             ("integer_condition", ("boolean", "kind", "conditional")),
             ("negative_mask", ("mask", "negative", "range", "bound")),
             ("negative_mask_swapped", ("mask", "negative", "range", "bound")),
             ("mixed_xor", ("boolean", "integer", "kind")),
             ("mixed_branch", ("boolean", "integer", "kind")),
             ("mixed_compare", ("boolean", "integer", "kind"))]
    for name, words in cases:
        fixture = "numeric_bad_" + name + ".py"
        absent = root / ("bad-" + name)
        rejected = compile_unit(fixture, absent, [child], accepted=False)
        assert any(word in rejected.stderr.lower() for word in words), rejected.stderr
        assert not absent.exists()
        (source / "numeric_root.py").write_text((source / fixture).read_text())
        compile_unit("numeric_root.py", numeric, [child], replace=True, accepted=False)
        assert snapshot(numeric) == before
        assert final.read_bytes() == final_before
        assert all(snapshot(products / target) == emitted_before[target] for target in emitted_before)
sys.stdout.write("exact integer public source gate passed: " +
                 ("2 high-bit X/Z C++/Icarus frames" if args.four_state else
                  "9 arithmetic + 9 captured-register + 6 mask C++/RTL frames and 36 protected rejections") + "\n")
