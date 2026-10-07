"""Independent source count-zero admission, generated DUTs, and publication guards."""

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
for name in (
    "repo",
    "source-compiler",
    "linker",
    "emitter",
    "optimizer",
    "cxx",
    "verilator",
    "iverilog",
    "vvp",
    "scratch",
):
    parser.add_argument("--" + name, required=True)
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


def run(command, code=0, diagnostic=None):
    command = list(map(str, command))
    result = subprocess.run(
        command, env=env, cwd=repo, capture_output=True, text=True, timeout=240
    )
    row = {
        "command": command,
        "exit_status": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
    }
    commands.append(row)
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == code, row
    assert (
        "Assertion failed" not in result.stderr and "Traceback" not in result.stderr
    ), row
    if diagnostic:
        assert diagnostic in result.stderr, row
    return result


def cli(*arguments, **options):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], **options)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def snapshot(path):
    return {
        p.relative_to(path).as_posix(): p.read_bytes()
        for p in path.rglob("*")
        if p.is_file()
    }


def managed(path):
    control = path.parent / ("." + path.name + ".pycircuit-publication")
    return {
        "product": snapshot(path) if path.is_dir() else path.read_bytes(),
        "publication": snapshot(control),
    }


def assert_absent(path):
    assert not path.exists(), path
    control = path.parent / ("." + path.name + ".pycircuit-publication")
    assert not (control / "stage").exists(), control
    assert not (control / "previous").exists(), control


def payload(unit, kind):
    return unit / json.loads((unit / "unit.json").read_text())["files"][kind]


PROVIDER = """from typing import Annotated
import pycircuit as ac
@ac.struct
class FixedResult:
    value: ac.u5
@ac.module
def Fixed(value: ac.u5) -> FixedResult:
    return FixedResult(value=value)
@ac.module
def Logical(value: Annotated[int, range(1 << 5)], flag: bool) -> {"integer": Annotated[int, range(1 << 5)], "boolean": bool}:
    return {"integer": value, "boolean": flag}
"""
WIDTHS = (1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 73, 127, 128, 129)
DIRECTIONS = ("leading", "trailing")
HELPERS = {direction: "count_" + direction + "_zeros" for direction in DIRECTIONS}
FIELDS = [
    (f"{direction}{width}", width.bit_length())
    for width in WIDTHS
    for direction in DIRECTIONS
] + [
    (direction + name, width)
    for name, width in (
        ("Widen1", 8),
        ("Widen65", 13),
        ("Slice73", 4),
        ("Arithmetic5", 3),
        ("Enum3", 2),
        ("Child5", 3),
        ("Nested73", 3),
    )
    for direction in DIRECTIONS
]
TRACE_WIDTH = sum(width for _, width in FIELDS)
DESIGN = """from enum import Enum
import pycircuit as ac
from pycircuit import count_leading_zeros as leading, count_trailing_zeros as trailing
from count_zero_probe.provider import Fixed
@ac.encoding(width=3)
class State(Enum):
    ZERO = 0
    ONE = 1
@ac.struct
class ProbeResult:
""" + "".join(f"    {name}: ac.bits[{width}]\n" for name, width in FIELDS)
DESIGN += (
    "@ac.rule\ndef evaluate("
    + ", ".join(f"w{w}" for w in WIDTHS)
    + ", state, child) -> ProbeResult:\n"
)
DESIGN += "    alias = w5\n    result = ProbeResult()\n"
DESIGN += "".join(
    f"    result.{direction}{w} = ac.{HELPERS[direction]}(w{w})\n"
    for w in WIDTHS
    for direction in DIRECTIONS
)
DESIGN += "    local = alias + 1\n"
for direction in DIRECTIONS:
    helper = "ac." + HELPERS[direction]
    other = "trailing" if direction == "leading" else "leading"
    DESIGN += f"""    result.{direction}Widen1 = {direction}(w1)
    result.{direction}Widen65 = {direction}(w65)
    result.{direction}Slice73 = {helper}(w73[61:70])
    result.{direction}Arithmetic5 = {helper}(local)
    result.{direction}Enum3 = {helper}(ac.enum_to_bits(state))
    result.{direction}Child5 = {helper}(child.value)
    result.{direction}Nested73 = {helper}({other}(w73))
"""
DESIGN += "    return result\n@ac.module\n"
DESIGN += (
    "def Top("
    + ", ".join(f"w{w}: ac.bits[{w}]" for w in WIDTHS)
    + ", state: State) -> ProbeResult:\n"
)
DESIGN += (
    "    child = Fixed(w5)\n    return evaluate("
    + ", ".join(f"w{w}" for w in WIDTHS)
    + ", state, child)\n"
)
PREFIX = "from typing import Annotated\nfrom enum import Enum\nimport pycircuit as ac\n"


def scalar(expression, base="ac.u5", extra="", prelude="", statements="", width=80):
    return (
        PREFIX
        + prelude
        + f"@ac.struct\nclass Result:\n    out: ac.bits[{width}]\n"
        + f"@ac.module\ndef Top(value: {base}{extra}) -> Result:\n"
        + statements
        + f"    return Result(out={expression})\n"
    )


def observation(expression, base="ac.u5", extra=""):
    return (
        PREFIX
        + "from pycircuit import report, rule\n"
        + f'@ac.module\ndef Top(value: {base}{extra}) -> {{"out": ac.u5}}:\n'
        + "    alias = value\n    @rule\n    def observe():\n"
        + f'        report("count_leading_zeros", {expression})\n'
        + '    observe()\n    return {"out": 0}\n'
    )


fixed = "count_leading_zeros operand requires explicitly unsigned fixed bits"
shape = "count_leading_zeros requires exactly one positional fixed-bit operand"
enum_decl = "@ac.encoding(width=3)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n"
struct_decl = "@ac.struct\nclass Record:\n    data: ac.u5\n"
cases = {
    "empty": (scalar("ac.count_leading_zeros()"), shape),
    "two-operands": (scalar("ac.count_leading_zeros(value, value)"), shape),
    "keyword": (scalar("ac.count_leading_zeros(value=value)"), shape),
    "mixed-keyword": (scalar("ac.count_leading_zeros(value, other=value)"), shape),
    "keyword-spread": (scalar("ac.count_leading_zeros(value, **value)"), shape),
    "starred": (
        scalar("ac.count_leading_zeros(*value)"),
        "count_leading_zeros does not accept starred operands",
    ),
    "subscribed": (
        scalar("ac.count_leading_zeros[5](value)"),
        "count_leading_zeros does not accept type subscription",
    ),
    "integer-input": (
        scalar("ac.count_leading_zeros(value)", "Annotated[int, range(1 << 5)]"),
        fixed,
    ),
    "one-bit-integer": (
        scalar("ac.count_leading_zeros(value)", "Annotated[int, range(1 << 1)]"),
        fixed,
    ),
    "boolean-input": (scalar("ac.count_leading_zeros(value)", "bool"), fixed),
    "integer-literal": (scalar("ac.count_leading_zeros(17)"), fixed),
    "boolean-literal": (scalar("ac.count_leading_zeros(True)"), fixed),
    "closed-integer": (scalar("ac.count_leading_zeros((3 * 7) + 1)"), fixed),
    "closed-boolean": (scalar("ac.count_leading_zeros(1 == 1)"), fixed),
    "integer-alias": (
        scalar("ac.count_leading_zeros(alias)", statements="    alias = 7\n"),
        fixed,
    ),
    "raw-enum": (
        scalar("ac.count_leading_zeros(value)", "State", prelude=enum_decl),
        fixed,
    ),
    "raw-struct": (
        scalar("ac.count_leading_zeros(value)", "Record", prelude=struct_decl),
        fixed,
    ),
    "tuple": (scalar("ac.count_leading_zeros((value, value))"), "unsupported"),
    "list": (scalar("ac.count_leading_zeros([value])"), "unsupported"),
    "string": (scalar('ac.count_leading_zeros("bits")'), "literal"),
    "table": (
        scalar(
            "ac.count_leading_zeros(entries)",
            statements="    entries = ac.table[2, ac.u5](init=0)\n",
        ),
        fixed,
    ),
    "empty-slice": (
        scalar("ac.count_leading_zeros(value[2:2])"),
        "slice bounds require 0 <= lower < upper <= source width",
    ),
    "dynamic-slice": (
        scalar("ac.count_leading_zeros(value[:count])", extra=", count: ac.u3"),
        "slice upper bound must be a bound static Integer",
    ),
    "narrowing": (
        scalar("ac.count_leading_zeros(value)", base="ac.u8", width=3),
        "unsigned boundary implicit narrowing is unsupported",
    ),
    "shadow-alias": (
        scalar(
            "count(value)",
            prelude="from pycircuit import count_leading_zeros as count\n",
            statements="    count = value\n",
        ),
        "source binding is shadowed in its lexical scope: 'count'",
    ),
    "shadow-namespace": (
        scalar("ac.count_leading_zeros(value)", statements="    ac = value\n"),
        "source binding is shadowed in its lexical scope: 'ac'",
    ),
    "unresolved-marker": (scalar("count_leading_zeros(value)"), "unsupported"),
    "zero-width": (
        scalar("ac.count_leading_zeros(value)", base="ac.bits[0]"),
        "hardware static value must be positive and fit u64",
    ),
    "negative-width": (
        scalar("ac.count_leading_zeros(value)", base="ac.bits[0 - 1]"),
        "hardware static value must be positive and fit u64",
    ),
    "unresolved-width": (
        scalar("ac.count_leading_zeros(value)", base="ac.bits[MISSING]"),
        "hardware static reference must name an integer formal",
    ),
    "width-over-u64": (
        scalar("ac.count_leading_zeros(value)", base="ac.bits[1 << 64]"),
        "hardware static value must be positive and fit u64",
    ),
    "observation-integer": (
        observation("ac.count_leading_zeros(alias)", "Annotated[int, range(1 << 5)]"),
        fixed,
    ),
    "observation-boolean": (
        observation("ac.count_leading_zeros(alias)", "bool"),
        fixed,
    ),
    "observation-empty": (observation("ac.count_leading_zeros()"), shape),
    "observation-narrowing-kind": (observation("ac.count_leading_zeros(0)"), fixed),
}

# Each direction independently rejects the same malformed shapes and source kinds.
cases = {
    direction + "-" + name: (
        text.replace("count_leading_zeros", helper),
        diagnostic.replace("count_leading_zeros", helper),
    )
    for direction, helper in HELPERS.items()
    for name, (text, diagnostic) in cases.items()
}


def split_fields(text):
    """Split MLIR dictionaries/lists only at their own nesting level."""
    stack, quoted, escaped, start, parts = [], False, False, 0, []
    for index, char in enumerate(text):
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
        elif char == '"':
            quoted = True
        elif char in "{[(":
            stack.append(char)
        elif char in "}])":
            stack.pop()
        elif char == "," and not stack:
            parts.append(text[start:index].strip())
            start = index + 1
    parts.append(text[start:].strip())
    return parts


def attribute(text, key):
    start = text.index(key + " = ") + len(key) + 3
    opening = text[start]
    assert opening in "{[", (key, text)
    closing = "}" if opening == "{" else "]"
    depth, quoted, escaped = 0, False, False
    for index in range(start, len(text)):
        char = text[index]
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
        elif char == '"':
            quoted = True
        elif char == opening:
            depth += 1
        elif char == closing:
            depth -= 1
            if depth == 0:
                return text[start + 1 : index]
    raise AssertionError((key, text))


def dictionary(text):
    return dict(part.split(" = ", 1) for part in split_fields(text))


def module_header(header, symbol):
    return next(
        line for line in header.splitlines() if f'sym_name = "{symbol}"' in line
    )


def parameter_kinds(header, symbol):
    return {
        json.loads(fields["name"]): dictionary(fields["constraint"][1:-1])[
            "source_kind"
        ]
        for item in split_fields(
            attribute(module_header(header, symbol), "ac.parameters")
        )
        for fields in [dictionary(item[1:-1])]
    }


def dependency_ports(header, symbol):
    return {
        json.loads(dictionary(output[1:-1])["path"])[0]: {
            int(dictionary(item[1:-1])["port"].split()[0])
            for item in split_fields(inputs[1:-1])
        }
        for entry in split_fields(
            attribute(module_header(header, symbol), "dependency_summary")
        )
        for fields in [dictionary(entry[1:-1])]
        for inputs, output in [(fields["inputs"], fields["output"])]
    }


with tempfile.TemporaryDirectory(prefix="count-zeros-", dir=scratch) as temporary:
    build = Path(temporary)
    source = build / "source"
    source.mkdir()

    def compile_source(name, output, imports=(), replace=False, **options):
        arguments = [
            "compile",
            "-c",
            source / name,
            "--source-root",
            source,
            "--package-prefix",
            "count_zero_probe",
            "-o",
            output,
        ]
        for unit in imports:
            arguments.extend(("-I", unit))
        if replace:
            arguments.append("--replace")
        return cli(*arguments, **options)

    # Public syntax markers raise on host execution; they are not evaluators.
    for helper in HELPERS.values():
        run(
            [
                sys.executable,
                "-c",
                f"from pycircuit import {helper}; "
                f"\ntry: {helper}(1)\nexcept RuntimeError: pass\nelse: raise AssertionError('{helper} executed')",
            ]
        )
    (source / "provider.py").write_text(PROVIDER)
    (scratch / "provider.py").write_text(PROVIDER)
    provider = build / "provider-unit"
    compile_source("provider.py", provider)
    provider_header = payload(provider, "interface").read_text()
    assert parameter_kinds(provider_header, "count_zero_probe.provider.Fixed") == {
        "value": '"fixed_bits"'
    }
    fixed_line = module_header(provider_header, "count_zero_probe.provider.Fixed")
    result_constraint = dictionary(
        split_fields(attribute(fixed_line, "ac.result_constraints"))[0][1:-1]
    )
    assert result_constraint["source_kind"] == '"nominal"'
    assert (
        result_constraint["type"]
        == '!ac.struct<"count_zero_probe.provider.FixedResult">'
    )
    logical_line = module_header(provider_header, "count_zero_probe.provider.Logical")
    assert "ac.result_constraints = " not in logical_line
    (source / "provider.py").unlink()
    (source / "design.py").write_text(DESIGN)
    (scratch / "design.py").write_text(DESIGN)
    unit = build / "unit"
    compile_source("design.py", unit, (provider,))
    controls = {}
    for direction, helper in HELPERS.items():
        call = "ac." + helper
        controls.update(
            {
                direction + "-typed-boundary": scalar(
                    f"{call}(converted)",
                    statements="    converted: ac.u5 = 7\n",
                    width=3,
                ),
                direction + "-structural-wire": PREFIX
                + f"""@ac.module
 def Top(value: ac.u5) -> {{"out": ac.u3}}:
     local = {call}(value)
     return {{"out": local}}
""".replace("\n ", "\n"),
                direction + "-natural-widening": scalar(
                    f"{call}(value)", base="ac.bits[65]", width=13
                ),
                direction + "-explicit-narrowing": scalar(
                    f"{call}(value)[:2]", base="ac.u8", width=2
                ),
                direction + "-fixed-singleton-kind": scalar(
                    f"{call}({call}(value))", base="ac.u1", width=1
                ),
                direction + "-evaluate-once": scalar(f"{call}(value + 1)", width=3),
                direction + "-captured-rule": PREFIX
                + f"""@ac.module
 def Child(value: ac.u3) -> {{"out": ac.u3}}:
     return {{"out": value}}
 @ac.module
 def Top(value: ac.u5) -> {{"out": ac.u3}}:
     alias = value
     child = Child()
     @ac.rule
     def bind():
         child(value={call}(alias))
     bind()
     return {{"out": child.out}}
""".replace("\n ", "\n"),
            }
        )
    control_paths = []
    for name, text in controls.items():
        filename = name.replace("-", "_") + ".py"
        (source / filename).write_text(text)
        (scratch / filename).write_text(text)
        control = build / name
        compile_source(filename, control)
        control_body = payload(control, "body").read_text()
        assert '"ac.bits.extract"' in control_body or name.endswith(
            "fixed-singleton-kind"
        )
        if name.endswith("fixed-singleton-kind"):
            assert '"ac.bits.binary"' not in control_body
            assert control_body.count('opcode = "not"') == 2
        if name.endswith("natural-widening"):
            # Count completes at its natural width, then the boundary zero-extends.
            resize = next(
                line
                for line in control_body.splitlines()
                if '"ac.bits.resize"' in line
                and re.findall(r"#ac.math_int<([0-9]+)>", line.rsplit("->", 1)[1])[-1]
                == "13"
            )
            assert 'mode = "zext"' in resize
            assert (
                re.findall(r"#ac.math_int<([0-9]+)>", resize.split("->", 1)[0])[-1]
                == "7"
            )
        if name.endswith("evaluate-once"):
            operand_adds = [
                line
                for line in control_body.splitlines()
                if '"ac.bits.binary"' in line
                and 'opcode = "add"' in line
                and re.findall(r"#ac.math_int<([0-9]+)>", line.rsplit("->", 1)[1])[-1]
                == "5"
            ]
            assert len(operand_adds) == 1, operand_adds
        control_final = build / (name + ".ac")
        cli(
            "link",
            control,
            "--top",
            "count_zero_probe." + filename.removesuffix(".py") + ".Top",
            "-o",
            control_final,
        )
        control_paths.extend((control, control_final))
    body = payload(unit, "body").read_text()
    header = payload(unit, "interface").read_text()
    (scratch / "consumer.interface.ac").write_text(header)
    (scratch / "provider.interface.ac").write_text(provider_header)
    parameters = parameter_kinds(header, "count_zero_probe.design.Top")
    assert parameters == {
        **{f"w{w}": '"fixed_bits"' for w in WIDTHS},
        "state": '"nominal"',
    }, parameters
    dependencies = dependency_ports(header, "count_zero_probe.design.Top")
    expected_dependencies = {
        f"{direction}{width}": {index}
        for index, width in enumerate(WIDTHS)
        for direction in DIRECTIONS
    }
    for direction in DIRECTIONS:
        expected_dependencies.update(
            {
                direction + "Widen1": {0},
                direction + "Widen65": {16},
                direction + "Slice73": {17},
                direction + "Arithmetic5": {4},
                direction + "Enum3": {21},
                direction + "Child5": {4},
                direction + "Nested73": {17},
            }
        )
    assert dependencies == expected_dependencies, dependencies
    assert (
        attribute(
            module_header(header, "count_zero_probe.design.Top"), "ac.domain_inputs"
        )
        == ""
    )
    assert '"ac.bits.binary"' in body and '"ac.bits.extract"' in body
    assert all(f'"ac.{name}"' not in body for name in ("reg", "variable", "table"))
    # Base field declarations retain natural count widths, including fixed W=1.
    result_line = next(
        line
        for line in header.splitlines()
        if 'ac.struct "count_zero_probe.design.ProbeResult" fields [' in line
    )
    result_fields = [
        dictionary(item[1:-1])
        for item in split_fields(
            attribute(result_line.replace(" fields [", " fields = [", 1), "fields")
        )
    ]
    assert [
        (
            json.loads(field["name"]),
            int(re.findall(r"#ac.math_int<([0-9]+)>", field["type"])[-1]),
        )
        for field in result_fields
    ] == FIELDS
    assert 'opcode = "sub"' in body, (
        "four-state endpoint poison must survive source lowering"
    )
    (scratch / "source-body.ac").write_text(body)
    output = build / "products"
    output.mkdir()
    final = output / "design.ac"
    cli("link", provider, unit, "--top", "count_zero_probe.design.Top", "-o", final)
    run(
        [
            args.optimizer,
            final,
            "--ac-verify-hardware",
            "-o",
            output / "verified.ac",
        ]
    )
    final_hash = digest(final)
    (scratch / "final.ac").write_bytes(final.read_bytes())
    for target in ("cpp", "verilog"):
        cli("emit", final, "--target", target, "-o", output / target)
        assert digest(final) == final_hash
    receipt = json.loads((output / "cpp/generated.json").read_text())
    cpp = [
        output / "cpp" / row["path"]
        for row in receipt["files"]
        if row["path"].endswith(".cpp")
    ]
    assert cpp
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
    runner = output / "runner"
    run(
        [
            args.cxx,
            "-std=c++20",
            "-pthread",
            "-I" + str(repo / "include"),
            "-I" + str(output / "cpp"),
            fixtures / "bits-count-zeros.cpp",
            *cpp,
            runtime,
            "-o",
            runner,
        ]
    )
    traces = []
    for workers in (1, 2):
        trace = run([runner, str(workers)]).stdout
        (scratch / f"native-workers-{workers}.stdout").write_text(trace)
        traces.append(
            [row for row in trace.splitlines() if row.startswith(("WORK ", "MASK "))]
        )
    assert traces[0] and traces[0] == traces[1]
    assert all(
        re.fullmatch(rf"(?:WORK|MASK) [01xz]{{{TRACE_WIDTH}}}", row)
        for row in traces[0]
    )
    known_frames = sum(row.startswith("WORK ") for row in traces[0])
    masked_frames = sum(row.startswith("MASK ") for row in traces[0])
    assert known_frames > 0 and masked_frames > 0
    assert all("z" not in row for row in traces[0]), "computed counts never produce Z"
    receipt = json.loads((output / "verilog/generated.json").read_text())
    rtl = [
        output / "verilog" / row["path"]
        for row in receipt["files"]
        if row["role"] == "rtl"
    ]
    rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
    rtl_runner = output / "four-state"
    run(
        [
            args.iverilog,
            "-g2012",
            "-DCOUNT_ZEROS_FOUR_STATE",
            "-s",
            "tb",
            "-o",
            rtl_runner,
            *sorted((repo / "include/verilog").glob("*.v")),
            *rtl,
            fixtures / "bits-count-zeros.sv",
        ]
    )
    rtl_trace = run([args.vvp, rtl_runner]).stdout
    (scratch / "icarus.stdout").write_text(rtl_trace)
    assert [
        row for row in rtl_trace.splitlines() if row.startswith(("WORK ", "MASK "))
    ] == traces[0]
    rtl_build = output / "rtl-build"
    run(
        [
            args.verilator,
            "--binary",
            "--timing",
            "--top-module",
            "tb",
            "--prefix",
            "VcountZeros",
            "--Mdir",
            rtl_build,
            "-j",
            "2",
            "-Wno-fatal",
            *sorted((repo / "include/verilog").glob("*.v")),
            *rtl,
            fixtures / "bits-count-zeros.sv",
        ]
    )
    known_trace = run([rtl_build / "VcountZeros"]).stdout
    (scratch / "verilator.stdout").write_text(known_trace)
    assert [row for row in known_trace.splitlines() if row.startswith("WORK ")] == [
        row for row in traces[0] if row.startswith("WORK ")
    ]

    protected_paths = [
        provider,
        unit,
        final,
        output / "cpp",
        output / "verilog",
        *control_paths,
    ]
    before = {str(path): managed(path) for path in protected_paths}

    def assert_protected():
        assert {str(path): managed(path) for path in protected_paths} == before

    # Compile-only observation preflight plus actual capture lowering; cpp now
    # publishes the observation surface while verilog still rejects
    # instrumentation. Neither may displace a product owned by another design.
    for direction, helper in HELPERS.items():
        filename = "observe_" + direction + ".py"
        (source / filename).write_text(observation(f"ac.{helper}(alias[:3])"))
        observe_unit = build / ("observe-" + direction + "-unit")
        compile_source(filename, observe_unit)
        observed_body = payload(observe_unit, "body").read_text()
        assert '"ac.bits.extract"' in observed_body and '"ac.observe"' in observed_body
        observe_final = build / ("observe-" + direction + ".ac")
        cli(
            "link",
            observe_unit,
            "--top",
            "count_zero_probe.observe_" + direction + ".Top",
            "-o",
            observe_final,
        )
        protected_paths.extend((observe_unit, observe_final))
        before.update(
            {str(path): managed(path) for path in (observe_unit, observe_final)}
        )
        for target in ("cpp", "verilog"):
            absent = build / ("observe-" + direction + "-" + target)
            if target == "cpp":
                # C++ lowers `ac.observe` into a wired descriptor table.
                cli("emit", observe_final, "--target", target, "-o", absent)
                observe_receipt = json.loads((absent / "generated.json").read_text())
                observe_header = next(
                    row["path"]
                    for row in observe_receipt["files"]
                    if row["path"].endswith("pycircuit_system.hpp")
                )
                observe_system = (absent / observe_header).read_text()
                assert "pyc_observation_descriptors" in observe_system
                assert "Configure(pyc_observation_descriptors" in observe_system
                assert "Configure({}, 0)" not in observe_system
            else:
                rejected = cli(
                    "emit", observe_final, "--target", target, "-o", absent, code=1
                )
                assert any(
                    word in rejected.stderr.lower()
                    for word in ("instrument", "observation", "observe")
                ), rejected.stderr
                assert_absent(absent)
            cli(
                "emit",
                observe_final,
                "--target",
                target,
                "-o",
                output / target,
                "--replace",
                code=1,
                # The published cpp product comes from the unobserved `design.py`,
                # so its owner differs and the publisher refuses the replacement;
                # verilog still rejects the instrumentation itself.
                diagnostic=(
                    "generated.json entry does not match its owner"
                    if target == "cpp"
                    else "ac.observe"
                ),
            )
            assert_protected()

    # Legacy mapping outputs carry no serialized logical source-kind constraint.
    # Their Integer/Boolean physical carriers must not acquire fixed authority.
    for field, base in (
        ("integer", "Annotated[int, range(1 << 5)]"),
        ("boolean", "bool"),
    ):
        text = (
            PREFIX
            + "from count_zero_probe.provider import Logical\n"
            + f'@ac.module\ndef Top(value: {base}, flag: bool) -> {{"out": ac.bits[80]}}:\n'
            + "    child = Logical()\n    @ac.rule\n    def bind():\n        child(value=value, flag=flag)\n    bind()\n"
            + f'    return {{"out": ac.count_leading_zeros(child.{field})}}\n'
        )
        if field == "boolean":
            text = text.replace(
                "child(value=value, flag=flag)", "child(value=0, flag=value)"
            )
        for direction, helper in HELPERS.items():
            cases[direction + "-imported-" + field] = (
                text.replace("count_leading_zeros", helper),
                fixed.replace("count_leading_zeros", helper),
            )

    huge_receipts = []
    for width in (1 << 32, 1 << 63, (1 << 64) - 1):
        for direction, helper in HELPERS.items():
            filename = f"huge_{direction}_{width}.py"
            text = scalar(
                f"ac.{helper}(value)",
                base=f"ac.bits[{width}]",
                width=width.bit_length(),
            )
            (source / filename).write_text(text)
            (scratch / filename).write_text(text)
            huge = build / f"huge-{direction}-{width}"
            compile_source(filename, huge)
            huge_body = payload(huge, "body").read_text()
            operations = re.findall(r'"(ac\.bits\.[a-z]+)"', huge_body)
            tail_stages = (width - 2).bit_length()
            correction_stages = (width.bit_length() - 1).bit_length()
            # Tail fill <=3/stage; shared population reduction <=12/stage;
            # count-mask fill <=3/stage, with generous constant setup slack.
            bound = 18 * tail_stages + 4 * correction_stages + 40
            assert len(operations) <= bound, (width, direction, len(operations), bound)
            literals = list(map(int, re.findall(r"#ac.math_int<([0-9]+)>", huge_body)))
            assert literals and all(value.bit_length() <= 64 for value in literals), (
                width,
                literals,
            )
            assert len(huge_body.encode()) <= 131072 * (
                tail_stages + correction_stages + 1
            ), (width, len(huge_body))
            # Count's final XOR produces K bits independently of the W-bit port.
            correction = next(
                line
                for line in reversed(huge_body.splitlines())
                if 'opcode = "xor"' in line
            )
            assert re.findall(r"#ac.math_int<([0-9]+)>", correction.rsplit("->", 1)[1])[
                -1
            ] == str(width.bit_length())
            huge_final = build / f"huge-{direction}-{width}.ac"
            cli(
                "link",
                huge,
                "--top",
                f"count_zero_probe.huge_{direction}_{width}.Top",
                "-o",
                huge_final,
            )
            protected_paths.extend((huge, huge_final))
            before.update({str(path): managed(path) for path in (huge, huge_final)})
            failures = {}
            for target in ("cpp", "verilog"):
                absent = build / f"huge-{direction}-{width}-{target}"
                rejected = cli(
                    "emit", huge_final, "--target", target, "-o", absent, code=1
                )
                assert_absent(absent)
                assert (
                    "exceeds the current Runtime bit-width capacity" in rejected.stderr
                ), rejected.stderr
                cli(
                    "emit",
                    huge_final,
                    "--target",
                    target,
                    "-o",
                    output / target,
                    "--replace",
                    code=1,
                )
                assert_protected()
                failures[target] = rejected.stderr
            (scratch / f"huge-{direction}-{width}-body.ac").write_text(huge_body)
            huge_receipts.append(
                {
                    "helper": helper,
                    "width": width,
                    "natural_width": width.bit_length(),
                    "operations": len(operations),
                    "operation_bound": bound,
                    "tail_stages": tail_stages,
                    "correction_stages": correction_stages,
                    "body_bytes": len(huge_body.encode()),
                    "largest_literal_bits": max(
                        value.bit_length() for value in literals
                    ),
                    "emission_diagnostics": failures,
                    "scope": "bounded compilation graph/literals; existing emission capacity guard, no Runtime allocation claim",
                }
            )

    negative_receipts = []
    for name, (text, diagnostic) in cases.items():
        (source / "design.py").write_text(text)
        (scratch / (name + ".py")).write_text(text)
        absent = build / ("invalid-" + name)
        rejected = compile_source(
            "design.py", absent, (provider,), code=1, diagnostic=diagnostic
        )
        assert_absent(absent)
        # Helper-owned diagnostics identify the exact call or offending operand.
        # Earlier declaration/shadow guards retain their existing diagnostic spans.
        captured_span = bool(re.search(r'design\.py"?:[0-9]+:[0-9]+', rejected.stderr))
        helper_diagnostic = re.search(
            r"error: (count_(?:leading|trailing)_zeros [^\n]+)", rejected.stderr
        )
        if helper_diagnostic:
            owning_diagnostic = helper_diagnostic.group(1)
            assert captured_span, rejected.stderr
            call = next(
                node
                for node in ast.walk(ast.parse(text))
                if isinstance(node, ast.Call)
                and any(helper in ast.unparse(node.func) for helper in HELPERS.values())
            )
            position = call.args[0] if " operand " in owning_diagnostic else call
            if owning_diagnostic.endswith("does not accept starred operands"):
                position = call.args[0]
            assert (
                f'loc("design.py":{position.lineno}:{position.col_offset + 1}): error: {owning_diagnostic}'
                in rejected.stderr
            ), rejected.stderr
        compile_source(
            "design.py", unit, (provider,), replace=True, code=1, diagnostic=diagnostic
        )
        assert_protected()
        negative_receipts.append(
            {
                "case": name,
                "diagnostic": diagnostic,
                "captured_source_span": captured_span,
                "stderr": rejected.stderr,
            }
        )

    for supplied, name in (((unit,), "missing-provider"),):
        absent = build / (name + ".ac")
        rejected = cli(
            "link",
            *supplied,
            "--top",
            "count_zero_probe.design.Top",
            "-o",
            absent,
            code=1,
        )
        assert "dependency" in rejected.stderr.lower(), rejected.stderr
        assert_absent(absent)
        cli(
            "link",
            *supplied,
            "--top",
            "count_zero_probe.design.Top",
            "-o",
            final,
            "--replace",
            code=1,
        )
        assert_protected()
    (source / "provider.py").write_text(
        PROVIDER.replace("value: ac.u5", "value: ac.u7")
    )
    mismatch = build / "mismatched-provider"
    compile_source("provider.py", mismatch)
    protected_paths.append(mismatch)
    before[str(mismatch)] = managed(mismatch)
    mismatch_final = build / "mismatch.ac"
    rejected = cli(
        "link",
        mismatch,
        unit,
        "--top",
        "count_zero_probe.design.Top",
        "-o",
        mismatch_final,
        code=1,
    )
    assert_absent(mismatch_final)
    assert any(
        word in rejected.stderr.lower()
        for word in ("match", "interface", "header", "hash")
    ), rejected.stderr
    cli(
        "link",
        mismatch,
        unit,
        "--top",
        "count_zero_probe.design.Top",
        "-o",
        final,
        "--replace",
        code=1,
    )
    assert_protected()
    # Matching forged declaration snapshots cannot override the provider body.
    # A five-bit Integer carrier physically fits; it has no fixed-bit authority.
    forged_units = build / "forged-units"
    forged_units.mkdir()

    def copy_managed_unit(original, name):
        destination = forged_units / name
        shutil.copytree(original, destination)
        source_control = original.parent / (
            "." + original.name + ".pycircuit-publication"
        )
        destination_control = forged_units / ("." + name + ".pycircuit-publication")
        shutil.copytree(source_control, destination_control)
        (destination_control / "owner.json").write_text(
            json.dumps(
                {
                    "destination": name,
                    "kind": "pycircuit-publication-control",
                },
                separators=(",", ":"),
            )
        )
        return destination

    forged_provider = copy_managed_unit(provider, "forged-provider")
    forged_consumer = copy_managed_unit(unit, "forged-consumer")
    needle = 'source_kind = "fixed_bits"'
    replacement = (
        'source_kind = "integer", domain = '
        '#ac.source_domain<{kind = "integer", lower = #ac.math_int<0>, upper = #ac.math_int<32>}>'
    )

    def forge_fixed_header(path):
        lines = path.read_text().splitlines(keepends=True)
        matches = [
            index
            for index, line in enumerate(lines)
            if 'sym_name = "count_zero_probe.provider.Fixed"' in line and needle in line
        ]
        assert len(matches) == 1, (path, matches)
        lines[matches[0]] = lines[matches[0]].replace(needle, replacement, 1)
        path.write_text("".join(lines))

    forge_fixed_header(payload(forged_provider, "interface"))
    snapshot_mutations = 0
    for kind in ("body", "interface"):
        path = payload(forged_consumer, kind)
        if 'sym_name = "count_zero_probe.provider.Fixed"' in path.read_text():
            forge_fixed_header(path)
            snapshot_mutations += 1
    assert snapshot_mutations, "consumer must retain the provider declaration snapshot"
    forged_before = {
        str(path): managed(path) for path in (forged_provider, forged_consumer)
    }
    absent = build / "forged-authority.ac"
    forged = cli(
        "link",
        forged_provider,
        forged_consumer,
        "--top",
        "count_zero_probe.design.Top",
        "-o",
        absent,
        code=1,
    )
    assert_absent(absent)
    assert any(
        word in forged.stderr.lower()
        for word in ("published interface", "contract", "source", "constraint")
    ), forged.stderr
    cli(
        "link",
        forged_provider,
        forged_consumer,
        "--top",
        "count_zero_probe.design.Top",
        "-o",
        final,
        "--replace",
        code=1,
    )
    assert_protected()
    assert {
        str(path): managed(path) for path in (forged_provider, forged_consumer)
    } == forged_before
    (scratch / "forged-provider.interface.ac").write_bytes(
        payload(forged_provider, "interface").read_bytes()
    )

    (scratch / "candidate.json").write_text(
        json.dumps(
            {
                "fixture_contract": {
                    "trace_width": TRACE_WIDTH,
                    "fields": FIELDS,
                    "first_declared_field": "most significant",
                },
                "fixtures": {
                    str(path.relative_to(repo)): digest(path)
                    for path in [
                        fixtures / ("bits-count-zeros" + extension)
                        for extension in (".py", ".cpp", ".sv")
                    ]
                },
                "tools": {
                    str(Path(tool).resolve()): digest(Path(tool))
                    for tool in (
                        args.source_compiler,
                        args.linker,
                        args.emitter,
                        args.optimizer,
                        args.iverilog,
                        args.vvp,
                        args.verilator,
                    )
                },
                "runtime": {str(runtime): digest(runtime)},
                "same_final_for_both_targets": final_hash,
                "units": {
                    str(path): {
                        kind: digest(payload(path, kind))
                        for kind in ("body", "interface")
                    }
                    for path in (provider, unit)
                },
                "known_frames": known_frames,
                "native_four_state_frames": masked_frames,
                "icarus_four_state_frames": masked_frames,
                "native_workers": [1, 2],
                "protected_compile_rejections": 2 * len(cases),
                "negative_receipts": negative_receipts,
                "explicit_closure_guards": [
                    "missing-provider",
                    "mismatched-provider",
                    "forged-fixed-authority",
                ],
                "authority_tamper": {
                    "snapshot_mutations": snapshot_mutations,
                    "diagnostic": forged.stderr,
                },
                "compile_link_controls": sorted(controls),
                "pure_dependencies": {
                    name: sorted(ports) for name, ports in dependencies.items()
                },
                "observation_scope": "compile/link only; both emitters reject/protect outputs",
                "imported_logical_scope": "legacy mapping outputs reject for lack of fixed authority; no logical source-kind metadata roundtrip claimed",
                "huge_width_guards": huge_receipts,
                "historical_roots_closed": 0,
            },
            indent=2,
        )
        + "\n"
    )

print(  # noqa: T201 - CLI fixture status
    f"count-zero gate passed: {known_frames} known/{masked_frames} four-state native workers1/2 and genuine Icarus frames, Verilator known frames; {len(cases) * 2} protected compile rejections"
)
