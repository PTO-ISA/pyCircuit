"""Table source rejection cases, independent of the compiler implementation."""

MAP_CONTROL = """import pycircuit as ac
@ac.struct
class Box:
    lanes: ac.table[5, ac.bits[9]]
@ac.struct
class Result:
    value: ac.table[3, ac.bits[9]]
@ac.module
def Top(values: ac.table[3, ac.bits[9]], other: ac.table[3, ac.bits[3]],
        different: ac.table[5, ac.bits[9]], scalar: ac.bits[9]) -> Result:
    return Result(value=values.map(lambda lane: lane))
"""
MAP_CALL = "values.map(lambda lane: lane)"

FOLD_CONTROL = """import pycircuit as ac
@ac.struct
class Result:
    value: ac.bits[9]
@ac.module
def Top(values: ac.table[3, ac.bits[9]], kind: ac.bits[9]) -> Result:
    return Result(value=values.fold(kind="add"))
"""
FOLD_CALL = 'values.fold(kind="add")'

PREDICATE_CONTROL = """import pycircuit as ac
@ac.struct
class Result:
    value: ac.u1
@ac.module
def Top(values: ac.table[3, ac.u1]) -> Result:
    return Result(value=values.all())
"""

QUERY_CONTROL = """import pycircuit as ac
@ac.struct
class Result:
    index: ac.bits[2]
    valid: ac.u1
@ac.rule
def evaluate(values) -> Result:
    index, valid = values.first(where=lambda lane: lane == 0)
    return Result(index=index, valid=valid)
@ac.module
def Top(values: ac.table[3, ac.bits[9]]) -> Result:
    return evaluate(values)
"""
QUERY_CALL = "values.first(where=lambda lane: lane == 0)"


def admission_cases():
    # Diagnostic categories are operation/type boundaries; publication assertions
    # below the driver require every case to reject, freshly and with --replace.
    cases = []

    def add(name, text, categories):
        cases.append({"name": name, "text": text, "categories": categories})

    for name, replacement in {
        "map-missing-callback": "values.map()",
        "map-zero-arity": "values.map(lambda: scalar)",
        "map-extra-arity": "values.map(lambda lane, tag: lane)",
        "map-missing-arity": "values.map(lambda lane: lane, other)",
        "map-default": "values.map(lambda lane=0: lane)",
        "map-varargs": "values.map(lambda *lane: lane)",
        "map-kwargs": "values.map(lambda **lane: lane)",
        "map-keyword-only": "values.map(lambda *, lane: lane)",
        "map-callback-keyword": "values.map(callback=lambda lane: lane)",
        "map-input-keyword": "values.map(lambda lane, tag: lane, other=other)",
        "map-starred-input": "values.map(lambda lane: lane, *other)",
        "map-shape": "values.map(lambda lane, tag: lane, different)",
        "map-scalar-input": "values.map(lambda lane, tag: lane, scalar)",
        "map-scalar-receiver": "scalar.map(lambda lane: lane)",
        "map-named-callback": "values.map(identity)",
        "map-dynamic-callback": "values.map(scalar)",
        "map-tuple-result": "values.map(lambda lane: (lane, lane))",
        "map-list-result": "values.map(lambda lane: [lane])",
        "map-bare-table-result": "values.map(lambda lane: values)",
        "map-hidden-whole-table": "values.map(lambda lane: Box(lanes=different))",
        "map-nested-lambda": "values.map(lambda lane: (lambda tag: tag)(lane))",
        "map-nested-method": "values.map(lambda lane: different.fold(kind='add'))",
        "map-walrus": "values.map(lambda lane: (saved := lane))",
        "map-yield": "values.map(lambda lane: (yield lane))",
        "map-shadow-namespace": "values.map(lambda ac: ac.popcount(ac))",
        "map-dead-instrumentation": "values.map(lambda lane: lane if True else ac.report(lane))",
        "map-dead-state": "values.map(lambda lane: lane if True else ac.table[3, ac.u1](init=0))",
    }.items():
        text = MAP_CONTROL.replace(MAP_CALL, replacement)
        if name == "map-hidden-whole-table":
            text = text.replace(
                "value: ac.table[3, ac.bits[9]]", "value: ac.table[3, Box]"
            )
        if name == "map-named-callback":
            text = text.replace(
                "@ac.module",
                "@ac.rule\ndef identity(lane: ac.bits[9]) -> ac.bits[9]:\n    return lane\n@ac.module",
            )
        add(
            name,
            text,
            (
                "map",
                "lambda",
                "callback",
                "table",
                "pure",
                "unsupported",
                "captured ast",
            ),
        )

    for name, replacement in {
        "fold-missing-kind": "values.fold()",
        "fold-positional-kind": 'values.fold("add")',
        "fold-unknown-kind": 'values.fold(kind="subtract")',
        "fold-case-sensitive-kind": 'values.fold(kind="ADD")',
        "fold-dynamic-kind": "values.fold(kind=kind)",
        "fold-integer-kind": "values.fold(kind=1)",
        "fold-initial": 'values.fold(kind="add", initial=0)',
        "fold-callback": 'values.fold(lambda a, b: a + b, kind="add")',
        "fold-scalar-receiver": 'kind.fold(kind="add")',
    }.items():
        add(
            name,
            FOLD_CONTROL.replace(FOLD_CALL, replacement),
            ("fold", "kind", "lambda", "table"),
        )

    enum_fold = """from enum import Enum
import pycircuit as ac
@ac.encoding(width=9)
class Tag(Enum):
    ZERO = 0
@ac.struct
class Result:
    value: ac.bits[9]
@ac.module
def Top(values: ac.table[3, Tag]) -> Result:
    return Result(value=values.fold(kind="add"))
"""
    add("fold-enum", enum_fold, ("fold", "bits", "unsigned"))
    add(
        "fold-struct",
        MAP_CONTROL.replace(MAP_CALL, 'values.fold(kind="add")').replace(
            "values: ac.table[3, ac.bits[9]]", "values: ac.table[3, Box]"
        ),
        ("fold", "bits", "unsigned"),
    )
    for method in ("all", "any", "count"):
        text = PREDICATE_CONTROL.replace("values.all()", f"values.{method}()")
        add(
            f"{method}-wide",
            text.replace("ac.table[3, ac.u1]", "ac.table[3, ac.bits[2]]"),
            (method, "one-bit", "one bit", "bits<1>"),
        )
        add(
            f"{method}-argument",
            text.replace(f"values.{method}()", f"values.{method}(0)"),
            (method, "argument"),
        )
        add(
            f"{method}-keyword",
            text.replace(f"values.{method}()", f"values.{method}(where=True)"),
            (method, "argument", "keyword"),
        )
        onebit_enum = text.replace(
            "import pycircuit as ac",
            "import pycircuit as ac\nfrom enum import Enum\n@ac.encoding(width=1)\nclass Flag(Enum):\n    ZERO = 0\n    ONE = 1",
        ).replace("ac.table[3, ac.u1]", "ac.table[3, Flag]")
        add(
            f"{method}-enum-carrier",
            onebit_enum,
            (method, "bits", "one-bit", "one bit"),
        )

    for name, callback in {
        "zero-args": "lambda: True",
        "two-args": "lambda lane, other: lane == 0",
        "default": "lambda lane=0: lane == 0",
        "varargs": "lambda *lane: True",
    }.items():
        add(
            "first-" + name,
            QUERY_CONTROL.replace("lambda lane: lane == 0", callback),
            ("lambda", "callback", "parameter", "argument", "captured ast"),
        )
        call = f"values.argmin(where=lambda lane: lane == 0, key={callback.replace('lane == 0', 'lane')})"
        add(
            "argmin-key-" + name,
            QUERY_CONTROL.replace(QUERY_CALL, call),
            ("lambda", "callback", "parameter", "argument", "captured ast"),
        )
        call = f"values.argmin(where={callback}, key=lambda lane: lane)"
        add(
            "argmin-where-" + name,
            QUERY_CONTROL.replace(QUERY_CALL, call),
            ("lambda", "callback", "parameter", "argument", "captured ast"),
        )

    add(
        "standalone-lambda",
        MAP_CONTROL.replace(MAP_CALL, "(lambda lane: lane)(scalar)"),
        ("lambda", "unsupported", "call"),
    )
    add(
        "unrecognized-lambda-method",
        MAP_CONTROL.replace(MAP_CALL, "values.unknown(lambda lane: lane)"),
        ("lambda", "unsupported", "unknown", "method"),
    )
    add(
        "map-nominal-result",
        MAP_CONTROL.replace(
            "@ac.struct\nclass Result:",
            "@ac.struct\nclass Peer:\n    payload: ac.bits[9]\n@ac.struct\nclass Result:",
        ).replace("value: ac.table[3, ac.bits[9]]", "value: ac.table[3, Peer]"),
        ("type", "nominal", "table", "boundary"),
    )
    add(
        "count-too-narrow",
        PREDICATE_CONTROL.replace("values.all()", "values.count()"),
        ("type", "width", "boundary", "count", "assign"),
    )
    count_interval = """import pycircuit as ac
@ac.struct
class Result:
    value: ac.bits[9]
@ac.rule
def evaluate(flags, lookup) -> Result:
    count = flags.count()
    return Result(value=lookup[count])
@ac.module
def Top(flags: ac.table[5, ac.u1], lookup: ac.table[5, ac.bits[9]]) -> Result:
    return evaluate(flags, lookup)
"""
    add("count-upper-bound", count_interval, ("range", "interval", "bound", "index"))

    # No host literal expansion: one closed shape exceeds the invocation limit.
    add(
        "map-work-budget",
        MAP_CONTROL.replace("ac.table[3, ac.bits[9]]", "ac.table[1048577, ac.bits[9]]"),
        ("budget", "limit"),
    )
    # Three payload planes per 64-bit word, across actual source, result,
    # per-row region values and captures, exceed the documented 1,048,576 words.
    capture_budget = (
        MAP_CONTROL.replace("ac.table[3, ac.bits[9]]", "ac.table[100000, ac.bits[64]]")
        .replace("scalar: ac.bits[9]", "scalar: ac.bits[64]")
        .replace(MAP_CALL, "values.map(lambda lane: lane ^ scalar)")
    )
    add("map-capture-payload-budget", capture_budget, ("budget", "limit"))
    # The source payload alone fits (600,000 words); the N-1 balanced tree
    # combines require another 599,997 physical value/known/Z words.
    fold_budget = FOLD_CONTROL.replace("ac.bits[9]", "ac.bits[64]").replace(
        "ac.table[3,", "ac.table[200000,"
    )
    add("fold-tree-payload-budget", fold_budget, ("budget", "limit"))
    constant_gather_budget = """import pycircuit as ac
@ac.struct
class Result:
    value: ac.table[3, ac.bits[64]]
@ac.module
def Top(values: ac.table[3, ac.bits[9]], lookup: ac.table[400000, ac.bits[64]]) -> Result:
    return Result(value=values.map(lambda lane: lookup[0]))
"""
    add(
        "map-constant-index-capture-budget", constant_gather_budget, ("budget", "limit")
    )
    # Every individual map is small; their repeated results share one ledger.
    repeated = MAP_CONTROL.replace(
        "ac.table[3, ac.bits[9]]", "ac.table[1024, ac.bits[9]]"
    )
    statements = "\n".join(f"    mapped_{i} = {MAP_CALL}" for i in range(512))
    repeated = repeated.replace(
        "    return Result(value=" + MAP_CALL + ")",
        statements + "\n    return Result(value=mapped_511)",
    )
    add("map-shared-budget", repeated, ("budget", "limit"))
    return cases
