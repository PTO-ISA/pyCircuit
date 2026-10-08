"""Table query source cases only; execution/publication use the existing owner."""

CONTROL = """import pycircuit as ac
@ac.struct
class Entry:
    valid: ac.u1
    key: ac.u4
@ac.struct
class Result:
    index: ac.u3
    valid: ac.u1
@ac.rule
def evaluate(entries) -> Result:
    index, valid = entries.first(where=lambda row: row.valid)
    return Result(index=index, valid=valid)
@ac.module
def Top() -> Result:
    entries = ac.table[3, Entry](init=0)
    return evaluate(entries)
"""
CALL = "entries.first(where=lambda row: row.valid)"
ASSIGNMENT = "index, valid = " + CALL


def admission_cases():
    replacements = {
        "first-positional": (CALL, "entries.first(lambda row: row.valid)"),
        "missing-where": (CALL, "entries.first()"),
        "extra-keyword": (
            CALL,
            "entries.first(where=lambda row: row.valid, key=lambda row: row.key)",
        ),
        "argmin-missing-key": (CALL, "entries.argmin(where=lambda row: row.valid)"),
        "argmin-positional-key": (
            CALL,
            "entries.argmin(lambda row: row.key, where=lambda row: row.valid)",
        ),
        "named-def": (CALL, "entries.first(where=predicate)"),
        "predicate-integer": (CALL, "entries.first(where=lambda row: 1)"),
        "predicate-wide": (CALL, "entries.first(where=lambda row: row.key)"),
        "key-integer": (
            CALL,
            "entries.argmin(where=lambda row: row.valid, key=lambda row: 1)",
        ),
        "key-boolean": (
            CALL,
            "entries.argmin(where=lambda row: row.valid, key=lambda row: row.key == 0)",
        ),
        "key-record": (
            CALL,
            "entries.argmin(where=lambda row: row.valid, key=lambda row: row)",
        ),
        "namespace-shadow-call": (
            CALL,
            "entries.first(where=lambda ac: ac.popcount(ac.key) == 0)",
        ),
        "dead-module-call": (
            CALL,
            "entries.first(where=lambda row: row.valid if True else Child(row.valid).valid)",
        ),
        "dead-instrumentation": (
            CALL,
            "entries.first(where=lambda row: row.valid if True else ac.report(row.valid))",
        ),
        "dead-allocation": (
            CALL,
            "entries.first(where=lambda row: row.valid if True else ac.table[1, Entry](init=0))",
        ),
        "nested-query": (
            CALL,
            "entries.first(where=lambda row: row.valid if True else entries.first(where=lambda other: other.valid))",
        ),
        "walrus": (CALL, "entries.first(where=lambda row: (captured := row.valid))"),
        "yield": (CALL, "entries.first(where=lambda row: (yield row.valid))"),
        "starred-callback-call": (
            CALL,
            "entries.first(where=lambda row: ac.concat(*row))",
        ),
        "tuple-as-value": (
            ASSIGNMENT,
            "choice = " + CALL + "\n    index = choice[0]\n    valid = choice[1]",
        ),
        "field-as-result": (ASSIGNMENT, "index, result.valid = " + CALL),
        "duplicate-result": (ASSIGNMENT, "index, index = " + CALL),
        "one-result": (ASSIGNMENT, "index = " + CALL),
        "three-results": (ASSIGNMENT, "index, valid, other = " + CALL),
        "starred-result": (ASSIGNMENT, "index, *valid = " + CALL),
        "chained-result": (ASSIGNMENT, "index = valid = " + CALL),
        "second-boundary-failure": (
            ASSIGNMENT,
            "index: ac.u3 = 0\n    valid: bool = False\n    " + ASSIGNMENT,
        ),
        "non-table-same-spelling": (
            CALL,
            "entries[0].key.first(where=lambda row: row.valid)",
        ),
        "non-table-argmin-spelling": (
            CALL,
            "entries[0].key.argmin(where=lambda row: row.valid, key=lambda row: row.key)",
        ),
        "shadowed-receiver": (
            ASSIGNMENT,
            "receiver = entries[0].key\n    index, valid = receiver.first(where=lambda row: row.valid)",
        ),
        "unproved-callback-index": (
            CALL,
            "entries.first(where=lambda row: entries[row.key].valid)",
        ),
        "out-of-range-callback-read": (
            CALL,
            "entries.first(where=lambda row: entries[3].valid)",
        ),
    }
    cases = {
        name: CONTROL.replace(before, after, 1)
        for name, (before, after) in replacements.items()
    }
    cases["named-def"] = cases["named-def"].replace(
        "@ac.rule\ndef evaluate",
        "def predicate(row):\n    return row.valid\n@ac.rule\ndef evaluate",
        1,
    )
    cases["dead-module-call"] = cases["dead-module-call"].replace(
        "@ac.rule\ndef evaluate",
        "@ac.module\ndef Child(value: ac.u1) -> Result:\n    return Result(index=0, valid=value)\n@ac.rule\ndef evaluate",
        1,
    )
    cases["key-enum"] = CONTROL.replace(
        "import pycircuit as ac",
        "import pycircuit as ac\nfrom enum import Enum\n@ac.encoding(width=1)\nclass Mark(Enum):\n    ZERO = 0\n    ONE = 1",
        1,
    ).replace(
        CALL, "entries.argmin(where=lambda row: row.valid, key=lambda row: Mark.ONE)", 1
    )
    cases["owner-result"] = CONTROL.replace(
        "def evaluate(entries)", "def evaluate(entries, index)", 1
    ).replace(
        "return evaluate(entries)",
        "index: ac.u3 = 0\n    return evaluate(entries, index)",
        1,
    )
    return cases


def syntax_cases():
    source = """import pycircuit as ac
@ac.struct
class Result:
    value: ac.u8
@ac.rule
def Write(state):
    state = 1
@ac.module
def Top() -> Result:
    state: ac.u8 = 0
    callback = entries.first(where=lambda row: Write(state))
    Write(state)
    return Result(value=state)
"""
    valid = {
        "ordinary": source,
        "namespace-shadow": source.replace(
            "lambda row: Write(state)", "lambda ac: ac.valid"
        ),
        "posonly": source.replace("lambda row:", "lambda row, /:"),
    }
    invalid = {
        name: source.replace("lambda row:", replacement)
        for name, replacement in {
            "zero-args": "lambda:",
            "two-args": "lambda row, other:",
            "default": "lambda row=0:",
            "kwonly": "lambda *, row:",
            "vararg": "lambda *row:",
            "kwarg": "lambda **row:",
        }.items()
    }
    return valid, invalid


def resource_cases():
    # Native invariants isolate every scalar counter; public fixtures verify
    # cumulative source traversal/instantiation and Cartesian precharging.
    one = CONTROL.replace("ac.table[3, Entry]", "ac.table[1, Entry]")
    deep = one.replace(
        "lambda row: row.valid", "lambda row: " + "not " * 130 + "row.valid"
    )
    wide = one.replace("key: ac.u4", "key: ac.bits[1 << 28]").replace(
        CALL, "entries.argmin(where=lambda row: row.valid, key=lambda row: row.key)"
    )
    repeated = one.replace(
        ASSIGNMENT, (ASSIGNMENT + "\n    ") * 4097 + "index, valid = " + CALL
    )
    unused = one + one.replace("import pycircuit as ac\n", "").replace(
        "class Entry:", "class OtherEntry:"
    ).replace("class Result:", "class OtherResult:").replace(
        "Entry]", "OtherEntry]"
    ).replace(
        " -> Result:", " -> OtherResult:"
    ).replace(
        "Result(index=", "OtherResult(index="
    ).replace(
        "def evaluate(", "def other_evaluate("
    ).replace(
        "def Top(", "def Unused("
    ).replace(
        "return evaluate(", "return other_evaluate("
    ).replace(
        ASSIGNMENT, (ASSIGNMENT + "\n    ") * 4097 + "index, valid = " + CALL
    )
    registered = one.replace(
        "    return evaluate(entries)",
        "    " + "evaluate(entries)\n    " * 4097 + "return evaluate(entries)",
    )
    cartesian = """import pycircuit as ac
@ac.struct
class Entry:
    valid: ac.u1
    tag: ac.u32
@ac.struct
class Result:
    index: ac.u10
    valid: ac.u1
@ac.module
def Top(entries: ac.table[1024, Entry], ready: ac.table[2048, ac.u1]) -> Result:
    index, valid = entries.first(where=lambda row: row.valid and ready[row.tag % 2048])
    return Result(index=index, valid=valid)
"""
    return {
        "nesting": deep,
        "payload-words": wide,
        "occurrences": repeated,
        "unused-module": unused,
        "repeated-registration": registered,
        "cartesian-product": cartesian,
    }


def structure_cases():
    cases = {}
    for extent in (1, 3, 5, 65):
        width = max(1, (extent - 1).bit_length())
        cases[
            f"callback-{extent}"
        ] = f"""import pycircuit as ac
@ac.struct
class Entry:
    valid: ac.u1
    key: ac.u4
@ac.struct
class Result:
    index: ac.bits[{width}]
    valid: ac.u1
@ac.module
def Top(entries: ac.table[{extent}, Entry]) -> Result:
    first_index, first_valid = entries.first(where=lambda row: row.valid)
    index, valid = entries.argmin(where=lambda row: row.valid, key=lambda row: row.key)
    return Result(index=index, valid=valid)
"""
    sparse = CONTROL.replace(
        "@ac.rule\ndef evaluate(entries) -> Result:\n",
        "@ac.module\ndef Top(entries: ac.table[1, Entry], required: ac.u1) -> Result:\n",
    )
    sparse = sparse[: sparse.index("@ac.module\ndef Top()")]
    aliases = "".join(f"    unused_{i} = required\n" for i in range(1024))
    queries = "".join(
        f"    index_{i}, valid_{i} = entries.first(where=lambda row: row.valid and required)\n"
        for i in range(512)
    )
    sparse = sparse.replace("    " + ASSIGNMENT + "\n", aliases + queries)
    sparse = sparse.replace(
        "Result(index=index, valid=valid)", "Result(index=index_511, valid=valid_511)"
    )
    cases["capture-sparse"] = sparse
    return cases
