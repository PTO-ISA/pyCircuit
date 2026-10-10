"""Independent source popcount admission, generated DUTs, and publication guards."""

import argparse
import ast
import hashlib
import json
import os
import re
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
    PYTHONPATH=str(repo / "python"),
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
FIELDS = [(f"n{width}", width.bit_length()) for width in WIDTHS] + [
    ("widen1", 8),
    ("widen65", 13),
    ("slice73", 4),
    ("arithmetic5", 3),
    ("enum3", 2),
    ("child5", 3),
    ("nested73", 3),
]
DESIGN = """from enum import Enum
import pycircuit as ac
from pycircuit import popcount as count
from popcount_probe.provider import Fixed
@ac.encoding(width=3)
class State(Enum):
    ZERO = 0
    ONE = 1
@ac.struct
class ProbeResult:
""" + "".join(
    f"    {name}: ac.bits[{width}]\n" for name, width in FIELDS
)
DESIGN += (
    "@ac.rule\ndef evaluate("
    + ", ".join(f"w{w}" for w in WIDTHS)
    + ", state, child) -> ProbeResult:\n"
)
DESIGN += "    alias = w5\n    result = ProbeResult()\n"
DESIGN += "".join(f"    result.n{w} = ac.popcount(w{w})\n" for w in WIDTHS)
DESIGN += """    result.widen1 = count(w1)
    result.widen65 = count(w65)
    result.slice73 = ac.popcount(w73[61:70])
    local = alias + 1
    result.arithmetic5 = ac.popcount(local)
    result.enum3 = ac.popcount(ac.enum_to_bits(state))
    result.child5 = ac.popcount(child.value)
    result.nested73 = ac.popcount(count(w73))
    return result
@ac.module
"""
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
        + f'        report("popcount", {expression})\n'
        + '    observe()\n    return {"out": 0}\n'
    )


fixed = "popcount operand requires explicitly unsigned fixed bits"
shape = "popcount requires exactly one positional fixed-bit operand"
enum_decl = "@ac.encoding(width=3)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n"
struct_decl = "@ac.struct\nclass Record:\n    data: ac.u5\n"
cases = {
    "empty": (scalar("ac.popcount()"), shape),
    "two-operands": (scalar("ac.popcount(value, value)"), shape),
    "keyword": (scalar("ac.popcount(value=value)"), shape),
    "mixed-keyword": (scalar("ac.popcount(value, other=value)"), shape),
    "keyword-spread": (scalar("ac.popcount(value, **value)"), shape),
    "starred": (
        scalar("ac.popcount(*value)"),
        "popcount does not accept starred operands",
    ),
    "subscribed": (
        scalar("ac.popcount[5](value)"),
        "popcount does not accept type subscription",
    ),
    "integer-input": (
        scalar("ac.popcount(value)", "Annotated[int, range(1 << 5)]"),
        fixed,
    ),
    "one-bit-integer": (
        scalar("ac.popcount(value)", "Annotated[int, range(1 << 1)]"),
        fixed,
    ),
    "boolean-input": (scalar("ac.popcount(value)", "bool"), fixed),
    "integer-literal": (scalar("ac.popcount(17)"), fixed),
    "boolean-literal": (scalar("ac.popcount(True)"), fixed),
    "closed-integer": (scalar("ac.popcount((3 * 7) + 1)"), fixed),
    "closed-boolean": (scalar("ac.popcount(1 == 1)"), fixed),
    "integer-alias": (
        scalar("ac.popcount(alias)", statements="    alias = 7\n"),
        fixed,
    ),
    "raw-enum": (scalar("ac.popcount(value)", "State", prelude=enum_decl), fixed),
    "raw-struct": (scalar("ac.popcount(value)", "Record", prelude=struct_decl), fixed),
    "tuple": (scalar("ac.popcount((value, value))"), "unsupported"),
    "list": (scalar("ac.popcount([value])"), "unsupported"),
    "string": (scalar('ac.popcount("bits")'), "literal"),
    "table": (
        scalar(
            "ac.popcount(entries)",
            statements="    entries = ac.table[2, ac.u5](init=0)\n",
        ),
        fixed,
    ),
    "empty-slice": (
        scalar("ac.popcount(value[2:2])"),
        "slice bounds require 0 <= lower < upper <= source width",
    ),
    "dynamic-slice": (
        scalar("ac.popcount(value[:count])", extra=", count: ac.u3"),
        "slice upper bound must be a bound static Integer",
    ),
    "narrowing": (
        scalar("ac.popcount(value)", base="ac.u8", width=3),
        "unsigned boundary implicit narrowing is unsupported",
    ),
    "shadow-alias": (
        scalar(
            "count(value)",
            prelude="from pycircuit import popcount as count\n",
            statements="    count = value\n",
        ),
        "source binding is shadowed in its lexical scope: 'count'",
    ),
    "shadow-namespace": (
        scalar("ac.popcount(value)", statements="    ac = value\n"),
        "source binding is shadowed in its lexical scope: 'ac'",
    ),
    "unresolved-marker": (scalar("popcount(value)"), "unsupported"),
    "zero-width": (
        scalar("ac.popcount(value)", base="ac.bits[0]"),
        "hardware static value must be positive and fit u64",
    ),
    "negative-width": (
        scalar("ac.popcount(value)", base="ac.bits[0 - 1]"),
        "hardware static value must be positive and fit u64",
    ),
    "unresolved-width": (
        scalar("ac.popcount(value)", base="ac.bits[MISSING]"),
        "hardware static reference must name an integer formal",
    ),
    "width-over-u64": (
        scalar("ac.popcount(value)", base="ac.bits[1 << 64]"),
        "hardware static value must be positive and fit u64",
    ),
    "observation-integer": (
        observation("ac.popcount(alias)", "Annotated[int, range(1 << 5)]"),
        fixed,
    ),
    "observation-boolean": (observation("ac.popcount(alias)", "bool"), fixed),
    "observation-empty": (observation("ac.popcount()"), shape),
    "observation-narrowing-kind": (observation("ac.popcount(0)"), fixed),
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


with tempfile.TemporaryDirectory(prefix="popcount-", dir=scratch) as temporary:
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
            "popcount_probe",
            "-o",
            output,
        ]
        for unit in imports:
            arguments.extend(("-I", unit))
        if replace:
            arguments.append("--replace")
        return cli(*arguments, **options)

    # The public syntax marker raises on host execution; it is not an evaluator.
    run(
        [
            sys.executable,
            "-c",
            "from pycircuit import popcount; "
            "\ntry: popcount(1)\nexcept RuntimeError: pass\nelse: raise AssertionError('popcount executed')",
        ]
    )
    (source / "provider.py").write_text(PROVIDER)
    (scratch / "provider.py").write_text(PROVIDER)
    provider = build / "provider-unit"
    compile_source("provider.py", provider)
    provider_header = payload(provider, "interface").read_text()
    assert parameter_kinds(provider_header, "popcount_probe.provider.Fixed") == {
        "value": '"fixed_bits"'
    }
    fixed_line = module_header(provider_header, "popcount_probe.provider.Fixed")
    result_constraint = dictionary(
        split_fields(attribute(fixed_line, "ac.result_constraints"))[0][1:-1]
    )
    assert result_constraint["source_kind"] == '"nominal"'
    assert (
        result_constraint["type"] == '!ac.struct<"popcount_probe.provider.FixedResult">'
    )
    logical_line = module_header(provider_header, "popcount_probe.provider.Logical")
    assert "ac.result_constraints = " not in logical_line
    (source / "provider.py").unlink()
    (source / "design.py").write_text(DESIGN)
    (scratch / "design.py").write_text(DESIGN)
    unit = build / "unit"
    compile_source("design.py", unit, (provider,))
    controls = {
        "typed-boundary": scalar(
            "ac.popcount(converted)", statements="    converted: ac.u5 = 7\n", width=3
        ),
        "structural-wire": PREFIX
        + """@ac.module
def Top(value: ac.u5) -> {"out": ac.u3}:
    local = ac.popcount(value)
    return {"out": local}
""",
        "natural-widening": scalar("ac.popcount(value)", base="ac.bits[65]", width=13),
        "explicit-narrowing": scalar("ac.popcount(value)[:2]", base="ac.u8", width=2),
        "fixed-singleton-kind": scalar(
            "ac.popcount(ac.popcount(value))", base="ac.u1", width=1
        ),
        "captured-rule": PREFIX
        + """@ac.module
def Child(value: ac.u3) -> {"out": ac.u3}:
    return {"out": value}
@ac.module
def Top(value: ac.u5) -> {"out": ac.u3}:
    alias = value
    child = Child()
    @ac.rule
    def bind():
        child(value=ac.popcount(alias))
    bind()
    return {"out": child.out}
""",
    }
    control_paths = []
    for name, text in controls.items():
        filename = name.replace("-", "_") + ".py"
        (source / filename).write_text(text)
        (scratch / filename).write_text(text)
        control = build / name
        compile_source(filename, control)
        control_body = payload(control, "body").read_text()
        assert '"ac.bits.extract"' in control_body or name == "fixed-singleton-kind"
        if name == "fixed-singleton-kind":
            assert '"ac.bits.binary"' not in control_body
        if name == "natural-widening":
            extracts = [
                line
                for line in control_body.splitlines()
                if '"ac.bits.extract"' in line
            ]
            assert len(extracts) == 1
            natural = extracts[0]
            assert (
                re.findall(r"#ac.math_int<([0-9]+)>", natural.rsplit("->", 1)[1])[-1]
                == "7"
            )
            result_name = natural.strip().split(" = ", 1)[0]
            resize = next(
                line
                for line in control_body.splitlines()
                if f'"ac.bits.resize"({result_name})' in line
            )
            assert 'mode = "zext"' in resize
            assert (
                re.findall(r"#ac.math_int<([0-9]+)>", resize.rsplit("->", 1)[1])[-1]
                == "13"
            )
        control_final = build / (name + ".ac")
        cli(
            "link",
            control,
            "--top",
            "popcount_probe." + filename.removesuffix(".py") + ".Top",
            "-o",
            control_final,
        )
        control_paths.extend((control, control_final))
    body = payload(unit, "body").read_text()
    header = payload(unit, "interface").read_text()
    (scratch / "consumer.interface.ac").write_text(header)
    (scratch / "provider.interface.ac").write_text(provider_header)
    parameters = parameter_kinds(header, "popcount_probe.design.Top")
    assert parameters == {
        **{f"w{w}": '"fixed_bits"' for w in WIDTHS},
        "state": '"nominal"',
    }, parameters
    dependencies = dependency_ports(header, "popcount_probe.design.Top")
    expected_dependencies = {f"n{w}": {index} for index, w in enumerate(WIDTHS)}
    expected_dependencies.update(
        widen1={0},
        widen65={16},
        slice73={17},
        arithmetic5={4},
        enum3={21},
        child5={4},
        nested73={17},
    )
    assert dependencies == expected_dependencies, dependencies
    assert (
        attribute(
            module_header(header, "popcount_probe.design.Top"), "ac.domain_inputs"
        )
        == ""
    )
    assert '"ac.bits.binary"' in body and '"ac.bits.extract"' in body
    assert all(f'"ac.{name}"' not in body for name in ("reg", "variable", "table"))
    # Natural K-bit extraction precedes any destination conversion.
    natural_widths = [
        re.findall(r"#ac.math_int<([0-9]+)>", line.rsplit("->", 1)[1])[-1]
        for line in body.splitlines()
        if '"ac.bits.extract"' in line
    ]
    assert {str(w.bit_length()) for w in WIDTHS if w > 1} <= set(
        natural_widths
    ), natural_widths
    (scratch / "source-body.ac").write_text(body)
    output = build / "products"
    output.mkdir()
    final = output / "design.ac"
    cli("link", provider, unit, "--top", "popcount_probe.design.Top", "-o", final)
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
                toolroot / "runtime/libpyc6_runtime.a",
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
            fixtures / "bits-popcount.cpp",
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
    assert all(re.fullmatch(r"(?:WORK|MASK) [01xz]{139}", row) for row in traces[0])
    known_frames = sum(row.startswith("WORK ") for row in traces[0])
    masked_frames = sum(row.startswith("MASK ") for row in traces[0])
    assert known_frames == 660 and masked_frames == 524
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
            "-DPOPCOUNT_FOUR_STATE",
            "-s",
            "tb",
            "-o",
            rtl_runner,
            *sorted((repo / "include/verilog").glob("*.v")),
            *rtl,
            fixtures / "bits-popcount.sv",
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
            "Vpopcount",
            "--Mdir",
            rtl_build,
            "-j",
            "2",
            "-Wno-fatal",
            *sorted((repo / "include/verilog").glob("*.v")),
            *rtl,
            fixtures / "bits-popcount.sv",
        ]
    )
    known_trace = run([rtl_build / "Vpopcount"]).stdout
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
    (source / "observe.py").write_text(observation("ac.popcount(alias[:3])"))
    observe_unit = build / "observe-unit"
    compile_source("observe.py", observe_unit)
    observed_body = payload(observe_unit, "body").read_text()
    assert '"ac.bits.extract"' in observed_body and '"ac.observe"' in observed_body
    observe_final = build / "observe.ac"
    cli(
        "link", observe_unit, "--top", "popcount_probe.observe.Top", "-o", observe_final
    )
    protected_paths.extend((observe_unit, observe_final))
    before.update({str(path): managed(path) for path in (observe_unit, observe_final)})
    for target in ("cpp", "verilog"):
        absent = build / ("observe-" + target)
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
            assert not absent.exists()
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
            + "from popcount_probe.provider import Logical\n"
            + f'@ac.module\ndef Top(value: {base}, flag: bool) -> {{"out": ac.bits[80]}}:\n'
            + "    child = Logical()\n    @ac.rule\n    def bind():\n        child(value=value, flag=flag)\n    bind()\n"
            + f'    return {{"out": ac.popcount(child.{field})}}\n'
        )
        if field == "boolean":
            text = text.replace(
                "child(value=value, flag=flag)", "child(value=0, flag=value)"
            )
        cases["imported-" + field] = (text, fixed)

    huge_receipts = []
    for width in (1 << 32, 1 << 63, (1 << 64) - 1):
        filename = f"huge_{width}.py"
        text = scalar(
            "ac.popcount(value)", base=f"ac.bits[{width}]", width=width.bit_length()
        )
        (source / filename).write_text(text)
        (scratch / filename).write_text(text)
        huge = build / f"huge-{width}"
        compile_source(filename, huge)
        huge_body = payload(huge, "body").read_text()
        operations = re.findall(r'"(ac\.bits\.[a-z]+)"', huge_body)
        stages = (width - 1).bit_length()
        assert len(operations) <= 12 * stages + 8, (width, len(operations))
        literals = list(map(int, re.findall(r"#ac.math_int<([0-9]+)>", huge_body)))
        assert literals and all(value.bit_length() <= 64 for value in literals), (
            width,
            literals,
        )
        assert len(huge_body.encode()) <= 131072 * (stages + 1), (width, len(huge_body))
        extractions = [
            line for line in huge_body.splitlines() if '"ac.bits.extract"' in line
        ]
        assert extractions
        assert re.findall(r"#ac.math_int<([0-9]+)>", extractions[0].rsplit("->", 1)[1])[
            -1
        ] == str(width.bit_length())
        huge_final = build / f"huge-{width}.ac"
        cli("link", huge, "--top", f"popcount_probe.huge_{width}.Top", "-o", huge_final)
        protected_paths.extend((huge, huge_final))
        before.update({str(path): managed(path) for path in (huge, huge_final)})
        failures = {}
        for target in ("cpp", "verilog"):
            absent = build / f"huge-{width}-{target}"
            rejected = cli("emit", huge_final, "--target", target, "-o", absent, code=1)
            assert not absent.exists()
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
        huge_receipts.append(
            {
                "width": width,
                "natural_width": width.bit_length(),
                "operations": len(operations),
                "stages": stages,
                "body_bytes": len(huge_body.encode()),
                "largest_literal_bits": max(value.bit_length() for value in literals),
                "emission_diagnostics": failures,
                "scope": "bounded compilation graph/literals; existing emission representation guard, no Runtime allocation claim",
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
        assert not absent.exists()
        # Popcount-owned diagnostics identify the exact call or offending operand.
        # Earlier declaration/shadow guards retain their existing diagnostic spans.
        captured_span = bool(re.search(r'design\.py"?:[0-9]+:[0-9]+', rejected.stderr))
        popcount_diagnostic = re.search(r"error: (popcount [^\n]+)", rejected.stderr)
        if popcount_diagnostic:
            owning_diagnostic = popcount_diagnostic.group(1)
            assert captured_span, rejected.stderr
            call = next(
                node
                for node in ast.walk(ast.parse(text))
                if isinstance(node, ast.Call) and "popcount" in ast.unparse(node.func)
            )
            position = (
                call.args[0]
                if owning_diagnostic.startswith("popcount operand ")
                else call
            )
            if owning_diagnostic == "popcount does not accept starred operands":
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
            "popcount_probe.design.Top",
            "-o",
            absent,
            code=1,
        )
        assert "dependency" in rejected.stderr.lower(), rejected.stderr
        assert not absent.exists()
        cli(
            "link",
            *supplied,
            "--top",
            "popcount_probe.design.Top",
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
        "popcount_probe.design.Top",
        "-o",
        mismatch_final,
        code=1,
    )
    assert not mismatch_final.exists()
    assert any(
        word in rejected.stderr.lower()
        for word in ("match", "interface", "header", "hash")
    ), rejected.stderr
    cli(
        "link",
        mismatch,
        unit,
        "--top",
        "popcount_probe.design.Top",
        "-o",
        final,
        "--replace",
        code=1,
    )
    assert_protected()
    (scratch / "candidate.json").write_text(
        json.dumps(
            {
                "fixtures": {
                    str(path.relative_to(repo)): digest(path)
                    for path in [
                        fixtures / ("bits-popcount" + extension)
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
                "explicit_closure_guards": ["missing-provider", "mismatched-provider"],
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
    f"popcount gate passed: {known_frames} known/{masked_frames} four-state native workers1/2 and genuine Icarus frames, Verilator known frames; {len(cases)*2} protected compile rejections"
)
