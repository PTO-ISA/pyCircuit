"""Independent four-state symbol oracles and vectors; no DUT execution."""

import itertools
import random

CLOCK = "pyc_7079635f636c6b"
RESET = "pyc_7079635f727374"
KINDS = ("add", "mul", "and", "or", "xor", "min", "max")


def word(value, width):
    return format(value % (1 << width), f"0{width}b")


def pack(lanes):
    # spec-collections: the first Table element occupies the most significant bits.
    return "".join(lanes)


def binary(kind, left, right):
    width = len(left)
    if kind in ("and", "or", "xor"):
        result = []
        for a, b in zip(left, right, strict=True):
            if kind == "and":
                result.append("0" if "0" in (a, b) else "1" if a == b == "1" else "x")
            elif kind == "or":
                result.append("1" if "1" in (a, b) else "0" if a == b == "0" else "x")
            else:
                result.append(str(int(a != b)) if a in "01" and b in "01" else "x")
        return "".join(result)
    unknown = any(c in "xz" for c in left + right)
    if kind in ("add", "mul"):
        if unknown:
            return "x" * width
        a, b = int(left, 2), int(right, 2)
        return word(a + b if kind == "add" else a * b, width)
    if unknown:
        # Unknown unsigned comparison merges equal branch symbols, including Z.
        return "".join(a if a == b else "x" for a, b in zip(left, right, strict=True))
    choose_left = (
        int(left, 2) < int(right, 2) if kind == "min" else int(left, 2) > int(right, 2)
    )
    return left if choose_left else right


def balanced(kind, lanes):
    nodes = list(lanes)
    assert nodes
    while len(nodes) > 1:
        nodes = [
            nodes[i] if i + 1 == len(nodes) else binary(kind, nodes[i], nodes[i + 1])
            for i in range(0, len(nodes), 2)
        ]
    return nodes[0]


def reduction_cases(design):
    cases = []
    for extent in (1, 3, 5, 65):
        count_width = max(1, extent.bit_length())
        fields = dict.fromkeys(
            ("add", "mul", "band", "bor", "bxor", "minimum", "maximum"), 9
        )
        fields.update(
            complete=1,
            present=1,
            count=count_width,
            identity=9 * extent,
            zipped=12 * extent,
            projected=9 * extent,
            captured=9 * extent,
            shadowed=9 * extent,
        )
        vectors = []
        for symbol in "01xz":
            vectors.append(
                (
                    [symbol * 9] * extent,
                    [symbol * 3] * extent,
                    [symbol] * extent,
                    symbol * 9,
                    "000001101",
                )
            )
        for index in sorted({0, extent // 2, extent - 1}):
            for symbol in "01xz":
                lanes = [word((i * 67 + 11) & 511, 9) for i in range(extent)]
                flags = ["0"] * extent
                lanes[index] = symbol * 9
                flags[index] = symbol
                vectors.append(
                    (
                        lanes,
                        [word(i, 3) for i in range(extent)],
                        flags,
                        "101001011",
                        "010110100",
                    )
                )
        if extent == 3:
            # Exhaustive predicate planes distinguish count from any/all.
            for predicates in itertools.product("01xz", repeat=3):
                vectors.append(
                    (
                        ["000000011", "000001001", "000000101"],
                        ["001", "010", "100"],
                        list(predicates),
                        "000001100",
                        "000000000",
                    )
                )
        rng = random.Random(20261008 + extent)
        for _ in range(10):
            vectors.append(
                (
                    [word(rng.getrandbits(9), 9) for _ in range(extent)],
                    [word(rng.getrandbits(3), 3) for _ in range(extent)],
                    [str(rng.randrange(2)) for _ in range(extent)],
                    word(rng.getrandbits(9), 9),
                    word(rng.getrandbits(9), 9),
                )
            )
        rows, gold = [], []
        for left, right, flags, bias, replacement in vectors:
            rows.append(
                {
                    "left": pack(left),
                    "right": pack(right),
                    "flags": pack(flags),
                    "bias": bias,
                    "replacement": replacement,
                }
            )
            result = {
                field: balanced(kind, left)
                for field, kind in zip(list(fields)[:7], KINDS, strict=True)
            }
            result.update(
                complete=balanced("and", flags),
                present=balanced("or", flags),
                count=balanced("add", ["0" * (count_width - 1) + b for b in flags]),
                identity=pack(left),
                zipped=pack([a + b for a, b in zip(left, right, strict=True)]),
                projected=pack(left),
                captured=pack([binary("xor", a, bias) for a in left]),
                shadowed=pack(left),
            )
            gold.append(result)
        text = design.replace("ac.table[3,", f"ac.table[{extent},").replace(
            "ac.bits[2]  # count-width", f"ac.bits[{count_width}]  # count-width"
        )
        cases.append(
            {
                "name": f"methods_{extent}",
                "text": text,
                "top": "Top",
                "inputs": {
                    "left": 9 * extent,
                    "right": 3 * extent,
                    "flags": extent,
                    "bias": 9,
                    "replacement": 9,
                },
                "fields": fields,
                "rows": rows,
                "gold": gold,
                "plane_inputs": {
                    "identity": "left",
                    "projected": "left",
                    "shadowed": "left",
                },
                "latent": True,
            }
        )
    return cases


NOMINAL_DESIGN = """import pycircuit as ac
from enums.table_methods_provider import Item, Parcel, Tag, Produce
@ac.struct
class Result:
    identity: ac.table[3, Item]
    tags: ac.table[3, Tag]
    before: ac.table[3, Parcel]
    after: ac.table[3, Parcel]
    before0: Parcel
    before1: Parcel
    before2: Parcel
    after0: Parcel
    after1: Parcel
    after2: Parcel
    captured_data: ac.table[3, ac.bits[9]]
    captured0: ac.bits[9]
    captured1: ac.bits[9]
    captured2: ac.bits[9]
@ac.rule
def evaluate(parcel, replacement) -> Result:
    saved = parcel
    before = parcel.entries.map(lambda item: saved)
    captured_data = parcel.entries.map(lambda item: saved.entries[0].data)
    saved = replacement
    after = parcel.entries.map(lambda item: saved)
    return Result(identity=parcel.entries.map(lambda item: item),
                  tags=parcel.entries.map(lambda item: item.tag), before=before, after=after,
                  before0=before[0], before1=before[1], before2=before[2],
                  after0=after[0], after1=after[1], after2=after[2],
                  captured_data=captured_data, captured0=captured_data[0],
                  captured1=captured_data[1], captured2=captured_data[2])
@ac.module
def Top(parcel: Parcel, replacement: Parcel) -> Result:
    return evaluate(parcel, replacement)
"""


def nominal_case():
    rows, gold = [], []

    parcels = []
    for symbol in "01xz":
        parcels.append(
            pack([symbol * 11, "11" + word(13, 9), "01" + word(511, 9)]) + symbol * 9
        )
    parcels.append(
        pack(["10" + word(19, 9), "xz" + "z01x01011", "z1" + "01z010x01"]) + "1z0x10101"
    )
    for parcel, replacement in itertools.product(parcels, repeat=2):
        entries = parcel[:33]
        rows.append({"parcel": parcel, "replacement": replacement})
        result = {
            "identity": entries,
            "tags": pack([entries[i * 11 : i * 11 + 2] for i in range(3)]),
            "before": pack([parcel] * 3),
            "after": pack([replacement] * 3),
        }
        result.update({f"before{i}": parcel for i in range(3)})
        result.update({f"after{i}": replacement for i in range(3)})
        result["captured_data"] = pack([parcel[2:11]] * 3)
        result.update({f"captured{i}": parcel[2:11] for i in range(3)})
        gold.append(result)
    return {
        "name": "methods_nominal",
        "text": NOMINAL_DESIGN,
        "top": "Top",
        "inputs": {"parcel": 42, "replacement": 42},
        "fields": {
            "identity": 33,
            "tags": 6,
            "before": 126,
            "after": 126,
            "before0": 42,
            "before1": 42,
            "before2": 42,
            "after0": 42,
            "after1": 42,
            "after2": 42,
            "captured_data": 27,
            "captured0": 9,
            "captured1": 9,
            "captured2": 9,
        },
        "rows": rows,
        "gold": gold,
        "plane_inputs": dict(
            identity=("parcel", 9),
            **{f"before{i}": "parcel" for i in range(3)},
            **{f"after{i}": "replacement" for i in range(3)},
            **{f"captured{i}": ("parcel", 31) for i in range(3)},
        ),
    }


CONSTRUCTION_DESIGN = """import pycircuit as ac
@ac.struct
class ZeroBox:
    value: ac.bits[9]
    lanes: ac.table[3, ac.bits[2]]
@ac.struct
class DefaultBox:
    value: ac.bits[9]
    lanes: ac.table[3, ac.bits[2]] = (1, 2, 3)
@ac.struct
class Result:
    zero: ac.table[3, ZeroBox]
    default: ac.table[3, DefaultBox]
    explicit: ac.table[3, DefaultBox]
    zero_values: ac.table[3, ac.bits[9]]
    default_values: ac.table[3, ac.bits[9]]
    explicit_values: ac.table[3, ac.bits[9]]
@ac.rule
def evaluate(values) -> Result:
    zero = values.map(lambda lane: ZeroBox(value=lane))
    default = values.map(lambda lane: DefaultBox(value=lane))
    explicit = values.map(lambda lane: DefaultBox(value=lane, lanes=(0, 1, 2)))
    return Result(zero=zero, default=default, explicit=explicit,
                  zero_values=zero.map(lambda box: box.value),
                  default_values=default.map(lambda box: box.value),
                  explicit_values=explicit.map(lambda box: box.value))
@ac.module
def Top(values: ac.table[3, ac.bits[9]]) -> Result:
    return evaluate(values)
"""


def construction_case():
    rows, gold = [], []
    for lanes in (
        [word(0, 9), word(19, 9), word(511, 9)],
        ["x" * 9] * 3,
        ["z" * 9] * 3,
        ["1z0x10101", "x010z0101", "01xz10101"],
    ):
        raw = pack(lanes)
        rows.append({"values": raw})
        gold.append(
            {
                "zero": pack([lane + "000000" for lane in lanes]),
                "default": pack([lane + "011011" for lane in lanes]),
                "explicit": pack([lane + "000110" for lane in lanes]),
                "zero_values": raw,
                "default_values": raw,
                "explicit_values": raw,
            }
        )
    return {
        "name": "methods_construction",
        "text": CONSTRUCTION_DESIGN,
        "top": "Top",
        "inputs": {"values": 27},
        "fields": {
            "zero": 45,
            "default": 45,
            "explicit": 45,
            "zero_values": 27,
            "default_values": 27,
            "explicit_values": 27,
        },
        "rows": rows,
        "gold": gold,
        "plane_inputs": {
            "zero_values": "values",
            "default_values": "values",
            "explicit_values": "values",
        },
        "latent": True,
    }


def gather_case():
    text = """import pycircuit as ac
@ac.struct
class Result:
    value: ac.table[3, ac.bits[9]]
    constant: ac.table[3, ac.bits[9]]
@ac.rule
def evaluate(indices, lookup) -> Result:
    return Result(value=indices.map(lambda index: lookup[index % 5]),
                  constant=indices.map(lambda index: lookup[0]))
@ac.module
def Top(indices: ac.table[3, ac.bits[9]], lookup: ac.table[5, ac.bits[9]]) -> Result:
    return evaluate(indices, lookup)
"""
    rows, gold = [], []
    for indices in (
        [word(0, 9), word(4, 9), word(511, 9)],
        ["x" * 9, "z" * 9, word(2, 9)],
    ):
        for lookup in (
            [word(i * 67 + 1, 9) for i in range(5)],
            ["z" * 9] * 5,
            ["x" * 9, word(2, 9), "z" * 9, word(4, 9), word(5, 9)],
        ):
            rows.append({"indices": pack(indices), "lookup": pack(lookup)})
            gold.append(
                {
                    "value": pack(
                        [
                            (
                                "x" * 9
                                if any(c in "xz" for c in index)
                                else lookup[int(index, 2) % 5]
                            )
                            for index in indices
                        ]
                    ),
                    "constant": pack([lookup[0]] * 3),
                }
            )
    return {
        "name": "methods_gather",
        "text": text,
        "top": "Top",
        "inputs": {"indices": 27, "lookup": 45},
        "fields": {"value": 27, "constant": 27},
        "rows": rows,
        "gold": gold,
        "plane_inputs": {},
    }


COUNT_LOOKUP_DESIGN = """import pycircuit as ac
@ac.struct
class Result:
    count: ac.bits[3]
    value: ac.bits[9]
@ac.rule
def evaluate(flags, lookup) -> Result:
    count = flags.count()
    return Result(count=count, value=lookup[count])
@ac.module
def Top(flags: ac.table[5, ac.u1], lookup: ac.table[6, ac.bits[9]]) -> Result:
    return evaluate(flags, lookup)
"""


def count_lookup_case():
    # Three physical bits alone describe [0,8); count's true [0,6) fact
    # must make Table6 indexing legal, including the all-five-true endpoint.
    rows, gold = [], []
    lookup = [word(v, 9) for v in (0, 19, 31, 123, 255, 511)]
    predicates = [["1"] * n + ["0"] * (5 - n) for n in range(6)]
    predicates += [["x", "0", "0", "0", "0"], ["0", "0", "0", "0", "z"]]
    for flags in predicates:
        count = balanced("add", ["00" + flag for flag in flags])
        rows.append({"flags": pack(flags), "lookup": pack(lookup)})
        gold.append(
            {
                "count": count,
                "value": (
                    "x" * 9 if "x" in count or "z" in count else lookup[int(count, 2)]
                ),
            }
        )
    return {
        "name": "methods_count_lookup",
        "text": COUNT_LOOKUP_DESIGN,
        "top": "Top",
        "inputs": {"flags": 5, "lookup": 54},
        "fields": {"count": 3, "value": 9},
        "rows": rows,
        "gold": gold,
        "plane_inputs": {},
    }


def self_check():
    assert [max(1, n.bit_length()) for n in (1, 3, 5, 65)] == [1, 2, 3, 7]
    for kind in KINDS:
        assert balanced(kind, ["10z0x1001"]) == "10z0x1001"
    assert balanced("add", ["111", "010", "011"]) == "100"
    assert balanced("mul", ["011", "011", "011"]) == "011"
    assert binary("and", "00", "xz") == "00"
    assert binary("or", "11", "xz") == "11"
    assert binary("min", "z001", "z011") == "z0x1"
    assert pack(["001", "010", "100"]) == "001010100"
