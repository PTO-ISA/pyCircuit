"""Public C0 admission and execution, with ordinary host divmod goldens."""

import argparse
import hashlib
import itertools
import json
import os
import random
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
parser.add_argument(
    "--runtime", action="store_true", help="exercise runtime fixed-bit divisors"
)
parser.add_argument(
    "--historical",
    action="store_true",
    help="exercise the historical typed-integer result",
)
args = parser.parse_args()
if args.runtime and args.historical:
    parser.error("runtime and historical select different fixture modes")
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
evidence = Path(args.scratch).resolve()
evidence.mkdir(parents=True, exist_ok=True)
# Preserve outputs for diagnosis while making repeated lit runs independent.
scratch = Path(tempfile.mkdtemp(prefix="run-", dir=evidence))
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
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
    (evidence / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == (0 if accepted else 1), commands[-1]
    assert (
        "Assertion failed" not in result.stderr and "Traceback" not in result.stderr
    ), commands[-1]
    return result


def cli(*arguments, accepted=True):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], accepted)


def snapshot(path):
    return {
        p.relative_to(path).as_posix(): p.read_bytes()
        for p in path.rglob("*")
        if p.is_file()
    }


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


widths = (1, 3, 5, 6, 65, 130, 257)
divisors = {
    w: (
        list(range(1, 1 << w))
        if w <= 6
        else [1, 2, 3, 5, 7, 10, 1 << (w - 1), (1 << w) - 1, (1 << 64) + 1]
    )
    for w in widths
}
# Each tuple records the operation's original width and destination width.
fields = [
    (f"w{w}_d{d}_{kind}", w, w, f"n{w}", d, kind)
    for w in widths
    for d in divisors[w]
    for kind in ("q", "r")
]
fields += [
    ("local", 5, 5, "local", 3, "q"),
    ("field", 65, 65, "packet.value", 7, "r"),
    ("direct", 130, 130, "child.value", 5, "q"),
    ("narrow_first", 8, 16, "(a + 1)", 10, "q"),
    ("wide_first", 16, 16, "(wide + 1)", 10, "q"),
    ("wrapped_q", 8, 8, "(a + 1)", 10, "q"),
    ("wrapped_r", 8, 8, "(a + 1)", 10, "r"),
]
result_width = sum(f[2] for f in fields)
design = (
    """import pycircuit as ac
@ac.struct
class Packet:
    value: ac.bits[65]
@ac.struct
class ChildResult:
    value: ac.bits[130]
@ac.module
def Child(value: ac.bits[130]) -> ChildResult:
    return ChildResult(value=value)
@ac.struct
class Result:
"""
    + "\n".join(f"    {name}: ac.bits[{out}]" for name, _, out, *_ in fields)
    + """
@ac.rule
def evaluate(n1, n3, n5, n6, n65, n130, n257, a, child) -> Result:
    local: ac.u5 = n5
    packet = Packet(value=n65)
    wide: ac.u16 = a
    return Result(
"""
    + ",\n".join(
        f"        {name}={expr} {'//' if kind == 'q' else '%'} {d}"
        for name, _, _, expr, d, kind in fields
    )
    + """
    )
@ac.module
def Top(n1: ac.u1, n3: ac.u3, n5: ac.u5, n6: ac.u6,
        n65: ac.bits[65], n130: ac.bits[130], n257: ac.bits[257], a: ac.u8) -> Result:
    child = Child(n130)
    return evaluate(n1, n3, n5, n6, n65, n130, n257, a, child)
"""
)
source = scratch / "source"
source.mkdir(exist_ok=True)
source_path = source / "design.py"
source_path.write_text(design)
(scratch / "design.py").write_text(design)
unit = scratch / "unit"


def compile_source(output, accepted=True, replace=False):
    arguments = [
        "compile",
        "-c",
        source_path,
        "--source-root",
        source,
        "--package-prefix",
        "static_divrem",
        "-o",
        output,
    ]
    if replace:
        arguments.append("--replace")
    return cli(*arguments, accepted=accepted)


# The RNG generates stimuli only; expected q/r always use Python's divmod.
rng = random.Random(20261004)
rows = []
for row in range(64):
    values = {
        w: (
            row % (1 << w)
            if w <= 6
            else (
                [
                    0,
                    (1 << w) - 1,
                    1 << (w - 1),
                    1,
                    1 << 63,
                    1 << 64,
                    (1 << 64) - 1,
                    (1 << 64) + 1,
                ][row]
                if row < 8
                else rng.getrandbits(w)
            )
        )
        for w in widths
    }
    values[8] = [0, 255, 254, 1, 9, 10, 19, 20][row % 8]
    rows.append({w: format(v, f"0{w}b") for w, v in values.items()})
known_sample_count = len(rows)
for mask in range(4):
    values = {}
    for w in (*widths, 8):
        text = list(rows[7][w])
        positions = [0, w - 1, min(63, w - 1), min(64, w - 1)]
        if mask < 2:
            text[w - 1 - positions[mask]] = "x" if mask == 0 else "z"
            if w > 64:
                text[w - 1 - positions[2 + mask]] = "z" if mask == 0 else "x"
        else:
            text = ["x" if mask == 2 else "z"] * w
        values[w] = "".join(text)
    rows.extend([values, rows[1]])

expected = []
for row in rows:
    bits = []
    for _name, width, out, expr, d, kind in fields:
        key = (
            5
            if expr == "local"
            else (
                65
                if expr == "packet.value"
                else (
                    130
                    if expr == "child.value"
                    else (
                        8
                        if expr in ("a", "wide", "(a + 1)", "(wide + 1)")
                        else int(expr[1:])
                    )
                )
            )
        )
        raw = row[key]
        if "x" in raw or "z" in raw:
            bits.append("0" * (out - width) + "x" * width)
            continue
        a = int(raw, 2)
        if expr in ("(a + 1)", "(wide + 1)"):
            a = (a + 1) % (1 << width)
        q, r = divmod(a, d)
        assert q * d + r == a and 0 <= r < d
        bits.append(format(q if kind == "q" else r, f"0{out}b"))
    expected.append("".join(bits))

ports = {**{f"n{w}": w for w in widths}, "a": 8}
drive_rows = [{name: row[w] for name, w in ports.items()} for row in rows]
if args.runtime:
    widths = (1, 3, 5, 6, 13, 32, 64, 65, 130, 257)
    ports = {name + str(w): w for w in widths for name in ("n", "d")}
    fields = [
        (f"{kind}{w}", w, w, f"n{w}", f"d{w}", kind)
        for w in widths
        for kind in ("q", "r")
    ]
    result_width = sum(field[2] for field in fields)
    design = "import pycircuit as ac\n@ac.struct\nclass Result:\n" + "\n".join(
        f"    {name}: ac.bits[{w}]" for name, w, *_ in fields
    )
    design += (
        "\n@ac.module\ndef Top("
        + ", ".join(f"{name}: ac.bits[{w}]" for name, w in ports.items())
        + ") -> Result:\n"
    )
    design += (
        "    return Result("
        + ", ".join(
            f"{kind}{w}=n{w} {'//' if kind == 'q' else '%'} d{w}"
            for w in widths
            for kind in ("q", "r")
        )
        + ")\n"
    )
    source_path.write_text(design)
    (scratch / "design.py").write_text(design)
    drive_rows = []
    for row in range(64):
        values = {}
        for w in widths:
            mask = (1 << w) - 1
            n = (
                [0, mask, 1 << (w - 1), 1, mask - 1][row % 5]
                if row < 20
                else rng.getrandbits(w)
            )
            d = (
                [0, 1, mask, 1 << (w - 1), min(mask, 3)][row % 5]
                if row < 20
                else rng.getrandbits(w)
            )
            values[f"n{w}"] = format(n, f"0{w}b")
            values[f"d{w}"] = format(d, f"0{w}b")
        drive_rows.append(values)
    known_sample_count = len(drive_rows)
    # Keep every original zero-divisor stimulus, but run it in the genuine
    # four-state lane: Verilator's two-state execution cannot assert X results.
    zero_rows = [
        dict(row)
        for row in drive_rows
        if any(int(row[f"d{w}"], 2) == 0 for w in widths)
    ]
    zero_rows.append(
        {
            name: ("0" * w if name.startswith("d") else "1" * w)
            for name, w in ports.items()
        }
    )
    for row in drive_rows:
        for w in widths:
            if int(row[f"d{w}"], 2) == 0:
                row[f"d{w}"] = format(1, f"0{w}b")
    for case in range(6):
        values = dict(drive_rows[3])
        for w in widths:
            operand = "n" if case % 2 == 0 else "d"
            position = (0, w - 1, w // 2)[case // 2]
            text = list(values[f"{operand}{w}"])
            text[w - 1 - position] = "x" if case < 3 else "z"
            values[f"{operand}{w}"] = "".join(text)
            if case == 4:
                values[f"d{w}"] = "0" * w  # unknown numerator must dominate known-zero
        drive_rows.extend((values, drive_rows[1]))
    for zero_row in zero_rows:
        drive_rows.extend((zero_row, dict(drive_rows[1])))
    expected = []
    for row in drive_rows:
        result = []
        for w in widths:
            n, d = row[f"n{w}"], row[f"d{w}"]
            if any(c in n + d for c in "xz") or int(d, 2) == 0:
                result.extend(("x" * w, "x" * w))
            else:
                n, d = int(n, 2), int(d, 2)
                q, rem = divmod(n, d)
                if d:
                    assert q * d + rem == n and rem < d
                result.extend((format(q, f"0{w}b"), format(rem, f"0{w}b")))
        expected.append("".join(result))
    rows = drive_rows

if args.historical:
    source_fixture = fixtures / "static-divrem/typed_integer_operations.py"
    design = source_fixture.read_text()
    source_path.write_text(design)
    (scratch / "design.py").write_text(design)
    ports = {"value": 64}
    result_width = 64
    widths = (64,)
    divisors = {64: "input low byte"}
    word_mask = (1 << 64) - 1
    known_values = list(range(1, 256))
    known_values += [0xFEDCBA9876543200 | divisor for divisor in range(1, 256)]
    known_values += [1 << bit | 1 for bit in range(8, 64)]
    known_values += [word_mask, word_mask - 1, 65535, 65537, 0x8000000000000007]
    known_values += [rng.getrandbits(64) | 1 for _ in range(128)]
    drive_rows = [
        {"value": format(value, "064b")} for value in dict.fromkeys(known_values)
    ]
    known_sample_count = len(drive_rows)
    recovery = {"value": format(0xFEDCBA9876543201, "064b")}
    special_values = [
        format(value, "064b") for value in (0, 256, 1 << 63, word_mask & ~255)
    ]
    for bit in range(64):
        for symbol in "xz":
            value = list(format(0x123456789ABCDEF7, "064b"))
            value[63 - bit] = symbol
            special_values.append("".join(value))
    for symbols in itertools.product("01xz", repeat=3):
        special_values.append(
            format(0x123456789ABCDEF0, "064b")[:-3] + "".join(symbols)
        )
    special_values.extend(("x" * 64, "z" * 64))
    for value in special_values:
        drive_rows.extend(({"value": value}, dict(recovery)))

    def known_historical(value):
        divisor = value & 255
        assert divisor
        quotient, remainder = divmod(value, divisor)
        assert quotient * divisor + remainder == value and remainder < divisor
        low3 = value & 7
        signed_byte = low3 if low3 < 4 else low3 + 248
        return (
            (quotient & 65535)
            | (remainder << 16)
            | (low3 << 24)
            | (signed_byte << 32)
            | (17 << 40)
            | (((value // 8) & 255) << 48)
            | ((value % 8) << 56)
        )

    def symbol_historical(value):
        # Model the original replication/mask/OR equation, independently from
        # the migrated conditional-prefix source and emitted IR.
        def bit_and(a, b):
            return "0" if "0" in (a, b) else "1" if a == b == "1" else "x"

        def bit_or(a, b):
            return "1" if "1" in (a, b) else "0" if a == b == "0" else "x"

        def binary(a, b, operation):
            return "".join(operation(x, y) for x, y in zip(a, b, strict=True))

        def integer(number):
            return format(number, "064b")

        def arithmetic(divisor, remainder=False):
            if any(c in value + divisor for c in "xz") or int(divisor, 2) == 0:
                return "x" * 64
            quotient, rem = divmod(int(value, 2), int(divisor, 2))
            return integer(rem if remainder else quotient)

        def widen(bits):
            return bits.zfill(64)

        def shift(bits, count):
            return bits[count:] + "0" * count

        divisor = binary(value, integer(255), bit_and)
        narrow = value[-3:]
        sign_extended = narrow[0] * 61 + narrow
        terms = (
            widen(arithmetic(divisor)[-16:]),
            shift(widen(arithmetic(divisor, True)[-8:]), 16),
            shift(widen(narrow), 24),
            shift(binary(sign_extended, integer(255), bit_and), 32),
            shift(integer(17), 40),
            shift(widen(arithmetic(integer(8))[-8:]), 48),
            shift(arithmetic(integer(8), True), 56),
        )
        result = integer(0)
        for term in terms:
            result = binary(result, term, bit_or)
        return result

    expected = []
    for row in drive_rows:
        value = row["value"]
        symbols = symbol_historical(value)
        if all(c in "01" for c in value) and int(value, 2) & 255:
            golden = format(known_historical(int(value, 2)), "064b")
            assert symbols == golden, "independent integer and symbol oracles disagree"
            expected.append(golden)
        else:
            expected.append(symbols)
    rows = drive_rows

header = [
    f"constexpr unsigned result_width = {result_width};",
    f"constexpr unsigned known_sample_count = {known_sample_count};",
    "const std::string_view expected[] = {",
    *[f'  "{v}",' for v in expected],
    "};",
]
for name in ports:
    header += [
        f"const std::string_view inputs_{name}[] = {{",
        *[f'  "{row[name]}",' for row in drive_rows],
        "};",
    ]
header += ["void drive(pyc_dut::Inputs &ports, unsigned row) {"]
for name, w in ports.items():
    header += [f"  ports.{name} = input<{w}>(inputs_{name}[row]);"]
header += ["}"]
(scratch / "static-divrem-vectors.hpp").write_text("\n".join(header) + "\n")
sv = [
    f"wire [{result_width - 1}:0] result;",
    f"localparam integer SAMPLE_COUNT = {len(rows)};",
    f"localparam integer KNOWN_SAMPLE_COUNT = {known_sample_count};",
]
for name, w in ports.items():
    sv += [f"logic [{w - 1}:0] {name};"]
sv += ["task drive(input integer row);", "case(row)"]
for index, row in enumerate(drive_rows):
    sv += [f"{index}: begin"]
    for name, w in ports.items():
        sv += [f"{name}={w}'b{row[name]};"]
    sv += ["end"]
sv += [
    "endcase",
    "endtask",
    f"function automatic logic [{result_width - 1}:0] golden(input integer row);",
    "case(row)",
]
sv += [f"{i}:golden={result_width}'b{v};" for i, v in enumerate(expected)]
sv += ["default:golden='x;", "endcase", "endfunction"]
(scratch / "static-divrem-vectors.svh").write_text("\n".join(sv) + "\n")
compile_source(unit)
final = scratch / "design_top.ac"
cli(
    "link",
    unit,
    "--top",
    "static_divrem.design." + ("TypedIntegerOperations" if args.historical else "Top"),
    "-o",
    final,
)
for target in ("cpp", "verilog"):
    cli("emit", final, "--target", target, "-o", scratch / target)
toolroot = Path(args.source_compiler).resolve().parent.parent
runtime = next(
    (
        p
        for p in (
            toolroot / "runtime/libpyc6_runtime.a",
            toolroot / "lib/libpyc6_runtime.a",
        )
        if p.is_file()
    ),
    None,
)
assert runtime, "Runtime archive missing"
receipt = json.loads((scratch / "cpp/generated.json").read_text())
cpp = [
    scratch / "cpp" / row["path"]
    for row in receipt["files"]
    if row["path"].endswith(".cpp")
]
runner = scratch / "runner"
run(
    [
        args.cxx,
        "-std=c++20",
        "-pthread",
        "-I" + str(repo / "include"),
        "-I" + str(scratch / "cpp"),
        "-I" + str(scratch),
        fixtures / "static-divrem.cpp",
        *cpp,
        runtime,
        "-o",
        runner,
    ]
)
traces = []
for workers in (1, 2):
    trace = run([runner, workers]).stdout
    (scratch / f"workers-{workers}.stdout").write_text(trace)
    traces.append(
        [row for row in trace.splitlines() if row.startswith(("WORK ", "MASK "))]
    )
assert len(traces[0]) == len(rows) and traces[0] == traces[1]
receipt = json.loads((scratch / "verilog/generated.json").read_text())
rtl = [
    scratch / "verilog" / row["path"]
    for row in receipt["files"]
    if row["role"] == "rtl"
]
rtl.sort(key=lambda p: (p.name != "design_top.sv", str(p)))
run(
    [
        args.verilator,
        "--binary",
        "--timing",
        "--top-module",
        "tb",
        "--prefix",
        "Vstatic_divrem",
        "--Mdir",
        scratch / "rtl-build",
        "-j",
        "2",
        "-Wno-fatal",
        "-I" + str(scratch),
        *rtl,
        fixtures / "static-divrem.sv",
    ]
)
trace = run([scratch / "rtl-build/Vstatic_divrem"]).stdout
(scratch / "rtl.stdout").write_text(trace)
assert [row for row in trace.splitlines() if row.startswith("WORK ")] == traces[0][
    :known_sample_count
]
if args.iverilog and args.vvp:
    run(
        [
            args.iverilog,
            "-g2012",
            "-DSTATIC_DIVREM_FOUR_STATE",
            "-I" + str(scratch),
            "-s",
            "tb",
            "-o",
            scratch / "rtl-four-state",
            *rtl,
            fixtures / "static-divrem.sv",
        ]
    )
    trace = run([args.vvp, scratch / "rtl-four-state"]).stdout
    (scratch / "rtl-four-state.stdout").write_text(trace)
    assert [
        row for row in trace.splitlines() if row.startswith(("WORK ", "MASK "))
    ] == traces[0]


def scalar(expression, base="ac.u5", extra="", output="ac.u5", body=""):
    return (
        "import pycircuit as ac\nfrom typing import Annotated\n@ac.struct\nclass Result:\n"
        f"    value: {output}\n@ac.module\ndef Top(value: {base}{extra}) -> Result:\n"
        + body
        + f"    return Result(value={expression})\n"
    )


def local_divisor(operator, use=True):
    expression = f"value {operator} divisor" if use else "value"
    return (
        "import pycircuit as ac\n@ac.struct\nclass Result:\n    value: ac.u5\n"
        "@ac.rule\ndef evaluate(value) -> Result:\n    divisor = 3\n"
        f"    return Result(value={expression})\n"
        "@ac.module\ndef Top(value: ac.u5) -> Result:\n    return evaluate(value)\n"
    )


def formal(operator, use=True):
    return (
        "import pycircuit as ac\n@ac.module\n"
        'def Top(value: ac.u5, *, divisor: int = 3) -> {"out": ac.u5}:\n'
        f"    return {{\"out\": value {operator} {'divisor' if use else '3'}}}\n"
    )


def register(operator, use=True):
    return (
        "import pycircuit as ac\n@ac.struct\nclass Result:\n    value: ac.u5\n"
        "@ac.rule\ndef evaluate(value, divisor) -> Result:\n"
        f"    return Result(value=value {operator} {'divisor' if use else '10'})\n"
        "@ac.module\ndef Top(value: ac.u5) -> Result:\n"
        "    divisor: ac.u5 = 10\n    return evaluate(value, divisor)\n"
    )


unsupported = "source operator requires supported exact integer or width-preserving bitwise lowering"
static_guard = (
    "runtime unsigned division/remainder requires authoritative fixed Bits operands"
)
positive_guard = "unsigned division/remainder divisor must be positive"
fit_guard = "unsigned division/remainder divisor must fit input width"
cases, controls = {}, {}
for label, operator in (("quotient", "//"), ("remainder", "%")):
    for name, divisor, extra in (
        ("boolean-true", "True", ""),
        ("boolean-false", "False", ""),
        (
            "singleton-runtime",
            "(divisor * 0 + 3)",
            ", divisor: Annotated[int, range(1 << 5)]",
        ),
        ("conditional", "(3 if choose else 5)", ", choose: ac.u1"),
        ("unbound", "MISSING", ""),
    ):
        cases[label + "-" + name] = (
            scalar(f"value {operator} {divisor}", extra=extra),
            "unknown hardware value" if name == "unbound" else static_guard,
        )
    cases[label + "-runtime-width-mismatch"] = (
        scalar(f"value {operator} divisor", extra=", divisor: ac.u6"),
        "same positive operand width",
    )
    cases[label + "-runtime-boolean-divisor"] = (
        scalar(
            f"value {operator} divisor",
            base="ac.u1",
            extra=", divisor: bool",
            output="ac.u1",
        ),
        static_guard,
    )
    cases[label + "-runtime-integer-divisor"] = (
        scalar(
            f"value {operator} divisor",
            extra=", divisor: Annotated[int, range(1 << 5)]",
        ),
        static_guard,
    )
    for name, divisor, guard in (
        ("zero", "0", positive_guard),
        ("negative", "(0 - 1)", positive_guard),
        ("too-wide", "32", fit_guard),
        ("huge", "(1 << 300)", fit_guard),
    ):
        cases[label + "-" + name] = (scalar(f"value {operator} {divisor}"), guard)
        cases[label + "-zero-input-" + name] = (
            scalar(f"(value * 0) {operator} {divisor}"),
            guard,
        )
    for name, expr, base, extra in (
        ("boolean-input", "value", "bool", ""),
        ("comparison-input", "(value == value)", "ac.u5", ""),
        (
            "runtime-boolean-fixed-peer",
            "(value if choose else (value == value))",
            "ac.u1",
            ", choose: ac.u1",
        ),
        ("runtime-integer", "value", "Annotated[int, range(1 << 5)]", ""),
    ):
        cases[label + "-" + name] = (
            scalar(f"{expr} {operator} 1", base=base, extra=extra, output="ac.u1"),
            (
                "fixed branch peer requires a closed source Integer or Boolean constant"
                if name == "runtime-boolean-fixed-peer"
                else unsupported
            ),
        )
    cases[label + "-local-alias"] = (local_divisor(operator), static_guard)
    cases[label + "-default-formal"] = (
        formal(operator),
        "static integer width is unproved without concrete bindings",
    )
    controls[label + "-register"] = register(operator)
    controls[label + "-runtime-port"] = scalar(
        f"value {operator} divisor", extra=", divisor: ac.u5"
    )
    cases[label + "-static-source"] = (
        scalar(f"7 {operator} 3"),
        "unsupported static binary operator",
    )
    cases[label + "-implicit-narrow"] = (
        scalar(f"value {operator} 1", output="ac.u1"),
        "unsigned boundary implicit narrowing is unsupported",
    )
    for dead in (False, True):
        text = (
            "import pycircuit as ac\n@ac.struct\nclass Cell:\n    value: ac.u5\n"
            "@ac.module\ndef Child(value: ac.u5) -> Cell:\n"
            f"    return Cell(value=value {operator} 1)\n"
            "@ac.module\ndef Top(value: ac.u5) -> Cell:\n"
            "    child = Child(child.value)\n"
            + ("    return Cell(value=value)\n" if dead else "    return child\n")
        )
        cases[label + ("-dead-cycle" if dead else "-observed-cycle")] = (
            text,
            "'ac.module' op combinational cycle in hardware field dependencies",
        )
    controls[label + "-local-declaration"] = local_divisor(operator, False)
    controls[label + "-formal-declaration"] = formal(operator, False)
    controls[label + "-register-declaration"] = register(operator, False)
controls["closed-static-quotient"] = scalar("value // (1 + 2)")
controls["closed-static-remainder"] = scalar("value % ((1 << 4) - 6)")
controls["conditional-authority"] = scalar(
    "(value if choose else value) // 1",
    base="ac.u1",
    extra=", choose: ac.u1",
    output="ac.u1",
)
if args.historical:
    # Existing static/runtime invocations retain the complete rejection matrix.
    cases, controls = {}, {}
protected_paths = [unit, final, scratch / "cpp", scratch / "verilog"]
protected = {p: snapshot(p) if p.is_dir() else p.read_bytes() for p in protected_paths}
for name, text in controls.items():
    source_path.write_text(text)
    (scratch / ("control-" + name + ".py")).write_text(text)
    compile_source(scratch / ("control-" + name))
for name, (text, diagnostic) in cases.items():
    source_path.write_text(text)
    (scratch / (name + ".py")).write_text(text)
    absent = scratch / ("invalid-" + name)
    rejected = compile_source(absent, accepted=False)
    assert diagnostic in rejected.stderr, (name, rejected.stderr)
    assert not absent.exists()
    rejected = compile_source(unit, accepted=False, replace=True)
    assert diagnostic in rejected.stderr, (name, rejected.stderr)
    assert all(
        (snapshot(p) if p.is_dir() else p.read_bytes()) == original
        for p, original in protected.items()
    )
inputs = [fixtures / ("static-divrem" + ext) for ext in (".py", ".cpp", ".sv")]
inputs.append(fixtures.parent / "static-divrem.test")
if args.historical:
    inputs.append(source_fixture)
(evidence / "candidate.json").write_text(
    json.dumps(
        {
            "artifact_directory": str(scratch),
            "fixtures": {str(p.relative_to(repo)): digest(p) for p in inputs},
            "design_sha256": hashlib.sha256(design.encode()).hexdigest(),
            "final_sha256": digest(final),
            "runner_sha256": digest(runner),
            "runtime_divisor": args.runtime,
            "historical_typed_operations": args.historical,
            "small_unique_pairs": (
                0
                if args.runtime or args.historical
                else sum((1 << w) * ((1 << w) - 1) for w in widths if w <= 6)
            ),
            "widths": widths,
            "divisors": divisors,
            "known_rows": known_sample_count,
            "mask_rows": (len(rows) - known_sample_count) // 2,
            "known_recovery_rows": (len(rows) - known_sample_count) // 2,
            "native_workers": [1, 2],
            "icarus_four_state": bool(args.iverilog and args.vvp),
            "rejected_cases": sorted(cases),
            "protected_rejections": len(cases) * 2,
            "positive_controls": sorted(controls),
            "missing_origin_guard_gap": "mixed Boolean/fixed conditional rejects at shared join; no missing-origin div/rem source guard coverage claimed",
            "products": {
                str(p.relative_to(scratch)): {
                    name: hashlib.sha256(value).hexdigest()
                    for name, value in snapshot(p).items()
                }
                for p in (scratch / "cpp", scratch / "verilog")
            },
        },
        indent=2,
    )
    + "\n"
)
print(
    f"{'historical' if args.historical else 'runtime' if args.runtime else 'static'} divrem gate passed: {len(rows)} samples, widths {widths}; native workers1/2, Verilator; {len(cases) * 2} protected rejections; "
    + (
        "native/Icarus four-state and recovery rows"
        if args.iverilog and args.vvp
        else "native four-state and recovery rows; Icarus unavailable"
    )
)  # noqa: T201 - CLI gate status
