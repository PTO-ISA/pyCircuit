"""Explicit unsigned slices, independent bit positions and unchanged Table indexing."""

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
for name in (
    "repo",
    "source-compiler",
    "linker",
    "emitter",
    "cxx",
    "verilator",
    "scratch",
):
    parser.add_argument("--" + name, required=True)
for name in ("iverilog", "vvp"):
    parser.add_argument("--" + name, default=shutil.which(name))
parser.add_argument("--baseline-rejection", action="store_true")
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python/pycircuit/src"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
    PYCIRCUIT_EMITTER=args.emitter,
)
commands = []


def run(command, accepted=True):
    command = list(map(str, command))
    result = subprocess.run(
        command, env=env, cwd=repo, capture_output=True, text=True, timeout=240
    )
    commands.append(
        {
            "command": command,
            "exit_status": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
        }
    )
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == (0 if accepted else 1), commands[-1]
    assert (
        "Assertion failed" not in result.stderr and "Traceback" not in result.stderr
    ), commands[-1]
    return result


def cli(*arguments, accepted=True):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], accepted)


def snapshot(directory):
    return {
        path.relative_to(directory).as_posix(): path.read_bytes()
        for path in directory.rglob("*")
        if path.is_file()
    }


DESIGN = """import pycircuit as ac

@ac.struct
class Address:
    value: ac.bits[40]

@ac.struct
class ProbeResult:
    one: ac.u1
    low: ac.u5
    middle: ac.u7
    high: ac.u5
    full: ac.u13
    nested: ac.u4
    tag: ac.bits[28]
    wide_low: ac.u9
    wide_middle: ac.bits[17]
    wide_high: ac.u9
    wide_full: ac.bits[73]
    rule_low: ac.u6
    arithmetic: ac.u13
    arithmetic_low: ac.u6
    selected: ac.bits[40]
    selected_slice: ac.u13

@ac.rule
def extract(tiny, n13, address, wide, choose) -> ProbeResult:
    packet = Address(value=address)
    local: ac.bits[13] = n13
    arithmetic = n13 + 1
    selected = address if choose else wide[:40]
    return ProbeResult(one=tiny[::(3 - 2)], low=n13[:5],
                       middle=n13[(1 + 2):(5 * 2)], high=n13[8:],
                       full=n13[:], nested=n13[2:11][3:7],
                       tag=packet.value[12:40], wide_low=wide[:9],
                       wide_middle=wide[55:72:(2 - 1)], wide_high=wide[64:],
                       wide_full=wide[:], rule_low=local[1:7],
                       arithmetic=arithmetic, arithmetic_low=arithmetic[0:6],
                       selected=selected, selected_slice=selected[4:17])

@ac.module
def Top(tiny: ac.u1, n13: ac.u13, address: ac.bits[40],
        wide: ac.bits[73], choose: ac.u1) -> ProbeResult:
    return extract(tiny, n13, address, wide, choose)

@ac.struct
class Entry:
    payload: ac.u13

@ac.struct
class TableResult:
    whole: ac.u13
    part: ac.u7

@ac.rule
def table_extract(entries, index, value, write) -> TableResult:
    saved = entries[index]
    if write:
        entries[index] = Entry(payload=value)
    return TableResult(whole=saved.payload, part=saved.payload[2:9])

@ac.module
def TableTop(index: ac.u1, value: ac.u13, write: ac.u1) -> TableResult:
    entries = ac.table[2, Entry](init=0)
    return table_extract(entries, index, value, write)
"""

PREFIX = "import pycircuit as ac\n@ac.struct\nclass Result:\n    value: ac.u5\n"

FACT_DESIGN = """from typing import Annotated
@ac.struct
class FactResult:
    identity: ac.u3
    widened: ac.u8
    low: ac.u3
    full: ac.u3
    high: ac.u2
    old_alias: ac.u4
    closed: ac.u8
    fixed_left: ac.u8
    fixed_right: ac.u8
    literal_left: ac.u8
    literal_right: ac.u8
    boolean_left: ac.u1
    boolean_right: ac.u1
@ac.rule
def transport_facts(value, raw, choose) -> FactResult:
    original = (value & 3) + 1
    old_alias = original
    identity: ac.u3 = original
    widened: ac.u8 = identity
    closed: ac.u3 = 2
    closed_wide: ac.u8 = closed
    one: ac.u1 = 1
    return FactResult(identity=identity, widened=widened,
                      low=widened[:3], full=identity[:], high=widened[1:3],
                      old_alias=old_alias + 1, closed=closed_wide,
                      fixed_left=closed_wide if choose else raw,
                      fixed_right=raw if choose else closed_wide,
                      literal_left=closed_wide if choose else 3,
                      literal_right=3 if choose else closed_wide,
                      boolean_left=one if choose else False,
                      boolean_right=False if choose else one)
@ac.module
def FactTop(value: Annotated[int, range(1 << 8)], raw: ac.u8,
            choose: ac.u1) -> FactResult:
    return transport_facts(value, raw, choose)
"""
DESIGN += FACT_DESIGN


def fact_index(body, index, extent=5, inputs="value: ac.u8"):
    return (
        PREFIX
        + "from typing import Annotated\n@ac.rule\ndef evaluate(entries, value) -> Result:\n"
        + body
        + f"    return Result(value=entries[{index}])\n"
        + f"@ac.module\ndef Top({inputs}) -> Result:\n"
        + f"    entries = ac.table[{extent}, ac.u5](init=0)\n"
        + "    return evaluate(entries, value)\n"
    )


logical_input = "value: Annotated[int, range(1 << 8)]"
fact_body = "    original = (value & 3) + 1\n    converted: ac.u3 = original\n"
fact_controls = {
    "integer-fixed-identity": fact_index(fact_body, "converted", inputs=logical_input),
    "integer-fixed-zext": fact_index(
        fact_body + "    widened: ac.u8 = converted\n", "widened", inputs=logical_input
    ),
    "low-fitting": fact_index(
        fact_body + "    widened: ac.u8 = converted\n",
        "widened[:3]",
        inputs=logical_input,
    ),
    "full-low-zero": fact_index(fact_body, "converted[:]", inputs=logical_input),
    "fixed-remainder-zext": fact_index(
        "    original = value[:3] % 5\n    widened: ac.u8 = original\n", "widened"
    ),
    "closed-fixed-index": fact_index(
        "    closed: ac.u3 = 2\n    widened: ac.u8 = closed\n", "widened", extent=3
    ),
    "high-carrier": fact_index("    original = value % 3\n", "original[1:2]", extent=2),
    "nonfitting-carrier": fact_index(
        "    original = value % 5\n", "original[:2]", extent=4
    ),
}
fact_negatives = {
    "low-nonfitting-no-narrow-proof": fact_index(
        "    original = value % 5\n", "original[:2]", extent=3
    ),
    "wrapping-add-no-inheritance": fact_index(
        "    original = value % 5\n", "original + 1"
    ),
    "wrapping-sub-no-inheritance": fact_index(
        "    original = value % 5\n", "original - 1"
    ),
    "wrapping-mul-no-inheritance": fact_index(
        "    original = value % 5\n", "original * 2"
    ),
    "negative-integer-boundary": fact_index(
        "    original = 0 - 1\n    converted: ac.u3 = original\n", "converted"
    ),
    "fixed-not-integer-boundary": fact_index(
        "    converted: ac.u3 = 2\n    logical: Annotated[int, range(1 << 3)] = converted\n",
        "logical",
    ),
    "old-integer-alias-not-fixed": fact_index(
        "    original = (value & 3) + 1\n    old_alias = original\n    converted: ac.u3 = original\n",
        "old_alias[:3]",
        inputs=logical_input,
    ),
    "old-boolean-alias-not-fixed": fact_index(
        "    original = True\n    old_alias = original\n    converted: ac.u1 = original\n",
        "old_alias[:1]",
    ),
}
for arm, expression in (
    ("left", "closed if value[:1] else False"),
    ("right", "False if value[:1] else closed"),
):
    fact_negatives["wide-fixed-boolean-peer-" + arm] = fact_index(
        "    closed: ac.u3 = 2\n", expression
    )
fact_negatives["fixed-proof-not-integer-formal"] = fact_index("", "value").replace(
    "def evaluate(entries, value)",
    "def evaluate(entries, value: Annotated[int, range(1 << 8)])",
)


def scalar(expression, base="ac.u13", extra=""):
    return (
        PREFIX
        + "@ac.module\ndef Top(value: "
        + base
        + extra
        + ") -> Result:\n    return Result(value="
        + expression
        + ")\n"
    )


huge = "1" + "0" * 200
cases = {
    "negative-lower": scalar("value[(0 - 1):4]"),
    "negative-upper": scalar("value[0:(0 - 1)]"),
    "empty": scalar("value[3:3]"),
    "reversed": scalar("value[5:3]"),
    "outside-lower": scalar("value[14:15]"),
    "outside-upper": scalar("value[0:14]"),
    "enormous-upper": scalar("value[0:" + huge + "]"),
    "enormous-lower": scalar("value[" + huge + ":" + huge + "1]"),
    "boolean-lower": scalar("value[True:7]"),
    "boolean-upper": scalar("value[0:True]"),
    "boolean-step": scalar("value[0:7:True]"),
    "dynamic-lower": scalar("value[count:9]", extra=", count: ac.u3"),
    "dynamic-upper": scalar("value[:count]", extra=", count: ac.u3"),
    "dynamic-step": scalar("value[0:7:count]", extra=", count: ac.u3"),
    "unbound-lower": scalar("value[MISSING:5]"),
    "unbound-upper": scalar("value[:MISSING]"),
    "unbound-step": scalar("value[0:7:MISSING]"),
    "zero-step": scalar("value[0:7:0]"),
    "nonunit-step": scalar("value[0:7:2]"),
    "negative-step": scalar("value[0:7:(0 - 1)]"),
    "enormous-step": scalar("value[0:7:" + huge + "]"),
    "boolean-input-base": scalar("value[0:1]", base="bool"),
    "boolean-comparison-base": scalar("(value == value)[0:1]"),
    "integer-base": "from typing import Annotated\n"
    + scalar("value[0:5]", base="Annotated[int, range(1 << 13)]"),
    "runtime-boolean-fixed-peer-base": scalar(
        "(value if choose else (value == value))[0:1]",
        base="ac.u1",
        extra=", choose: ac.u1",
    ),
    "scalar-index": scalar("value[3]"),
    "implicit-narrowing": scalar("value"),
    "slice-write": PREFIX
    + """@ac.rule
def evaluate(value) -> Result:
    value[0:3] = 0
    return Result(value=0)
@ac.module
def Top(value: ac.u13) -> Result:
    return evaluate(value)
""",
    "table-slice": PREFIX
    + """@ac.rule
def evaluate(entries) -> Result:
    return Result(value=entries[:1])
@ac.module
def Top() -> Result:
    entries = ac.table[2, ac.u13](init=0)
    return evaluate(entries)
""",
    "table-index-complete-width": DESIGN.replace(
        "def TableTop(index: ac.u1", "def TableTop(index: ac.u2"
    ),
}
diagnostics = dict.fromkeys(
    (
        "negative-lower",
        "negative-upper",
        "empty",
        "reversed",
        "outside-lower",
        "outside-upper",
        "enormous-upper",
        "enormous-lower",
    ),
    "slice bounds require 0 <= lower < upper <= source width",
)
for endpoint in ("lower", "upper", "step"):
    noun = endpoint if endpoint == "step" else endpoint + " bound"
    diagnostics["boolean-" + endpoint] = "slice " + noun + " must be a static Integer"
    for prefix in ("dynamic-", "unbound-"):
        diagnostics[prefix + endpoint] = (
            "slice " + noun + " must be a bound static Integer"
        )
for name in ("zero-step", "nonunit-step", "negative-step", "enormous-step"):
    diagnostics[name] = "slice step must be the static Integer 1"
for name in (
    "boolean-input-base",
    "boolean-comparison-base",
    "integer-base",
    "table-slice",
):
    diagnostics[name] = "slice requires explicitly unsigned fixed bits"
diagnostics["runtime-boolean-fixed-peer-base"] = (
    "fixed branch peer requires a closed source Integer or Boolean constant"
)
diagnostics.update(
    {
        "scalar-index": "subscript requires a table",
        "implicit-narrowing": "unsigned boundary implicit narrowing is unsupported",
        "slice-write": "indexed assignment requires a table",
        "table-index-complete-width": "table index complete width is not proven in range",
    }
)
assert set(diagnostics) == set(cases)
cases.update(fact_negatives)
diagnostics.update(
    dict.fromkeys(fact_negatives, "table index complete width is not proven in range")
)
diagnostics.update(
    {
        "negative-integer-boundary": "integer interval is not proven within destination bounds",
        "fixed-not-integer-boundary": "binding boundary requires declared Integer source kind",
        "old-integer-alias-not-fixed": "slice requires explicitly unsigned fixed bits",
        "old-boolean-alias-not-fixed": "slice requires explicitly unsigned fixed bits",
        "wide-fixed-boolean-peer-left": "closed Boolean branch requires a bits[1] peer",
        "wide-fixed-boolean-peer-right": "closed Boolean branch requires a bits[1] peer",
        "fixed-proof-not-integer-formal": "rule parameter annotation disagrees with binding source kind",
    }
)
assert set(diagnostics) == set(cases)

with tempfile.TemporaryDirectory(prefix="fixed-slices-", dir=scratch) as temporary:
    build = Path(temporary)
    source = build / "source"
    source.mkdir()
    design = source / "design.py"
    design.write_text(DESIGN)
    (scratch / "design.py").write_text(DESIGN)
    unit = build / "unit"

    def compile_source(output, accepted=True, replace=False):
        arguments = [
            "compile",
            "-c",
            design,
            "--source-root",
            source,
            "--package-prefix",
            "slices",
            "-o",
            output,
        ]
        if replace:
            arguments.append("--replace")
        return cli(*arguments, accepted=accepted)

    if args.baseline_rejection:
        result = compile_source(unit, accepted=False)
        assert any(
            word in result.stderr.lower()
            for word in ("table", "slice", "index", "subscript")
        ), result.stderr
        assert not unit.exists()
        sys.stdout.write("fixed slices pre-fix baseline rejects unsigned slicing\n")
        sys.exit(0)
    compile_source(unit)
    retained = scratch / "retained"
    retained.mkdir()
    shutil.copyfile(design, retained / "design.py")
    shutil.copytree(unit, retained / "unit")
    toolroot = Path(args.source_compiler).resolve().parent.parent
    runtime = next(
        (
            path
            for path in (
                toolroot / "simulator/gfsim/libpyc6_runtime.a",
                toolroot / "lib/libpyc6_runtime.a",
            )
            if path.is_file()
        ),
        None,
    )
    assert runtime is not None, "Runtime archive missing from this build/install"
    products = []
    for top, defines, frames, masks in (
        ("Top", [], 10, 4),
        ("TableTop", ["-DTABLE_SLICES"], 16, 4),
        ("FactTop", ["-DFACT_SLICES"], 12, 4),
    ):
        output = build / top
        output.mkdir()
        final = output / "design_top.ac"
        cli("link", unit, "--top", "slices.design." + top, "-o", final)
        for target in ("cpp", "verilog"):
            cli("emit", final, "--target", target, "-o", output / target)
        receipt = json.loads((output / "cpp/generated.json").read_text())
        cpp = [
            output / "cpp" / row["path"]
            for row in receipt["files"]
            if row["path"].endswith(".cpp")
        ]
        assert cpp, "Generated source-owned C++ translation unit missing"
        runner = output / "runner"
        run(
            [
                args.cxx,
                "-std=c++20",
                "-pthread",
                *defines,
                "-I" + str(repo / "include"),
                "-I" + str(output / "cpp"),
                fixtures / "fixed-slices.cpp",
                *cpp,
                runtime,
                "-o",
                runner,
            ]
        )
        traces, mask_traces = [], []
        for workers in (1, 2):
            trace = run([runner, str(workers)]).stdout
            (scratch / f"{top}-workers-{workers}.stdout").write_text(trace)
            traces.append(
                [row for row in trace.splitlines() if row.startswith("WORK ")]
            )
            mask_traces.append(
                [row for row in trace.splitlines() if row.startswith("MASK ")]
            )
        assert len(traces[0]) == frames and traces[0] == traces[1]
        assert len(mask_traces[0]) == masks and mask_traces[0] == mask_traces[1]
        receipt = json.loads((output / "verilog/generated.json").read_text())
        rtl = [
            output / "verilog" / row["path"]
            for row in receipt["files"]
            if row["role"] == "rtl"
        ]
        rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
        rtl_build = output / "rtl-build"
        run(
            [
                args.verilator,
                "--binary",
                "--timing",
                "--top-module",
                "tb",
                "--prefix",
                "Vslices",
                "--Mdir",
                rtl_build,
                "-j",
                "2",
                "-Wno-fatal",
                *defines,
                *sorted((repo / "include/verilog").glob("*.v")),
                *rtl,
                fixtures / "fixed-slices.sv",
            ]
        )
        rtl_trace = run([rtl_build / "Vslices"]).stdout
        (scratch / f"{top}-rtl.stdout").write_text(rtl_trace)
        assert [
            row for row in rtl_trace.splitlines() if row.startswith("WORK ")
        ] == traces[0]
        if args.iverilog and args.vvp:
            rtl_runner = output / "rtl-four-state"
            run(
                [
                    args.iverilog,
                    "-g2012",
                    "-DSLICES_FOUR_STATE",
                    *defines,
                    "-s",
                    "tb",
                    "-o",
                    rtl_runner,
                    *sorted((repo / "include/verilog").glob("*.v")),
                    *rtl,
                    fixtures / "fixed-slices.sv",
                ]
            )
            four_state_trace = run([args.vvp, rtl_runner]).stdout
            (scratch / f"{top}-rtl-four-state.stdout").write_text(four_state_trace)
            assert [
                row for row in four_state_trace.splitlines() if row.startswith("WORK ")
            ] == traces[0]
            assert [
                row for row in four_state_trace.splitlines() if row.startswith("MASK ")
            ] == mask_traces[0]
        root_stages = retained / top
        root_stages.mkdir()
        shutil.copyfile(final, root_stages / "design_top.ac")
        for target in ("cpp", "verilog"):
            shutil.copytree(output / target, root_stages / target)
        products.extend([final, output / "cpp", output / "verilog"])

    for name, content in fact_controls.items():
        design.write_text(content)
        (scratch / (name + ".py")).write_text(content)
        control = build / ("fact-control-" + name)
        compile_source(control)
        final = build / ("fact-control-" + name + ".ac")
        cli("link", control, "--top", "slices.design.Top", "-o", final)
        control_stages = retained / "fact-controls" / name
        control_stages.mkdir(parents=True)
        shutil.copyfile(design, control_stages / "design.py")
        shutil.copytree(control, control_stages / "unit")
        shutil.copyfile(final, control_stages / "design_top.ac")
        products.extend([control, final])
    before = snapshot(unit)
    publication_control = unit.parent / ("." + unit.name + ".pycircuit-publication")
    before_publication = snapshot(publication_control)
    protected = {
        path: snapshot(path) if path.is_dir() else path.read_bytes()
        for path in products
    }
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
        assert snapshot(publication_control) == before_publication
        assert all(
            (snapshot(path) if path.is_dir() else path.read_bytes()) == value
            for path, value in protected.items()
        )
    inputs = [
        fixtures / ("fixed-slices" + extension) for extension in (".py", ".cpp", ".sv")
    ]
    (scratch / "candidate.json").write_text(
        json.dumps(
            {
                "fixtures": {
                    str(path.relative_to(repo)): hashlib.sha256(
                        path.read_bytes()
                    ).hexdigest()
                    for path in inputs
                },
                "known_frames": 38,
                "four_state_frames": 12,
                "workers": [1, 2],
                "rejected_cases": sorted(cases),
                "fact_controls": sorted(fact_controls),
                "fact_rejections": sorted(fact_negatives),
                "singleton_runtime_boundary": "existing ExactIntegerLoweringTest.SingletonDynamicValueIsNotAKnownConstantMask covers a nonconstant singleton directly",
                "missing_origin_guard_gap": "mixed Boolean/fixed conditional rejects at shared join; no missing-origin slice guard coverage claimed",
                "protected_rejections": len(cases) * 2,
                "icarus_four_state": bool(args.iverilog and args.vvp),
                "retained_artifacts_sha256": {
                    str(path.relative_to(retained)): hashlib.sha256(
                        path.read_bytes()
                    ).hexdigest()
                    for path in sorted(retained.rglob("*"))
                    if path.is_file()
                },
            },
            indent=2,
        )
        + "\n"
    )

sys.stdout.write(
    f"fixed slices gate passed: 38 CPP worker-1/2 and RTL frames; 12 native X/Z frames; {len(cases) * 2} protected rejections; "
    + (
        "12 Icarus X/Z frames\n"
        if args.iverilog and args.vvp
        else "Icarus X/Z unavailable\n"
    )
)
