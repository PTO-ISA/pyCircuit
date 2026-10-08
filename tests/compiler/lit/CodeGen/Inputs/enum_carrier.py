"""Execute nominal enum common IR with independent arithmetic/symbol oracles.

No Python enum syntax, source-unit publication or F3 provider claim is made.
"""

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
    "opt",
    "emitter",
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
evidence = Path(args.scratch).resolve()
evidence.mkdir(parents=True, exist_ok=True)
build = Path(tempfile.mkdtemp(prefix="run-", dir=evidence))
commands = []
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python/pycircuit/src"),
    PYCIRCUIT_EMITTER=args.emitter,
    PYTHONDONTWRITEBYTECODE="1",
)


def run(command, code=0):
    command = list(map(str, command))
    result = subprocess.run(
        command, env=env, cwd=repo, text=True, capture_output=True, timeout=240
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
    assert "Assertion failed" not in result.stderr, commands[-1]
    return result


def bits(value, width):
    return format(value % (1 << width), f"0{width}b")


def pattern(code, width):
    result = []
    for _ in range(width):
        result.append("01xz"[code % 4])
        code //= 4
    return "".join(reversed(result))


def selected(control, yes, no):
    if control == "1":
        return yes
    if control == "0":
        return no
    return "".join(a if a == b else "x" for a, b in zip(yes, no, strict=True))


def membership(raw, codes):
    equalities = []
    for code in codes:
        word = bits(code, len(raw))
        if any(a in "01" and a != b for a, b in zip(raw, word, strict=True)):
            equalities.append("0")
        elif all(a in "01" for a in raw):
            equalities.append("1")
        else:
            equalities.append("x")
    return (
        "1" if "1" in equalities else "0" if all(x == "0" for x in equalities) else "x"
    )


ENUMS = {
    "one": ("suite.One", 1, [1], "explicit"),
    "left": ("left.State", 2, [0, 1], "explicit"),
    "right": ("right.State", 2, [0, 1], "explicit"),
    "sparse": ("suite.Sparse", 3, [0, 3, 5], "explicit"),
    "sequential": ("suite.Sequential", 3, [0, 1, 2], "binary_sequential"),
    "hot": ("suite.Hot", 3, [1, 2, 4], "binary_one_hot"),
    "gray": ("suite.Gray", 3, [0, 1, 3], "gray_sequential"),
    "many": ("suite.Many", 11, list(range(1025)), "explicit"),
    "wide": ("suite.Wide", 65, [0, 1, (1 << 64) + 3, (1 << 65) - 1], "explicit"),
    "huge": ("suite.Huge", 130, [0, 5, (1 << 100) + 7, (1 << 130) - 1], "explicit"),
}


def occurrence(owner):
    return f"{{site = {{definition = @{owner}, ast_path = []}}, expansion = []}}"


def static(value):
    return (
        '#ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, '
        f'origin = {occurrence("top")}, value = {{kind = "integer", value = #ac.math_int<{value}>}}'
        + "}>"
    )


WIDTHS = (0, 1, 2, 3, 11, 13, 65, 130)
PREAMBLE = (
    "\n".join(
        f"#w{w} = {static(w)}\n!b{w} = !ac.bits<#w{w}>" if w else f"#w0 = {static(0)}"
        for w in WIDTHS
    )
    + "\n"
)
for key, (name, _, _, _) in ENUMS.items():
    PREAMBLE += f'!e_{key} = !ac.enum<"{name}">\n'
DECLS = []
for _, (name, width, codes, encoding) in ENUMS.items():
    members = ", ".join(
        f'{{name = "M{i}", code = #ac.math_int<{code}>}}'
        for i, code in enumerate(codes)
    )
    DECLS.append(
        f'"ac.enum"() {{sym_name = "{name}", width = #ac.math_int<{width}>, encoding = "{encoding}", members = [{members}]}} : () -> ()'
    )
DECLS += [
    'ac.struct "Inner" fields [{name = "state", type = !e_left}, {name = "token", type = !e_wide}, {name = "guard", type = !b1}]',
    'ac.struct "Outer" fields [{name = "inner", type = !ac.struct<"Inner">}, {name = "large", type = !e_huge}, {name = "tag", type = !b3}]',
]


def module(name, inputs, outputs, body, type_parameters=()):
    args = ", ".join(f"%{n}: {t}" for n, t in inputs)
    sig = (
        "("
        + ", ".join(t for _, t in inputs)
        + ") -> ("
        + ", ".join(t for _, t in outputs)
        + ")"
    )

    def names(rows):
        return ", ".join('"' + n + '"' for n, _ in rows)

    # Distinct allocation sites belong to this module's actual body positions.
    body = [
        line.replace(
            "occurrence = " + occurrence("top"),
            f'occurrence = {{site = {{definition = @{name}, ast_path = [{{kind = "field", name = "body"}}, {{kind = "index", value = {index} : i64}}]}}, expansion = []}}',
        )
        for index, line in enumerate(body)
    ]
    types = ", ".join('"' + t + '"' for t in type_parameters)
    return (
        '"ac.module"() ({\n^bb0('
        + args
        + "):\n"
        + "\n".join("  " + x for x in body)
        + '\n}) {sym_name = "'
        + name
        + '", source_owner = {package = "", path = "'
        + name
        + '.py"}, parameters = [], '
        f"type_parameters = [{types}], function_type = {sig}, input_names = [{names(inputs)}], output_names = [{names(outputs)}]"
        + "} : () -> ()"
    )


def yield_values(rows):
    return (
        '"ac.yield"('
        + ", ".join(v for v, _ in rows)
        + ") : ("
        + ", ".join(t for _, t in rows)
        + ") -> ()"
    )


def instance(name, callee, args, types, results, actuals=(), collection=False):
    attr = (
        f'instance_name = "{name}", callee = @{callee}, parameters = [], type_arguments = ['
        + ", ".join(actuals)
        + "]"
    )
    if collection:
        attr += ", shape = [#w2]"
    attr += ", occurrence = " + occurrence("top")
    return (
        '"ac.'
        + ("collection" if collection else "instance")
        + '"('
        + ", ".join(args)
        + ") {"
        + attr
        + "} : ("
        + ", ".join(types)
        + ") -> ("
        + ", ".join(results)
        + ")"
    )


T = '!ac.type_param<@identity, "T">'
U = '!ac.type_param<@forward, "U">'
F = '!ac.type_param<@family, "F">'
GENERIC = [
    module(
        "identity",
        [("value", T)],
        [("out", T)],
        [yield_values([("%value", T)])],
        ("T",),
    ),
    module(
        "forward",
        [("value", U)],
        [("out", U)],
        [
            "%out = " + instance("child", "identity", ["%value"], [U], [U], [U]),
            yield_values([("%out", U)]),
        ],
        ("U",),
    ),
    module(
        "family",
        [("value", f"!ac.table<[#w2], {F}>")],
        [("out", f"!ac.table<[#w2], {F}>")],
        [
            "%out = "
            + instance(
                "lanes",
                "forward",
                ["%value"],
                [f"!ac.table<[#w2], {F}>"],
                [f"!ac.table<[#w2], {F}>"],
                [F],
                True,
            ),
            yield_values([("%out", f"!ac.table<[#w2], {F}>")]),
        ],
        ("F",),
    ),
]
# The same verified standard kernel underlies direct, generic and collection owners.
STORAGE = (
    '"ac.module.import"() {sym_name = "storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], '
    'function_type = (!b1, !b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, '
    'input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()'
)
C = '!ac.type_param<@cell, "T">'
CELL = module(
    "cell",
    [("clk", "!b1"), ("rst", "!b1"), ("en", "!b1"), ("d", C), ("init", C)],
    [("q", C)],
    [
        "%q = "
        + instance(
            "state",
            "storage",
            ["%clk", "%rst", "%en", "%d", "%init"],
            ["!b1", "!b1", "!b1", C, C],
            [C],
            [C],
        ),
        yield_values([("%q", C)]),
    ],
    ("T",),
)


def enum_from(body, stem, raw, key):
    width = ENUMS[key][1]
    body.append(
        f'%{stem}, %{stem}_member = "ac.enum.from_bits"({raw}) : (!b{width}) -> (!e_{key}, !b1)'
    )
    return "%" + stem, "%" + stem + "_member"


def enum_bits(body, stem, value, key):
    width = ENUMS[key][1]
    body.append(f'%{stem} = "ac.enum.to_bits"({value}) : (!e_{key}) -> !b{width}')
    return "%" + stem


def enum_create(body, stem, key, index):
    body.append(
        f'%{stem} = "ac.enum.create"() {{member = "M{index}"}} : () -> !e_{key}'
    )
    return "%" + stem


def build_com():
    inputs = [
        ("r1", "!b1"),
        ("r2", "!b2"),
        ("r2b", "!b2"),
        ("r3", "!b3"),
        ("r11", "!b11"),
        ("r65", "!b65"),
        ("r130", "!b130"),
        ("select", "!b1"),
    ]
    body = []
    out = []
    spec = []

    def output(name, value, type_, width, oracle):
        out.append((name, value, type_))
        spec.append((name, width, oracle))

    for key in ENUMS:
        width = ENUMS[key][1]
        raw = (
            "%r2"
            if key == "left"
            else "%r2b" if key == "right" else "%r3" if width == 3 else f"%r{width}"
        )
        value, member = enum_from(body, "decoded_" + key, raw, key)
        roundtrip = enum_bits(body, "round_" + key, value, key)
        input_name = raw[1:]
        output(
            "carrier_" + key,
            value,
            "!e_" + key,
            width,
            lambda row, n=input_name: row[n],
        )
        output(
            "round_" + key,
            roundtrip,
            "!b" + str(width),
            width,
            lambda row, n=input_name: row[n],
        )
        output(
            "member_" + key,
            member,
            "!b1",
            1,
            lambda row, n=input_name, codes=ENUMS[key][2]: membership(row[n], codes),
        )
    other, _ = enum_from(body, "other_left", "%r2b", "left")
    array_type = "!ac.table<[#w2], !e_left>"
    body.append(
        f'%pair = "ac.table.create"(%decoded_left, {other}) : (!e_left, !e_left) -> {array_type}'
    )
    body.append(
        "%family = "
        + instance(
            "generic", "family", ["%pair"], [array_type], [array_type], ["!e_left"]
        )
    )
    output("family", "%family", array_type, 4, lambda row: row["r2"] + row["r2b"])
    body.append(
        "%forwarded = "
        + instance(
            "generic_wide",
            "forward",
            ["%decoded_wide"],
            ["!e_wide"],
            ["!e_wide"],
            ["!e_wide"],
        )
    )
    output("forwarded", "%forwarded", "!e_wide", 65, lambda row: row["r65"])
    # Both enum identities also instantiate the same template definitions.
    body.append(
        "%forward_right = "
        + instance(
            "generic_right",
            "forward",
            ["%decoded_right"],
            ["!e_right"],
            ["!e_right"],
            ["!e_right"],
        )
    )
    output("forward_right", "%forward_right", "!e_right", 2, lambda row: row["r2b"])
    raw_left = enum_bits(body, "bridge_bits", "%decoded_left", "left")
    bridged, _ = enum_from(body, "bridged", raw_left, "right")
    output("bridged", bridged, "!e_right", 2, lambda row: row["r2"])
    body.append(
        '%inner = "ac.struct.create"(%decoded_left, %decoded_wide, %select) : (!e_left, !e_wide, !b1) -> !ac.struct<"Inner">'
    )
    body.append(
        '%outer = "ac.struct.create"(%inner, %decoded_huge, %r3) : (!ac.struct<"Inner">, !e_huge, !b3) -> !ac.struct<"Outer">'
    )

    def nested(row):
        return row["r2"] + row["r65"] + row["select"] + row["r130"] + row["r3"]

    def nested_other(row):
        return row["r2b"] + row["r65"] + row["select"] + row["r130"] + row["r3"]

    output("nested", "%outer", '!ac.struct<"Outer">', 201, nested)
    body.append(
        f'%inner_other = "ac.struct.create"({other}, %decoded_wide, %select) : (!e_left, !e_wide, !b1) -> !ac.struct<"Inner">'
    )
    body.append(
        '%outer_other = "ac.struct.create"(%inner_other, %decoded_huge, %r3) : (!ac.struct<"Inner">, !e_huge, !b3) -> !ac.struct<"Outer">'
    )
    records_type = '!ac.table<[#w2], !ac.struct<"Outer">>'
    body.append(
        f'%records = "ac.table.create"(%outer, %outer_other) : (!ac.struct<"Outer">, !ac.struct<"Outer">) -> {records_type}'
    )
    body.append(
        "%forward_records = "
        + instance(
            "records_generic",
            "family",
            ["%records"],
            [records_type],
            [records_type],
            ['!ac.struct<"Outer">'],
        )
    )
    output(
        "records",
        "%forward_records",
        records_type,
        402,
        lambda row: nested(row) + nested_other(row),
    )
    max65 = enum_create(body, "max65", "wide", 3)
    max65bits = enum_bits(body, "max65_bits", max65, "wide")
    body.append(
        f'%selected_bits = "ac.bits.select"(%select, %round_wide, {max65bits}) : (!b1, !b65, !b65) -> !b65'
    )
    selected65, _ = enum_from(body, "selected", "%selected_bits", "wide")
    output(
        "selected",
        selected65,
        "!e_wide",
        65,
        lambda row: selected(row["select"], row["r65"], "1" * 65),
    )
    max130 = enum_create(body, "max130", "huge", 3)
    body.append(
        f'%merged, %merge_enable = "ac.value.merge"(%decoded_huge, %select, {max130}) {{paths = [[]], operandSegmentSizes = array<i32: 1, 1, 1>}} : (!e_huge, !b1, !e_huge) -> (!e_huge, !b1)'
    )
    output(
        "merged",
        "%merged",
        "!e_huge",
        130,
        lambda row: selected(row["select"], "1" * 130, row["r130"]),
    )
    output(
        "merge_enable",
        "%merge_enable",
        "!b1",
        1,
        lambda row: row["select"] if row["select"] in "01" else "x",
    )
    for key in ("wide", "huge"):
        value = enum_create(body, "created_" + key, key, 2)
        width = ENUMS[key][1]
        output(
            "created_" + key,
            value,
            "!e_" + key,
            width,
            lambda row, k=key: bits(ENUMS[k][2][2], ENUMS[k][1]),
        )
    # Separate modules exercise each from_bits result without requiring the other.
    value, member = enum_from(body, "single_carrier", "%r65", "wide")
    output("carrier_only", value, "!e_wide", 65, lambda row: row["r65"])
    value, member = enum_from(body, "single_member", "%r130", "huge")
    output(
        "member_only",
        member,
        "!b1",
        1,
        lambda row: membership(row["r130"], ENUMS["huge"][2]),
    )
    body.append(yield_values([(v, t) for _, v, t in out]))
    ir = (
        PREAMBLE
        + "module {\n"
        + "\n".join(
            DECLS + GENERIC + [module("top", inputs, [(n, t) for n, _, t in out], body)]
        )
        + '\n"ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()\n}\n'
    )
    return ir, {n: int(t[2:]) for n, t in inputs}, spec


def build_storage():
    names = {
        "clk": 1,
        "rst": 1,
        "ena": 1,
        "enb": 1,
        "enc": 1,
        "en0": 1,
        "en1": 1,
        "r2": 2,
        "r2b": 2,
        "r65": 65,
        "r130": 130,
        "data": 13,
    }
    inputs = [(n, "!b" + str(w)) for n, w in names.items()]
    body = []
    ea, _ = enum_from(body, "a", "%r65", "wide")
    eb, _ = enum_from(body, "b", "%r130", "huge")
    e0, _ = enum_from(body, "lane0", "%r2", "left")
    e1, _ = enum_from(body, "lane1", "%r2b", "left")
    ia = enum_create(body, "ia", "wide", 1)
    ib = enum_create(body, "ib", "huge", 1)
    iz = enum_create(body, "iz", "left", 0)
    io = enum_create(body, "io", "left", 1)
    body.append(
        "%qa = "
        + instance(
            "a_owner",
            "cell",
            ["%clk", "%rst", "%ena", ea, ia],
            ["!b1", "!b1", "!b1", "!e_wide", "!e_wide"],
            ["!e_wide"],
            ["!e_wide"],
        )
    )
    body.append(
        "%qb = "
        + instance(
            "b_owner",
            "cell",
            ["%clk", "%rst", "%enb", eb, ib],
            ["!b1", "!b1", "!b1", "!e_huge", "!e_huge"],
            ["!e_huge"],
            ["!e_huge"],
        )
    )
    body.append('%zero = "ac.bits.constant"() {value = #w0} : () -> !b13')
    body.append(
        "%sibling = "
        + instance(
            "register",
            "storage",
            ["%clk", "%rst", "%enc", "%data", "%zero"],
            ["!b1", "!b1", "!b1", "!b13", "!b13"],
            ["!b13"],
            ["!b13"],
        )
    )
    et = "!ac.table<[#w2], !e_left>"
    gt = "!ac.table<[#w2], !b1>"
    for stem, value in (("clocks", "%clk"), ("resets", "%rst")):
        body.append(
            f'%{stem} = "ac.table.splat"({value}) {{shape = [#w2]}} : (!b1) -> {gt}'
        )
    body.append(f'%enables = "ac.table.create"(%en0, %en1) : (!b1, !b1) -> {gt}')
    body.append(
        f'%data_pair = "ac.table.create"({e0}, {e1}) : (!e_left, !e_left) -> {et}'
    )
    body.append(
        f'%init_pair = "ac.table.create"({iz}, {io}) : (!e_left, !e_left) -> {et}'
    )
    body.append(
        "%lanes = "
        + instance(
            "family_owners",
            "cell",
            ["%clocks", "%resets", "%enables", "%data_pair", "%init_pair"],
            [gt, gt, gt, et, et],
            [et],
            ["!e_left"],
            True,
        )
    )
    rawa = enum_bits(body, "rawa", "%qa", "wide")
    rawb = enum_bits(body, "rawb", "%qb", "huge")
    _, ma = enum_from(body, "checked_a", rawa, "wide")
    _, mb = enum_from(body, "checked_b", rawb, "huge")
    out = [
        ("qa", "%qa", "!e_wide"),
        ("qb", "%qb", "!e_huge"),
        ("sibling", "%sibling", "!b13"),
        ("lanes", "%lanes", et),
        ("ma", ma, "!b1"),
        ("mb", mb, "!b1"),
    ]
    body.append(yield_values([(v, t) for _, v, t in out]))
    ir = (
        PREAMBLE
        + "module {\n"
        + "\n".join(
            DECLS
            + [STORAGE, CELL, module("top", inputs, [(n, t) for n, _, t in out], body)]
        )
        + '\n"ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()\n}\n'
    )
    return (
        ir,
        names,
        [("qa", 65), ("qb", 130), ("sibling", 13), ("lanes", 4), ("ma", 1), ("mb", 1)],
    )


def wide_pattern(index, width, codes):
    known = [0, 1, 3, 5, codes[2], (1 << width) - 1, 1 << (width - 1), (1 << 64) - 1]
    if index < 8:
        return bits(known[index], width)
    if index == 8:
        return "x" * width
    if index == 9:
        return "z" * width
    raw = list(bits((1 << (width - 1)) + 7, width))
    for position, symbol in ((0, "x"), (width - 1, "z"), (63, "z"), (64, "x")):
        raw[width - position - 1] = (
            symbol if index % 2 else ("z" if symbol == "x" else "x")
        )
    return "".join(raw)


COM_IR, COM_INPUTS, COM_SPEC = build_com()
com_rows = []
for code in range(64):
    row = {
        "r1": pattern(code % 4, 1),
        "r2": pattern(code % 16, 2),
        "r2b": pattern((code * 5 + 1) % 16, 2),
        "r3": pattern(code, 3),
        "r11": (
            [
                bits(0, 11),
                bits(1, 11),
                bits(1024, 11),
                bits(1025, 11),
                bits(2047, 11),
                "0000000000x",
                "0000000000z",
                "11xxxxxxxxx",
                "11zzzzzzzzz",
                "x0000000000",
                "z0000000000",
                "xxxxxxxxxxx",
                "zzzzzzzzzzz",
            ]
        )[code % 13],
        "r65": wide_pattern(code % 16, 65, ENUMS["wide"][2]),
        "r130": wide_pattern((code + 3) % 16, 130, ENUMS["huge"][2]),
        "select": "01xz"[code % 4],
    }
    com_rows.append(row)
    if any(s in "xz" for text in row.values() for s in text):
        com_rows.append(
            {
                "r1": "1",
                "r2": "01",
                "r2b": "10",
                "r3": "101",
                "r11": bits(1024, 11),
                "r65": bits(1, 65),
                "r130": bits(5, 130),
                "select": "1",
            }
        )
com_gold = [{name: oracle(row) for name, _, oracle in COM_SPEC} for row in com_rows]


STATE_IR, STATE_INPUTS, STATE_SPEC = build_storage()


def state_row(clk="0", rst="0", ena="0", enb="0", enc="0", en0="0", en1="0", index=0):
    return {
        "clk": clk,
        "rst": rst,
        "ena": ena,
        "enb": enb,
        "enc": enc,
        "en0": en0,
        "en1": en1,
        "r2": pattern(index % 16, 2),
        "r2b": pattern((index + 7) % 16, 2),
        "r65": wide_pattern(index % 16, 65, ENUMS["wide"][2]),
        "r130": wide_pattern((index + 1) % 16, 130, ENUMS["huge"][2]),
        "data": bits(index * 31 + 3, 13),
    }


class StorageOracle:
    def __init__(self):
        self.qa, self.qb, self.sibling, self.lanes, self.clock = (
            bits(1, 65),
            bits(5, 130),
            bits(0, 13),
            "0001",
            "0",
        )

    def step(self, row, action=0):
        golden = {
            "qa": self.qa,
            "qb": self.qb,
            "sibling": self.sibling,
            "lanes": self.lanes,
            "ma": membership(self.qa, ENUMS["wide"][2]),
            "mb": membership(self.qb, ENUMS["huge"][2]),
        }
        rising = self.clock == "0" and row["clk"] == "1"
        failure = row["clk"] in "xz" or (
            rising
            and (
                row["rst"] in "xz"
                or (
                    row["rst"] == "0"
                    and any(row[e] in "xz" for e in ("ena", "enb", "enc", "en0", "en1"))
                )
            )
        )
        assert failure == (action == 2)
        if action == 0:
            if rising:
                if row["rst"] == "1":
                    self.qa, self.qb, self.sibling, self.lanes = (
                        bits(1, 65),
                        bits(5, 130),
                        bits(0, 13),
                        "0001",
                    )
                else:
                    if row["ena"] == "1":
                        self.qa = row["r65"]
                    if row["enb"] == "1":
                        self.qb = row["r130"]
                    if row["enc"] == "1":
                        self.sibling = row["data"]
                    self.lanes = (
                        row["r2"] if row["en0"] == "1" else self.lanes[:2]
                    ) + (row["r2b"] if row["en1"] == "1" else self.lanes[2:])
            self.clock = row["clk"]
        return golden


state_rows = []
for index in range(8):
    row = state_row(
        ena=str(index % 2),
        enb=str((index // 2) % 2),
        enc="1",
        en0="1",
        en1=str(index % 2),
        index=index,
    )
    # Known prefix remains independent for two-state Verilator.
    row["r2"], row["r2b"] = bits(index % 4, 2), bits((index + 1) % 4, 2)
    row["r130"] = wide_pattern((index + 1) % 8, 130, ENUMS["huge"][2])
    state_rows.extend([row, row | {"clk": "1"}])
state_rows += [
    state_row("1"),
    state_row("1", "1"),
    state_row("0", "1"),
    state_row("1", "1"),
    state_row(),
]
state_known_prefix = len(state_rows)
for index in (8, 9, 10, 11):
    row = state_row(ena="1", enb="1", enc="1", en0="1", en1="1", index=index)
    state_rows += [
        row,
        row | {"clk": "1"},
        row,
        state_row("1", "1", "x", "z", "x", "z", "x"),
        state_row(),
    ]
state_oracle = StorageOracle()
state_gold = [state_oracle.step(row) for row in state_rows]
probe_rows = [
    (state_row(), 0),
    (state_row("1", ena="1", enb="1", enc="1", en0="1", en1="1", index=3), 0),
    (state_row(), 0),
]
proposal = state_row("1", ena="1", enb="1", enc="1", en0="1", en1="1", index=6)
probe_rows += [(proposal, 1), (proposal, 0), (state_row(), 0)]
failure_rows = []
for control in ("ena", "enb", "enc", "en0", "en1", "clk", "rst"):
    for symbol in "xz":
        row = state_row("1", ena="1", enb="1", enc="1", en0="1", en1="1", index=5)
        row[control] = symbol
        failure_rows.append(row)
        probe_rows += [(row, 2), (proposal, 0), (state_row(), 0)]
probe_oracle = StorageOracle()
probe_gold = [probe_oracle.step(row, action) for row, action in probe_rows]


def vectors(directory, inputs, spec, rows, gold, storage=False):
    cpp = [
        f"constexpr unsigned row_count={len(rows)};",
        "const std::string_view golden_trace[]={",
        *[f'"{" ".join(g.values())}",' for g in gold],
        "};",
    ]
    types = {name: width for name, width, *_ in spec}

    def input_arrays(label, data):
        out = []
        for name in inputs:
            out += [
                f"const std::string_view {label}_{name}[]={{",
                *[f'"{r[name]}",' for r in data],
                "};",
            ]
        return out

    def drive_cpp(name, label):
        return [
            f"void {name}(pyc_dut::Inputs &p,unsigned row){{",
            *[f"p.{n}=input<{w}>({label}_{n}[row]);" for n, w in inputs.items()],
            "}",
        ]

    def check_cpp(name, label, data, templated=False):
        out = []
        for n in types:
            out += [
                f"const std::string_view {label}_{n}[]={{",
                *[f'"{g[n]}",' for g in data],
                "};",
            ]
        out += (["template<class Outputs>"] if templated else []) + [
            f"void {name}(const {'Outputs' if templated else 'pyc_dut::Outputs'} &o,unsigned row){{",
            *[f"expected<{w}>(o.{n},{label}_{n}[row]);" for n, w in types.items()],
        ]
        if storage:
            out += [
                f"samePlanes(o.qa,input<65>({label}_qa[row]));",
                f"samePlanes(o.qb,input<130>({label}_qb[row]));",
                f"samePlanes(o.lanes.element(0),input<2>({label}_lanes[row].substr(0,2)));",
                f"samePlanes(o.lanes.element(1),input<2>({label}_lanes[row].substr(2,2)));",
            ]
        else:
            out += ["pyc_dut::Inputs p;drive(p,row);"]
            for key in ENUMS:
                width = ENUMS[key][1]
                raw = (
                    "r2"
                    if key == "left"
                    else (
                        "r2b" if key == "right" else "r3" if width == 3 else f"r{width}"
                    )
                )
                out += [
                    f"samePlanes(o.carrier_{key},p.{raw});",
                    f"samePlanes(o.round_{key},p.{raw});",
                ]
        return out + ["}"]

    cpp += (
        input_arrays("normal", rows)
        + drive_cpp("drive", "normal")
        + check_cpp("check", "gold", gold)
    )
    if storage:
        cpp += [
            f"constexpr unsigned probe_count={len(probe_rows)},failure_count={len(failure_rows)};",
            "const unsigned probe_action[]={"
            + ",".join(str(a) for _, a in probe_rows)
            + "};",
        ]
        cpp += (
            input_arrays("probe", [r for r, _ in probe_rows])
            + drive_cpp("driveProbe", "probe")
            + check_cpp("checkProbe", "probe_expected", probe_gold, True)
        )
        cpp += input_arrays("failure", failure_rows) + drive_cpp(
            "driveFailure", "failure"
        )
        cpp += [
            "void driveRoot(pyc_root &root,const pyc_dut::Inputs &p){",
            *[f"root.{n}=p.{n};" for n in inputs],
            "}",
            "void cold(const pyc_root &root){require(!root.qa.isFullyKnown()&&!root.qb.isFullyKnown()&&!root.lanes.isFullyKnown());}",
        ]
    else:
        cpp += [
            "using Left=typename std::remove_cvref_t<decltype(std::declval<pyc_dut::Outputs>().carrier_left)>::value_type;",
            "using Right=typename std::remove_cvref_t<decltype(std::declval<pyc_dut::Outputs>().carrier_right)>::value_type;",
            "static_assert(!std::is_same_v<Left,Right>);",
            "static_assert(gfsim::hardware_traits<Left>::width==2&&gfsim::hardware_traits<Right>::width==2);",
        ]
    (directory / "enum_vectors.hpp").write_text("\n".join(cpp) + "\n")
    sv = [
        f"localparam integer row_count={len(rows)};",
        *[f"logic[{w - 1}:0] {n}=0;" for n, w in inputs.items()],
        *[f"wire[{w - 1}:0] {n};" for n, w in types.items()],
    ]
    if storage:
        sv += ["logic next_clock;"]
    sv += ["task drive(input integer row);case(row)"]
    for i, row in enumerate(rows):
        sv += [
            f"{i}:begin",
            *[
                f"{'next_clock' if storage and n == 'clk' else n}={inputs[n]}'b{v};"
                for n, v in row.items()
            ],
            "end",
        ]
    sv += ["endcase endtask", "task check(input integer row);case(row)"]
    for i, g in enumerate(gold):
        sv += [
            f"{i}:begin",
            *[
                f'if({n} !== {types[n]}\'b{value})$fatal(1,"enum carrier/membership/storage {n} row{i}");'
                for n, value in g.items()
            ],
            "end",
        ]
    sv += [
        "endcase endtask",
        "function automatic string golden_trace(input integer row);case(row)",
    ]
    sv += [f'{i}:golden_trace="{" ".join(g.values())}";' for i, g in enumerate(gold)]
    sv += [
        'default:golden_trace="invalid";endcase endfunction',
        "function automatic bit known_row(input integer row);case(row)",
    ]
    known = [
        (
            i < state_known_prefix
            if storage
            else all(c in "01" for s in r.values() for c in s)
        )
        for i, r in enumerate(rows)
    ]
    sv += [f"{i}:known_row={int(k)};" for i, k in enumerate(known)] + [
        "default:known_row=0;endcase endfunction"
    ]
    if storage:
        sv += [
            "task drive_failure;clk=0;rst=0;ena=1;enb=1'bx;enc=1;en0=1;en1=1;endtask"
        ]
    (directory / "enum_vectors.svh").write_text("\n".join(sv) + "\n")
    return known


receipts = []
for name, ir, inputs, spec, rows, gold, storage in (
    ("carrier", COM_IR, COM_INPUTS, COM_SPEC, com_rows, com_gold, False),
    ("storage", STATE_IR, STATE_INPUTS, STATE_SPEC, state_rows, state_gold, True),
):
    output = build / name
    output.mkdir()
    original = output / "fixture.mlir"
    original.write_text(ir)
    verified = output / "verified.mlir"
    run([args.opt, original, "--ac-verify-hardware", "-o", verified])
    known = vectors(output, inputs, spec, rows, gold, storage)
    payloads = {}
    for target in ("cpp", "verilog"):
        payload = run([args.emitter, verified, "--target", target]).stdout
        (output / (target + ".json")).write_text(payload)
        payloads[target] = json.loads(payload)
        run(
            [
                sys.executable,
                repo / "tests/compiler/lit/CodeGen/Inputs/unpack_bundle.py",
                output / (target + ".json"),
                output / target,
            ]
        )
    runtime_root = Path(args.emitter).resolve().parent.parent
    runtime = next(
        p
        for p in (
            runtime_root / "lib/libpyc6_runtime.a",
            runtime_root / "simulator/gfsim/libpyc6_runtime.a",
        )
        if p.is_file()
    )
    defines = ["-DENUM_STORAGE"] if storage else []
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
            fixtures / "enum_carrier.cpp",
            "@" + str(output / "cpp/sources.rsp"),
            runtime,
            "-o",
            runner,
        ]
    )
    config = output / "config.json"
    config.write_text(
        '{"deadlock_window":null,"max_domain_cycles":{},"max_ticks":256,"schema":"pycircuit-model-config","version":"1"}\n'
    )
    traces = []
    for workers in (1, 2):
        trace = run([runner, "--workers", workers, "--config", config]).stdout
        (output / f"workers-{workers}.stdout").write_text(trace)
        traces.append([s for s in trace.splitlines() if s.startswith("WORK ")])
    assert traces[0] == traces[1] and len(traces[0]) == len(rows)
    rtl = [
        output / "verilog/design_top.sv",
        *[
            output / "verilog" / g["path"]
            for g in payloads["verilog"]["rtl_source_groups"]
        ],
    ]
    primitives = [repo / path for path in payloads["verilog"]["rtl_standard_sources"]]
    run(
        [
            args.verilator,
            "--binary",
            "--timing",
            "--top-module",
            "tb",
            "--prefix",
            "Venum",
            "--Mdir",
            output / "rtl-build",
            "-j",
            "2",
            "-Wno-fatal",
            *defines,
            "-I" + str(output),
            *primitives,
            *rtl,
            fixtures / "enum_carrier.sv",
        ]
    )
    observed = run([output / "rtl-build/Venum"]).stdout
    assert [s for s in observed.splitlines() if s.startswith("WORK ")] == [
        s for s, k in zip(traces[0], known, strict=True) if k
    ]
    run(
        [
            args.iverilog,
            "-g2012",
            "-DENUM_FOUR_STATE",
            *defines,
            "-I" + str(output),
            "-s",
            "tb",
            "-o",
            output / "four.vvp",
            *primitives,
            *rtl,
            fixtures / "enum_carrier.sv",
        ]
    )
    observed = run([args.vvp, output / "four.vvp"]).stdout
    assert [s for s in observed.splitlines() if s.startswith("WORK ")] == traces[0]
    if storage:
        run(
            [
                args.iverilog,
                "-g2012",
                "-DENUM_FOUR_STATE",
                "-DENUM_FAILURE",
                *defines,
                "-I" + str(output),
                "-s",
                "tb",
                "-o",
                output / "failure.vvp",
                *primitives,
                *rtl,
                fixtures / "enum_carrier.sv",
            ]
        )
        failed = run([args.vvp, output / "failure.vvp"], 1)
        assert "enable must be known" in failed.stdout + failed.stderr
    # Native bad-IR rejection cannot publish JSON; public source-unit --replace is F3.
    previous = {
        str(p): p.read_bytes()
        for target in ("cpp", "verilog")
        for p in (output / target).rglob("*")
        if p.is_file()
    }
    bad = output / "bad-width.mlir"
    changed = ir.replace(
        'sym_name = "suite.Wide", width = #ac.math_int<65>',
        'sym_name = "suite.Wide", width = #ac.math_int<66>',
        1,
    )
    assert changed != ir
    bad.write_text(changed)
    for target in ("cpp", "verilog"):
        rejected = run([args.emitter, bad, "--target", target], 1)
        assert not rejected.stdout.strip()
        assert previous == {
            str(p): p.read_bytes()
            for target in ("cpp", "verilog")
            for p in (output / target).rglob("*")
            if p.is_file()
        }
    receipts.append(
        {
            "variant": name,
            "rows": len(rows),
            "known_rows": sum(known),
            "verified_sha256": hashlib.sha256(verified.read_bytes()).hexdigest(),
            "runner_sha256": hashlib.sha256(runner.read_bytes()).hexdigest(),
        }
    )


# These well-formed packages isolate shared emitter preflight from IR validation.
# Nominals are deliberately unused by the executable Bits1 passthrough top.
def guard_enum(name, width=2):
    return (
        f'"ac.enum"() {{sym_name = "{name}", width = #ac.math_int<{width}>, encoding = "explicit", '
        'members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()'
    )


def guard_struct(name, width=1):
    return f'ac.struct "{name}" fields [{{name = "payload", type = !ac.bits<{static(width)}>}}]'


def guard_package(declarations):
    return (
        f"#guard_w1 = {static(1)}\n!guard_b1 = !ac.bits<#guard_w1>\nmodule {{\n"
        + "\n".join(declarations)
        + "\n"
        + module(
            "top",
            [("a", "!guard_b1")],
            [("y", "!guard_b1")],
            [yield_values([("%a", "!guard_b1")])],
        )
        + '\n"ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()\n}\n'
    )


collision_diagnostic = "emitted hardware name collision"
capacity_diagnostic = "hardware payload plane including enclosing collections exceeds the current Runtime bit-width capacity"
guard_cases = [
    (
        "enum-enum-flat-collision",
        [guard_enum("a_b.State"), guard_enum("a.b_State")],
        "ac.enum",
        collision_diagnostic,
    ),
    (
        "enum-struct-flat-collision",
        [guard_enum("a_b.State"), guard_struct("a.b_State")],
        "ac.struct",
        collision_diagnostic,
    ),
    (
        "struct-struct-flat-collision",
        [guard_struct("a_b.State"), guard_struct("a.b_State")],
        "ac.struct",
        collision_diagnostic,
    ),
    (
        "enum-class-namespace-collision",
        [guard_enum("ns.Value"), guard_enum("ns.Value.Child")],
        "ac.enum",
        collision_diagnostic,
    ),
    (
        "unused-enum-capacity",
        [guard_enum("wide.Unused", 4294967296)],
        "ac.enum",
        capacity_diagnostic,
    ),
    (
        "unused-struct-capacity",
        [guard_struct("wide.Unused", 4294967296)],
        "ac.struct",
        capacity_diagnostic,
    ),
    (
        "enum-enum-flat-nearby",
        [guard_enum("a_b.State"), guard_enum("a.b_Other")],
        None,
        None,
    ),
    (
        "enum-struct-flat-nearby",
        [guard_enum("a_b.State", 65), guard_struct("a.b_Other", 130)],
        None,
        None,
    ),
    (
        "struct-struct-flat-nearby",
        [guard_struct("a_b.State"), guard_struct("a.b_Other")],
        None,
        None,
    ),
    (
        "enum-class-namespace-nearby",
        [guard_enum("ns.Value"), guard_enum("ns.Other.Child")],
        None,
        None,
    ),
]
# Snapshot actual good artifacts once; rejected native emission must publish zero
# bytes and leave those earlier unpacked products unchanged.
published = {
    str(p): p.read_bytes()
    for product in ("carrier", "storage")
    for target in ("cpp", "verilog")
    for p in (build / product / target).rglob("*")
    if p.is_file()
}
guard_receipts = []
for name, declarations, owner, diagnostic in guard_cases:
    output = build / name
    output.mkdir()
    original = output / "fixture.mlir"
    original.write_text(guard_package(declarations))
    verified = output / "verified.mlir"
    run([args.opt, original, "--ac-verify-hardware", "-o", verified])
    targets = {}
    for target in ("cpp", "verilog"):
        result = run(
            [args.emitter, verified, "--target", target], 1 if diagnostic else 0
        )
        targets[target] = {
            "exit_status": result.returncode,
            "stdout_bytes": len(result.stdout.encode()),
        }
        if diagnostic:
            assert result.stdout == "", commands[-1]
            assert f"'{owner}' op {diagnostic}" in result.stderr, commands[-1]
            assert published == {
                str(p): p.read_bytes()
                for product in ("carrier", "storage")
                for emitted_target in ("cpp", "verilog")
                for p in (build / product / emitted_target).rglob("*")
                if p.is_file()
            }
            continue
        payload = output / (target + ".json")
        payload.write_text(result.stdout)
        bundle = json.loads(result.stdout)
        run(
            [
                sys.executable,
                repo / "tests/compiler/lit/CodeGen/Inputs/unpack_bundle.py",
                payload,
                output / target,
            ]
        )
        if target == "cpp":
            driver = output / "driver.cpp"
            driver.write_text(
                '#include "pycircuit_system.hpp"\nint main(){pyc_root root("control");root.a=gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{1});root.Work();return root.y.isFullyKnown() && root.y.packed().value()==gfsim::Bits<1>{1}?0:1;}\n'
            )
            runner = output / "control"
            run(
                [
                    args.cxx,
                    "-O2",
                    "-std=c++20",
                    "-pthread",
                    "-I" + str(repo / "include"),
                    "-I" + str(output / target),
                    driver,
                    "@" + str(output / target / "sources.rsp"),
                    runtime,
                    "-o",
                    runner,
                ]
            )
            run([runner])
        else:
            rtl = [
                output / target / "design_top.sv",
                *[
                    output / target / group["path"]
                    for group in bundle["rtl_source_groups"]
                ],
            ]
            primitives = [repo / path for path in bundle["rtl_standard_sources"]]
            run(
                [
                    args.iverilog,
                    "-g2012",
                    "-s",
                    bundle["root_rtl_name"],
                    "-o",
                    output / "control.vvp",
                    *primitives,
                    *rtl,
                ]
            )
    guard_receipts.append(
        {
            "case": name,
            "common_ir_verify_exit_status": 0,
            "verified_sha256": hashlib.sha256(verified.read_bytes()).hexdigest(),
            "owning_operation": owner,
            "diagnostic": diagnostic,
            "targets": targets,
        }
    )


fixture_paths = [
    fixtures / ("enum_carrier" + extension) for extension in (".py", ".cpp", ".sv")
]
fixture_paths.append(
    (fixtures / "enum-carrier.test")
    if (fixtures / "enum-carrier.test").is_file()
    else fixtures.parent / "enum-carrier.test"
)
(evidence / "candidate.json").write_text(
    json.dumps(
        {
            "scope": "common-IR verified native/RTL execution; no F3 Python enum or source-unit publication claim",
            "artifact_directory": str(build),
            "fixtures": {
                str(p): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in fixture_paths
            },
            "products": receipts,
            "tools": {
                str(Path(tool).resolve()): hashlib.sha256(
                    Path(tool).read_bytes()
                ).hexdigest()
                for tool in (
                    args.opt,
                    args.emitter,
                    args.cxx,
                    args.verilator,
                    args.iverilog,
                    args.vvp,
                )
            },
            "runtime": {str(runtime): hashlib.sha256(runtime.read_bytes()).hexdigest()},
            "emission_preflight": guard_receipts,
            "enum_schema": ENUMS,
            "native_workers": [1, 2],
            "probe_actions": len(probe_rows),
            "failure_recovery_cases": len(failure_rows),
            "negative_scope": "native bad IR and verified nominal name/capacity preflight reject with zero JSON and unchanged prior good artifacts; public final/source-unit replacement deferred F3",
        },
        indent=2,
    )
    + "\n"
)
print(  # noqa: T201 - CLI gate status
    "enum common-IR gate passed: exhaustive small X/Z,65/130 carriers/membership, generic/layout/merge, native1/2/Verilator/Icarus and storage atomicity"
)  # noqa: T201
