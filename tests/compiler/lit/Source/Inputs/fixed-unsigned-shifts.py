"""Fixed-width shifts use bit-position oracles, never a DUT lowering recipe."""

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
for name in ("iverilog", "vvp"):
    parser.add_argument("--" + name, default=shutil.which(name))
parser.add_argument("--baseline-rejection", action="store_true")
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
env = dict(os.environ, PYTHONPATH=str(repo / "python/pycircuit/src"),
           PYTHONDONTWRITEBYTECODE="1", PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
           PYCIRCUIT_LINKER=args.linker, PYCIRCUIT_EMITTER=args.emitter)
commands = []


def run(command, accepted=True):
    command = list(map(str, command))
    result = subprocess.run(command, env=env, cwd=repo, capture_output=True,
                            text=True, timeout=240)
    commands.append({"command": command, "exit_status": result.returncode,
                     "stdout": result.stdout, "stderr": result.stderr})
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == (0 if accepted else 1), commands[-1]
    assert "Assertion failed" not in result.stderr and "Traceback" not in result.stderr, commands[-1]
    return result


def cli(*arguments, accepted=True):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], accepted)


def snapshot(path):
    return {p.relative_to(path).as_posix(): p.read_bytes()
            for p in path.rglob("*") if p.is_file()}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


# The matrix is declared in this order; the independent C++/SV oracle follows
# the source schema, not generated IR node counts, names or instruction shapes.
widths = (1, 5, 13, 65, 130)
fields, values = [], []
for width in widths:
    counts = ("0", "1", str(width - 1), str(width), str(width + 1), "(1 << 300)")
    for direction, operator in (("left", "<<"), ("right", ">>")):
        for index, count in enumerate(counts):
            name = f"w{width}_{direction}_{index}"
            fields.append(f"    {name}: ac.bits[{width}]")
            values.append(f"        {name}=n{width} {operator} {count}")
fields.extend(["    local: ac.u13", "    field: ac.bits[65]", "    fixed_wrap: ac.u8",
               "    narrow_first: ac.u16", "    wide_first: ac.u16", "    direct: ac.bits[130]"])
values.extend(["        local=local << (1 + 2)", "        field=packet.value >> 1",
               "        fixed_wrap=(a + 1) >> 8", "        narrow_first=a << 1",
               "        wide_first=wide << 1", "        direct=child.value"])
DESIGN = """from typing import Annotated
import pycircuit as ac

@ac.struct
class Packet:
    value: ac.bits[65]

@ac.struct
class Wide:
    value: ac.bits[130]

@ac.module
def ShiftChild(value: ac.bits[130]) -> Wide:
    return Wide(value=value << 1)

@ac.struct
class ShiftResult:
""" + "\n".join(fields) + """

@ac.rule
def evaluate(n1, n5, n13, n65, n130, a, child) -> ShiftResult:
    local: ac.u13 = n13
    packet = Packet(value=n65)
    wide: ac.u16 = a
    return ShiftResult(
""" + ",\n".join(values) + """
    )

@ac.module
def Top(n1: ac.u1, n5: ac.u5, n13: ac.u13, n65: ac.bits[65],
        n130: ac.bits[130], a: ac.u8) -> ShiftResult:
    child = ShiftChild(n130)
    return evaluate(n1, n5, n13, n65, n130, a, child)

@ac.module
def ExactInteger(a: Annotated[int, range(1 << 8)]) -> {
    "identity": Annotated[int, range(1 << 8)],
    "add_before": Annotated[int, range(1 << 1)],
    "static_signed": Annotated[int, range(1 << 8)]}:
    return {"identity": a >> 0, "add_before": (a + 1) >> 8,
            "static_signed": ((0 - 7) >> 1) & 255}
"""


def scalar(expression, base="ac.u5", extra="", output="ac.u5", body=""):
    return ("import pycircuit as ac\nfrom typing import Annotated\n@ac.struct\nclass Result:\n"
            f"    value: {output}\n@ac.module\ndef Top(value: {base}{extra}) -> Result:\n"
            + body + f"    return Result(value={expression})\n")


def cycle(operator, dead):
    return ("import pycircuit as ac\n@ac.struct\nclass Cell:\n    value: ac.u5\n"
            "@ac.module\ndef Child(value: ac.u5) -> Cell:\n"
            f"    return Cell(value=value {operator} (1 << 300))\n"
            "@ac.module\ndef Top(value: ac.u5) -> Cell:\n"
            "    child = Child(child.value)\n"
            + ("    return Cell(value=value)\n" if dead else "    return child\n"))


def local_count(operator, use_count=True):
    expression = f"value {operator} count" if use_count else "value"
    return ("import pycircuit as ac\n@ac.struct\nclass Result:\n    value: ac.u5\n"
            "@ac.rule\ndef evaluate(value) -> Result:\n    count = 1\n"
            f"    return Result(value={expression})\n"
            "@ac.module\ndef Top(value: ac.u5) -> Result:\n    return evaluate(value)\n")


def formal_count(operator, count="count"):
    # Static formals belong to the existing structural module profile. A typed
    # struct behavioral module would reject the declaration before this guard.
    return ("import pycircuit as ac\n@ac.module\n"
            "def Top(value: ac.u5, *, count: int = 1) -> {\"out\": ac.u5}:\n"
            f"    return {{\"out\": value {operator} {count}}}\n")


unsupported = "source operator requires supported exact integer or width-preserving bitwise lowering"
integer_input = "mathematical arithmetic requires known Integer kind and finite interval"
cycle_guard = "'ac.module' op combinational cycle in hardware field dependencies"
cases = {}
diagnostics = {}
for label, operator in (("left", "<<"), ("right", ">>")):
    static_guard = ("unsigned shift requires a proven static Integer count" if label == "left" else
                    "integer right shift requires a proven static Integer count")
    negative_guard = ("unsigned shift count must be nonnegative" if label == "left" else
                      "integer right-shift count must be nonnegative")
    for name, count, extra in (
            ("true-count", "True", ""), ("false-count", "False", ""),
            ("dynamic-count", "count", ", count: ac.u3"),
            ("singleton-runtime-count", "(count * 0)", ", count: Annotated[int, range(1 << 5)]"),
            ("unbound-count", "MISSING", ""),
            ("conditional-count", "(1 if choose else 0)", ", choose: ac.u1")):
        key = label + "-" + name
        cases[key] = scalar(f"value {operator} {count}", extra=extra)
        diagnostics[key] = static_guard
    cases[label + "-default-formal-count"] = formal_count(operator)
    diagnostics[label + "-default-formal-count"] = static_guard
    cases[label + "-local-count"] = local_count(operator)
    diagnostics[label + "-local-count"] = static_guard
    cases[label + "-negative-count"] = scalar(f"value {operator} (0 - 1)")
    cases[label + "-zero-negative-count"] = scalar(f"(value * 0) {operator} (0 - 1)")
    cases[label + "-zero-huge-negative-count"] = scalar(f"(value * 0) {operator} (0 - (1 << 300))")
    for name in ("negative-count", "zero-negative-count", "zero-huge-negative-count"):
        diagnostics[label + "-" + name] = negative_guard
    cases[label + "-zero-dynamic-count"] = scalar(f"(value * 0) {operator} count", extra=", count: ac.u3")
    diagnostics[label + "-zero-dynamic-count"] = static_guard
    for suffix, count in (("zero", "0"), ("overshift", "(1 << 300)")):
        key = label + "-boolean-input-" + suffix
        cases[key] = scalar(f"value {operator} {count}", base="bool", output="ac.u1")
        diagnostics[key] = unsupported if label == "left" else integer_input
        key = label + "-comparison-input-" + suffix
        cases[key] = scalar(f"(value == value) {operator} {count}", output="ac.u1")
        diagnostics[key] = unsupported if label == "left" else integer_input
        key = label + "-runtime-boolean-fixed-peer-" + suffix
        cases[key] = scalar(f"(value if choose else (value == value)) {operator} {count}",
                            base="ac.u1", extra=", choose: ac.u1", output="ac.u1")
        diagnostics[key] = "fixed branch peer requires a closed source Integer or Boolean constant"
    # A result stays width5 even when a right shift mathematically fits width1,
    # and even when the result is known zero. Implicit narrowing still rejects.
    key = label + "-narrow-overshift"
    cases[key] = scalar(f"value {operator} 5", output="ac.u1")
    diagnostics[key] = "unsigned boundary implicit narrowing is unsupported"
    for dead in (False, True):
        key = label + ("-dead-overshift-cycle" if dead else "-observed-overshift-cycle")
        cases[key] = cycle(operator, dead)
        diagnostics[key] = cycle_guard
for count in ("0", "1", "(1 << 300)", "True", "(0 - 1)"):
    key = "runtime-integer-left-" + count
    cases[key] = scalar("value << " + count, base="Annotated[int, range(1 << 5)]")
    diagnostics[key] = unsupported
cases["integer-negative-input-overshift"] = scalar(
    "(value - 3) >> (1 << 300)", base="Annotated[int, range(1 << 5)]")
diagnostics["integer-negative-input-overshift"] = "integer right shift requires a proven nonnegative input interval"
positive_controls = {
    "conditional-authority": scalar("(value if choose else value) << 0",
                                    base="ac.u1", extra=", choose: ac.u1", output="ac.u1"),
    "ordinary-local": local_count("<<", use_count=False),
    "static-formal-profile": formal_count("<<", count="0"),
}

with tempfile.TemporaryDirectory(prefix="fixed-shifts-", dir=scratch) as temporary:
    build = Path(temporary)
    source = build / "source"
    source.mkdir()
    design = source / "design.py"
    design.write_text(DESIGN)
    (scratch / "design.py").write_text(DESIGN)
    unit = build / "unit"

    def compile_source(output, accepted=True, replace=False):
        arguments = ["compile", "-c", design, "--source-root", source,
                     "--package-prefix", "unsigned_shifts", "-o", output]
        if replace:
            arguments.append("--replace")
        return cli(*arguments, accepted=accepted)

    if args.baseline_rejection:
        rejected = compile_source(unit, accepted=False)
        assert "source operator requires supported exact integer or width-preserving bitwise lowering" in rejected.stderr
        assert not unit.exists()
        sys.stdout.write("fixed unsigned shifts baseline rejects the public positive fixture\n")
        sys.exit(0)
    compile_source(unit)
    toolroot = Path(args.source_compiler).resolve().parent.parent
    runtime = next((path for path in (toolroot / "simulator/gfsim/libpyc6_runtime.a",
                                     toolroot / "lib/libpyc6_runtime.a") if path.is_file()), None)
    assert runtime is not None, "Runtime archive missing from this build/install"
    products, receipts = [], []
    for top, exact, masks in (("Top", False, 4), ("ExactInteger", True, 0)):
        output = build / top
        output.mkdir()
        final = output / "design_top.ac"
        cli("link", unit, "--top", "unsigned_shifts.design." + top, "-o", final)
        for target in ("cpp", "verilog"):
            cli("emit", final, "--target", target, "-o", output / target)
        receipt = json.loads((output / "cpp/generated.json").read_text())
        cpp = [output / "cpp" / row["path"] for row in receipt["files"]
               if row["path"].endswith(".cpp")]
        assert cpp, "Generated source-owned C++ translation unit missing"
        defines = ["-DEXACT_INTEGER"] if exact else []
        runner = output / "runner"
        run([args.cxx, "-std=c++20", "-pthread", *defines, "-I" + str(repo / "include"),
             "-I" + str(output / "cpp"), fixtures / "fixed-unsigned-shifts.cpp", *cpp, runtime, "-o", runner])
        traces = []
        for workers in (1, 2):
            trace = run([runner, str(workers)]).stdout
            (scratch / f"{top}-workers-{workers}.stdout").write_text(trace)
            traces.append([row for row in trace.splitlines() if row.startswith(("WORK ", "MASK "))])
        assert len(traces[0]) == 16 + masks and traces[0] == traces[1]
        receipt = json.loads((output / "verilog/generated.json").read_text())
        rtl = [output / "verilog" / row["path"] for row in receipt["files"] if row["role"] == "rtl"]
        rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
        rtl_build = output / "rtl-build"
        run([args.verilator, "--binary", "--timing", "--top-module", "tb", "--prefix", "Vunsigned_shifts",
             "--Mdir", rtl_build, "-j", "2", "-Wno-fatal", *defines, *rtl, fixtures / "fixed-unsigned-shifts.sv"])
        rtl_trace = run([rtl_build / "Vunsigned_shifts"]).stdout
        (scratch / f"{top}-rtl.stdout").write_text(rtl_trace)
        assert [row for row in rtl_trace.splitlines() if row.startswith("WORK ")] == [
            row for row in traces[0] if row.startswith("WORK ")]
        if args.iverilog and args.vvp:
            rtl_runner = output / "rtl-four-state"
            run([args.iverilog, "-g2012", "-DFIXED_SHIFTS_FOUR_STATE", *defines, "-s", "tb",
                 "-o", rtl_runner, *rtl, fixtures / "fixed-unsigned-shifts.sv"])
            four = run([args.vvp, rtl_runner]).stdout
            (scratch / f"{top}-rtl-four-state.stdout").write_text(four)
            assert [row for row in four.splitlines() if row.startswith(("WORK ", "MASK "))] == traces[0]
        products.extend([final, output / "cpp", output / "verilog"])
        receipts.append({"top": top, "final_sha256": digest(final), "runner_sha256": digest(runner),
                         "generated": {target: {name: hashlib.sha256(value).hexdigest()
                                                for name, value in snapshot(output / target).items()}
                                       for target in ("cpp", "verilog")}})

    before = snapshot(unit)
    protected = {path: snapshot(path) if path.is_dir() else path.read_bytes() for path in products}
    for name, text in positive_controls.items():
        design.write_text(text)
        (scratch / ("control-" + name + ".py")).write_text(text)
        compile_source(build / ("control-" + name))
    assert cases.keys() == diagnostics.keys()
    for name, text in cases.items():
        design.write_text(text)
        (scratch / (name + ".py")).write_text(text)
        absent = build / ("invalid-" + name)
        rejected = compile_source(absent, accepted=False)
        assert diagnostics[name] in rejected.stderr, rejected.stderr
        assert not absent.exists()
        replacement = compile_source(unit, accepted=False, replace=True)
        assert diagnostics[name] in replacement.stderr, replacement.stderr
        assert snapshot(unit) == before
        assert all((snapshot(path) if path.is_dir() else path.read_bytes()) == value
                   for path, value in protected.items())
    inputs = [fixtures / ("fixed-unsigned-shifts" + extension) for extension in (".py", ".cpp", ".sv")]
    inputs.append(fixtures.parent / "fixed-unsigned-shifts.test")
    (scratch / "candidate.json").write_text(json.dumps({
        "fixtures": {str(path.relative_to(repo)): digest(path) for path in inputs},
        "design_sha256": hashlib.sha256(DESIGN.encode()).hexdigest(), "products": receipts,
        "known_frames": 32, "four_state_frames": 4, "workers": [1, 2],
        "rejected_cases": sorted(cases), "protected_rejections": len(cases) * 2,
        "positive_controls": sorted(positive_controls),
        "unreachable_source_guard": "unresolved fixed width: active declarations reject before shift; helper inspection required",
        "missing_origin_guard_gap": "mixed Boolean/fixed conditional rejects at shared join; no missing-origin shift guard coverage claimed",
        "icarus_four_state": bool(args.iverilog and args.vvp),
    }, indent=2) + "\n")

sys.stdout.write(f"fixed unsigned shifts gate passed: 32 worker-1/2 and RTL frames; 4 native X/Z frames; {len(cases) * 2} protected rejections; " +
                 ("4 Icarus X/Z frames\n" if args.iverilog and args.vvp else "Icarus X/Z unavailable\n"))
