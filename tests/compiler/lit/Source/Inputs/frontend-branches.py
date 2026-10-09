"""Independent shared-branch values, effects, constants and publication gate."""

import argparse
import hashlib
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
    "emitter",
    "cxx",
    "verilator",
    "iverilog",
    "vvp",
    "scratch",
):
    parser.add_argument("--" + name, required=True)
parser.add_argument("--baseline", action="store_true")
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
evidence = Path(args.scratch).resolve()
evidence.mkdir(parents=True, exist_ok=True)
build = Path(tempfile.mkdtemp(prefix="run-", dir=evidence))
source = build / "source"
source.mkdir()
design = source / "design.py"
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
    PYCIRCUIT_EMITTER=args.emitter,
)
commands = []


def run(command, code=0):
    command = list(map(str, command))
    result = subprocess.run(
        command, cwd=repo, env=env, text=True, capture_output=True, timeout=240
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
    assert result.returncode == code, commands[-1]
    assert (
        "Assertion failed" not in result.stderr and "Traceback" not in result.stderr
    ), commands[-1]
    return result


def cli(*arguments, code=0):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], code)


def compile_source(output, code=0, replace=False):
    command = [
        "compile",
        "-c",
        design,
        "--source-root",
        source,
        "--package-prefix",
        "branches",
        "-o",
        output,
    ]
    if replace:
        command.append("--replace")
    return cli(*command, code=code)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def snapshot(path):
    return {
        p.relative_to(path).as_posix(): p.read_bytes()
        for p in path.rglob("*")
        if p.is_file()
    }


# Goldens operate on observable 0/1/X/Z symbols, never Runtime select or compiler IR.
def number(value, width):
    return format(value % (1 << width), f"0{width}b")


def choose(control, yes, no):
    if control == "1":
        return yes
    if control == "0":
        return no
    return "".join(a if a == b else "x" for a, b in zip(yes, no, strict=True))


def add(raw, amount):
    return (
        "x" * len(raw)
        if any(s in raw for s in "xz")
        else number(int(raw, 2) + amount, len(raw))
    )


def logic(op, left, right="0"):
    if op == "not":
        return "0" if left == "1" else "1" if left == "0" else "x"
    if op == "and":
        return "0" if "0" in (left, right) else "1" if left == right == "1" else "x"
    return "1" if "1" in (left, right) else "0" if left == right == "0" else "x"


def equal(left, right):
    if any(
        a in "01" and b in "01" and a != b for a, b in zip(left, right, strict=True)
    ):
        return "0"
    return "1" if left == right and all(a in "01" for a in left) else "x"


COM = """import pycircuit as ac
@ac.struct
class Packet:
    head: ac.u13
    tail: ac.bits[65]
@ac.struct
class ComResult:
    math_statement: ac.u4
    math_same: ac.u4
    math_expression: ac.u4
    math_initialized: ac.u4
    math_sequential: ac.u4
    math_signed: ac.u4
    math_wide: ac.bits[130]
    negative_capture: ac.u8
    wide_negative_capture: ac.u8
    fixed_wrap: ac.u1
    literal: ac.u13
    folded: ac.u13
    alias: ac.u13
    captured: ac.u13
    conditional_peer: ac.u13
    wide_peer: ac.bits[65]
    integer_bit_peer: ac.u1
    boolean_literal: ac.u1
    boolean_folded: ac.u1
    boolean_inverted: ac.u1
    producer: ac.u16
    and_left: ac.u1
    and_right: ac.u1
    or_left: ac.u1
    or_right: ac.u1
    long_and: ac.u1
    not_fixed: ac.u1
    not_boolean: ac.u1
    invert_boolean: ac.u1
    selected: ac.u13
    packet: Packet
@ac.rule
def evaluate(data, other, wide, bit_flag, flag, choice, logical, captured, captured_negative, captured_wide_negative) -> ComResult:
    if flag:
        n = 1
        same = 1
        large = (1 << 100)
    else:
        n = 2
        same = 0
        large = (1 << 100) + 1
    sequential = 1
    sequential = sequential + 1
    initialized = 1
    if flag:
        initialized = 2
    else:
        initialized = 0
    signed = (0 - 1) if flag else 2
    saved: ac.u1 = bit_flag
    if bit_flag:
        saved = bit_flag
    else:
        pass
    alias = 1 + 1
    predicate = data == other
    and_left = predicate and bit_flag
    and_right = bit_flag and predicate
    or_left = predicate or bit_flag
    or_right = bit_flag or predicate
    long_and = logical and bit_flag and predicate
    packet = Packet(head=data, tail=wide)
    if choice:
        packet.head = other
    else:
        pass
    return ComResult(
        math_statement=n + 1, math_same=same + 1,
        math_expression=(1 if flag else 2) + 1,
        math_initialized=initialized + 1, math_sequential=sequential, math_signed=signed + 3, math_wide=large + 1,
        negative_capture=captured_negative + 2, wide_negative_capture=captured_wide_negative + 130,
        fixed_wrap=saved + 1,
        literal=(data if choice else 2) + 1,
        folded=(data if choice else (1 + 1)) + 1,
        alias=(alias if choice else data) + 1,
        captured=(data if choice else captured) + 1,
        conditional_peer=(data if choice else (1 if True else (1 << 13))) + 1,
        wide_peer=(wide if choice else ((1 << 65) - 1)) + 1,
        integer_bit_peer=(bit_flag if choice else (1 + 0)) + 1,
        boolean_literal=(bit_flag if choice else False) + 1,
        boolean_folded=((not True) if choice else bit_flag) + 1,
        boolean_inverted=((~True) if choice else bit_flag) + 1,
        producer=alias + 255,
        and_left=and_left + 1, and_right=and_right + 1,
        or_left=or_left + 1, or_right=or_right + 1,
        long_and=long_and + 1, not_fixed=not bit_flag,
        not_boolean=not logical, invert_boolean=~logical,
        selected=data if choice else other, packet=packet)
@ac.module
def Com(data: ac.u13, other: ac.u13, wide: ac.bits[65], bit_flag: ac.u1,
        flag: ac.u1, choice: ac.u1, logical: bool) -> ComResult:
    captured = 1 + 1
    negative = 0 - 1
    negative_wide = 0 - 129
    return evaluate(data, other, wide, bit_flag, flag, choice, logical, captured, negative, negative_wide)
"""
STATE = """
@ac.struct
class StateResult:
    old_a: ac.u13
    old_b: ac.bits[65]
    old_packet: Packet
    old_row0: ac.u5
    old_row1: ac.u5
    old_flag: ac.u1
    next_a: ac.u13
    next_b: ac.bits[65]
    next_packet: Packet
    next_row0: ac.u5
    next_row1: ac.u5
    next_flag: ac.u1
    read_after: ac.u5
@ac.rule
def advance(a, b, packet, entries, flag_state, data, wide, bool_data, prior, choice, pupd, tupd) -> StateResult:
    old_a = a
    old_b = b
    old_packet = packet
    old_row0 = entries[0]
    old_row1 = entries[1]
    old_flag = flag_state
    if prior:
        a = a + 1
    if choice:
        a = data
    else:
        pass
    if choice:
        b = wide
    else:
        b = b + 1
    if choice:
        flag_state = bool_data
    else:
        flag_state = not bool_data
    if pupd:
        packet.head = data
        if choice:
            packet.tail = wide
        else:
            packet.tail = wide
    if tupd:
        entries[0] = data[:5]
        after = entries[0]
        entries[1] = after + 1
    read_after = entries[0]
    return StateResult(old_a=old_a, old_b=old_b, old_packet=old_packet,
        old_row0=old_row0, old_row1=old_row1, old_flag=old_flag, next_a=a, next_b=b,
        next_packet=packet, next_row0=entries[0], next_row1=entries[1], next_flag=flag_state,
        read_after=read_after)
@ac.module
def State(data: ac.u13, wide: ac.bits[65], bool_data: bool, prior: ac.u1, choice: ac.u1,
          pupd: ac.u1, tupd: ac.u1) -> StateResult:
    a: ac.u13 = 0
    b: ac.bits[65] = 0
    packet: Packet = Packet()
    entries = ac.table[2, ac.u5](init=0)
    flag_state: bool = False
    return advance(a, b, packet, entries, flag_state, data, wide, bool_data, prior, choice, pupd, tupd)
"""
DESIGN = COM + STATE
com_inputs = {
    "data": 13,
    "other": 13,
    "wide": 65,
    "bit_flag": 1,
    "flag": 1,
    "choice": 1,
    "logical": 1,
}
state_inputs = {
    "data": 13,
    "wide": 65,
    "bool_data": 1,
    "prior": 1,
    "choice": 1,
    "pupd": 1,
    "tupd": 1,
    "pyc_7079635f636c6b": 1,
    "pyc_7079635f727374": 1,
}
com_rows, com_expected = [], []
for index in range(64):
    row = {
        "data": number((index * 113) % 8192, 13),
        "other": number((index * 79) % 8192, 13),
        "wide": number((1 << 64) + index * 12345, 65),
        "bit_flag": str(index % 2),
        "flag": str((index // 2) % 2),
        "choice": "01xz"[(index // 4) % 4],
        "logical": "01xz"[(index // 16) % 4],
    }
    if index >= 32:
        row["data"] = "01xz"[index % 4] * 13
        row["other"] = row["data"] if index % 3 == 0 else "z" * 13
        row["wide"] = "1" + ("xz" * 32 if index % 2 else "z0" * 32)
    p = equal(row["data"], row["other"])
    a, o = logic("and", p, row["bit_flag"]), logic("or", p, row["bit_flag"])
    long = logic("and", logic("and", row["logical"], row["bit_flag"]), p)
    flag = int(row["flag"])
    values = [
        number(2 if flag else 3, 4),
        number(2 if flag else 1, 4),
        number(2 if flag else 3, 4),
        number(3 if flag else 1, 4),
        number(2, 4),
        number(2 if flag else 5, 4),
        number((1 << 100) + (1 if flag else 2), 130),
        number(1, 8),
        number(1, 8),
        add(row["bit_flag"], 1),
    ]
    values += [add(choose(row["choice"], row["data"], number(2, 13)), 1)] * 2
    values += [
        add(choose(row["choice"], number(2, 13), row["data"]), 1),
        add(choose(row["choice"], row["data"], number(2, 13)), 1),
        add(choose(row["choice"], row["data"], number(1, 13)), 1),
        add(choose(row["choice"], row["wide"], "1" * 65), 1),
        add(choose(row["choice"], row["bit_flag"], "1"), 1),
        add(choose(row["choice"], row["bit_flag"], "0"), 1),
        add(choose(row["choice"], "0", row["bit_flag"]), 1),
        add(choose(row["choice"], "0", row["bit_flag"]), 1),
        number(257, 16),
        add(a, 1),
        add(a, 1),
        add(o, 1),
        add(o, 1),
        add(long, 1),
        logic("not", row["bit_flag"]),
        logic("not", row["logical"]),
        logic("not", row["logical"]),
        choose(row["choice"], row["data"], row["other"]),
        choose(row["choice"], row["other"], row["data"]) + row["wide"],
    ]
    com_rows.append(row)
    com_expected.append("".join(values))


def state_row(
    clock="0", reset="0", prior="0", choice="0", pupd="0", tupd="0", data=7, wide=19
):
    return {
        "data": number(data, 13),
        "wide": number(wide, 65),
        "prior": prior,
        "choice": choice,
        "pupd": pupd,
        "tupd": tupd,
        "pyc_7079635f636c6b": clock,
        "pyc_7079635f727374": reset,
        "bool_data": "1",
    }


class StateOracle:
    def __init__(self):
        self.a, self.b, self.packet, self.rows, self.clock, self.flag = (
            "0" * 13,
            "0" * 65,
            "0" * 78,
            ["0" * 5] * 2,
            "0",
            "0",
        )

    def sample(self, row, action=0):
        a_before_if = choose(row["prior"], add(self.a, 1), self.a)
        a = choose(row["choice"], row["data"], a_before_if)
        b = choose(row["choice"], row["wide"], add(self.b, 1))
        packet = choose(row["pupd"], row["data"] + row["wide"], self.packet)
        zero = choose(row["tupd"], row["data"][-5:], self.rows[0])
        one = choose(row["tupd"], add(row["data"][-5:], 1), self.rows[1])
        flag = choose(row["choice"], row["bool_data"], logic("not", row["bool_data"]))
        output = (
            self.a
            + self.b
            + self.packet
            + "".join(self.rows)
            + self.flag
            + a
            + b
            + packet
            + zero
            + one
            + flag
            + zero
        )
        enables = [
            choose(row["choice"], "1", row["prior"]),
            "1",
            row["pupd"],
            row["tupd"],
        ]
        clock, reset = row["pyc_7079635f636c6b"], row["pyc_7079635f727374"]
        rising = self.clock == "0" and clock == "1"
        failure = clock in "xz" or (
            rising
            and (reset in "xz" or (reset == "0" and any(e in "xz" for e in enables)))
        )
        if action == 2:
            assert failure
        else:
            assert not failure
        if not action:
            if rising:
                if reset == "1":
                    self.a, self.b, self.packet, self.rows, self.flag = (
                        "0" * 13,
                        "0" * 65,
                        "0" * 78,
                        ["0" * 5] * 2,
                        "0",
                    )
                else:
                    if enables[0] == "1":
                        self.a = a
                    self.b = b
                    self.flag = flag
                    if enables[2] == "1":
                        self.packet = packet
                    if enables[3] == "1":
                        self.rows = [zero, one]
            self.clock = clock
        return output


state_rows = []
for index in range(16):
    row = state_row(
        prior=str(index % 2),
        choice=str((index // 2) % 2),
        pupd=str((index // 4) % 2),
        tupd=str((index // 8) % 2),
        data=index * 31,
        wide=(1 << 64) + index,
    )
    state_rows.extend([row, row | {"pyc_7079635f636c6b": "1"}])
state_rows += [
    state_row("1", "0", "1", "1"),
    state_row("1", "1"),
    state_row("0", "1"),
    state_row("1", "1"),
    state_row(),
]
state_known_count = len(state_rows)
# Both-arm writes/prior enables allow selected X/Z payloads without enable failure.
for selector in "xz":
    row = state_row(prior="1", choice=selector, pupd="1", tupd="1")
    row["bool_data"] = selector
    row["data"] = "z" * 13
    row["wide"] = "x" + "z" * 64
    state_rows += [
        row,
        row | {"pyc_7079635f636c6b": "1"},
        row,
        state_row("1", "1", "x", "x", "x", "x"),
        state_row(),
    ]
# A known selected arm transports real Boolean X/Z into storage unchanged.
for symbol in "xz":
    row = state_row(choice="1")
    row["bool_data"] = symbol
    state_rows += [
        row,
        row | {"pyc_7079635f636c6b": "1"},
        row,
        state_row("1", "1"),
        state_row(),
    ]
oracle = StateOracle()
state_expected = [oracle.sample(row) for row in state_rows]
# Native direct-root actions prove discard and late-failure same-edge retry.
probe_rows = [
    (state_row(), 0),
    (state_row("1", prior="1", choice="1", pupd="1", tupd="1"), 0),
    (state_row(), 0),
]
probe_rows += [
    (state_row("1", prior="1", choice="0", data=91), 1),
    (state_row("1", prior="1", choice="0", data=91), 0),
    (state_row(), 0),
]
for control in ("choice", "pupd", "tupd", "pyc_7079635f636c6b", "pyc_7079635f727374"):
    for symbol in "xz":
        bad = state_row("1", prior="0", choice="1", pupd="1", tupd="1")
        bad[control] = symbol
        probe_rows += [
            (bad, 2),
            (state_row("1", choice="1", pupd="1", tupd="1", data=37), 0),
            (state_row(), 0),
        ]
probe_oracle = StateOracle()
probe_expected = [probe_oracle.sample(row, action) for row, action in probe_rows]
failure_rows = []
for control in ("choice", "pupd", "tupd", "pyc_7079635f636c6b", "pyc_7079635f727374"):
    for symbol in "xz":
        bad = state_row("1", prior="0", choice="1", pupd="1", tupd="1")
        bad[control] = symbol
        failure_rows.append(bad)


def vectors(output, names, rows, expected, state=False):
    width = len(expected[0])
    assert all(len(e) == width for e in expected)
    (output / "frontend-branches-width.hpp").write_text(
        f"constexpr unsigned result_width={width};\n"
    )
    header = [
        f"constexpr unsigned row_count={len(rows)};",
        "const std::string_view expected[]={",
        *[f'"{e}",' for e in expected],
        "};",
    ]

    def cpp_rows(label, data):
        result = []
        for name in names:
            result += [
                f"const std::string_view {label}_{name}[]={{",
                *[f'"{r[name]}",' for r in data],
                "};",
            ]
        return result

    def drive_function(name, label):
        return [
            f"void {name}(pyc_dut::Inputs &p,unsigned row){{",
            *[f"p.{n}=input<{w}>({label}_{n}[row]);" for n, w in names.items()],
            "}",
        ]

    header += cpp_rows("normal", rows) + drive_function("drive", "normal")
    if state:
        header += [
            f"constexpr unsigned probe_count={len(probe_rows)},failure_count={len(failure_rows)};",
            "const unsigned probe_action[]={"
            + ",".join(str(a) for _, a in probe_rows)
            + "};",
            "const std::string_view probe_expected[]={",
            *[f'"{e}",' for e in probe_expected],
            "};",
        ]
        header += cpp_rows("probe", [r for r, _ in probe_rows]) + drive_function(
            "driveProbe", "probe"
        )
        header += cpp_rows("failure", failure_rows) + drive_function(
            "driveFailure", "failure"
        )
        header += [
            "void driveRoot(pyc_root &root,const pyc_dut::Inputs &p){",
            *[f"root.{n}=p.{n};" for n in names],
            "}",
        ]
    (output / "frontend-branches-vectors.hpp").write_text("\n".join(header) + "\n")
    sv = [f"localparam integer row_count={len(rows)};", f"wire[{width - 1}:0] result;"]
    sv += [f"logic[{w - 1}:0] {n}=0;" for n, w in names.items()]
    if state:
        sv += ["logic next_clock;"]
    sv += ["task drive(input integer row);case(row)"]
    for index, row in enumerate(rows):
        sv += [f"{index}:begin"]
        sv += [
            f"{'next_clock' if state and n == 'pyc_7079635f636c6b' else n}={names[n]}'b{value};"
            for n, value in row.items()
        ]
        sv += ["end"]
    sv += [
        "endcase endtask",
        f"function automatic logic[{width - 1}:0] golden(input integer row);case(row)",
    ]
    sv += [f"{i}:golden={width}'b{e};" for i, e in enumerate(expected)]
    sv += [
        "default:golden='x;endcase endfunction",
        "function automatic bit known_row(input integer row);case(row)",
    ]
    known = [
        (
            i < state_known_count
            if state
            else all(s in "01" for v in row.values() for s in v)
        )
        for i, row in enumerate(rows)
    ]
    sv += [f"{i}:known_row={int(k)};" for i, k in enumerate(known)]
    sv += ["default:known_row=0;endcase endfunction"]
    if state:
        sv += [
            "task drive_failure;pyc_7079635f636c6b=0;pyc_7079635f727374=0;prior=0;choice=1'bx;pupd=1;tupd=1;endtask"
        ]
    (output / "frontend-branches-vectors.svh").write_text("\n".join(sv) + "\n")
    return known


if args.baseline:
    # Accepted controls are checked before each targeted old rejection.
    controls = {
        "integer-expression": "n = 1 if flag else 0",
        "literal-peer": "n = data if flag else 2",
    }
    template = "import pycircuit as ac\n@ac.struct\nclass Result:\n    value: ac.u8\n@ac.module\ndef Top(data: ac.u8,flag: ac.u1)->Result:\n    BODY\n    return Result(value=n + 1)\n"
    for name, body in controls.items():
        design.write_text(template.replace("BODY", body))
        compile_source(build / name)
    design.write_text(template.replace("BODY", "n = data if flag else (1 + 1)"))
    result = compile_source(build / "folded-peer", code=1)
    assert (
        "mathematical arithmetic requires known Integer kind and finite interval"
        in result.stderr
    )
    design.write_text(DESIGN)
    result = compile_source(build / "new-branches", code=1)
    assert (
        "unsupported behavioral rule statement 'Pass'" in result.stderr
        or "branch values require equal hardware types" in result.stderr
    )
    print(
        "frontend branch baseline: legal controls pass; folded-peer/full new fixture reject"
    )  # noqa: T201
    sys.exit(0)

design.write_text(DESIGN)
unit = build / "unit"
compile_source(unit)
toolroot = Path(args.source_compiler).resolve().parent.parent
runtime = next(
    p
    for p in (
        toolroot / "lib/libpyc6_runtime.a",
        toolroot / "simulator/gfsim/libpyc6_runtime.a",
    )
    if p.is_file()
)
protected_paths = [unit]
receipts = []
for top, names, rows, expected, state in (
    ("Com", com_inputs, com_rows, com_expected, False),
    ("State", state_inputs, state_rows, state_expected, True),
):
    output = build / top
    output.mkdir()
    known = vectors(output, names, rows, expected, state)
    final = output / "design.ac"
    cli("link", unit, "--top", "branches.design." + top, "-o", final)
    for target in ("cpp", "verilog"):
        cli("emit", final, "--target", target, "-o", output / target)
    cpp = [
        output / "cpp" / row["path"]
        for row in json.loads((output / "cpp/generated.json").read_text())["files"]
        if row["path"].endswith(".cpp")
    ]
    runner = output / "runner"
    defines = ["-DSTATE_BRANCHES"] if state else []
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
            fixtures / "frontend-branches.cpp",
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
                "deadlock_window": None,
                "max_domain_cycles": {},
                "max_ticks": 128,
                "schema": "pycircuit-model-config",
                "version": "1",
            },
            separators=(",", ":"),
        )
        + "\n"
    )
    traces = []
    for workers in (1, 2):
        trace = run([runner, "--workers", workers, "--config", config]).stdout
        (output / f"workers-{workers}.stdout").write_text(trace)
        traces.append([line for line in trace.splitlines() if line.startswith("WORK ")])
    assert len(traces[0]) == len(rows) and traces[0] == traces[1]
    rtl = [
        output / "verilog" / row["path"]
        for row in json.loads((output / "verilog/generated.json").read_text())["files"]
        if row["role"] == "rtl"
    ]
    rtl.sort(key=lambda p: (p.name != "design_top.sv", str(p)))
    primitives = sorted((repo / "include/verilog").glob("*.v"))
    run(
        [
            args.verilator,
            "--binary",
            "--timing",
            "--top-module",
            "tb",
            "--prefix",
            "Vbranches",
            "--Mdir",
            output / "rtl-build",
            "-j",
            "2",
            "-Wno-fatal",
            *defines,
            "-I" + str(output),
            *primitives,
            *rtl,
            fixtures / "frontend-branches.sv",
        ]
    )
    trace = run([output / "rtl-build/Vbranches"]).stdout
    assert [line for line in trace.splitlines() if line.startswith("WORK ")] == [
        line for line, k in zip(traces[0], known, strict=True) if k
    ]
    icarus = output / "icarus"
    run(
        [
            args.iverilog,
            "-g2012",
            "-DFRONTEND_BRANCHES_FOUR_STATE",
            *defines,
            "-I" + str(output),
            "-s",
            "tb",
            "-o",
            icarus,
            *primitives,
            *rtl,
            fixtures / "frontend-branches.sv",
        ]
    )
    trace = run([args.vvp, icarus]).stdout
    assert [line for line in trace.splitlines() if line.startswith("WORK ")] == traces[
        0
    ]
    if state:
        run(
            [
                args.iverilog,
                "-g2012",
                "-DFRONTEND_BRANCHES_FOUR_STATE",
                "-DFRONTEND_BRANCHES_FAILURE",
                *defines,
                "-I" + str(output),
                "-s",
                "tb",
                "-o",
                output / "icarus-failure",
                *primitives,
                *rtl,
                fixtures / "frontend-branches.sv",
            ]
        )
        failed = run([args.vvp, output / "icarus-failure"], code=1)
        assert "enable must be known" in failed.stdout + failed.stderr
    protected_paths += [final, output / "cpp", output / "verilog"]
    receipts.append(
        {
            "top": top,
            "frames": len(rows),
            "verilator_frames": sum(known),
            "final_sha256": digest(final),
            "runner_sha256": digest(runner),
            "generated": {
                target: {
                    name: hashlib.sha256(data).hexdigest()
                    for name, data in snapshot(output / target).items()
                }
                for target in ("cpp", "verilog")
            },
        }
    )


def scalar(expression, inputs="data: ac.u13, flag: ac.u1", output="ac.u13", body=""):
    return (
        "import pycircuit as ac\nfrom typing import Annotated\n@ac.struct\nclass Result:\n"
        f"    value: {output}\n@ac.module\ndef Top({inputs})->Result:\n"
        + body
        + f"    return Result(value={expression})\n"
    )


def statement(test, inputs="flag: ac.u1", prefix="", suffix="", alternate="pass"):
    return (
        "import pycircuit as ac\nfrom typing import Annotated\n@ac.struct\nclass Result:\n    value: ac.u13\n"
        "@ac.rule\ndef step(state, flag)->Result:\n"
        + prefix
        + f"    if {test}:\n        state = state + 1\n    else:\n        {alternate}\n"
        + suffix
        + "    return Result(value=state)\n"
        + f"@ac.module\ndef Top({inputs})->Result:\n    state: ac.u13 = 0\n    return step(state, flag)\n"
    )


integer_guard = "conditional requires a Boolean source condition, not Integer"
authority_guard = "conditional requires authoritative Boolean or bits[1]"
closed_guard = "fixed branch peer requires a closed source Integer or Boolean constant"
bounds_guard = "integer interval is not proven within destination bounds"
cases = {
    "integer-if-literal": (statement("1"), integer_guard),
    "integer-if-alias": (statement("alias", prefix="    alias = 1\n"), integer_guard),
    "integer-if-capture": (
        statement("flag", inputs="flag: Annotated[int, range(1 << 1)]"),
        integer_guard,
    ),
    "wide-if": (statement("flag", inputs="flag: ac.u2"), authority_guard),
    "integer-ifexp": (
        scalar(
            "data if flag else 2",
            inputs="data: ac.u13, flag: Annotated[int, range(1 << 1)]",
        ),
        integer_guard,
    ),
    "integer-not": (
        scalar(
            "not flag", inputs="flag: Annotated[int, range(1 << 1)]", output="ac.u1"
        ),
        integer_guard,
    ),
    "integer-and": (
        statement("flag and alias", prefix="    alias = 1\n"),
        integer_guard,
    ),
    "integer-or": (statement("alias or flag", prefix="    alias = 1\n"), integer_guard),
    "runtime-integer-peer": (
        scalar(
            "data if flag else integer",
            inputs="data: ac.u13, flag: ac.u1, integer: Annotated[int, range(1 << 3)]",
        ),
        closed_guard,
    ),
    "singleton-peer": (
        scalar(
            "data if flag else (integer * 0 + 2)",
            inputs="data: ac.u13, flag: ac.u1, integer: Annotated[int, range(1 << 3)]",
        ),
        closed_guard,
    ),
    "boolean-annihilator-peer": (
        scalar(
            "data if flag else (False and boolean)",
            inputs="data: ac.u1, flag: ac.u1, boolean: bool",
            output="ac.u1",
        ),
        closed_guard,
    ),
    "runtime-boolean-peer": (
        scalar(
            "data if flag else boolean",
            inputs="data: ac.u1, flag: ac.u1, boolean: bool",
            output="ac.u1",
        ),
        closed_guard,
    ),
    "fixed-widths": (
        scalar(
            "data if flag else other",
            inputs="data: ac.u13, flag: ac.u1, other: ac.bits[65]",
        ),
        "branch values require equal hardware types",
    ),
    "negative-peer": (scalar("data if flag else (0 - 1)"), bounds_guard),
    "too-wide-peer": (scalar("(1 << 13) if flag else data"), bounds_guard),
    "boolean-wide-peer": (
        scalar("data if flag else (not True)"),
        "closed Boolean branch requires a bits[1] peer",
    ),
    "boolean-integer-join": (
        scalar("True if flag else 1", inputs="flag: ac.u1", output="ac.u1"),
        "conditional branches have different known Boolean and Integer kinds",
    ),
    "boolean-producer-unmodified": (
        scalar(
            "original + 1",
            inputs="data: ac.u1, flag: ac.u1",
            output="ac.u1",
            body="    original = not True\n    converted = data if flag else original\n",
        ),
        "mathematical arithmetic requires known Integer kind and finite interval",
    ),
    "divisor-alias": (
        scalar("data // divisor", body="    divisor = 1 + 2\n"),
        "runtime unsigned division/remainder requires authoritative fixed Bits operands",
    ),
    "shift-count-alias": (
        scalar("data << count", body="    count = 1 + 2\n"),
        "unsigned shift requires a proven static Integer count",
    ),
    "negative-owner-value": (
        statement("flag", alternate="state = (0 - 1)"),
        bounds_guard,
    ),
}

for label, annotation, initializer, yes, no in (
    ("boolean", "bool", "False", "1", "0"),
    ("integer", "Annotated[int, range(1 << 1)]", "0", "True", "False"),
):
    text = (
        "import pycircuit as ac\nfrom typing import Annotated\n@ac.struct\nclass Result:\n    value: ac.u1\n"
        "@ac.rule\ndef step(state, flag)->Result:\n"
        + f"    if flag:\n        state = {yes}\n    else:\n        state = {no}\n"
        + "    return Result(value=state)\n@ac.module\ndef Top(flag: ac.u1)->Result:\n"
        + f"    state: {annotation} = {initializer}\n    return step(state, flag)\n"
    )
    cases[label + "-owner-kind-crossing"] = (
        text,
        f"binding boundary requires declared {label.title()} source kind",
    )

partial = (
    "import pycircuit as ac\n@ac.struct\nclass Result:\n    value: ac.u13\n"
    "@ac.rule\ndef choose(data, flag)->Result:\n    if flag:\n        local = data\n"
    "    else:\n        pass\n    return Result(value=local)\n"
    "@ac.module\ndef Top(data: ac.u13,flag: ac.u1)->Result:\n    return choose(data, flag)\n"
)
cases["partial-local-read"] = (partial, "unknown hardware value 'local'")
branch_return = statement("flag").replace(
    "        state = state + 1", "        return Result(value=state)"
)
cases["branch-return"] = (branch_return, "rule return inside a branch is unsupported")
formal = (
    'import pycircuit as ac\n@ac.module\ndef Top(data: ac.u13, flag: ac.u1, *, default: int = 2)->{"out": ac.u13}:\n'
    '    return {"out": data if flag else default}\n'
)
cases["default-formal-peer"] = (formal, closed_guard)
controls = {
    "plain-boolean-if": statement("True"),
    "unused-partial-local": partial.replace(
        "Result(value=local)", "Result(value=data)"
    ),
    "partial-rebound": partial.replace(
        "    return Result(value=local)",
        "    local = data\n    return Result(value=local)",
    ),
    "total-new-local": partial.replace("        pass", "        local = data"),
    "closed-max-peer": scalar("data if flag else ((1 << 13) - 1)"),
    "closed-zero-peer": scalar("0 if flag else data"),
    "closed-wide-peer": scalar(
        "data if flag else ((1 << 65) - 1)",
        inputs="data: ac.bits[65], flag: ac.u1",
        output="ac.bits[65]",
    ),
    "default-formal-declaration": formal.replace("data if flag else default", "data"),
    "integer-input-declaration": scalar(
        "data", inputs="data: ac.u13, flag: Annotated[int, range(1 << 1)]"
    ),
    "boolean-input-declaration": scalar(
        "data", inputs="data: ac.u1, flag: ac.u1, boolean: bool", output="ac.u1"
    ),
}

# Declaration negatives reach binding authority, not a prior annotation parser failure.
bool_owner = (
    "import pycircuit as ac\n@ac.struct\nclass Result:\n    value: ac.u1\n"
    "@ac.rule\ndef step(state, flag)->Result:\n    if flag:\n        state = True\n    else:\n        state = False\n    return Result(value=state)\n"
    "@ac.module\ndef Top(flag: ac.u1)->Result:\n    state: bool = False\n    return step(state, flag)\n"
)
controls["boolean-owner-declaration"] = bool_owner
cases["boolean-owner-initializer"] = (
    bool_owner.replace("state: bool = False", "state: bool = 1"),
    "binding boundary requires declared Boolean source kind",
)
cases["boolean-owner-single-arm"] = (
    bool_owner.replace("state = True", "state = 1"),
    "binding boundary requires declared Boolean source kind",
)
cases["boolean-formal-kind-conflict"] = (
    bool_owner.replace(
        "import pycircuit as ac", "import pycircuit as ac\nfrom typing import Annotated"
    ).replace(
        "def step(state, flag)", "def step(state: Annotated[int, range(1 << 1)], flag)"
    ),
    "rule parameter annotation disagrees with binding source kind",
)
cases["owner-reannotation"] = (
    bool_owner.replace("state = True", "state: ac.u1 = 1"),
    "annotation cannot replace a declared binding boundary",
)
rule_local = (
    "import pycircuit as ac\nfrom typing import Annotated\n@ac.struct\nclass Result:\n    value: ac.u13\n"
    "@ac.rule\ndef step(flag)->Result:\n    BODY\n    return Result(value=local)\n"
    "@ac.module\ndef Top(flag: ac.u1)->Result:\n    return step(flag)\n"
)
controls["explicit-boolean-local-declaration"] = rule_local.replace(
    "value: ac.u13", "value: ac.u1"
).replace("BODY", "local: bool = False")
cases["explicit-boolean-local-kind"] = (
    rule_local.replace("value: ac.u13", "value: ac.u1").replace(
        "BODY", "local: bool = False\n    local = 1"
    ),
    "binding boundary requires declared Boolean source kind",
)
cases["explicit-fixed-local-overflow"] = (
    rule_local.replace("BODY", "local: ac.u1 = 0\n    local = 2"),
    bounds_guard,
)
cases["explicit-range-local-overflow"] = (
    rule_local.replace(
        "BODY", "local: Annotated[int, range(1 << 1)] = 0\n    local = 2"
    ),
    bounds_guard,
)
cases["explicit-local-reannotation"] = (
    rule_local.replace("BODY", "local: ac.u1 = 0\n    local: ac.u8 = 1"),
    "annotation cannot replace a declared binding boundary",
)
cases["branch-declaration-conflict"] = (
    rule_local.replace(
        "BODY",
        "if flag:\n        local: ac.u1 = 0\n    else:\n        local: ac.u2 = 0",
    ),
    "branch declarations have incompatible binding boundaries",
)
cases["partial-declaration-bound"] = (
    rule_local.replace(
        "BODY",
        "if flag:\n        local: ac.u1 = 0\n    else:\n        pass\n    local = 2",
    ),
    bounds_guard,
)
controls["partial-declaration-rebound"] = rule_local.replace(
    "BODY", "if flag:\n        local: ac.u1 = 0\n    else:\n        pass\n    local = 1"
)
controls["branch-declaration-adoption"] = rule_local.replace(
    "BODY", "if flag:\n        local: ac.u8 = 2\n    else:\n        local = 3"
)
controls["branch-declaration-adoption-reversed"] = rule_local.replace(
    "BODY", "if flag:\n        local = 3\n    else:\n        local: ac.u8 = 2"
)


formal_capture = (
    "import pycircuit as ac\nfrom typing import Annotated\n@ac.struct\nclass Result:\n    value: ac.u8\n"
    "@ac.rule\ndef step(value)->Result:\n    BODY\n    return Result(value=value + 2)\n"
    "@ac.module\ndef Top()->Result:\n    actual = ACTUAL\n    return step(actual)\n"
)
controls["negative-formal-declaration"] = formal_capture.replace(
    "BODY", "pass"
).replace("ACTUAL", "0 - 1")
controls["explicit-range-formal-declaration"] = (
    formal_capture.replace(
        "def step(value)", "def step(value: Annotated[int, range(1 << 1)])"
    )
    .replace("BODY", "pass")
    .replace("ACTUAL", "1")
)
cases["representation-formal-reassign"] = (
    formal_capture.replace("BODY", "value = 2").replace("ACTUAL", "0 - 1"),
    "Integer representation binding requires original hardware type",
)
cases["explicit-range-negative-capture"] = (
    formal_capture.replace(
        "def step(value)", "def step(value: Annotated[int, range(1 << 1)])"
    )
    .replace("BODY", "pass")
    .replace("ACTUAL", "0 - 1"),
    bounds_guard,
)
cases["representation-range-reannotation"] = (
    formal_capture.replace("BODY", "value: Annotated[int, range(1 << 1)] = 0").replace(
        "ACTUAL", "0 - 1"
    ),
    "annotation cannot replace a declared binding boundary",
)

protected = {p: snapshot(p) if p.is_dir() else p.read_bytes() for p in protected_paths}
for name, text in controls.items():
    design.write_text(text)
    (evidence / ("control-" + name + ".py")).write_text(text)
    compile_source(build / ("control-" + name))
for name, (text, diagnostic) in cases.items():
    design.write_text(text)
    (evidence / (name + ".py")).write_text(text)
    absent = build / ("invalid-" + name)
    result = compile_source(absent, code=1)
    assert diagnostic in result.stderr, (name, result.stderr)
    assert not absent.exists()
    result = compile_source(unit, code=1, replace=True)
    assert diagnostic in result.stderr, (name, result.stderr)
    assert all(
        (snapshot(p) if p.is_dir() else p.read_bytes()) == original
        for p, original in protected.items()
    )
fixture_paths = [
    fixtures / ("frontend-branches" + extension) for extension in (".py", ".cpp", ".sv")
]
fixture_paths.append(fixtures.parent / "frontend-branches.test")
(evidence / "candidate.json").write_text(
    json.dumps(
        {
            "artifact_directory": str(build),
            "fixtures": {str(p.relative_to(repo)): digest(p) for p in fixture_paths},
            "design_sha256": hashlib.sha256(DESIGN.encode()).hexdigest(),
            "products": receipts,
            "native_workers": [1, 2],
            "icarus": True,
            "state_probe_actions": len(probe_rows),
            "native_failure_recovery_cases": len(failure_rows),
            "rejected_cases": sorted(cases),
            "protected_rejections": 2 * len(cases),
            "positive_controls": sorted(controls),
            "public_guard_gap": "missing-origin predicate cannot be constructed by these admitted source forms; no earlier rejection counted as that private guard",
        },
        indent=2,
    )
    + "\n"
)
print(
    f"frontend branch gate passed: {len(com_rows)} combinational/{len(state_rows)} state epochs; native1/2, Verilator, Icarus; {len(probe_rows)} discard/retry actions, {len(failure_rows)} failure/recoveries; {len(cases) * 2} protected rejections"
)  # noqa: T201
