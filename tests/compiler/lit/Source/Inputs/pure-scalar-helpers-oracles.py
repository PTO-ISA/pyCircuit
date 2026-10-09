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

NAMED_MAP_DESIGN = """from pycircuit import module, rule, struct, u1, u8, bits, table
@rule
def bump(value: u8) -> u8:
    return value + 1
@rule
def bump_twice(value: u8) -> u8:
    return bump(bump(value))
@rule
def decrement(value: u8) -> u8:
    return value - 1
@rule
def mix(left: u8, right: u8) -> u8:
    return bump(left) ^ decrement(right)
@rule
def equal_rows(left: u8, right: u8) -> u1:
    return left == right
@rule
def minimum_rows(left: u8, right: u8) -> u8:
    return left if left < right else right
@rule
def identity(value: u8) -> u8:
    return value
@rule
def fixed(value: u8) -> u8:
    return 23
@rule
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
    first = mix(left[0], right[0])
    second = mix(left[1], right[1])
    third = mix(left[2], right[2])
    same = left.map(identity)
    constant = left.map(fixed)
    stepped = wide.map(wide_step)
    after = mix(left[0], right[0])
    finite = left.map(identity).map(identity)
    finite_helper = left.map(bump_twice)
    ordered = left.map(lambda a: a + 1).map(mix, right.map(lambda b: b - 1))
    return Result(named=named, lambda_value=equivalent, equality=equality,
        minimum=minimum, direct0=first, direct1=second, direct2=third,
        identity_value=same, splat=constant,
        wide=stepped, after=after, finite=finite, finite_helper=finite_helper,
        ordered=ordered)
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


def self_check():
    assert binary("add", "11111111", "00000001") == "00000000"
    assert binary("and", "zzxx", "0000") == "0000"
    assert equal("x0", "x1") == "0"
    assert select("x", "10zz", "10zz") == "10zz"
    assert select("z", "1010", "1000") == "10x0"
    assert below("x0", "00") == "x"
