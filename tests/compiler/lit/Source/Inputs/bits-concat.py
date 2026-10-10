"""Source concat admission, independent generated transport, and publication guards."""

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
DESIGN = """from enum import Enum
import pycircuit as ac
from pycircuit import concat as join
from concat_probe.provider import Fixed
@ac.encoding(width=3)
class State(Enum):
    ZERO = 0
    ONE = 1
@ac.struct
class ProbeResult:
    one: ac.u1
    pair: ac.u6
    five: ac.bits[33]
    cross: ac.bits[78]
    nested: ac.bits[10]
    widened: ac.u13
    narrowed: ac.bits[15]
    rule_local: ac.u4
    arithmetic: ac.u6
    enum_bits: ac.u4
    child_bits: ac.u6
@ac.rule
def evaluate(tiny, n5, middle, wide, state, child) -> ProbeResult:
    alias = n5
    result = ProbeResult()
    result.one = ac.concat(tiny)
    result.pair = join(alias, tiny)
    result.five = ac.concat(tiny, n5, wide[:11], middle, wide[61:70])
    result.cross = ac.concat(wide, n5)
    result.nested = ac.concat(join(n5, tiny), wide[60:69])[2:12]
    result.widened = ac.concat(n5, tiny)
    result.narrowed = ac.concat(wide, n5)[62:77]
    local = ac.concat(n5[1:4], tiny)
    result.rule_local = local
    result.arithmetic = ac.concat(n5 + 1, tiny)
    result.enum_bits = ac.concat(ac.enum_to_bits(state), tiny)
    result.child_bits = ac.concat(child.value, tiny)
    return result
@ac.module
def Top(tiny: ac.u1, n5: ac.u5, middle: ac.u7, wide: ac.bits[73], state: State) -> ProbeResult:
    child = Fixed(n5)
    shared = ac.concat(n5, tiny)
    return evaluate(tiny, shared[1:6], middle, wide, state, child)
"""
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
        + f'        report("concat", {expression})\n'
        + '    observe()\n    return {"out": 0}\n'
    )


fixed = "requires explicitly unsigned fixed bits"
shape = "concat requires one or more positional fixed-bit operands"
enum_decl = "@ac.encoding(width=3)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n"
struct_decl = "@ac.struct\nclass Record:\n    data: ac.u5\n"
cases = {
    "empty": (scalar("ac.concat()"), shape),
    "keyword": (scalar("ac.concat(value=value)"), shape),
    "mixed-keyword": (scalar("ac.concat(value, other=value)"), shape),
    "keyword-spread": (scalar("ac.concat(value, **value)"), shape),
    "starred": (scalar("ac.concat(*value)"), "concat does not accept starred operands"),
    "subscribed": (
        scalar("ac.concat[5](value)"),
        "concat does not accept type subscription",
    ),
    "integer-input": (
        scalar("ac.concat(value)", "Annotated[int, range(1 << 5)]"),
        "concat operand 1 " + fixed,
    ),
    "boolean-input": (scalar("ac.concat(value)", "bool"), "concat operand 1 " + fixed),
    "integer-literal": (scalar("ac.concat(17)"), "concat operand 1 " + fixed),
    "boolean-literal": (scalar("ac.concat(True)"), "concat operand 1 " + fixed),
    "closed-integer": (scalar("ac.concat((3 * 7) + 1)"), "concat operand 1 " + fixed),
    "closed-boolean": (scalar("ac.concat(1 == 1)"), "concat operand 1 " + fixed),
    "late-integer": (
        scalar("ac.concat(value, value[:2], 3)"),
        "concat operand 3 " + fixed,
    ),
    "late-boolean": (
        scalar("ac.concat(value, value == value)"),
        "concat operand 2 " + fixed,
    ),
    "integer-alias": (
        scalar("ac.concat(alias)", statements="    alias = 7\n"),
        "concat operand 1 " + fixed,
    ),
    "raw-enum": (
        scalar("ac.concat(value)", "State", prelude=enum_decl),
        "concat operand 1 " + fixed,
    ),
    "raw-struct": (
        scalar("ac.concat(value)", "Record", prelude=struct_decl),
        "concat operand 1 " + fixed,
    ),
    "tuple": (scalar("ac.concat((value, value))"), "unsupported"),
    "list": (scalar("ac.concat([value])"), "unsupported"),
    "string": (scalar('ac.concat("bits")'), "literal"),
    "table": (
        scalar(
            "ac.concat(entries)",
            statements="    entries = ac.table[2, ac.u5](init=0)\n",
        ),
        "concat operand 1 " + fixed,
    ),
    "empty-slice": (
        scalar("ac.concat(value[2:2])"),
        "slice bounds require 0 <= lower < upper <= source width",
    ),
    "dynamic-slice": (
        scalar("ac.concat(value[:count])", extra=", count: ac.u3"),
        "slice upper bound must be a bound static Integer",
    ),
    "narrowing": (
        scalar("ac.concat(value, value)", width=5),
        "unsigned boundary implicit narrowing is unsupported",
    ),
    "shadow-alias": (
        scalar(
            "join(value)",
            prelude="from pycircuit import concat as join\n",
            statements="    join = value\n",
        ),
        "source binding is shadowed in its lexical scope: 'join'",
    ),
    "shadow-namespace": (
        scalar("ac.concat(value)", statements="    ac = value\n"),
        "source binding is shadowed in its lexical scope: 'ac'",
    ),
    "unresolved-marker": (scalar("concat(value)"), "unsupported"),
    "zero-width": (
        scalar("ac.concat(value)", base="ac.bits[0]"),
        "hardware static value must be positive and fit u64",
    ),
    "negative-width": (
        scalar("ac.concat(value)", base="ac.bits[0 - 1]"),
        "hardware static value must be positive and fit u64",
    ),
    "unresolved-width": (
        scalar("ac.concat(value)", base="ac.bits[MISSING]"),
        "hardware static reference must name an integer formal",
    ),
    "width-sum-overflow": (
        scalar("ac.concat(value, value)", base="ac.bits[1 << 63]"),
        "width",
    ),
    "observation-integer": (
        observation("ac.concat(alias)", "Annotated[int, range(1 << 5)]"),
        "concat operand 1 " + fixed,
    ),
    "observation-boolean": (
        observation("ac.concat(alias)", "bool"),
        "concat operand 1 " + fixed,
    ),
    "observation-late-integer": (
        observation("ac.concat(value, 3)"),
        "concat operand 2 " + fixed,
    ),
    "observation-empty": (observation("ac.concat()"), shape),
}

with tempfile.TemporaryDirectory(prefix="concat-", dir=scratch) as temporary:
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
            "concat_probe",
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
            "from pycircuit import concat; "
            "\ntry: concat(1)\nexcept RuntimeError: pass\nelse: raise AssertionError('concat executed')",
        ]
    )
    (source / "provider.py").write_text(PROVIDER)
    provider = build / "provider-unit"
    compile_source("provider.py", provider)
    provider_header = payload(provider, "interface").read_text()
    assert 'source_kind = "fixed_bits"' in provider_header, provider_header
    (source / "provider.py").unlink()
    (source / "design.py").write_text(DESIGN)
    unit = build / "unit"
    compile_source("design.py", unit, (provider,))
    controls = {
        "typed-boundary": scalar(
            "ac.concat(converted, value)", statements="    converted: ac.u5 = 7\n"
        ),
        "structural-wire": PREFIX
        + """@ac.module
def Top(value: ac.u5) -> {"out": ac.u7}:
    local = ac.concat(value, value[:2])
    return {"out": local}
""",
    }
    control_paths = []
    for name, text in controls.items():
        filename = name.replace("-", "_") + ".py"
        (source / filename).write_text(text)
        (scratch / filename).write_text(text)
        control = build / name
        compile_source(filename, control)
        assert '"ac.bits.concat"' in payload(control, "body").read_text()
        control_final = build / (name + ".ac")
        cli(
            "link",
            control,
            "--top",
            "concat_probe." + filename.removesuffix(".py") + ".Top",
            "-o",
            control_final,
        )
        control_paths.extend((control, control_final))
    body = payload(unit, "body").read_text()
    header = payload(unit, "interface").read_text()
    (scratch / "consumer.interface.ac").write_text(header)
    (scratch / "provider.interface.ac").write_text(provider_header)
    assert 'path = "provider.py"' in header and 'path = "design.py"' in header
    top_header = next(
        line
        for line in header.splitlines()
        if 'sym_name = "concat_probe.design.Top"' in line
    )
    dependencies = {
        name: set(map(int, re.findall(r"port = ([0-9]+) : i64", inputs)))
        for inputs, name in re.findall(
            r'inputs = \[(.*?)\], output = \{path = \["([^"]+)"\]', top_header
        )
    }
    assert dependencies["five"] == {0, 1, 2, 3}, dependencies
    assert dependencies["enum_bits"] == {0, 4}, dependencies
    assert dependencies["child_bits"] == {0, 1}, dependencies
    assert "ac.domain_inputs = {}" in top_header, top_header
    assert '"ac.bits.concat"' in body and '"ac.bits.extract"' in body
    assert (
        '"ac.reg"' not in body
        and '"ac.variable"' not in body
        and '"ac.table"' not in body
    )
    # Natural six-bit concat exists independently of its thirteen-bit destination.
    concat_widths = [
        re.findall(r"#ac.math_int<([0-9]+)>", line.rsplit("->", 1)[1])
        for line in body.splitlines()
        if '"ac.bits.concat"' in line
    ]
    assert ["6"] in concat_widths, concat_widths
    (scratch / "source-body.ac").write_text(body)
    output = build / "products"
    output.mkdir()
    final = output / "design.ac"
    cli("link", provider, unit, "--top", "concat_probe.design.Top", "-o", final)
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
            fixtures / "bits-concat.cpp",
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
    assert len(traces[0]) == 16 and traces[0] == traces[1]
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
            "-DCONCAT_FOUR_STATE",
            "-s",
            "tb",
            "-o",
            rtl_runner,
            *sorted((repo / "include/verilog").glob("*.v")),
            *rtl,
            fixtures / "bits-concat.sv",
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
            "Vconcat",
            "--Mdir",
            rtl_build,
            "-j",
            "2",
            "-Wno-fatal",
            *sorted((repo / "include/verilog").glob("*.v")),
            *rtl,
            fixtures / "bits-concat.sv",
        ]
    )
    known_trace = run([rtl_build / "Vconcat"]).stdout
    (scratch / "verilator.stdout").write_text(known_trace)
    assert [
        row for row in known_trace.splitlines() if row.startswith("WORK ")
    ] == traces[0][:8]

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
    (source / "observe.py").write_text(observation("ac.concat(alias[:3], value)"))
    observe_unit = build / "observe-unit"
    compile_source("observe.py", observe_unit)
    observed_body = payload(observe_unit, "body").read_text()
    assert '"ac.bits.concat"' in observed_body and '"ac.observe"' in observed_body
    observe_final = build / "observe.ac"
    cli("link", observe_unit, "--top", "concat_probe.observe.Top", "-o", observe_final)
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
            + "from concat_probe.provider import Logical\n"
            + f'@ac.module\ndef Top(value: {base}, flag: bool) -> {{"out": ac.bits[80]}}:\n'
            + "    child = Logical()\n    @ac.rule\n    def bind():\n        child(value=value, flag=flag)\n    bind()\n"
            + f'    return {{"out": ac.concat(child.{field})}}\n'
        )
        if field == "boolean":
            text = text.replace(
                "child(value=value, flag=flag)", "child(value=0, flag=value)"
            )
        cases["imported-" + field] = (text, "concat operand 1 " + fixed)

    negative_receipts = []
    for name, (text, diagnostic) in cases.items():
        (source / "design.py").write_text(text)
        (scratch / (name + ".py")).write_text(text)
        absent = build / ("invalid-" + name)
        rejected = compile_source(
            "design.py", absent, (provider,), code=1, diagnostic=diagnostic
        )
        assert not absent.exists()
        # Concat-owned diagnostics identify the exact call or offending operand.
        # Earlier declaration/shadow guards retain their existing diagnostic spans.
        captured_span = bool(re.search(r'design\.py"?:[0-9]+:[0-9]+', rejected.stderr))
        concat_diagnostic = re.search(r"error: (concat [^\n]+)", rejected.stderr)
        if concat_diagnostic:
            owning_diagnostic = concat_diagnostic.group(1)
            assert captured_span, rejected.stderr
            call = next(
                node
                for node in ast.walk(ast.parse(text))
                if isinstance(node, ast.Call) and "concat" in ast.unparse(node.func)
            )
            operand = re.match(r"concat operand ([0-9]+)", owning_diagnostic)
            position = call.args[int(operand.group(1)) - 1] if operand else call
            if owning_diagnostic == "concat does not accept starred operands":
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
            "link", *supplied, "--top", "concat_probe.design.Top", "-o", absent, code=1
        )
        assert "dependency" in rejected.stderr.lower(), rejected.stderr
        assert not absent.exists()
        cli(
            "link",
            *supplied,
            "--top",
            "concat_probe.design.Top",
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
        "concat_probe.design.Top",
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
        "concat_probe.design.Top",
        "-o",
        final,
        "--replace",
        code=1,
    )
    assert_protected()
    overflow = next(
        row for row in negative_receipts if row["case"] == "width-sum-overflow"
    )
    (scratch / "candidate.json").write_text(
        json.dumps(
            {
                "fixtures": {
                    str(path.relative_to(repo)): digest(path)
                    for path in [
                        fixtures / ("bits-concat" + extension)
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
                "known_frames": 8,
                "native_four_state_frames": 8,
                "icarus_four_state_frames": 8,
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
                "overflow_guard_executed": "concat width sum overflows uint64"
                in overflow["stderr"],
                "overflow_diagnostic": overflow["stderr"],
                "historical_roots_closed": 0,
            },
            indent=2,
        )
        + "\n"
    )

print(  # noqa: T201 - CLI fixture status
    f"concat gate passed: 8 known/8 four-state native workers1/2 and genuine Icarus frames, Verilator known frames; {len(cases)*2} protected compile rejections"
)
