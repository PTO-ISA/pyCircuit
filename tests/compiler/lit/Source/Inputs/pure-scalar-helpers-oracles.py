"""Independent bit-symbol reference values; the source never imports this module."""

import itertools

CLOCK = "pyc_7079635f636c6b"

SCALAR_DESIGN = '''from pycircuit import module, rule, struct, u1, u8, bits
@rule
def bump(value: u8) -> u8:
    """A fixed-width expression helper."""
    pass
    return value + 1
@rule
def difference(left: u8, right: u8) -> u8:
    return left - right
@rule
def product(left: u8, right: u8) -> u8:
    return left * right
@rule
def composed(value: u8) -> u8:
    return difference(bump(value), bump(value))
@rule
def complement(value: bits[65]) -> bits[65]:
    return ~value
@rule
def wide_add(value: bits[65]) -> bits[65]:
    return value + 1
@rule
def band(left: u8, right: u8) -> u8:
    return left & right
@rule
def bor(left: u8, right: u8) -> u8:
    return left | right
@rule
def bxor(left: u8, right: u8) -> u8:
    return left ^ right
@rule
def equal(left: u8, right: u8) -> u1:
    return left == right
@rule
def below(left: u8, right: u8) -> u1:
    return left < right
@rule
def choose(flag: u1, left: u8, right: u8) -> u8:
    return left if flag else right
@rule
def minimum(left: u8, right: u8) -> u8:
    return left if left < right else right
@rule
def right_literal(flag: u1, value: u8) -> u8:
    return value if flag else 7
@rule
def left_literal(flag: u1, value: u8) -> u8:
    return 7 if flag else value
@rule
def fixed_literal(value: u8) -> u8:
    return 23
@rule
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
@rule
def step(value: u2) -> u2:
    return value + 1
@rule
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


def self_check():
    assert binary("add", "11111111", "00000001") == "00000000"
    assert binary("and", "zzxx", "0000") == "0000"
    assert equal("x0", "x1") == "0"
    assert select("x", "10zz", "10zz") == "10zz"
    assert select("z", "1010", "1000") == "10x0"
    assert below("x0", "00") == "x"
