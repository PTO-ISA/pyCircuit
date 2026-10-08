"""Independent source/case data and bit-symbol query oracles; no runner logic."""

import itertools
import random

CLOCK = "pyc_7079635f636c6b"
RESET = "pyc_7079635f727374"


def word(value, width):
    return format(value % (1 << width), f"0{width}b")


def bit_or(a, b):
    return "1" if "1" in (a, b) else "0" if a == b == "0" else "x"


def bit_and(a, b):
    return "0" if "0" in (a, b) else "1" if a == b == "1" else "x"


def invert(a):
    return "1" if a == "0" else "0" if a == "1" else "x"


def select(control, yes, no):
    if control == "1":
        return yes
    if control == "0":
        return no
    return "".join(a if a == b else "x" for a, b in zip(yes, no, strict=True))


def less(a, b):
    return "x" if any(c in a + b for c in "xz") else str(int(int(a, 2) < int(b, 2)))


def first(predicates):
    width = max(1, (len(predicates) - 1).bit_length())
    index, valid = word(0, width), "0"
    for ordinal in reversed(range(len(predicates))):
        predicate = bit_or("0", predicates[ordinal])
        index = select(predicate, word(ordinal, width), index)
        valid = bit_or(predicate, valid)
    return index, valid


def argmin(predicates, keys):
    # Ascending leaves, adjacent pairs, exact odd-tail carry at every level.
    width = max(1, (len(predicates) - 1).bit_length())
    nodes = [
        (bit_or("0", p), k, word(i, width))
        for i, (p, k) in enumerate(zip(predicates, keys, strict=True))
    ]
    while len(nodes) > 1:
        next_level = []
        for position in range(0, len(nodes), 2):
            left = nodes[position]
            if position + 1 == len(nodes):
                next_level.append(left)
                continue
            right = nodes[position + 1]
            take = bit_and(right[0], bit_or(invert(left[0]), less(right[1], left[1])))
            next_level.append(
                (
                    bit_or(left[0], right[0]),
                    select(take, right[1], left[1]),
                    select(take, right[2], left[2]),
                )
            )
        nodes = next_level
    return nodes[0][2], nodes[0][0]


def raw_get(values, index):
    # Ordinary TableGet poisons all payload bits for any X/Z index, even if
    # every candidate payload agrees. This does not grant source OOB admission.
    width = len(values[0])
    if any(c in index for c in "xz") or int(index, 2) >= len(values):
        return "x" * width
    return values[int(index, 2)]


def entry_rows(depth, width):
    zero, maximum = word(0, width), word((1 << width) - 1, width)
    rows = [
        [("0", zero)] * depth,
        [("0", "x" * width)] * depth,
        [("1", maximum)] * depth,
        [("1", word(depth - i - 1, width)) for i in range(depth)],
    ]
    for winner in sorted({0, depth // 2, depth - 1}):
        for key in (zero, maximum, "x" * width, "z" * width):
            entries = [
                ("0", "z" * width if i % 2 else "x" * width) for i in range(depth)
            ]
            entries[winner] = "1", key
            rows.append(entries)
    for symbol in "xz":
        entries = [("0", zero)] * depth
        entries[0] = symbol, zero
        entries[-1] = "1", maximum
        rows.append(entries)
        equal_payloads = [("0", maximum)] * depth
        equal_payloads[0] = symbol, maximum
        equal_payloads[-1] = "1", maximum
        rows.append(equal_payloads)
        if depth > 1:
            entries = [("0", zero)] * depth
            entries[0] = "1", symbol * width
            entries[-1] = "1", maximum
            rows.append(entries)
    if depth == 1:
        rows += [[(valid, key)] for valid, key in itertools.product("01xz", "01xz")]
    if depth == 3:
        for sequence, predicates in enumerate(itertools.product("01xz", repeat=depth)):
            keys = (zero, maximum, "x" * width, "z" * width)
            rows.append(
                [
                    (p, keys[(sequence + i) % len(keys)])
                    for i, p in enumerate(predicates)
                ]
            )
    rng = random.Random(20261007 + depth + width)
    for _ in range(12):
        rows.append(
            [
                (str(rng.randrange(2)), word(rng.getrandbits(width), width))
                for _ in range(depth)
            ]
        )
    return rows


def packed_entries(entries):
    return "".join(valid + key for valid, key in reversed(entries))


def base_source(
    depth, width, predicate_name="row", key_name="row", alias=False, enum_shadow=False
):
    index_width = max(1, (depth - 1).bit_length())
    packed_width = depth * (width + 1)
    enum = (
        "from enum import Enum\n@ac.encoding(width=1)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n"
        if enum_shadow
        else ""
    )
    imports = "from pycircuit import concat as cat\n" if alias else ""
    fields = {
        "first_index": index_width,
        "first_valid": 1,
        "minimum_index": index_width,
        "minimum_valid": 1,
        "first_read": width,
        "minimum_read": width,
        "raw": packed_width,
    }
    if enum_shadow:
        fields["outside"] = 1
    source = "import pycircuit as ac\n" + imports + enum
    source += f"@ac.struct\nclass Entry:\n    valid: ac.u1\n    key: ac.bits[{width}]\n"
    source += "@ac.struct\nclass Result:\n" + "".join(
        f"    {name}: ac.bits[{size}]\n" for name, size in fields.items()
    )
    source += "@ac.rule\ndef evaluate(entries, entry_bits) -> Result:\n"
    for i in range(depth):
        offset = i * (width + 1)
        source += f"    entries[{i}] = Entry(valid=entry_bits[{offset + width}:{offset + width + 1}], key=entry_bits[{offset}:{offset + width}])\n"
    key = f"cat({key_name}.key)" if alias else f"{key_name}.key"
    source += f"    a, av = entries.first(where=lambda {predicate_name}: {predicate_name}.valid)\n"
    source += f"    b, bv = entries.argmin(where=lambda {predicate_name}: {predicate_name}.valid, key=lambda {key_name}: {key})\n"
    source += "    return Result(first_index=a, first_valid=av, minimum_index=b, minimum_valid=bv, first_read=entries[a].key, minimum_read=entries[b].key, raw=entry_bits"
    source += ", outside=ac.enum_to_bits(State.ONE)" if enum_shadow else ""
    source += (
        ")\n@ac.module\ndef Top(entry_bits: ac.bits["
        + str(packed_width)
        + "]) -> Result:\n"
    )
    source += f"    entries = ac.table[{depth}, Entry](init=0)\n    return evaluate(entries, entry_bits)\n"
    return source, fields


def basic_case(
    depth,
    width,
    name,
    predicate_name="row",
    key_name="row",
    alias=False,
    enum_shadow=False,
):
    source, fields = base_source(
        depth, width, predicate_name, key_name, alias, enum_shadow
    )
    rows, gold = [], []
    for entries in entry_rows(depth, width):
        entry_bits = packed_entries(entries)
        predicates, keys = zip(*entries, strict=True)
        a, av = first(predicates)
        b, bv = argmin(predicates, keys)
        rows.append({"entry_bits": entry_bits, CLOCK: "0", RESET: "0"})
        expected = {
            "first_index": a,
            "first_valid": av,
            "minimum_index": b,
            "minimum_valid": bv,
            "first_read": raw_get(keys, a),
            "minimum_read": raw_get(keys, b),
            "raw": entry_bits,
        }
        if enum_shadow:
            expected["outside"] = "1"
        gold.append(expected)
    return {
        "name": name,
        "text": source,
        "top": "Top",
        "inputs": {"entry_bits": depth * (width + 1), CLOCK: 1, RESET: 1},
        "fields": fields,
        "rows": rows,
        "gold": gold,
        "plane_inputs": {"raw": "entry_bits"},
    }


def sole_transport_case(depth, width, winner):
    case = basic_case(depth, width, f"query_transport_{depth}_{width}_{winner}")
    rows, gold = [], []
    for key in (
        word(0, width),
        word((1 << width) - 1, width),
        "x" * width,
        "z" * width,
    ):
        inactive = word(0, width) if all(c in "01" for c in key) else "x" * width
        entries = [("0", inactive)] * depth
        entries[winner] = "1", key
        entry_bits = packed_entries(entries)
        rows.append({"entry_bits": entry_bits, CLOCK: "0", RESET: "0"})
        gold.append(
            {
                "first_index": word(winner, max(1, (depth - 1).bit_length())),
                "first_valid": "1",
                "minimum_index": word(winner, max(1, (depth - 1).bit_length())),
                "minimum_valid": "1",
                "first_read": key,
                "minimum_read": key,
                "raw": entry_bits,
            }
        )
    case.update(
        rows=rows,
        gold=gold,
        plane_inputs={
            "raw": "entry_bits",
            "first_read": ("entry_bits", winner * (width + 1)),
            "minimum_read": ("entry_bits", winner * (width + 1)),
        },
    )
    return case


def snapshot_case():
    text = """import pycircuit as ac
@ac.struct
class Entry:
    valid: ac.u1
    key: ac.u4
@ac.struct
class Result:
    saved_index: ac.u2
    saved_valid: ac.u1
    current_index: ac.u2
    current_valid: ac.u1
    captured_index: ac.u2
    captured_valid: ac.u1
@ac.rule
def evaluate(entries, key, threshold) -> Result:
    entries[0] = Entry(valid=1, key=9)
    entries[1] = Entry(valid=1, key=3)
    entries[2] = Entry(valid=1, key=7)
    saved = entries
    limit = threshold
    entries[0].key = key
    a, av = saved.argmin(where=lambda row: row.valid, key=lambda row: row.key)
    b, bv = entries.argmin(where=lambda row: row.valid, key=lambda row: row.key)
    c, cv = entries.first(where=lambda row: row.key < limit)
    limit = key
    entries[1].key = key
    return Result(saved_index=a, saved_valid=av, current_index=b, current_valid=bv, captured_index=c, captured_valid=cv)
@ac.module
def Top(key: ac.u4, threshold: ac.u4) -> Result:
    entries = ac.table[3, Entry](init=0)
    return evaluate(entries, key, threshold)
"""
    fields = {
        "saved_index": 2,
        "saved_valid": 1,
        "current_index": 2,
        "current_valid": 1,
        "captured_index": 2,
        "captured_valid": 1,
    }
    rows, gold = [], []
    for key, threshold in itertools.product(
        ("0000", "0011", "1111", "xxxx", "zzzz"),
        ("0000", "0100", "1111", "xxxx", "zzzz"),
    ):
        keys = (key, "0011", "0111")
        b, bv = argmin("111", keys)
        c, cv = first([less(item, threshold) for item in keys])
        rows.append({"key": key, "threshold": threshold, CLOCK: "0", RESET: "0"})
        gold.append(
            {
                "saved_index": "01",
                "saved_valid": "1",
                "current_index": b,
                "current_valid": bv,
                "captured_index": c,
                "captured_valid": cv,
            }
        )
    return {
        "name": "query_snapshots",
        "text": text,
        "top": "Top",
        "inputs": {"key": 4, "threshold": 4, CLOCK: 1, RESET: 1},
        "fields": fields,
        "rows": rows,
        "gold": gold,
        "plane_inputs": {},
    }


def bounded_modulo(bits, extent):
    return (
        "x" * len(bits)
        if any(c in bits for c in "xz")
        else word(int(bits, 2) % extent, len(bits))
    )


def gather_case(depth, extent, enum_key=False, independent=False, chained=False):
    key_width, tag_width = 4, 8
    index_width = max(1, (depth - 1).bit_length())
    entry_width, ready_width = 1 + key_width + tag_width, 7
    ordered_width = 2 if enum_key else key_width
    fields = {
        "first_index": index_width,
        "first_valid": 1,
        "minimum_index": index_width,
        "minimum_valid": 1,
        "first_read": ordered_width,
        "minimum_read": ordered_width,
        "raw_entries": depth * entry_width,
        "raw_ready": extent * ready_width,
    }
    source = """import pycircuit as ac
from enum import Enum
@ac.encoding(width=2)
class Mark(Enum):
    ZERO = 0
    ONE = 1
    TWO = 2
    THREE = 3
@ac.struct
class Payload:
    key: ac.u4
    mark: Mark
@ac.struct
class Ready:
    eligible: ac.u1
    payload: Payload
@ac.struct
class Entry:
    valid: ac.u1
    key: ac.u4
    tag: ac.u8
"""
    source += "@ac.struct\nclass Result:\n" + "".join(
        f"    {name}: ac.bits[{width}]\n" for name, width in fields.items()
    )
    source += "@ac.rule\ndef evaluate(entries, ready, entries_bits, ready_bits"
    source += ", tags, tags_bits" if chained else ""
    source += ") -> Result:\n"
    for ordinal in range(depth):
        offset = ordinal * entry_width
        source += f"    entries[{ordinal}] = Entry(valid=entries_bits[{offset + 12}:{offset + 13}], key=entries_bits[{offset + 8}:{offset + 12}], tag=entries_bits[{offset}:{offset + 8}])\n"
    for ordinal in range(extent):
        offset = ordinal * ready_width
        source += f"    decoded_{ordinal}, member_{ordinal} = ac.enum_from_bits[Mark](ready_bits[{offset}:{offset + 2}])\n"
        source += f"    ready[{ordinal}] = Ready(eligible=ready_bits[{offset + 6}:{offset + 7}], payload=Payload(key=ready_bits[{offset + 2}:{offset + 6}], mark=decoded_{ordinal}))\n"
        if chained:
            source += (
                f"    tags[{ordinal}] = tags_bits[{ordinal * 8}:{ordinal * 8 + 8}]\n"
            )
    address = (
        f"tags[row.tag % {extent}] % {extent}" if chained else f"row.tag % {extent}"
    )
    access = f"ready[{address}]"
    predicate = f"row.valid and {access}.eligible"
    if independent:
        predicate += " and ready[0].eligible"
    key = (
        f"ac.enum_to_bits({access}.payload.mark)"
        if enum_key
        else f"{access}.payload.key"
    )
    source += f"    a, av = entries.first(where=lambda row: {predicate})\n"
    source += f"    b, bv = entries.argmin(where=lambda row: {predicate}, key=lambda row: {key})\n"

    def read(index):
        address = (
            f"tags[entries[{index}].tag % {extent}] % {extent}"
            if chained
            else f"entries[{index}].tag % {extent}"
        )
        raw = (
            f"ready[{address}].payload.mark"
            if enum_key
            else f"ready[{address}].payload.key"
        )
        return f"ac.enum_to_bits({raw})" if enum_key else raw

    source += f"    return Result(first_index=a, first_valid=av, minimum_index=b, minimum_valid=bv, first_read={read('a')}, minimum_read={read('b')}, raw_entries=entries_bits, raw_ready=ready_bits)\n"
    source += f"@ac.module\ndef Top(entries_bits: ac.bits[{depth * entry_width}], ready_bits: ac.bits[{extent * ready_width}]"
    source += f", tags_bits: ac.bits[{extent * 8}]" if chained else ""
    source += ") -> Result:\n"
    source += f"    entries = ac.table[{depth}, Entry](init=0)\n    ready = ac.table[{extent}, Ready](init=0)\n"
    source += f"    tags = ac.table[{extent}, ac.u8](init=0)\n" if chained else ""
    source += (
        "    return evaluate(entries, ready, entries_bits, ready_bits"
        + (", tags, tags_bits" if chained else "")
        + ")\n"
    )
    rows, gold = [], []
    for sample in range(16):
        entries = [
            ("1" if i % 2 == sample % 2 else "0", word(i, 4), word(i + sample, 8))
            for i in range(depth)
        ]
        ready = [
            ("1", word((extent - i + sample) % 16, 4), word(i + sample, 2))
            for i in range(extent)
        ]
        tags = [word((i * 3 + sample) % 256, 8) for i in range(extent)]
        if sample == 1:
            ready = [("1", "1111", "11")] * extent
        if sample in (2, 3):
            ready = [("0", "x" * 4, "xx")] * extent
        if sample in (4, 5):
            symbol = "x" if sample == 4 else "z"
            ready = [("1", symbol * 4, symbol * 2)] * extent
        if sample in (6, 7):
            symbol = "x" if sample == 6 else "z"
            entries = [("1", word(i, 4), symbol + word(i, 7)) for i in range(depth)]
            ready = [("1", "1111", "11")] * extent
        if sample in (8, 9):
            symbol = "x" if sample == 8 else "z"
            ready = [(symbol, word(i, 4), word(i, 2)) for i in range(extent)]
        if sample in (10, 11):
            entries = [("0", "zzzz", "x" * 8)] * depth
            entries[-1] = "1", "1111", word(extent - 1, 8)
        if chained and sample in (12, 13):
            tags = ["x" * 8 if sample == 12 else "z" * 8] * extent

        def read_ready(tag, tags=tags, ready=ready):
            address = bounded_modulo(tag, extent)
            if chained:
                address = bounded_modulo(raw_get(tags, address), extent)
            return raw_get([valid + key + mark for valid, key, mark in ready], address)

        predicates, keys = [], []
        for valid, _, tag in entries:
            selected = read_ready(tag)
            predicate = bit_and(valid, selected[0])
            if independent:
                predicate = bit_and(predicate, ready[0][0])
            predicates.append(predicate)
            keys.append(selected[-2:] if enum_key else selected[1:5])
        a, av = first(predicates)
        b, bv = argmin(predicates, keys)

        def observed(index, entries=entries):
            entry = raw_get([valid + key + tag for valid, key, tag in entries], index)
            selected = read_ready(entry[-8:])
            return selected[-2:] if enum_key else selected[1:5]

        entry_bits = "".join(valid + key + tag for valid, key, tag in reversed(entries))
        packed_ready = "".join(
            valid + key + mark for valid, key, mark in reversed(ready)
        )
        row = {
            "entries_bits": entry_bits,
            "ready_bits": packed_ready,
            CLOCK: "0",
            RESET: "0",
        }
        if chained:
            row["tags_bits"] = "".join(reversed(tags))
        rows.append(row)
        gold.append(
            {
                "first_index": a,
                "first_valid": av,
                "minimum_index": b,
                "minimum_valid": bv,
                "first_read": observed(a),
                "minimum_read": observed(b),
                "raw_entries": entry_bits,
                "raw_ready": packed_ready,
            }
        )
    inputs = {
        "entries_bits": depth * entry_width,
        "ready_bits": extent * ready_width,
        CLOCK: 1,
        RESET: 1,
    }
    if chained:
        inputs["tags_bits"] = extent * 8
    name = f"query_gather_{depth}_{extent}" + (
        "_enum"
        if enum_key
        else "_independent" if independent else "_chained" if chained else ""
    )
    return {
        "name": name,
        "text": source,
        "top": "Top",
        "inputs": inputs,
        "fields": fields,
        "rows": rows,
        "gold": gold,
        "plane_inputs": {"raw_entries": "entries_bits", "raw_ready": "ready_bits"},
    }


def instance_shadow_case():
    text = """import pycircuit as ac
@ac.struct
class Entry:
    valid: ac.u1
    key: ac.u4
@ac.struct
class Result:
    first_index: ac.u2
    first_valid: ac.u1
    minimum_index: ac.u2
    minimum_valid: ac.u1
    first_read: ac.u4
    minimum_read: ac.u4
    outside: ac.u1
@ac.module
def Child(flag: bool) -> {"valid": bool}:
    return {"valid": flag}
@ac.module
def Top(entries: ac.table[3, Entry], fake: bool) -> {"result": Result}:
    child = Child()
    @ac.rule
    def bind_child():
        child(flag=fake)
    bind_child()
    a, av = entries.first(where=lambda child: child.valid)
    b, bv = entries.argmin(where=lambda child: child.valid, key=lambda child: child.key)
    return {"result": Result(first_index=a, first_valid=av, minimum_index=b, minimum_valid=bv, first_read=entries[a].key, minimum_read=entries[b].key, outside=child.valid)}
"""
    fields = {
        "first_index": 2,
        "first_valid": 1,
        "minimum_index": 2,
        "minimum_valid": 1,
        "first_read": 4,
        "minimum_read": 4,
        "outside": 1,
    }
    rows, gold = [], []
    for fake in "01xz":
        for entries in entry_rows(3, 4):
            predicates, keys = zip(*entries, strict=True)
            a, av = first(predicates)
            b, bv = argmin(predicates, keys)
            # A genuine Table input follows existing row-major packed order.
            rows.append(
                {
                    "entries": "".join(valid + key for valid, key in entries),
                    "fake": fake,
                }
            )
            gold.append(
                {
                    "first_index": a,
                    "first_valid": av,
                    "minimum_index": b,
                    "minimum_valid": bv,
                    "first_read": raw_get(keys, a),
                    "minimum_read": raw_get(keys, b),
                    "outside": fake,
                }
            )
    return {
        "name": "query_instance_shadow",
        "text": text,
        "top": "Top",
        "inputs": {"entries": 15, "fake": 1},
        "fields": fields,
        "rows": rows,
        "gold": gold,
        "plane_inputs": {"outside": "fake"},
    }


def execution_cases():
    cases = [
        basic_case(n, w, f"query_basic_{n}_{w}")
        for n, w in ((1, 1), (3, 4), (5, 5), (65, 5), (3, 65), (5, 130))
    ]
    cases += [
        basic_case(3, 4, "query_namespace_shadow", "ac", "ac"),
        basic_case(5, 5, "query_intrinsic_alias", alias=True),
        basic_case(3, 4, "query_enum_shadow", "State", "State", enum_shadow=True),
        sole_transport_case(3, 4, 1),
        sole_transport_case(5, 65, 4),
        snapshot_case(),
        instance_shadow_case(),
    ]
    cases += [gather_case(n, m) for n, m in ((3, 1), (3, 3), (5, 5), (3, 65))]
    cases += [
        gather_case(5, 3, enum_key=True),
        gather_case(3, 5, independent=True),
        gather_case(5, 3, chained=True),
    ]
    return cases
