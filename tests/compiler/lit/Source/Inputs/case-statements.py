"""Independent ordinary match statement public/native/four-state gate."""

import argparse
import ast
import hashlib
import itertools
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
for name in (
    "repo",
    "source-compiler",
    "linker",
    "optimizer",
    "emitter",
    "cxx",
    "verilator",
    "iverilog",
    "vvp",
    "scratch",
):
    parser.add_argument("--" + name, required=True)
parser.add_argument("--prepare-only", action="store_true")
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
evidence = Path(args.scratch).resolve()
evidence.mkdir(parents=True, exist_ok=True)
build = Path(tempfile.mkdtemp(prefix="run-", dir=evidence))
source = build / "source"
source.mkdir()
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
    PYCIRCUIT_EMITTER=args.emitter,
)
commands = []


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, code=0, diagnostic=None):
    command = list(map(str, command))
    result = subprocess.run(
        command, env=env, cwd=repo, text=True, capture_output=True, timeout=240
    )
    row = {
        "command": command,
        "exit_status": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
    }
    commands.append(row)
    (evidence / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == code, row
    assert (
        "Traceback" not in result.stderr and "Assertion failed" not in result.stderr
    ), row
    if diagnostic:
        assert diagnostic in result.stderr, row
    return result


def cli(*arguments, code=0, diagnostic=None):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], code, diagnostic)


def compile_unit(filename, output, imports=(), replace=False, code=0, diagnostic=None):
    command = [
        "compile",
        "-c",
        source / filename,
        "--source-root",
        source,
        "--package-prefix",
        "cases",
        "-o",
        output,
    ]
    for unit in imports:
        command.extend(("-I", unit))
    if replace:
        command.append("--replace")
    return cli(*command, code=code, diagnostic=diagnostic)


def unit_payload_path(unit, kind):
    return unit / json.loads((unit / "unit.json").read_text())["files"][kind]


def snapshot(path):
    return {
        p.relative_to(path).as_posix(): p.read_bytes()
        for p in path.rglob("*")
        if p.is_file()
    }


def managed_snapshot(path):
    return {
        "payload": snapshot(path) if path.is_dir() else path.read_bytes(),
        "control": snapshot(path.parent / ("." + path.name + ".pycircuit-publication")),
    }


CLOCK, RESET = "pyc_7079635f636c6b", "pyc_7079635f727374"
runtime_root = Path(args.source_compiler).resolve().parent.parent
runtime = (
    next(
        p
        for p in (
            runtime_root / "runtime/libpyc6_runtime.a",
            runtime_root / "lib/libpyc6_runtime.a",
        )
        if p.is_file()
    )
    if not args.prepare_only
    else None
)

COM_SOURCE = "import pycircuit as ac\nfrom typing import Annotated\nfrom cases.facade import State as State\n\n@ac.struct\nclass Packet:\n    head: ac.u2\n    tail: ac.bits[65]\n\n@ac.struct\nclass Result:\n    selected_pass: ac.u4\n    repeated_pass: ac.u1\n    no_default: ac.u4\n    boolean: ac.u2\n    integer_or: ac.u3\n    enum_fallback: ac.u4\n    subject_once: ac.u2\n    wide_key: ac.u2\n    huge_key: ac.u2\n    packet: Packet\n\n@ac.rule\ndef evaluate(selector,boolean,integer_value,enum_value,raw3,wide,huge,payload) -> Result:\n    x: ac.u4 = 7\n    match selector:\n        case 0:\n            x = 1\n        case 1:\n            pass\n        case _:\n            x = 9\n    repeated: ac.u1 = 0\n    match selector:\n        case 0:\n            pass\n        case -0:\n            repeated = 1\n        case _:\n            pass\n    fallthrough: ac.u4 = 7\n    match selector:\n        case 0:\n            fallthrough = 1\n    boolean_result: ac.u2 = 0\n    match boolean:\n        case True:\n            boolean_result = 1\n        case False:\n            pass\n        case _:\n            boolean_result = 3\n    integer_result: ac.u3 = 0\n    match integer_value:\n        case 0 | 2:\n            integer_result = 5\n        case 1:\n            pass\n        case _:\n            integer_result = 2\n    enum_result: ac.u4 = 7\n    match enum_value:\n        case State.ZERO:\n            enum_result = 2\n        case State.ONE:\n            pass\n        case _:\n            enum_result = 9\n    once: ac.u2 = 0\n    match raw3[1:3]:\n        case 0:\n            once = 1\n        case 1 | 2:\n            once = 2\n        case _:\n            once = 3\n    wide_result: ac.u2 = 0\n    match wide:\n        case 0:\n            wide_result = 1\n        case 18446744073709551619:\n            wide_result = 2\n        case _:\n            wide_result = 3\n    huge_result: ac.u2 = 0\n    match huge:\n        case 0:\n            huge_result = 1\n        case 680564733841876926926749214863536422919:\n            huge_result = 2\n        case _:\n            huge_result = 3\n    packet = Packet(head=payload,tail=wide)\n    match selector:\n        case 0:\n            packet.head = 3\n        case 1:\n            pass\n        case _:\n            packet.tail = wide\n    return Result(selected_pass=x,repeated_pass=repeated,no_default=fallthrough,\n        boolean=boolean_result,integer_or=integer_result,enum_fallback=enum_result,\n        subject_once=once,wide_key=wide_result,huge_key=huge_result,packet=packet)\n\n@ac.module\ndef Top(selector: ac.u1,boolean: bool,integer_value: Annotated[int,range(1 << 2)],\n        enum_value: State,raw3: ac.bits[3],wide: ac.bits[65],huge: ac.bits[130],payload: ac.u2) -> Result:\n    return evaluate(selector,boolean,integer_value,enum_value,raw3,wide,huge,payload)\n"

STATE_SOURCE = "import pycircuit as ac\n@ac.struct\nclass Pair:\n    left: ac.u1\n    right: ac.u1\n@ac.struct\nclass Result:\n    prior: ac.u1\n    proposed: ac.u1\n    only_prior: ac.u1\n    only_proposed: ac.u1\n    sibling_prior: ac.u1\n    sibling_proposed: ac.u1\n    packet: Pair\n    table0: ac.u1\n    table1: ac.u1\n@ac.rule\ndef update(owner,only,sibling,packet,entries,selector,prior,payload) -> Result:\n    old = owner\n    sibling_old = sibling\n    old_only = only\n    if prior:\n        owner = payload\n    match selector:\n        case 0:\n            pass\n        case 0:\n            owner = 1\n        case _:\n            pass\n    match selector:\n        case 0:\n            only = payload\n    sibling = 1\n    match selector:\n        case 0:\n            packet.left = payload\n            match payload:\n                case 0:\n                    entries[0] = 1\n                case _:\n                    entries[1] = 0\n        case _:\n            if prior:\n                packet.right = payload\n    return Result(prior=old,proposed=owner,only_prior=old_only,only_proposed=only,sibling_prior=sibling_old,\n        sibling_proposed=sibling,packet=packet,table0=entries[0],table1=entries[1])\n@ac.module\ndef Top(selector: ac.u1,prior: ac.u1,payload: ac.u1) -> Result:\n    owner: ac.u1 = 0\n    only: ac.u1 = 0\n    sibling: ac.u1 = 0\n    packet: Pair = Pair()\n    entries = ac.table[2,ac.u1](init=0)\n    return update(owner,only,sibling,packet,entries,selector,prior,payload)\n"

PROVIDER_SOURCE = "from enum import Enum\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n"

FACADE_SOURCE = "from cases.provider import State\n"

CASE_VECTORS = json.loads((fixtures / "case-vectors.json").read_text())
COM_ROWS, COM_GOLD = CASE_VECTORS["rows"], CASE_VECTORS["gold"]


def word(value, width):
    return format(value % (1 << width), f"0{width}b")


def choose(control, yes, no):
    if control == "1":
        return yes
    if control == "0":
        return no
    return "".join(a if a == b else "x" for a, b in zip(yes, no, strict=True))


def equal(a, b):
    if any(x in "01" and y in "01" and x != y for x, y in zip(a, b, strict=True)):
        return "0"
    return "1" if a == b and all(x in "01" for x in a) else "x"


def invert(value):
    return "1" if value == "0" else "0" if value == "1" else "x"


def wide_patterns(width):
    patterns = [
        word(0, width),
        word(1, width),
        word((1 << width) - 1, width),
        word((1 << (width - 1)) + 7, width),
    ]
    for bit in sorted({0, 1, 63, 64, width - 1}):
        for symbol in "xz":
            value = list(word(0, width))
            value[width - bit - 1] = symbol
            patterns.append("".join(value))
    return patterns


def make_vectors(
    directory,
    inputs,
    fields,
    rows,
    gold,
    state=False,
    probes=(),
    probe_gold=(),
    failures=(),
    known_prefix=None,
    plane_rows=(),
    probe_planes=(),
    plane_inputs=None,
    latent=False,
):
    result_width = sum(fields.values())
    (directory / "case-statements-width.hpp").write_text(
        f"constexpr unsigned result_width={result_width};\n"
    )
    offsets = {
        name: sum(list(fields.values())[index + 1 :])
        for index, name in enumerate(fields)
    }
    cpp = [f"constexpr unsigned row_count={len(rows)};"]

    def arrays(label, frames):
        return [
            f"const std::string_view {label}_{name}[]={{"
            + ",".join(json.dumps(row[name]) for row in frames)
            + "};"
            for name in inputs
        ]

    def drive_cpp(function, label):
        return (
            [f"void {function}(pyc_dut::Inputs &p,unsigned row){{"]
            + [
                f"p.{name}=decltype(p.{name})::fromPacked(input<{width}>({label}_{name}[row]).packed());"
                for name, width in inputs.items()
            ]
            + ["}"]
        )

    cpp += arrays("normal", rows) + drive_cpp("drive", "normal")
    cpp += [
        "const std::string_view expected[]={"
        + ",".join(json.dumps("".join(g.values())) for g in gold)
        + "};"
    ]

    def plane_cpp(function, label, expected_rows, masks=()):
        code = [
            f"template<class Output> void {function}(const Output &o,unsigned row){{"
        ]
        if state:
            for name, width in fields.items():
                condition = f"plane_{label}_{name}[row]" if masks else "true"
                code.append(
                    f"if({condition})planes(o.result,{offsets[name]},input<{width}>({label}_{name}[row]));"
                )
        elif plane_inputs is not None:
            for field, specification in plane_inputs.items():
                if isinstance(specification, dict):
                    control, yes, no = (
                        specification["control"],
                        specification["yes"],
                        specification["no"],
                    )
                    code.append(
                        f'if(normal_{control}[row]=="0"||normal_{control}[row]=="1")planes(o.result,{offsets[field]},input<{fields[field]}>(normal_{control}[row]=="1"?normal_{yes}[row]:normal_{no}[row]));'
                    )
                else:
                    input_name, source_offset = (
                        specification
                        if isinstance(specification, tuple)
                        else (specification, 0)
                    )
                    code.append(
                        f"planes(o.result,{offsets[field]},input<{inputs[input_name]}>(normal_{input_name}[row]),{source_offset},{fields[field]});"
                    )
        else:
            pass
        return code + ["}"]

    if state:
        for label, frames in (("normal_gold", gold), ("probe_gold", probe_gold)):
            cpp += [
                f"const std::string_view {label}_{name}[]={{"
                + ",".join(json.dumps(g[name]) for g in frames)
                + "};"
                for name in fields
            ]
    if state:
        for label, masks in (("normal_gold", plane_rows), ("probe_gold", probe_planes)):
            if masks:
                cpp += [
                    f"const bool plane_{label}_{name}[]={{"
                    + ",".join("true" if name in mask else "false" for mask in masks)
                    + "};"
                    for name in fields
                ]
    cpp += plane_cpp("checkPlanes", "normal_gold", gold, plane_rows)
    if state:
        cpp += [
            f"constexpr unsigned probe_count={len(probes)},failure_count={len(failures)};",
            "const unsigned probe_action[]={"
            + ",".join(str(action) for _, action in probes)
            + "};",
        ]
        cpp += arrays("probe", [row for row, _ in probes]) + drive_cpp(
            "driveProbe", "probe"
        )
        cpp += [
            "const std::string_view probe_expected[]={"
            + ",".join(json.dumps("".join(g.values())) for g in probe_gold)
            + "};"
        ]
        cpp += plane_cpp("checkProbePlanes", "probe_gold", probe_gold, probe_planes)
        cpp += arrays("failure", failures) + drive_cpp("driveFailure", "failure")
        cpp += [
            "void driveRoot(pyc_root &root,const pyc_dut::Inputs &p){",
            *[f"root.{name}=p.{name};" for name in inputs],
            "}",
        ]
    if latent:
        cpp += [
            'void replayLatentTuples(unsigned workers){gfsim::WorkExecutor pool(workers);pyc_root root("latent",&pool);',
            "for(unsigned row=0;row<row_count;++row)for(unsigned pattern=0;pattern<4;++pattern){",
        ]
        cpp += [
            f"const auto {name}=latentInput<{width}>(normal_{name}[row],pattern);root.{name}=decltype(root.{name})::fromPacked({name}.packed());"
            for name, width in inputs.items()
        ]
        cpp += ["root.Work();check(root,expected[row]);"]
        cpp += [
            f"planes(root.result,{offsets[field]},{input_name});"
            for field, input_name in plane_inputs.items()
        ]
        cpp += ["root.DiscardNext();root.Xfer();}}"]
    (directory / "case-statements-vectors.hpp").write_text("\n".join(cpp) + "\n")
    known = [
        (
            index < known_prefix
            if known_prefix is not None
            else all(c in "01" for value in row.values() for c in value)
        )
        for index, row in enumerate(rows)
    ]
    sv = [
        f"localparam integer row_count={len(rows)};",
        *[f"logic[{width - 1}:0] {name}=0;" for name, width in inputs.items()],
        f"wire[{result_width - 1}:0] result;",
    ]
    if state:
        sv += ["logic next_clock;"]
    sv += ["task drive(input integer row);case(row)"]
    for index, row in enumerate(rows):
        sv += [
            f"{index}:begin",
            *[
                f"{'next_clock' if state and name == CLOCK else name}={inputs[name]}'b{value};"
                for name, value in row.items()
            ],
            "end",
        ]
    sv += [
        "endcase endtask",
        f"function automatic logic[{result_width - 1}:0] golden(input integer row);case(row)",
    ]
    sv += [
        f"{index}:golden={result_width}'b{''.join(g.values())};"
        for index, g in enumerate(gold)
    ]
    sv += [
        "default:golden='x;endcase endfunction",
        "function automatic bit known_row(input integer row);case(row)",
    ]
    sv += [f"{index}:known_row={int(value)};" for index, value in enumerate(known)] + [
        "default:known_row=0;endcase endfunction"
    ]
    if state and failures:
        sv += [
            f"task drive_failure;{CLOCK}=0;"
            + "".join(
                f"{name}={inputs[name]}'b{value};"
                for name, value in failures[0].items()
                if name != CLOCK
            )
            + "endtask"
        ]
    (directory / "case-statements-vectors.svh").write_text("\n".join(sv) + "\n")
    return known


def execute(
    name,
    text,
    top,
    inputs,
    fields,
    rows,
    gold,
    state=False,
    probes=(),
    probe_gold=(),
    failures=(),
    known_prefix=None,
    plane_rows=(),
    probe_planes=(),
    plane_inputs=None,
    latent=False,
):
    output = build / ("semantics-" + name)
    output.mkdir()
    filename = name + ".py"
    (source / filename).write_text(text)
    unit = output / "unit"
    compiled = compile_unit(filename, unit, list(units.values()))
    if name == "com":
        assert compiled.stdout == "", commands[-1]
        assert (
            "warning: match arm is shadowed for known keys" in compiled.stderr
            and "remark: match covers" in compiled.stderr
        ), commands[-1]
        # Capture is the existing syntax-only product transport, never source execution.
        sys.path.insert(0, str(repo / "python"))
        from pycircuit._source_capture import _capture_source_file
        from pycircuit._source_transport import _emit_source_transport

        transport = output / "native.capture.mlir"
        transport.write_text(
            _emit_source_transport(
                _capture_source_file(source / filename, source_root=source)
            )
        )
        command = [
            args.source_compiler,
            "--capture",
            transport,
            "--package",
            "cases",
            "--path",
            filename,
            "--body-out",
            output / "native.body.ac",
            "--interface-out",
            output / "native.header.ac",
            "--deps-out",
            output / "native.deps.json",
        ]
        for header in units.values():
            command += ["--header", unit_payload_path(header, "interface")]
        native = run(command)
        assert (
            native.stdout == ""
            and "warning: match arm is shadowed for known keys" in native.stderr
            and "remark: match covers" in native.stderr
        ), commands[-1]
        assert (output / "native.body.ac").read_bytes() == unit_payload_path(
            unit, "body"
        ).read_bytes()
        assert (output / "native.header.ac").read_bytes() == unit_payload_path(
            unit, "interface"
        ).read_bytes()
    final = output / "design.ac"
    cli(
        "link",
        *[units[n] for n in declaration_sources],
        unit,
        "--top",
        "cases." + name + "." + top,
        "-o",
        final,
    )
    run([args.optimizer, final, "--ac-verify-hardware", "-o", output / "verified.ac"])
    for target in ("cpp", "verilog"):
        cli("emit", final, "--target", target, "-o", output / target)
    known = make_vectors(
        output,
        inputs,
        fields,
        rows,
        gold,
        state,
        probes,
        probe_gold,
        failures,
        known_prefix,
        plane_rows,
        probe_planes,
        plane_inputs,
        latent,
    )
    cpp_receipt = json.loads((output / "cpp/generated.json").read_text())
    cpp = [
        output / "cpp" / item["path"]
        for item in cpp_receipt["files"]
        if item["path"].endswith(".cpp")
    ]
    defines = ["-DCASE_STATEMENTS_STATE"] if state else []
    if latent:
        defines.append("-DCASE_STATEMENTS_LATENT")
    runner = output / "runner"
    run(
        [
            args.cxx,
            "-O2",
            "-std=c++20",
            "-pthread",
            *defines,
            "-I" + str(repo / "include"),
            "-I" + str(output / "cpp"),
            "-I" + str(output),
            fixtures / "case-statements.cpp",
            *cpp,
            runtime,
            "-o",
            runner,
        ]
    )
    config = output / "config.json"
    config.write_text(
        json.dumps(
            {
                "schema": "pycircuit-model-config",
                "version": "1",
                "max_ticks": len(rows) + 16,
                "max_domain_cycles": {},
                "deadlock_window": None,
            },
            separators=(",", ":"),
            sort_keys=True,
        )
        + "\n"
    )
    traces = []
    for workers in (1, 2):
        result = run([runner, "--workers", workers, "--config", config])
        (output / f"workers-{workers}.stdout").write_text(result.stdout)
        traces.append(
            [line for line in result.stdout.splitlines() if line.startswith("WORK ")]
        )
    assert traces[0] == traces[1] and len(traces[0]) == len(rows)
    rtl_receipt = json.loads((output / "verilog/generated.json").read_text())
    rtl = [
        output / "verilog" / item["path"]
        for item in rtl_receipt["files"]
        if item["role"] == "rtl"
    ]
    rtl.sort(key=lambda p: (p.name != "design_top.sv", str(p)))
    primitives = [repo / "include/verilog/dff.v", repo / "include/verilog/dffe.v"]
    run(
        [
            args.iverilog,
            "-g2012",
            "-DCASE_STATEMENTS_FOUR_STATE",
            *defines,
            "-I" + str(output),
            "-s",
            "tb",
            "-o",
            output / "four.vvp",
            *primitives,
            *rtl,
            fixtures / "case-statements.sv",
        ]
    )
    observed = run([args.vvp, output / "four.vvp"]).stdout
    assert [
        line for line in observed.splitlines() if line.startswith("WORK ")
    ] == traces[0]
    run(
        [
            args.verilator,
            "--binary",
            "--timing",
            "--top-module",
            "tb",
            "--prefix",
            "Venumsource",
            "--Mdir",
            output / "rtl-build",
            "-j",
            "2",
            "-Wno-fatal",
            *defines,
            "-I" + str(output),
            *primitives,
            *rtl,
            fixtures / "case-statements.sv",
        ]
    )
    observed = run([output / "rtl-build/Venumsource"]).stdout
    assert [line for line in observed.splitlines() if line.startswith("WORK ")] == [
        line for line, included in zip(traces[0], known, strict=True) if included
    ]
    if state and failures:
        run(
            [
                args.iverilog,
                "-g2012",
                "-DCASE_STATEMENTS_FOUR_STATE",
                "-DCASE_STATEMENTS_FAILURE",
                *defines,
                "-I" + str(output),
                "-s",
                "tb",
                "-o",
                output / "failure.vvp",
                *primitives,
                *rtl,
                fixtures / "case-statements.sv",
            ]
        )
        failed = run([args.vvp, output / "failure.vvp"], 1)
        assert "enable must be known" in failed.stdout + failed.stderr
    return {
        "name": name,
        "unit": str(unit),
        "final": str(final),
        "output": str(output),
        "rows": len(rows),
        "known_rows": sum(known),
        "probe_actions": len(probes),
        "latent_replay_actions_per_worker": len(rows) * 4 if latent else 0,
        "failure_recovery_cases": len(failures),
        "final_sha256": digest(final),
        "runner_sha256": digest(runner),
    }


for name, text in (
    ("provider.py", PROVIDER_SOURCE),
    ("facade.py", FACADE_SOURCE),
    ("com.py", COM_SOURCE),
):
    ast.parse(text)
    (source / name).write_text(text)
if args.prepare_only:
    print(build)  # noqa: T201
    sys.exit(0)
units = {}
for name in ("provider.py", "facade.py"):
    output = build / name.removesuffix(".py")
    compile_unit(name, output, list(units.values()))
    units[name] = output
    (source / name).unlink()
declaration_sources = ("provider.py", "facade.py")
com_inputs = {
    "selector": 1,
    "boolean": 1,
    "integer_value": 2,
    "enum_value": 2,
    "raw3": 3,
    "wide": 65,
    "huge": 130,
    "payload": 2,
}
com_fields = {
    "selected_pass": 4,
    "repeated_pass": 1,
    "no_default": 4,
    "boolean": 2,
    "integer_or": 3,
    "enum_fallback": 4,
    "subject_once": 2,
    "wide_key": 2,
    "huge_key": 2,
    "packet": 67,
}
receipt = execute(
    "com",
    COM_SOURCE,
    "Top",
    com_inputs,
    com_fields,
    COM_ROWS,
    COM_GOLD,
    plane_inputs={},
)

ENUM_PAYLOAD_SOURCE = """import pycircuit as ac
from cases.facade import State
@ac.struct
class Result:
    value: State
    raw: ac.u2
    member: ac.u1
    fallback: ac.u2
@ac.rule
def evaluate(raw,alternate,selector) -> Result:
    decoded, member = ac.enum_from_bits[State](alternate)
    match selector:
        case 0:
            decoded, member = ac.enum_from_bits[State](raw)
        case 1:
            decoded, member = ac.enum_from_bits[State](alternate)
        case _:
            pass
    fallback: ac.u2 = 3
    match decoded:
        case State.ZERO:
            fallback = 0
        case State.ONE:
            fallback = 1
        case _:
            pass
    return Result(value=decoded,raw=ac.enum_to_bits(decoded),member=member,fallback=fallback)
@ac.module
def Top(raw: ac.u2,alternate: ac.u2,selector: ac.u1) -> Result:
    return evaluate(raw,alternate,selector)
"""
# Independent compact goldens: unknown selection merges carriers and membership
# independently; physical nonmembers retain their raw carrier and fallback.
enum_vectors = [
    ("00", "01", "0", "00", "1", "00"),
    ("00", "01", "1", "01", "1", "01"),
    ("10", "01", "0", "10", "0", "11"),
    ("00", "11", "1", "11", "0", "11"),
    ("11", "10", "z", "1x", "0", "11"),
    ("00", "01", "z", "0x", "1", "xx"),
    ("10", "01", "x", "xx", "x", "xx"),
    ("00", "01", "x", "0x", "1", "xx"),
]
enum_rows = [
    {"raw": raw, "alternate": alternate, "selector": selector}
    for raw, alternate, selector, *_ in enum_vectors
]
enum_gold = [
    {"value": carrier, "raw": carrier, "member": member, "fallback": fallback}
    for *_, carrier, member, fallback in enum_vectors
]
enum_receipt = execute(
    "enum_payload",
    ENUM_PAYLOAD_SOURCE,
    "Top",
    {"raw": 2, "alternate": 2, "selector": 1},
    {"value": 2, "raw": 2, "member": 1, "fallback": 2},
    enum_rows,
    enum_gold,
    plane_inputs={},
)


# State result and write-enable oracles use the same complete branch environments.
def match_values(subject, incoming, arms, default=None):
    result = dict(incoming if default is None else default)
    for keys, arm in reversed(arms):
        tests = [equal(subject, word(key, len(subject))) for key in keys]
        predicate = "1" if "1" in tests else "x" if "x" in tests else "0"
        result = {key: choose(predicate, arm[key], result[key]) for key in incoming}
    return result


state_inputs = {"selector": 1, "prior": 1, "payload": 1, CLOCK: 1, RESET: 1}
state_fields = {
    "prior": 1,
    "proposed": 1,
    "only_prior": 1,
    "only_proposed": 1,
    "sibling_prior": 1,
    "sibling_proposed": 1,
    "packet": 2,
    "table0": 1,
    "table1": 1,
}


def state_row(clock="0", reset="0", selector="1", prior="0", payload="1"):
    return {
        "selector": selector,
        "prior": prior,
        "payload": payload,
        CLOCK: clock,
        RESET: reset,
    }


class StateOracle:
    def __init__(self):
        self.clock = "0"
        self.initialize()

    def initialize(self):
        self.q = {
            "owner": "0",
            "only": "0",
            "sibling": "0",
            "left": "0",
            "right": "0",
            "table0": "0",
            "table1": "0",
        }

    def sample(self, row, action=0):
        selector, prior, payload = row["selector"], row["prior"], row["payload"]
        incoming = {}
        for field, value in self.q.items():
            incoming[field] = value
            incoming[field + "_en"] = "0"
        incoming["owner"] = choose(prior, payload, self.q["owner"])
        incoming["owner_en"] = prior
        repeated = dict(incoming)
        repeated["owner"] = "1"
        repeated["owner_en"] = "1"
        env = match_values(
            selector,
            incoming,
            [((0,), dict(incoming)), ((0,), repeated)],
            dict(incoming),
        )
        only = dict(env)
        only["only"] = payload
        only["only_en"] = "1"
        env = match_values(selector, env, [((0,), only)])
        env["sibling"] = "1"
        env["sibling_en"] = "1"
        arm0 = dict(env)
        arm0["left"] = payload
        arm0["left_en"] = "1"
        inner0 = dict(arm0)
        inner0["table0"] = "1"
        inner0["table0_en"] = "1"
        inner_default = dict(arm0)
        inner_default["table1"] = "0"
        inner_default["table1_en"] = "1"
        arm0 = match_values(payload, arm0, [((0,), inner0)], inner_default)
        default = dict(env)
        default["right"] = choose(prior, payload, self.q["right"])
        default["right_en"] = prior
        env = match_values(selector, env, [((0,), arm0)], default)
        result = {
            "prior": self.q["owner"],
            "proposed": env["owner"],
            "only_prior": self.q["only"],
            "only_proposed": env["only"],
            "sibling_prior": self.q["sibling"],
            "sibling_proposed": env["sibling"],
            "packet": env["left"] + env["right"],
            "table0": env["table0"],
            "table1": env["table1"],
        }
        rising = self.clock == "0" and row[CLOCK] == "1"
        failure = row[CLOCK] in "xz" or (
            rising
            and (
                row[RESET] in "xz"
                or (
                    row[RESET] == "0"
                    and any(env[key + "_en"] in "xz" for key in self.q)
                )
            )
        )
        assert failure == (action == 2), (row, action, env)
        if action == 0:
            if rising:
                if row[RESET] == "1":
                    self.initialize()
                else:
                    for field in self.q:
                        if env[field + "_en"] == "1":
                            self.q[field] = env[field]
            self.clock = row[CLOCK]
        return result


state_rows = []
for selector, prior, data_word in itertools.product("01", "01", "01"):
    row = state_row(selector=selector, prior=prior, payload=data_word)
    state_rows += [row, row | {CLOCK: "1"}, row]
state_rows += [state_row("1", "1"), state_row()]
state_known_prefix = len(state_rows)
for selector in "xz":
    state_rows += [
        state_row(selector=selector),
        state_row("1", "1", selector),
        state_row(),
    ]
state_oracle = StateOracle()
state_gold = [state_oracle.sample(row) for row in state_rows]
probe_rows = [
    (state_row(), 0),
    (state_row("1", selector="0", prior="1", payload="1"), 1),
    (state_row("1", selector="0", prior="1", payload="1"), 0),
    (state_row(), 0),
]
failure_rows = []
for control in ("selector", "prior", CLOCK, RESET):
    for symbol in "xz":
        failed = state_row("1", selector="1", prior="0")
        failed[control] = symbol
        failure_rows.append(failed)
        probe_rows += [
            (failed, 2),
            (state_row("1", selector="1", prior="1"), 0),
            (state_row(), 0),
        ]
probe_rows += [(state_row("1", "1", "x", "x", "x"), 0), (state_row(), 0)]
probe_oracle = StateOracle()
probe_gold = [probe_oracle.sample(row, action) for row, action in probe_rows]
state_receipt = execute(
    "state",
    STATE_SOURCE,
    "Top",
    state_inputs,
    state_fields,
    state_rows,
    state_gold,
    True,
    probe_rows,
    probe_gold,
    failure_rows,
    state_known_prefix,
    [{"prior", "only_prior", "sibling_prior"} for _ in state_rows],
    [{"prior", "only_prior", "sibling_prior"} for _ in probe_rows],
)

base = """import pycircuit as ac
from typing import Annotated
from cases.facade import State
@ac.struct
class Result:
    value: ac.u2
@ac.rule
def evaluate(subject,payload) -> Result:
    value: ac.u2 = 0
    match subject:
        case PATTERN:
            value = 1
        case _:
            pass
    return Result(value=value)
@ac.module
def Top(subject: SUBJECT, payload: ac.u2) -> Result:
    return evaluate(subject,payload)
"""


def scalar(pattern="0", subject="ac.u1", body=None):
    text = base.replace("PATTERN", pattern).replace("SUBJECT", subject)
    return text if body is None else text.replace("            value = 1", body)


controls = {
    "scalar": scalar(),
    "boolean": scalar("True", "bool"),
    "integer": scalar("0", "Annotated[int,range(1 << 2)]"),
    "enum": scalar("State.ZERO", "State"),
    "wide": scalar("18446744073709551619", "ac.bits[65]"),
    "sparse-large": scalar("0", "ac.bits[65536]").replace(
        "        case _:\n            pass", "        case 17:\n            value = 2"
    ),
    "capacity": scalar(subject="ac.bits[4294967296]").replace(
        "        case 0:\n            value = 1\n        case _:", "        case _:"
    ),
}
cases = {}


def add(name, text, control, intention):
    cases[name] = {"source": text, "control": control, "intention": intention}


add(
    "mathematical-selector",
    scalar(subject="Annotated[int,range(1 << 2)]").replace(
        "match subject:", "match subject + 1:"
    ),
    "integer",
    "arithmetic Integer selector is outside admitted profile",
)
add(
    "signed-selector",
    scalar(subject="Annotated[int,range(1 << 2)]").replace(
        "match subject:", "match subject - 2:"
    ),
    "integer",
    "selector interval crosses zero and is signed",
)
add(
    "unresolved-selector",
    scalar(subject="ac.bits[W]"),
    "scalar",
    "undeclared annotation width rejects at source signature scope",
)

for name, pattern in [
    ("guard", "0 if payload"),
    ("capture", "captured"),
    ("as", "0 as captured"),
    ("class", "State()"),
    ("sequence", "[0, 1]"),
    ("mapping", "{0: captured}"),
    ("star-sequence", "[*captured]"),
    ("none", "None"),
    ("string", '"text"'),
    ("bytes", 'b"text"'),
    ("float", "1.5"),
    ("complex", "1+2j"),
    ("duplicate-key", "0 | -0"),
    ("negative-key", "-1"),
    ("oversize-key", "2"),
    ("attribute", "ac.missing"),
    ("or-catchall", "0 | _"),
]:
    add(
        name,
        scalar(pattern),
        "scalar",
        "excluded pattern/guard or exact canonical unsigned key guard",
    )
add(
    "catchall-not-last",
    scalar().replace("case 0:", "case _:").replace("case _:", "case _:", 1),
    "scalar",
    "catch-all is unique/final",
)
add(
    "two-catchalls",
    scalar().replace("case 0:", "case _:"),
    "scalar",
    "catch-all is unique/final",
)
add(
    "bool-integer-key",
    scalar("0", "bool"),
    "boolean",
    "Boolean selector rejects Integer key",
)
add("u1-bool-key", scalar("True"), "scalar", "bits selector rejects Boolean key")
add(
    "integer-bool-key",
    scalar("True", "Annotated[int,range(1 << 2)]"),
    "integer",
    "Integer selector rejects Boolean key",
)
add(
    "enum-integer-key",
    scalar("0", "State"),
    "enum",
    "Enum selector requires canonical same-Enum member",
)
add(
    "enum-other-member",
    scalar("Peer.ONE", "State").replace(
        "from cases.facade import State",
        "from cases.facade import State\nfrom cases.peer import State as Peer",
    ),
    "enum",
    "distinct nominal key rejected even equal width/codes",
)
add(
    "wide-overflow",
    scalar(str(1 << 65), "ac.bits[65]"),
    "wide",
    "exact arbitrary-precision key checked before narrowing",
)
add(
    "huge-overflow",
    scalar(str(1 << 130), "ac.bits[130]"),
    "wide",
    "exact arbitrary-precision key checked before narrowing",
)
for name, body in [
    ("return", "            return Result(value=1)"),
    ("allocation", "            local: ac.u2 = 0"),
    ("module-call", "            child = Child(payload)"),
    ("rule-call", "            other(payload)"),
    ("external-effect", "            print(payload)"),
]:
    text = scalar(body=body)
    if name == "module-call":
        text = text.replace(
            "@ac.rule\ndef evaluate",
            "@ac.module\ndef Child(payload: ac.u2) -> Result:\n    return Result(value=payload)\n@ac.rule\ndef evaluate",
        )
    if name == "rule-call":
        text = text.replace(
            "@ac.rule\ndef evaluate",
            "@ac.rule\ndef other(payload) -> Result:\n    return Result(value=payload)\n@ac.rule\ndef evaluate",
        )
    add(
        "arm-" + name,
        text,
        "scalar",
        "existing branch statement subset remains bounded",
    )
# Exhaustive known coverage cannot create a fresh name under X/Z.
partial = (
    scalar()
    .replace("    value: ac.u2 = 0\n", "")
    .replace(
        "        case _:\n            pass", "        case 1:\n            value = 2"
    )
)
add(
    "partial-known-exhaustive",
    partial,
    "scalar",
    "no incoming and no catch-all leaves physical unknown fallback unbound",
)
add(
    "partial-arm",
    scalar().replace("    value: ac.u2 = 0\n", ""),
    "scalar",
    "partial local later read rejected",
)
controls["partial-recovered"] = partial.replace(
    "    return Result(value=value)", "    value = 3\n    return Result(value=value)"
)
controls["partial-unused"] = partial.replace(
    "    return Result(value=value)", "    return Result(value=3)"
)
# Ordinary assignment only inside Match must classify a real writable owner.
writer = """import pycircuit as ac
@ac.struct
class Result:
    old: ac.u1
    proposed: ac.u1
@ac.rule
def write(owner,selector,payload) -> Result:
    old = owner
    match selector:
        case 0:
            owner = payload
    return Result(old=old,proposed=owner)
@ac.module
def Top(selector: ac.u1,payload: ac.u1) -> Result:
    state: ac.u1 = 0
    return write(state,selector,payload)
"""
controls["match-writer"] = writer
add(
    "overlapping_writers",
    writer.replace(
        "    return write(state,selector,payload)",
        "    first = write(state,selector,payload)\n    return write(state,selector,payload)",
    ),
    "match-writer",
    "overlapping writes rejected through Match-only effect",
)
alias = (
    writer.replace(
        "def write(owner,selector,payload)", "def write(owner,alias,selector,payload)"
    )
    .replace(
        "            owner = payload",
        "            owner = payload\n            alias = payload",
    )
    .replace("write(state,selector,payload)", "write(state,state,selector,payload)")
)
add(
    "writable-alias",
    alias,
    "match-writer",
    "alias guard sees writable owner formals whose writes occur only in Match",
)
add(
    "enum-shadow-in-arm",
    scalar(
        "State.ZERO",
        "State",
        body="            State = payload\n            value = ac.enum_to_bits(State.ZERO)",
    ).replace("value: ac.u2 = 0", "value: ac.u2 = 0"),
    "enum",
    "lexical shadow walk traverses match arm body",
)
add(
    "marker-shadow-in-arm",
    scalar(
        body="            ac = payload\n            value = ac.enum_to_bits(State.ZERO)"
    ),
    "scalar",
    "namespace shadow walk traverses match arm body",
)

allocation = cases["arm-allocation"]
controls["arm-local-annotation"] = allocation["source"]
allocation["source"] = allocation["source"].replace(
    "local: ac.u2 = 0", "local = ac.table[2,ac.u1](init=0)"
)
for name, message in [
    ("enum-shadow-in-arm", "Enum member declaration is shadowed in its lexical scope"),
    ("marker-shadow-in-arm", "source binding is shadowed in its lexical scope: 'ac'"),
]:
    cases[name]["diagnostic"] = message
# Two annotated/source-kind controls and existing ordered F1 policy.
scalar = controls["scalar"]
ordered = (
    scalar.replace("    value: ac.u2 = 0\n", "")
    .replace("            value = 1", "            value = payload")
    .replace(
        "        case _:\n            pass",
        "        case 1:\n            value = 1\n        case _:\n            value = 2",
    )
)
cases["ordered-join-proof"] = {
    "source": ordered,
    "control": "ordered-typed",
    "intention": "retain ordered binary F1 joins; inner integer merge loses closed-source proof",
}
controls["ordered-typed"] = ordered.replace(
    "    match subject:", "    value: ac.u2 = 0\n    match subject:"
)
cases["arm-reannotation"] = {
    "source": scalar.replace("            value = 1", "            value: ac.u3 = 1"),
    "control": "scalar",
    "intention": "existing annotation cannot be replaced inside Match",
}
# Whole aggregates are payloads only; selector forms are deferred.
record = scalar.replace(
    "@ac.struct\nclass Result:",
    "@ac.struct\nclass Packet:\n    code: ac.u1\n@ac.struct\nclass Result:",
).replace("subject: ac.u1", "subject: Packet")
cases["struct-selector"] = {
    "source": record,
    "control": "struct-projection",
    "intention": "whole struct selector is outside ordinary closed-selector profile",
}
controls["struct-projection"] = record.replace("match subject:", "match subject.code:")
table = scalar.replace("subject: ac.u1, payload: ac.u2", "payload: ac.u2").replace(
    "    return evaluate(subject,payload)",
    "    subject = ac.table[2,ac.u1](init=0)\n    return evaluate(subject,payload)",
)
cases["table-selector"] = {
    "source": table,
    "control": "table-projection",
    "intention": "whole table selector is deferred; scalar projection remains admitted",
}
controls["table-projection"] = table.replace("match subject:", "match subject[0]:")

SOURCE_GUARDS = {
    "mathematical-selector": "match signed or mathematical Integer selectors are unsupported",
    "signed-selector": "match signed or mathematical Integer selectors are unsupported",
    "unresolved-selector": "hardware static reference must name an integer formal of its owning module",
    "guard": "match guards are unsupported",
    "capture": "match capture/as patterns are unsupported; use a qualified member",
    "as": "match capture/as patterns are unsupported; use a qualified member",
    "class": "unsupported match pattern 'MatchClass'",
    "sequence": "unsupported match pattern 'MatchSequence'",
    "mapping": "unsupported match pattern 'MatchMapping'",
    "star-sequence": "unsupported match pattern 'MatchSequence'",
    "none": "match singleton key requires True or False",
    "string": "match key requires an Integer literal or qualified Enum member",
    "bytes": "match key requires an Integer literal or qualified Enum member",
    "float": "match key requires an Integer literal or qualified Enum member",
    "complex": "match key requires an Integer literal or qualified Enum member",
    "duplicate-key": "duplicate canonical match alternatives within one arm",
    "negative-key": "match Integer key is outside the selector's unsigned width",
    "oversize-key": "match Integer key is outside the selector's unsigned width",
    "attribute": "match Attribute key requires a canonical Enum member",
    "or-catchall": "match catch-all must be a final standalone arm outside OR",
    "catchall-not-last": "match catch-all must be the final arm",
    "two-catchalls": "match catch-all must be the final arm",
    "bool-integer-key": "match Boolean selector requires True/False singleton keys",
    "u1-bool-key": "match unsigned/Integer selector requires Integer literal keys",
    "integer-bool-key": "match unsigned/Integer selector requires Integer literal keys",
    "enum-integer-key": "match Enum key requires a member of the same nominal type",
    "enum-other-member": "match Enum key requires a member of the same nominal type",
    "wide-overflow": "match Integer key is outside the selector's unsigned width",
    "huge-overflow": "match Integer key is outside the selector's unsigned width",
    "arm-return": "rule return inside a match arm is unsupported",
    "arm-allocation": "unsupported hardware expression 'Call'",
    "arm-module-call": "direct module calls are only admitted at module scope",
    "arm-rule-call": "unsupported behavioral rule statement 'Expr'",
    "arm-external-effect": "unsupported behavioral rule statement 'Expr'",
    "partial-known-exhaustive": "unknown hardware value 'value'",
    "partial-arm": "unknown hardware value 'value'",
    "overlapping_writers": "overlapping state writes across rule registrations",
    "writable-alias": "overlapping writable owner aliases",
    "enum-shadow-in-arm": "Enum member declaration is shadowed in its lexical scope",
    "marker-shadow-in-arm": "source binding is shadowed in its lexical scope: 'ac'",
    "ordered-join-proof": "fixed branch peer requires a closed source Integer or Boolean constant",
    "arm-reannotation": "annotation cannot replace a declared binding boundary",
    "struct-selector": "match selector requires authoritative Boolean, unsigned bits, finite nonnegative Integer or nominal Enum",
    "table-selector": "match selector requires authoritative Boolean, unsigned bits, finite nonnegative Integer or nominal Enum",
}

source_case_archive = build / "source-case-inputs"
source_case_archive.mkdir()
(source / "peer.py").write_text(PROVIDER_SOURCE)
peer_unit = build / "peer-unit"
compile_unit("peer.py", peer_unit)
(source / "peer.py").unlink()
source_imports = [*units.values(), peer_unit]
control_units = {}
source_control_receipts = []
for name, text in controls.items():
    filename = "guard_" + name.replace("-", "_") + ".py"
    (source / filename).write_text(text)
    (source_case_archive / filename).write_text(text)
    directory = build / ("guard-control-" + name)
    directory.mkdir()
    unit = directory / "unit"
    compiled = compile_unit(filename, unit, source_imports)
    if name == "sparse-large":
        assert (
            "remark: match covers" not in compiled.stderr and compiled.stdout == ""
        ), commands[-1]
    final = directory / "control.ac"
    cli(
        "link",
        *source_imports,
        unit,
        "--top",
        "cases." + filename.removesuffix(".py") + ".Top",
        "-o",
        final,
    )
    control_units[name] = {"unit": unit, "final": final, "filename": filename}
    source_control_receipts.append(
        {
            "name": name,
            "body_sha256": digest(unit_payload_path(unit, "body")),
            "final_sha256": digest(final),
        }
    )
protected_paths = [
    Path(item[key])
    for item in (receipt, state_receipt, enum_receipt)
    for key in ("unit", "final")
] + [
    Path(item["output"]) / target
    for item in (receipt, state_receipt, enum_receipt)
    for target in ("cpp", "verilog")
]
protected_paths += [
    item[key] for item in control_units.values() for key in ("unit", "final")
]
protected_paths += list(units.values()) + [peer_unit]
protected = {str(path): managed_snapshot(path) for path in protected_paths}
capacity_receipts = []
capacity_diagnostic = "hardware payload plane including enclosing collections exceeds the current Runtime bit-width capacity"
for target in ("cpp", "verilog"):
    absent = build / ("capacity-absent-" + target)
    failed = cli(
        "emit",
        control_units["capacity"]["final"],
        "--target",
        target,
        "-o",
        absent,
        code=1,
        diagnostic=capacity_diagnostic,
    )
    assert failed.stdout == "" and not absent.exists(), target
    replacement = Path(receipt["output"]) / target
    failed = cli(
        "emit",
        control_units["capacity"]["final"],
        "--target",
        target,
        "-o",
        replacement,
        "--replace",
        code=1,
        diagnostic=capacity_diagnostic,
    )
    assert (
        failed.stdout == ""
        and {str(path): managed_snapshot(path) for path in protected_paths} == protected
    ), target
    capacity_receipts.append(
        {
            "target": target,
            "selector_width": 4294967296,
            "diagnostic": capacity_diagnostic,
            "fresh_exit_status": 1,
            "replacement_exit_status": 1,
        }
    )
source_guard_receipts = []
for name, case in cases.items():
    control = control_units[case["control"]]
    (source / control["filename"]).write_text(case["source"])
    archived = source_case_archive / ("bad-" + name + ".py")
    archived.write_text(case["source"])
    absent = build / ("guard-absent-" + name)
    fresh = compile_unit(
        control["filename"],
        absent,
        source_imports,
        code=1,
        diagnostic=SOURCE_GUARDS[name],
    )
    assert fresh.stdout == "" and not absent.exists(), name
    replaced = compile_unit(
        control["filename"],
        control["unit"],
        source_imports,
        replace=True,
        code=1,
        diagnostic=SOURCE_GUARDS[name],
    )
    assert (
        replaced.stdout == ""
        and {str(path): managed_snapshot(path) for path in protected_paths} == protected
    ), name
    source_guard_receipts.append(
        {
            "case": name,
            "control": case["control"],
            "diagnostic": SOURCE_GUARDS[name],
            "fresh_exit_status": 1,
            "replacement_exit_status": 1,
            "source_sha256": digest(archived),
        }
    )
capture_directory = build / "capture-guards"
run(
    [
        sys.executable,
        fixtures / "case_capture_guards.py",
        "--repo",
        repo,
        "--source-compiler",
        args.source_compiler,
        "--optimizer",
        args.optimizer,
        "--scratch",
        capture_directory,
    ]
)
capture_result = json.loads((capture_directory / "results.json").read_text())
assert (
    len(capture_result["cases"]) == 12
    and capture_result["base_compile_exit_status"]
    == capture_result["base_lowered_body_interface_extraction_exit_status"]
    == 0
)
paths = [
    fixtures / ("case-statements" + suffix) for suffix in (".py", ".cpp", ".sv")
] + [
    fixtures.parent / "case-statements.test",
    fixtures / "case-vectors.json",
    fixtures / "case_capture_guards.py",
]
(evidence / "candidate.json").write_text(
    json.dumps(
        {
            "scope": "F4 closed ordinary Match public source/native1/2/four-state RTL and source/native-capture guards",
            "artifact_directory": str(build),
            "fixtures": {str(p): digest(p) for p in paths},
            "tools": {
                str(Path(p).resolve()): digest(Path(p))
                for p in (
                    args.source_compiler,
                    args.linker,
                    args.optimizer,
                    args.emitter,
                    args.cxx,
                    args.verilator,
                    args.iverilog,
                    args.vvp,
                )
            },
            "runtime": {str(runtime): digest(runtime)},
            "products": [receipt, state_receipt, enum_receipt],
            "compiler_only_boundaries": {
                "sparse_selector_width": 65536,
                "sparse_keys": [0, 17],
                "capacity_rejections": capacity_receipts,
            },
            "source_guards": source_guard_receipts,
            "source_controls": source_control_receipts,
            "capture_guard_result": {
                "path": str(capture_directory / "results.json"),
                "sha256": digest(capture_directory / "results.json"),
                "cases": 12,
            },
        },
        indent=2,
    )
    + "\n"
)
print(
    "case-statement gate passed: complete arms/pass/fallback/repeated keys, native1/2 and genuine RTL"
)  # noqa: T201
