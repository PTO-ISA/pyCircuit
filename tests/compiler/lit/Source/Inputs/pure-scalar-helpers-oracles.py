"""Independent bit-symbol reference values; the source never imports this module."""

import itertools

CLOCK = "pyc_7079635f636c6b"

SCALAR_DESIGN = '''from pycircuit import module, rule, struct, u1, u8, bits
def bump(value: u8) -> u8:
    """A fixed-width expression helper."""
    pass
    result: u8 = value + 1
    return result
def difference(left: u8, right: u8) -> u8:
    delta: u8 = left - right
    return delta
def product(left: u8, right: u8) -> u8:
    return left * right
def composed(value: u8) -> u8:
    return difference(bump(value), bump(value))
def complement(value: bits[65]) -> bits[65]:
    return ~value
def wide_add(value: bits[65]) -> bits[65]:
    return value + 1
def band(left: u8, right: u8) -> u8:
    return left & right
def bor(left: u8, right: u8) -> u8:
    return left | right
def bxor(left: u8, right: u8) -> u8:
    return left ^ right
def equal(left: u8, right: u8) -> u1:
    return left == right
def below(left: u8, right: u8) -> u1:
    return left < right
def choose(flag: u1, left: u8, right: u8) -> u8:
    if flag:
        selected = left
    else:
        selected = right
    return selected
def minimum(left: u8, right: u8) -> u8:
    return left if left < right else right
def right_literal(flag: u1, value: u8) -> u8:
    return value if flag else 7
def left_literal(flag: u1, value: u8) -> u8:
    return 7 if flag else value
def fixed_literal(value: u8) -> u8:
    return 23
def true_guard(value: u8) -> u8:
    return value if True else 255
@struct
class Result:
    increment: u8
    nested_actual: u8
    repeated_body: u8
    subtraction: u8
    multiplication: u8
    inversion: bits[65]
    wide_increment: bits[65]
    conjunction: u8
    disjunction: u8
    exclusive: u8
    equality: u1
    less: u1
    choice: u8
    minimum: u8
    right_literal: u8
    left_literal: u8
    literal: u8
    true_guard: u8
@module
def Top(left: u8, right: u8, wide: bits[65], flag: u1) -> Result:
    return Result(increment=bump(left), nested_actual=bump(bump(left)),
        repeated_body=composed(left), subtraction=difference(left, right),
        multiplication=product(left, right), inversion=complement(wide),
        wide_increment=wide_add(wide), conjunction=band(left, right),
        disjunction=bor(left, right), exclusive=bxor(left, right),
        equality=equal(left, right), less=below(left, right),
        choice=choose(flag, left, right), minimum=minimum(left, right),
        right_literal=right_literal(flag, left), left_literal=left_literal(flag, left),
        literal=fixed_literal(left), true_guard=true_guard(left))
'''

COUNT_DESIGN = """from pycircuit import module, rule, struct, u1, u2, table
def step(value: u2) -> u2:
    return value + 1
def pair(left: u2, right: u2) -> u2:
    return left ^ right
@struct
class Result:
    count: u2
    stepped: u2
    nested: u2
    repeated: u2
@module
def Top(flags: table[3, u1]) -> Result:
    count = flags.count()
    return Result(count=count, stepped=step(count), nested=step(step(count)),
        repeated=pair(flags.count(), flags.count()))
"""

NAMED_MAP_DESIGN = """from pycircuit import module, rule, struct, u1, u8, bits, table
def bump(value: u8) -> u8:
    return value + 1
def bump_twice(value: u8) -> u8:
    return bump(bump(value))
def decrement(value: u8) -> u8:
    return value - 1
def mix(left: u8, right: u8) -> u8:
    return bump(left) ^ decrement(right)
def equal_rows(left: u8, right: u8) -> u1:
    return left == right
def minimum_rows(left: u8, right: u8) -> u8:
    return left if left < right else right
def identity(value: u8) -> u8:
    return value
def fixed(value: u8) -> u8:
    return 23
def wide_step(value: bits[65]) -> bits[65]:
    return value + 1
@struct
class Result:
    named: table[3, u8]
    lambda_value: table[3, u8]
    equality: table[3, u1]
    minimum: table[3, u8]
    direct0: u8
    direct1: u8
    direct2: u8
    identity_value: table[3, u8]
    splat: table[3, u8]
    wide: table[3, bits[65]]
    after: u8
    finite: table[3, u8]
    finite_helper: table[3, u8]
    ordered: table[3, u8]
@module
def Top(left: table[3, u8], right: table[3, u8], wide: table[3, bits[65]]) -> Result:
    named = left.map(mix, right)
    equivalent = left.map(lambda a, b: (a + 1) ^ (b - 1), right)
    equality = left.map(equal_rows, right)
    minimum = left.map(minimum_rows, right)
    captured = left[0]
    def captured_mix(value: u8) -> u8:
        return bump(captured) ^ decrement(value)
    def captured_caller(__pyc_capture_0: u8) -> u8:
        return captured_mix(__pyc_capture_0)
    def captured_local(value: u8) -> u8:
        __pyc_capture_0 = value
        return captured_mix(__pyc_capture_0)
    captured = right[0]
    first = captured_local(right[0])
    second = mix(left[1], right[1])
    third = mix(left[2], right[2])
    same = left.map(identity)
    constant = left.map(fixed)
    stepped = wide.map(wide_step)
    after = captured_caller(right[0])
    finite = left.map(identity).map(identity)
    finite_helper = left.map(bump_twice)
    ordered = left.map(lambda a: a + 1).map(mix, right.map(lambda b: b - 1))
    return Result(named=named, lambda_value=equivalent, equality=equality,
        minimum=minimum, direct0=first, direct1=second, direct2=third,
        identity_value=same, splat=constant,
        wide=stepped, after=after, finite=finite, finite_helper=finite_helper,
        ordered=ordered)
"""


LOOP_DESIGN = """from pycircuit import module, struct, u1, u2, u8, table
@struct
class Cell:
    value: u8
    tag: u8
@struct
class Edits:
    before: table[3, Cell]
    after: table[3, Cell]
def fold(values: table[3, u8]) -> u8:
    total: u8 = 0
    for index in range(3):
        total = total + values[index]
    return total
def edit(values: table[3, Cell], flag: u1) -> Edits:
    candidate = values
    saved = candidate
    for index in range(3):
        candidate[index].value = candidate[index].value + 1
        candidate[index].tag = candidate[index].value ^ candidate[index].tag
        if flag:
            candidate[index].value = candidate[index].value ^ 3
        else:
            candidate[index].value = candidate[index].value + 2
    return Edits(before=saved, after=candidate)
def zero(value: u8) -> u8:
    for unused in range(0):
        value = value + 1
    return value
@struct
class Result:
    total: u8
    before: table[3, Cell]
    after: table[3, Cell]
    updated: table[3, u8]
    selected: u8
    zero: u8
@module
def Top(values: table[3, u8], cells: table[3, Cell], index: u1, flag: u1) -> Result:
    captured_index = index
    captured_values = values
    def update(value: u8) -> table[3, u8]:
        candidate = captured_values
        for ordinal in range(2):
            delta: u8 = ordinal
            candidate[captured_index] = value + delta
        return candidate
    def read(value: u8) -> u8:
        selected = value
        for ordinal in range(1):
            selected = captured_values[captured_index]
        return selected
    captured_index = 0
    captured_values = values.map(lambda row: row + 7)
    changed = edit(cells, flag)
    return Result(total=fold(values), before=changed.before, after=changed.after,
        updated=update(values[0]), selected=read(values[0]), zero=zero(values[0]))
"""


def word(value, width):
    return format(value % (1 << width), f"0{width}b")


def binary(kind, a, b):
    if kind in ("and", "or", "xor"):
        result = []
        for left, right in zip(a, b, strict=True):
            if kind == "and":
                bit = (
                    "0"
                    if "0" in (left, right)
                    else "1" if left == right == "1" else "x"
                )
            elif kind == "or":
                bit = (
                    "1"
                    if "1" in (left, right)
                    else "0" if left == right == "0" else "x"
                )
            else:
                bit = str(int(left != right)) if left in "01" and right in "01" else "x"
            result.append(bit)
        return "".join(result)
    if any(c in "xz" for c in a + b):
        return "x" * len(a)
    left, right = int(a, 2), int(b, 2)
    return word(
        {
            "add": lambda: left + right,
            "sub": lambda: left - right,
            "mul": lambda: left * right,
        }[kind](),
        len(a),
    )


def equal(a, b):
    if any(
        left in "01" and right in "01" and left != right
        for left, right in zip(a, b, strict=True)
    ):
        return "0"
    return "x" if any(c in "xz" for c in a + b) else "1"


def below(a, b):
    return "x" if any(c in "xz" for c in a + b) else str(int(int(a, 2) < int(b, 2)))


def select(flag, a, b):
    if flag in "01":
        return a if flag == "1" else b
    return "".join(
        left if left == right else "x" for left, right in zip(a, b, strict=True)
    )


def scalar_case():
    values = [
        (0, 0, 0, 0),
        (255, 1, (1 << 65) - 1, 1),
        (1, 255, 1 << 64, 0),
        (127, 128, (1 << 64) - 1, 1),
        (128, 127, 17, 0),
        (17, 17, 37, 1),
    ]
    rows = [
        {"left": word(a, 8), "right": word(b, 8), "wide": word(w, 65), "flag": str(f)}
        for a, b, w, f in values
    ]
    for symbol in "xz":
        rows += [
            {
                "left": symbol * 8,
                "right": "00000000",
                "wide": "1" + symbol * 64,
                "flag": symbol,
            },
            {
                "left": "101z0x11",
                "right": "101z0x11",
                "wide": symbol + "01" * 32,
                "flag": symbol,
            },
            {
                "left": "00000000",
                "right": "11111111",
                "wide": "0" * 64 + symbol,
                "flag": "1",
            },
        ]
    rows.extend(
        {
            "left": "0000000" + symbol,
            "right": "1111111" + symbol,
            "wide": "10" * 32 + symbol,
            "flag": symbol,
        }
        for symbol in "xz"
    )
    gold = []
    for row in rows:
        a, b, w, f = (row[n] for n in ("left", "right", "wide", "flag"))
        one = word(1, 8)
        inc = binary("add", a, one)
        gold.append(
            {
                "increment": inc,
                "nested_actual": binary("add", inc, one),
                "repeated_body": binary("sub", inc, inc),
                "subtraction": binary("sub", a, b),
                "multiplication": binary("mul", a, b),
                "inversion": "".join(
                    "1" if c == "0" else "0" if c == "1" else "x" for c in w
                ),
                "wide_increment": binary("add", w, word(1, 65)),
                "conjunction": binary("and", a, b),
                "disjunction": binary("or", a, b),
                "exclusive": binary("xor", a, b),
                "equality": equal(a, b),
                "less": below(a, b),
                "choice": select(f, a, b),
                "minimum": select(below(a, b), a, b),
                "right_literal": select(f, a, word(7, 8)),
                "left_literal": select(f, word(7, 8), a),
                "literal": word(23, 8),
                "true_guard": a,
            }
        )
    fields = {name: len(bits) for name, bits in gold[0].items()}
    return {
        "name": "pure_scalar",
        "text": SCALAR_DESIGN,
        "top": "Top",
        "inputs": {"left": 8, "right": 8, "wide": 65, "flag": 1},
        "plane_inputs": {"true_guard": "left"},
        "fields": fields,
        "rows": rows,
        "gold": gold,
    }


def count_case():
    rows, gold = [], []
    # Complete known Table3 count domain, plus X and Z in each source lane.
    for flags in itertools.product("01", repeat=3):
        rows.append({"flags": "".join(flags)})
    for lane in range(3):
        for symbol in "xz":
            flags = list("101")
            flags[lane] = symbol
            rows.append({"flags": "".join(flags)})
    for row in rows:
        flags = row["flags"]
        count = "xx" if any(c in "xz" for c in flags) else word(flags.count("1"), 2)
        step = binary("add", count, "01")
        gold.append(
            {
                "count": count,
                "stepped": step,
                "nested": binary("add", step, "01"),
                "repeated": binary("xor", count, count),
            }
        )
    return {
        "name": "pure_count",
        "text": COUNT_DESIGN,
        "top": "Top",
        "inputs": {"flags": 3},
        "plane_inputs": {},
        "fields": {"count": 2, "stepped": 2, "nested": 2, "repeated": 2},
        "rows": rows,
        "gold": gold,
    }


def named_map_case():
    # One finite three-lane domain, with distinct lanes and both sides of each
    # arithmetic wrap boundary. Unknown lanes remain full symbol strings.
    rows = []
    for a, b, w in (
        (0, 0, 0),
        (255, 1, (1 << 65) - 1),
        (1, 255, 1 << 64),
        (127, 128, (1 << 64) - 1),
        (128, 127, 17),
        (17, 17, 37),
    ):
        rows.append(
            {
                "left": "".join(word(v, 8) for v in (a, b, a + 5)),
                "right": "".join(word(v, 8) for v in (b, a, b + 9)),
                "wide": "".join(word(v, 65) for v in (w, w ^ 1, w + 2)),
            }
        )
    for symbol in "xz":
        rows.extend(
            (
                {
                    "left": symbol * 8 + "00000000" + "11111111",
                    "right": "00000001" + symbol * 8 + "11111111",
                    "wide": symbol * 65 + "1" * 65 + "0" * 65,
                },
                {
                    "left": "101z0x11" + "0000000" + symbol + "11111111",
                    "right": "101z0x11" + "1111111" + symbol + "00000000",
                    "wide": "1" + symbol * 64 + "01" * 32 + symbol + "0" * 65,
                },
                {
                    "left": "00000000" + "11111111" + symbol * 8,
                    "right": "11111111" + "00000000" + symbol * 8,
                    "wide": "10" * 32 + symbol + "0" * 64 + symbol + "1" * 65,
                },
            )
        )
    gold = []
    one = word(1, 8)
    for row in rows:
        left = [row["left"][i : i + 8] for i in range(0, 24, 8)]
        right = [row["right"][i : i + 8] for i in range(0, 24, 8)]
        wide = [row["wide"][i : i + 65] for i in range(0, 195, 65)]

        def mix(a, b):
            return binary("xor", binary("add", a, one), binary("sub", b, one))

        lanes = [mix(a, b) for a, b in zip(left, right, strict=True)]
        gold.append(
            {
                "named": "".join(lanes),
                "lambda_value": "".join(lanes),
                "equality": "".join(
                    equal(a, b) for a, b in zip(left, right, strict=True)
                ),
                "minimum": "".join(
                    select(below(a, b), a, b)
                    for a, b in zip(left, right, strict=True)
                ),
                "direct0": lanes[0],
                "direct1": lanes[1],
                "direct2": lanes[2],
                "identity_value": row["left"],
                "splat": word(23, 8) * 3,
                "wide": "".join(binary("add", w, word(1, 65)) for w in wide),
                "after": lanes[0],
                "finite": row["left"],
                "finite_helper": "".join(
                    binary("add", binary("add", a, one), one) for a in left
                ),
                "ordered": "".join(
                    mix(binary("add", a, one), binary("sub", b, one))
                    for a, b in zip(left, right, strict=True)
                ),
            }
        )
    return {
        "name": "named_fixed_map",
        "text": NAMED_MAP_DESIGN,
        "top": "Top",
        "inputs": {"left": 24, "right": 24, "wide": 195},
        "plane_inputs": {"identity_value": "left", "finite": "left"},
        "fields": {name: len(value) for name, value in gold[0].items()},
        "rows": rows,
        "gold": gold,
    }


def loop_case():
    rows = []
    for values, cells, index, flag in (
        ((0, 1, 2), ((3, 17), (127, 1), (255, 128)), 0, 0),
        ((255, 2, 17), ((255, 0), (0, 255), (128, 127)), 1, 1),
        ((128, 127, 255), ((17, 63), (253, 3), (254, 1)), 0, 0),
        ((7, 11, 13), ((1, 2), (3, 4), (5, 6)), 1, 1),
    ):
        rows.append({
            "values": "".join(word(v, 8) for v in values),
            "cells": "".join(word(v, 8) + word(t, 8) for v, t in cells),
            "index": word(index, 1), "flag": str(flag),
        })
    for symbol in "xz":
        rows.extend((
            {"values": "00000001" + symbol * 8 + "11111111",
             "cells": "101z0x11" + "00000000" + "11111111" + symbol * 8 + "00000001" + "11111111",
             "index": symbol, "flag": symbol},
            {"values": "101z0x11" + "00000001" + "11111111",
             "cells": "00000000" + "11111111" + "00000001" + "00000001" + "11111111" + "00000000",
             "index": "1", "flag": symbol},
        ))
    gold = []
    for row in rows:
        values = [row["values"][i:i + 8] for i in range(0, 24, 8)]
        cells = [row["cells"][i:i + 16] for i in range(0, 48, 16)]
        # Closed arithmetic expressions and independent per-lane mux truth
        # tables establish sequential updates and immutable saved snapshots.
        total = binary("add", binary("add", values[0], values[1]), values[2])
        after = []
        for cell in cells:
            incremented = binary("add", cell[:8], word(1, 8))
            tag = binary("xor", incremented, cell[8:])
            value = select(row["flag"], binary("xor", incremented, word(3, 8)),
                           binary("add", incremented, word(2, 8)))
            after.append(value + tag)
        updated_lanes = values[:]
        for replacement in (binary("add", values[0], word(0, 8)),
                            binary("add", values[0], word(1, 8))):
            updated_lanes = [select(equal("0" + row["index"], word(i, 2)), replacement, old)
                             for i, old in enumerate(updated_lanes)]
        updated = "".join(updated_lanes)
        index_known = all(c in "01" for c in row["index"])
        index = int(row["index"], 2) if index_known else 3
        selected = values[index] if index < 3 else "x" * 8
        gold.append({"total": total, "before": row["cells"], "after": "".join(after),
                     "updated": updated, "selected": selected, "zero": values[0]})
    return {"name": "pure_helper_loops", "text": LOOP_DESIGN, "top": "Top",
            "inputs": {"values": 24, "cells": 48, "index": 1, "flag": 1},
            "plane_inputs": {"before": "cells", "zero": ("values", 16)},
            "fields": {name: len(value) for name, value in gold[0].items()},
            "rows": rows, "gold": gold}


def self_check():
    assert binary("add", "11111111", "00000001") == "00000000"
    assert binary("and", "zzxx", "0000") == "0000"
    assert equal("x0", "x1") == "0"
    assert select("x", "10zz", "10zz") == "10zz"
    assert select("z", "1010", "1000") == "10x0"
    assert below("x0", "00") == "x"
    loop = loop_case()
    first = loop["gold"][0]
    assert first["total"] == word(3, 8)
    assert first["before"] == loop["rows"][0]["cells"]
    assert first["after"] == "".join(word(v, 8) for v in (6, 21, 130, 129, 2, 128))
    assert first["updated"] == "".join(word(v, 8) for v in (1, 1, 2))
    assert loop["gold"][4]["selected"] == "x" * 8
