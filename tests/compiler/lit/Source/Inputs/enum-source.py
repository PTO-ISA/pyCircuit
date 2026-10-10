"""Independent public Enum declarations and staged source semantics.

The default mode verifies S3a declarations/imports and protected compile/link.
--semantics adds S3b members, comparisons, selection, to_bits, storage/defaults,
native workers1/2 and genuine four-state RTL. S3c adds strict two-result binding,
carrier/membership independence and decoded owner updates.
"""

import argparse
import hashlib
import itertools
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
for name in ("repo", "source-compiler", "linker", "optimizer", "scratch"):
    parser.add_argument("--" + name, required=True)
parser.add_argument("--prepare-only", action="store_true")
parser.add_argument("--semantics", action="store_true")
parser.add_argument("--case-family", choices=("enum", "table-query"), default="enum")
for name in ("emitter", "cxx", "verilator", "iverilog", "vvp"):
    parser.add_argument("--" + name)
args = parser.parse_args()
if args.case_family == "table-query" and (not args.semantics or args.prepare_only):
    parser.error("the table-query case family requires semantics execution")
repo = Path(args.repo).resolve()
evidence = Path(args.scratch).resolve()
evidence.mkdir(parents=True, exist_ok=True)
build = Path(tempfile.mkdtemp(prefix="run-", dir=evidence))
source = build / "source"
source.mkdir()
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
)
if args.semantics:
    assert all(
        (args.emitter, args.cxx, args.verilator, args.iverilog, args.vvp)
    ), "semantics requires native and RTL tools"
    env["PYCIRCUIT_EMITTER"] = args.emitter
commands = []


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, code=0, diagnostic=None):
    command = list(map(str, command))
    result = subprocess.run(
        command, cwd=repo, env=env, text=True, capture_output=True, timeout=120
    )
    row = {
        "command": command,
        "exit_status": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
    }
    commands.append(row)
    (evidence / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == code, row
    assert (
        "Assertion failed" not in result.stderr and "Traceback" not in result.stderr
    ), row
    if diagnostic:
        assert diagnostic in result.stderr, row
    return result


def cli(*arguments, code=0, diagnostic=None):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], code, diagnostic)


def compile_unit(name, output, imports=(), replace=False, code=0, diagnostic=None):
    command = [
        "compile",
        "-c",
        source / name,
        "--source-root",
        source,
        "--package-prefix",
        "enums",
        "-o",
        output,
    ]
    for unit in imports:
        command.extend(("-I", unit))
    if replace:
        command.append("--replace")
    return cli(*command, code=code, diagnostic=diagnostic)


def payload(unit, kind):
    receipt = json.loads((unit / "unit.json").read_text())
    return unit / receipt["files"][kind]


def snapshot(path):
    return {
        p.relative_to(path).as_posix(): p.read_bytes()
        for p in path.rglob("*")
        if p.is_file()
    }


def managed_snapshot(path):
    control = path.parent / ("." + path.name + ".pycircuit-publication")
    return {
        "payload": snapshot(path) if path.is_dir() else path.read_bytes(),
        "publication": snapshot(control),
    }


def enum_schema(text):
    result = {}
    for line in text.splitlines():
        if '"ac.enum"()' not in line:
            continue
        symbol = re.search(r'sym_name = "([^"]+)"', line).group(1)
        width = int(re.search(r"width = #ac.math_int<([0-9]+)>", line).group(1))
        kind = re.search(r'encoding = "([^"]+)"', line).group(1)
        members = re.search(r"members = \[(.*?)\]", line).group(1)
        pairs = [
            (name, int(code))
            for code, name in re.findall(
                r'\{code = #ac.math_int<([0-9]+)>, name = "([^"]+)"\}', members
            )
        ]
        assert pairs and symbol not in result, line
        result[symbol] = (width, kind, pairs)
    return result


def owner_paths(text, attribute):
    match = re.search(re.escape(attribute) + r" = \[(.*?)\]", text, re.S)
    assert match, (attribute, text)
    return re.findall(r'path = "([^"]+)"', match.group(1))


def assert_unit(unit, kind):
    body = payload(unit, "body")
    header = payload(unit, "interface")
    body_text = body.read_text()
    header_text = header.read_text()
    assert f'ac.unit_kind = "{kind}"' in body_text, body_text
    assert 'ac.unit_kind = "interface"' in header_text, header_text
    if kind == "declarations":
        assert (
            '"ac.module"()' not in body_text and '"ac.module.import"()' not in body_text
        ), body_text
    extracted = build / (unit.name + "-extracted.ac")
    normalized = build / (unit.name + "-header.ac")
    run([args.optimizer, body, "--ac-extract-source-interface", "-o", extracted])
    run([args.optimizer, header, "-o", normalized])
    assert extracted.read_text() == normalized.read_text(), (extracted, normalized)
    return body_text, header_text


ENUM_PROVIDER = '''import enum as enum_ns
from enum import Enum as E, auto as next_member
import pycircuit as ac
from pycircuit import encoding as encoded

@ac.encoding(width=2)
class State(enum_ns.Enum):
    ZERO = 0
    ONE = 1

@encoded(width=1, kind="explicit")
class Singleton(E):
    ONLY = 1

@encoded(width=1 + 2)
class Sparse(E):
    """Codes retain source order and their explicit sparse values."""
    IDLE = 0
    WAIT = 1 + 2
    DONE = (1 << 2) + 1

@encoded(width=3, kind="binary_sequential")
class Sequential(E):
    LAST = next_member()
    FIRST = next_member()
    MIDDLE = next_member()
    END = next_member()

@ac.encoding(width=4, kind="binary_one_hot")
class Hot(enum_ns.Enum):
    A = enum_ns.auto()
    B = enum_ns.auto()
    C = enum_ns.auto()
    D = enum_ns.auto()

@encoded(width=3, kind="gray_sequential")
class Gray(E):
    A = next_member()
    B = next_member()
    C = next_member()
    D = next_member()

@encoded(width=64 + 1)
class Wide(E):
    LOW = 0
    HIGH = (1 << 64) + 3
    FULL = (1 << 65) - 1

@encoded(width=130)
class Huge(E):
    LOW = 0
    HIGH = (1 << 129) + 7
    FULL = (1 << 130) - 1
'''
SCHEMA = {
    "enums.types.State": (2, "explicit", [("ZERO", 0), ("ONE", 1)]),
    "enums.types.Singleton": (1, "explicit", [("ONLY", 1)]),
    "enums.types.Sparse": (3, "explicit", [("IDLE", 0), ("WAIT", 3), ("DONE", 5)]),
    "enums.types.Sequential": (
        3,
        "binary_sequential",
        [("LAST", 0), ("FIRST", 1), ("MIDDLE", 2), ("END", 3)],
    ),
    "enums.types.Hot": (4, "binary_one_hot", [("A", 1), ("B", 2), ("C", 4), ("D", 8)]),
    "enums.types.Gray": (
        3,
        "gray_sequential",
        [("A", 0), ("B", 1), ("C", 3), ("D", 2)],
    ),
    "enums.types.Wide": (
        65,
        "explicit",
        [("LOW", 0), ("HIGH", (1 << 64) + 3), ("FULL", (1 << 65) - 1)],
    ),
    "enums.types.Huge": (
        130,
        "explicit",
        [("LOW", 0), ("HIGH", (1 << 129) + 7), ("FULL", (1 << 130) - 1)],
    ),
}
PROVIDERS = {
    "types.py": ENUM_PROVIDER,
    "peer.py": """from enum import Enum
from pycircuit import encoding
@encoding(width=2)
class State(Enum):
    ZERO = 0
    ONE = 1
""",
    "records.py": """import pycircuit as ac
@ac.struct
class Packet:
    payload: ac.bits[13]
""",
    "empty.py": '"""A real source unit with no owned declarations."""\n',
    "facade.py": "from enums.types import State as Forward\n",
    "facade2.py": "from enums.facade import Forward as Exported\n",
    "unrelated.py": """import pycircuit as ac
@ac.struct
class Enum:
    data: ac.bits[1]
@ac.module
def encoding(x: bool) -> {"out": bool}:
    return {"out": x}
@ac.module
def auto(x: bool) -> {"out": bool}:
    return {"out": x}
""",
}
TOP = """import pycircuit as ac
from enums.facade2 import Exported as Reexported
from enums.peer import State as Peer
from enums.types import Wide, Huge
from enums.records import Packet
@ac.module
def Identity(value: Reexported) -> {"out": Reexported}:
    alias = value
    return {"out": alias}
@ac.module
def Top(left: Reexported, right: Peer, wide: Wide, huge: Huge, packet: Packet) -> {"out_left": Reexported, "out_right": Peer, "out_wide": Wide, "out_huge": Huge, "out_packet": Packet}:
    return {"out_left": left, "out_right": right, "out_wide": wide, "out_huge": huge, "out_packet": packet}
"""
PREFIX = "from enum import Enum, auto\nimport pycircuit as ac\n"
GOOD_GUARD = (
    PREFIX
    + "@ac.encoding(width=3)\nclass State(Enum):\n    FIRST = 0\n    SECOND = 3\n"
)


CONSTRUCTOR_CONTROL = (
    GOOD_GUARD
    + """from pycircuit import module, rule
@module
def Leaf() -> {"out": bool}:
    return {"out": False}
@module
def Top() -> {"out": bool}:
    child = Leaf()
    @rule
    def bind():
        child()
    bind()
    return {"out": child.out}
"""
)


def declaration(
    decorator="@ac.encoding(width=3)",
    base="Enum",
    body="    FIRST = 0\n    SECOND = 3\n",
    prefix=PREFIX,
):
    return prefix + decorator + "\nclass State(" + base + "):\n" + body


# Each diagnostic is tied to the owning syntax/static/nominal guard; matching a
# generic top-level or imported-module failure would not establish this matrix.
NEGATIVES = [
    (
        "missing-width",
        declaration("@ac.encoding()"),
        "encoding requires an explicit width",
    ),
    (
        "missing-decorator",
        PREFIX + "class State(Enum):\n    FIRST = 0\n",
        "enum requires exactly one encoding decorator",
    ),
    (
        "decorator-spread",
        declaration("@ac.encoding(width=3, **options)"),
        "encoding requires distinct named keywords",
    ),
    (
        "unknown-kind",
        declaration('@ac.encoding(width=3, kind="ordinal")'),
        "unknown enum encoding kind",
    ),
    (
        "kind-expression",
        declaration("@ac.encoding(width=3, kind=1)"),
        "encoding kind requires a literal string",
    ),
    (
        "positional-decorator",
        declaration("@ac.encoding(3)"),
        "enum requires a bound encoding decorator with keyword arguments",
    ),
    (
        "extra-keyword",
        declaration("@ac.encoding(width=3, extra=1)"),
        "unsupported encoding keyword 'extra'",
    ),
    (
        "two-decorators",
        declaration("@ac.encoding(width=3)\n@ac.encoding(width=3)"),
        "enum requires exactly one encoding decorator",
    ),
    ("mixin", declaration(base="int, Enum"), "enum requires one bound Enum base"),
    (
        "metaclass",
        declaration(base="Enum, metaclass=type"),
        "enum requires one bound Enum base",
    ),
    (
        "method",
        declaration(body="    FIRST = 0\n    def calculate(self):\n        return 1\n"),
        "enum body requires single-name member assignments",
    ),
    (
        "ignore-hook",
        declaration(body='    _ignore_ = "FIRST"\n    FIRST = 0\n'),
        "enum construction hooks are unsupported",
    ),
    (
        "next-value-hook",
        declaration(
            body="    def _generate_next_value_(name, start, count, last):\n        return count\n    FIRST = 0\n"
        ),
        "enum body requires single-name member assignments",
    ),
    (
        "duplicate-name",
        declaration(body="    FIRST = 0\n    FIRST = 1\n"),
        "enum member names must be valid and unique",
    ),
    (
        "empty-enum",
        declaration(body='    """No member is declared."""\n'),
        "enum requires at least one member",
    ),
    (
        "chained-member",
        declaration(body="    FIRST = SECOND = 1\n"),
        "enum body requires single-name member assignments",
    ),
    (
        "tuple-member",
        declaration(body="    FIRST, SECOND = 0, 1\n"),
        "enum body requires single-name member assignments",
    ),
    (
        "annotated-member",
        declaration(body="    FIRST: int = 0\n"),
        "enum body requires single-name member assignments",
    ),
    (
        "explicit-auto",
        declaration(body="    FIRST = auto()\n"),
        "explicit enum requires Integer expressions",
    ),
    (
        "derived-number",
        declaration('@ac.encoding(width=3, kind="binary_sequential")'),
        "derived encoding requires uniformly bound auto()",
    ),
    (
        "mixed-derived",
        declaration(
            '@ac.encoding(width=3, kind="gray_sequential")',
            body="    FIRST = auto()\n    SECOND = 1\n",
        ),
        "derived encoding requires uniformly bound auto()",
    ),
    (
        "auto-positional",
        declaration(
            '@ac.encoding(width=3, kind="binary_one_hot")', body="    FIRST = auto(1)\n"
        ),
        "enum auto requires an argument-free call",
    ),
    (
        "auto-keyword",
        declaration(
            '@ac.encoding(width=3, kind="binary_sequential")',
            body="    FIRST = auto(value=1)\n",
        ),
        "enum auto requires an argument-free call",
    ),
    (
        "unclosed-width",
        declaration("@ac.encoding(width=unbound_width)"),
        "enum width and codes require closed Integer expressions",
    ),
    (
        "unclosed-member",
        declaration(body="    FIRST = unbound_code\n"),
        "enum width and codes require closed Integer expressions",
    ),
    (
        "width-zero",
        declaration("@ac.encoding(width=0)"),
        "enum width must be positive and fit u64",
    ),
    (
        "width-negative",
        declaration("@ac.encoding(width=0-1)"),
        "enum width must be positive and fit u64",
    ),
    (
        "width-bool",
        declaration("@ac.encoding(width=True)"),
        "enum width and codes require mathematical Integer, never Boolean",
    ),
    (
        "member-bool",
        declaration(body="    FIRST = True\n"),
        "enum width and codes require mathematical Integer, never Boolean",
    ),
    (
        "member-negative",
        declaration(body="    FIRST = 0-1\n"),
        "enum member code must be nonnegative",
    ),
    (
        "duplicate-code",
        declaration(body="    FIRST = 3\n    SECOND = 1+2\n"),
        "enum member codes must be unique",
    ),
    (
        "overwide-code",
        declaration(body="    FIRST = 1<<3\n"),
        "enum member code does not fit declared width",
    ),
    (
        "overwide65-code",
        declaration("@ac.encoding(width=65)", body="    FIRST = 1<<65\n"),
        "enum member code does not fit declared width",
    ),
    (
        "overwide130-code",
        declaration("@ac.encoding(width=130)", body="    FIRST = 1<<130\n"),
        "enum member code does not fit declared width",
    ),
    (
        "derived-overwide",
        declaration(
            '@ac.encoding(width=2, kind="binary_one_hot")',
            body="    FIRST = auto()\n    SECOND = auto()\n    THIRD = auto()\n",
        ),
        "enum member code does not fit declared width",
    ),
    (
        "unbound-enum",
        declaration(prefix="import pycircuit as ac\n"),
        "enum requires one bound Enum base",
    ),
    (
        "unbound-encoding",
        declaration("@encoding(width=3)", prefix="from enum import Enum\n"),
        "enum requires a bound encoding decorator",
    ),
    (
        "unbound-auto",
        declaration(
            '@ac.encoding(width=3, kind="binary_sequential")',
            body="    FIRST = auto()\n",
            prefix="from enum import Enum\nimport pycircuit as ac\n",
        ),
        "derived encoding requires uniformly bound auto()",
    ),
    (
        "fake-enum",
        declaration(
            prefix="from enums.unrelated import Enum\nimport pycircuit as ac\n"
        ),
        "enum requires one bound Enum base",
    ),
    (
        "fake-encoding",
        declaration(
            "@encoding(width=3)",
            prefix="from enum import Enum\nfrom enums.unrelated import encoding\n",
        ),
        "enum requires a bound encoding decorator",
    ),
    (
        "fake-auto",
        declaration(
            '@ac.encoding(width=3, kind="binary_sequential")',
            body="    FIRST = auto()\n",
            prefix="from enum import Enum\nfrom enums.unrelated import auto\nimport pycircuit as ac\n",
        ),
        "derived encoding requires uniformly bound auto()",
    ),
]
NEGATIVES.append(
    (
        "conflicting-import",
        declaration(
            prefix="from enum import Enum\nfrom pycircuit import module as Enum\nimport pycircuit as ac\n"
        ),
        "duplicate or conflicting import binding",
    )
)
for source_type, destination_type in (("Left", "Right"), ("Right", "Left")):
    NEGATIVES.append(
        (
            f"nominal-{source_type.lower()}-to-{destination_type.lower()}",
            "import pycircuit as ac\nfrom enums.facade2 import Exported as Left\nfrom enums.peer import State as Right\n"
            + f'@ac.module\ndef Top(value: {source_type}) -> {{"out": {destination_type}}}:\n    return {{"out": value}}\n',
            "nominal enum boundary hardware type mismatch",
        )
    )
SHADOWS = [
    ("rebound-enum", declaration(prefix=PREFIX + "Enum = 3\n")),
    ("rebound-namespace", declaration(prefix=PREFIX + "ac = 3\n")),
    ("class-auto-shadow", declaration(body="    auto = 0\n    FIRST = auto()\n")),
    (
        "class-enum-shadow",
        PREFIX + "@ac.encoding(width=3)\nclass Enum(Enum):\n    FIRST = 0\n",
    ),
    (
        "parameter-nominal-shadow",
        GOOD_GUARD
        + '@ac.module\ndef Top(State: bool, value: State) -> {"out": State}:\n    return {"out": value}\n',
    ),
    (
        "parameter-namespace-shadow",
        GOOD_GUARD
        + '@ac.module\ndef Top(ac: bool, value: ac.bits[1]) -> {"out": ac.bits[1]}:\n    return {"out": value}\n',
    ),
    (
        "local-nominal-shadow",
        GOOD_GUARD
        + '@ac.module\ndef Top(value: State) -> {"out": State}:\n    State = value\n    alias: State = value\n    return {"out": alias}\n',
    ),
]
shadow_diagnostics = {
    "rebound-enum": "source binding is shadowed at top level",
    "rebound-namespace": "source binding is shadowed at top level",
    "class-auto-shadow": "source binding is shadowed in its lexical scope",
    "class-enum-shadow": "nominal declaration shadows an existing binding",
    "parameter-nominal-shadow": "module input shadows a source declaration or namespace",
    "parameter-namespace-shadow": "module input shadows a source declaration or namespace",
    "local-nominal-shadow": "assignment shadows a source declaration",
}
SHADOWS = [(name, text, shadow_diagnostics[name]) for name, text in SHADOWS]
for category, target in (
    ("namespace", "ac"),
    ("marker", "module"),
    ("nominal", "State"),
):
    SHADOWS.append(
        (
            "constructor-" + category + "-shadow",
            CONSTRUCTOR_CONTROL.replace("child", target),
            "module instance shadows a source binding",
        )
    )
sentinel = build / "source-executed"
NEGATIVES += [
    (
        "executed-member",
        declaration(body=f"    FIRST = open({str(sentinel)!r}, 'w').write('member')\n"),
        "unsupported static expression",
    ),
    (
        "executed-decorator",
        declaration(
            f"@ac.encoding(width=open({str(sentinel)!r}, 'w').write('decorator'))"
        ),
        "unsupported static expression",
    ),
]
negative_sources = build / "negative-sources"
negative_sources.mkdir()
for name, text, _ in NEGATIVES:
    (negative_sources / (name + ".py")).write_text(text)
for name, text, _ in SHADOWS:
    (negative_sources / (name + ".py")).write_text(text)
for name, text in PROVIDERS.items():
    (source / name).write_text(text)
(source / "top.py").write_text(TOP)
(source / "guard.py").write_text(GOOD_GUARD)
prepared = {
    "scope": "initial S3a declarations/import/public compile-link only",
    "artifact_directory": str(build),
    "source_hashes": {
        name: digest(source / name) for name in (*PROVIDERS, "top.py", "guard.py")
    },
    "source_text": {
        name: (source / name).read_text() for name in (*PROVIDERS, "top.py", "guard.py")
    },
    "negative_source_hashes": {
        str(p): digest(p) for p in negative_sources.glob("*.py")
    },
    "enum_schema": SCHEMA,
    "negative_cases": [name for name, _, _ in NEGATIVES],
    "shadow_cases": [name for name, _, _ in SHADOWS],
}
(evidence / "prepared.json").write_text(json.dumps(prepared, indent=2) + "\n")
if args.prepare_only:
    print(build)  # noqa: T201 - CLI fixture preparation status
    sys.exit(0)

if args.case_family == "table-query":
    from enum_source_execution import execution_tools

    fixtures = Path(__file__).resolve().parent
    query_directory = fixtures / "table-queries"
    sys.path.insert(0, str(query_directory))
    from table_query_cases import execution_cases

    runtime_root = Path(args.source_compiler).resolve().parent.parent
    runtime = next(
        p
        for p in (
            runtime_root / "runtime/libpyc6_runtime.a",
            runtime_root / "lib/libpyc6_runtime.a",
        )
        if p.is_file()
    )
    _, execute = execution_tools(
        args=args,
        repo=repo,
        fixtures=fixtures,
        build=build,
        source=source,
        units={},
        declaration_sources=(),
        runtime=runtime,
        compile_unit=compile_unit,
        cli=cli,
        run=run,
        payload=payload,
        digest=digest,
        clock="pyc_7079635f636c6b",
    )
    cases = execution_cases()
    receipts = [execute(**case) for case in cases]
    files = [
        Path(__file__),
        fixtures / "enum_source_execution.py",
        fixtures / "enum-source.cpp",
        fixtures / "enum-source.sv",
        query_directory / "table_query_cases.py",
    ]
    (evidence / "candidate.json").write_text(
        json.dumps(
            {
                "scope": "table-query case family through existing generic Enum execution owner",
                "artifact_directory": str(build),
                "cases": receipts,
                "fixtures": {str(p): digest(p) for p in files},
                "tools": {
                    str(Path(p).resolve()): digest(Path(p))
                    for p in (
                        args.source_compiler,
                        args.linker,
                        args.optimizer,
                        args.emitter,
                        args.cxx,
                        args.verilator,
                        args.iverilog,
                        args.vvp,
                    )
                },
                "runtime": {str(runtime): digest(runtime)},
                "limits": "Query selection/read equivalence; no address separation or reservations",
            },
            indent=2,
        )
        + "\n"
    )
    print(
        "Table query case family passed: existing public flow, native1/2 and genuine RTL"
    )  # noqa: T201
    sys.exit(0)

units = {}
declaration_sources = (
    "types.py",
    "peer.py",
    "records.py",
    "empty.py",
    "facade.py",
    "facade2.py",
)
for name in (*declaration_sources, "unrelated.py"):
    output = build / name.removesuffix(".py")
    imports = (
        [units["types.py"]]
        if name == "facade.py"
        else [units["types.py"], units["facade.py"]] if name == "facade2.py" else []
    )
    compile_unit(name, output, imports)
    units[name] = output
    body, header = assert_unit(
        output, "declarations" if name in declaration_sources else "implementation"
    )
    if name == "types.py":
        assert enum_schema(body) == SCHEMA and enum_schema(header) == SCHEMA
    if name == "peer.py":
        assert enum_schema(body) == {"enums.peer.State": SCHEMA["enums.types.State"]}
    # Removing providers before subsequent imports proves headers own authority.
    (source / name).unlink()

top_unit = build / "top"
compile_unit("top.py", top_unit, list(units.values()))
units["top.py"] = top_unit
top_body, top_header = assert_unit(top_unit, "implementation")
expected_closure = {
    "top.py",
    "types.py",
    "peer.py",
    "records.py",
    "facade.py",
    "facade2.py",
}
assert set(owner_paths(top_header, "ac.interfaces")) == expected_closure, top_header
assert "unrelated.py" not in top_header and "empty.py" not in top_header, top_header
assert enum_schema(top_body) == {
    **SCHEMA,
    "enums.peer.State": SCHEMA["enums.types.State"],
}, top_body
assert (
    '!ac.enum<"enums.types.State">' in top_body
    and '!ac.enum<"enums.peer.State">' in top_body
), top_body
assert (
    "enums.top.Reexported" not in top_body and "enums.facade2.Exported" not in top_body
), top_body

closure = [units[name] for name in (*declaration_sources, "top.py")]
final = build / "design.ac"
cli("link", *reversed(closure), "--top", "enums.top.Top", "-o", final)
run([args.optimizer, final, "--ac-verify-hardware", "-o", build / "verified.ac"])
final_text = final.read_text()
assert enum_schema(final_text) == {
    **SCHEMA,
    "enums.peer.State": SCHEMA["enums.types.State"],
}, final_text
assert set(owner_paths(final_text, "ac.source_units")) == {
    *declaration_sources,
    "top.py",
}, final_text
for line in final_text.splitlines():
    if '"ac.enum"()' in line:
        assert 'ac.declaration_role = "definition"' in line, line
before_final = managed_snapshot(final)
owner_path = final.parent / ("." + final.name + ".pycircuit-publication") / "owner.json"
assert json.loads(owner_path.read_text()) == {
    "kind": "pycircuit-publication-control",
    "destination": final.name,
}
root_line = next(
    line for line in final_text.splitlines() if 'sym_name = "enums.top.Top"' in line
)
assert 'source_owner = {package = "enums", path = "top.py"}' in root_line, root_line
before_units = {name: managed_snapshot(unit) for name, unit in units.items()}

for case, supplied, top, diagnostic in (
    (
        "missing-provider",
        [u for u in closure if u != units["types.py"]],
        "enums.top.Top",
        "ac.interfaces names a dependency without an explicitly supplied header",
    ),
    (
        "enum-is-not-top",
        closure,
        "enums.types.State",
        "link top is not a supplied module definition",
    ),
):
    absent = build / (case + ".ac")
    cli("link", *supplied, "--top", top, "-o", absent, code=1, diagnostic=diagnostic)
    assert not absent.exists(), absent
    cli(
        "link",
        *supplied,
        "--top",
        top,
        "-o",
        final,
        "--replace",
        code=1,
        diagnostic=diagnostic,
    )
    assert managed_snapshot(final) == before_final
    assert {
        name: managed_snapshot(unit) for name, unit in units.items()
    } == before_units

# The same bound child instance is legal with an ordinary local name. Its
# successful real source-unit and linked program are also replacement targets
# for the corresponding constructor shadow regressions.
(source / "guard.py").write_text(CONSTRUCTOR_CONTROL)
constructor_unit = build / "constructor-control"
compile_unit("guard.py", constructor_unit)
assert_unit(constructor_unit, "implementation")
constructor_final = build / "constructor.ac"
cli("link", constructor_unit, "--top", "enums.guard.Top", "-o", constructor_final)
run(
    [
        args.optimizer,
        constructor_final,
        "--ac-verify-hardware",
        "-o",
        build / "constructor-verified.ac",
    ]
)
constructor_before = managed_snapshot(constructor_unit)
constructor_final_before = managed_snapshot(constructor_final)
(source / "guard.py").write_text(GOOD_GUARD)

protected = build / "protected-unit"
compile_unit("guard.py", protected)
protected_bytes = managed_snapshot(protected)
negative_receipts = []
for name, text, diagnostic in [*NEGATIVES, *SHADOWS]:
    (source / "guard.py").write_text(text)
    absent = build / ("absent-" + name)
    fresh = compile_unit(
        "guard.py", absent, list(units.values()), code=1, diagnostic=diagnostic
    )
    assert not absent.exists() and not sentinel.exists(), name
    replacement = constructor_unit if name.startswith("constructor-") else protected
    replaced = compile_unit(
        "guard.py",
        replacement,
        list(units.values()),
        replace=True,
        code=1,
        diagnostic=diagnostic,
    )
    assert (
        managed_snapshot(protected) == protected_bytes
        and managed_snapshot(final) == before_final
        and not sentinel.exists()
    ), name
    assert (
        managed_snapshot(constructor_unit) == constructor_before
        and managed_snapshot(constructor_final) == constructor_final_before
    ), name
    negative_receipts.append(
        {
            "case": name,
            "source": str(negative_sources / (name + ".py")),
            "diagnostic": diagnostic,
            "fresh_exit_status": fresh.returncode,
            "replacement_exit_status": replaced.returncode,
            "replacement_artifact": str(replacement),
        }
    )

(evidence / "candidate.json").write_text(
    json.dumps(
        {
            **prepared,
            "fixtures": {
                str(Path(__file__).resolve()): digest(Path(__file__)),
                str(
                    Path(__file__).resolve().parent.parent / "enum-source.test"
                ): digest(Path(__file__).resolve().parent.parent / "enum-source.test"),
            },
            "tools": {
                str(Path(p).resolve()): digest(Path(p))
                for p in (args.source_compiler, args.linker, args.optimizer)
            },
            "units": {
                name: {
                    kind: digest(payload(unit, kind)) for kind in ("body", "interface")
                }
                for name, unit in units.items()
            },
            "final_sha256": digest(final),
            "publication_control_sha256": digest(owner_path),
            "common_ir_verify_exit_status": 0,
            "negative_results": negative_receipts,
            "constructor_control": {
                "source": CONSTRUCTOR_CONTROL,
                "unit": str(constructor_unit),
                "files": {
                    kind: digest(payload(constructor_unit, kind))
                    for kind in ("body", "interface")
                },
                "final_sha256": digest(constructor_final),
                "common_ir_verify_exit_status": 0,
            },
            "link_negative_cases": ["missing-provider", "enum-is-not-top"],
            "deferred": "member values/defaults/conversions/tuple binding, emitted source grouping, dual-backend runtime execution",
        },
        indent=2,
    )
    + "\n"
)
print(
    "initial Enum source gate passed: declarations/aliases/reexports/nominality, complete public link and protected failures"
)  # noqa: T201

B3_CASES = [
    {
        "name": "op-00",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state + state}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-01",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state + integer}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-02",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": integer + state}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-03",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state + boolean}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-04",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": boolean + state}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-05",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state + fixed}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-06",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": fixed + state}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "unsigned arithmetic operands require equal widths",
    },
    {
        "name": "op-07",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state - state}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-08",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state - integer}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-09",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": integer - state}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-10",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state - boolean}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-11",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": boolean - state}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-12",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state - fixed}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-13",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": fixed - state}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "unsigned arithmetic operands require equal widths",
    },
    {
        "name": "op-14",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state * state}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-15",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state * integer}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-16",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": integer * state}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-17",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state * boolean}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-18",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": boolean * state}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-19",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state * fixed}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "Enum arithmetic requires explicit enum_to_bits",
    },
    {
        "name": "op-20",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": fixed * state}\n',
        "positive": False,
        "category": "arithmetic",
        "expected_diagnostic": "unsigned arithmetic operands require equal widths",
    },
    {
        "name": "op-21",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state & state}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-22",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state & integer}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-23",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": integer & state}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-24",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state & boolean}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-25",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": boolean & state}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-26",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state & fixed}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-27",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": fixed & state}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-28",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state | state}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-29",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state | integer}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-30",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": integer | state}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-31",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state | boolean}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-32",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": boolean | state}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-33",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state | fixed}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-34",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": fixed | state}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-35",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state ^ state}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-36",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state ^ integer}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-37",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": integer ^ state}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-38",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state ^ boolean}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-39",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": boolean ^ state}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-40",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state ^ fixed}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "op-41",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": fixed ^ state}\n',
        "positive": False,
        "category": "bitwise",
        "expected_diagnostic": "Enum bitwise operations require explicit enum_to_bits",
    },
    {
        "name": "compare-42",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state == other}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-43",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": other == state}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-44",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state == integer}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-45",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": integer == state}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-46",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state == boolean}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-47",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": boolean == state}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-48",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state == fixed}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-49",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": fixed == state}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-50",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state == 0}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "compare-51",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": 0 == state}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "compare-52",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state == True}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "compare-53",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": True == state}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "control-eq-True",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state == State.ONE}\n',
        "positive": True,
        "category": "same nominal comparison",
        "expected_diagnostic": None,
    },
    {
        "name": "compare-55",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state != other}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-56",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": other != state}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-57",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state != integer}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-58",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": integer != state}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-59",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state != boolean}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-60",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": boolean != state}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-61",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state != fixed}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-62",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": fixed != state}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "compare-63",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state != 0}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "compare-64",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": 0 != state}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "compare-65",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state != True}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "compare-66",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": True != state}\n',
        "positive": False,
        "category": "mixed comparison",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "control-eq-False",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state != State.ONE}\n',
        "positive": True,
        "category": "same nominal comparison",
        "expected_diagnostic": None,
    },
    {
        "name": "order-68",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state < state}\n',
        "positive": False,
        "category": "order",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "order-69",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state <= state}\n',
        "positive": False,
        "category": "order",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "order-70",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state > state}\n',
        "positive": False,
        "category": "order",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "order-71",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state >= state}\n',
        "positive": False,
        "category": "order",
        "expected_diagnostic": "Enum comparison requires eq/ne and the same nominal type",
    },
    {
        "name": "truth-72",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": not state}\n',
        "positive": False,
        "category": "truthiness",
        "expected_diagnostic": "Enum unary operations require explicit enum_to_bits",
    },
    {
        "name": "truth-73",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": ~state}\n',
        "positive": False,
        "category": "truthiness",
        "expected_diagnostic": "Enum unary operations require explicit enum_to_bits",
    },
    {
        "name": "truth-74",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state and boolean}\n',
        "positive": False,
        "category": "truthiness",
        "expected_diagnostic": "conditional requires authoritative Boolean or bits[1]",
    },
    {
        "name": "truth-75",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": boolean and state}\n',
        "positive": False,
        "category": "truthiness",
        "expected_diagnostic": "conditional requires authoritative Boolean or bits[1]",
    },
    {
        "name": "truth-76",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": state or boolean}\n',
        "positive": False,
        "category": "truthiness",
        "expected_diagnostic": "conditional requires authoritative Boolean or bits[1]",
    },
    {
        "name": "truth-77",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": boolean or state}\n',
        "positive": False,
        "category": "truthiness",
        "expected_diagnostic": "conditional requires authoritative Boolean or bits[1]",
    },
    {
        "name": "truth-78",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": True if state else False}\n',
        "positive": False,
        "category": "truthiness",
        "expected_diagnostic": "conditional requires authoritative Boolean or bits[1]",
    },
    {
        "name": "control-not",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": not boolean}\n',
        "positive": True,
        "category": "truthiness control",
        "expected_diagnostic": None,
    },
    {
        "name": "control-and",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": boolean and boolean}\n',
        "positive": True,
        "category": "truthiness control",
        "expected_diagnostic": None,
    },
    {
        "name": "control-select",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": bool}:\n    return {"out": True if boolean else False}\n',
        "positive": True,
        "category": "truthiness control",
        "expected_diagnostic": None,
    },
    {
        "name": "control-bits-83",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": ac.enum_to_bits(state) + fixed}\n',
        "positive": True,
        "category": "explicit conversion control",
        "expected_diagnostic": None,
    },
    {
        "name": "control-bits-84",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": ac.enum_to_bits(state) - fixed}\n',
        "positive": True,
        "category": "explicit conversion control",
        "expected_diagnostic": None,
    },
    {
        "name": "control-bits-85",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": ac.enum_to_bits(state) * fixed}\n',
        "positive": True,
        "category": "explicit conversion control",
        "expected_diagnostic": None,
    },
    {
        "name": "control-bits-86",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": ac.enum_to_bits(state) & fixed}\n',
        "positive": True,
        "category": "explicit conversion control",
        "expected_diagnostic": None,
    },
    {
        "name": "control-bits-87",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": ac.enum_to_bits(state) | fixed}\n',
        "positive": True,
        "category": "explicit conversion control",
        "expected_diagnostic": None,
    },
    {
        "name": "control-bits-88",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": ac.enum_to_bits(state) ^ fixed}\n',
        "positive": True,
        "category": "explicit conversion control",
        "expected_diagnostic": None,
    },
    {
        "name": "to-bits-missing",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": ac.enum_to_bits()}\n',
        "positive": False,
        "category": "toBits invalid arguments",
        "expected_diagnostic": "enum_to_bits requires exactly one positional Enum value",
    },
    {
        "name": "to-bits-extra",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": ac.enum_to_bits(state, state)}\n',
        "positive": False,
        "category": "toBits invalid arguments",
        "expected_diagnostic": "enum_to_bits requires exactly one positional Enum value",
    },
    {
        "name": "to-bits-keyword",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": ac.enum_to_bits(value=state)}\n',
        "positive": False,
        "category": "toBits invalid arguments",
        "expected_diagnostic": "enum_to_bits requires exactly one positional Enum value",
    },
    {
        "name": "to-bits-integer",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": ac.enum_to_bits(integer)}\n',
        "positive": False,
        "category": "toBits invalid arguments",
        "expected_diagnostic": "enum_to_bits requires a nominal Enum value",
    },
    {
        "name": "to-bits-boolean",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": ac.enum_to_bits(boolean)}\n',
        "positive": False,
        "category": "toBits invalid arguments",
        "expected_diagnostic": "enum_to_bits requires a nominal Enum value",
    },
    {
        "name": "to-bits-fixed",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": ac.enum_to_bits(fixed)}\n',
        "positive": False,
        "category": "toBits invalid arguments",
        "expected_diagnostic": "enum_to_bits requires a nominal Enum value",
    },
    {
        "name": "control-to-bits-exact",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top(state: State, other: Other, integer: Annotated[int, range(1 << 2)], boolean: bool, fixed: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": ac.enum_to_bits(state)}\n',
        "positive": True,
        "category": "toBits control",
        "expected_diagnostic": None,
    },
    {
        "name": "init-zero",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top() -> {"out": State}:\n    value: State = 0\n    return {"out": value}\n',
        "positive": False,
        "category": "explicit Enum init",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "ctor-zero",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: State\n@ac.module\ndef Top() -> Entry:\n    return Entry(state=0)\n",
        "positive": False,
        "category": "explicit Enum field constructor",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "init-false",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top() -> {"out": State}:\n    value: State = False\n    return {"out": value}\n',
        "positive": False,
        "category": "explicit Enum init",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "ctor-false",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: State\n@ac.module\ndef Top() -> Entry:\n    return Entry(state=False)\n",
        "positive": False,
        "category": "explicit Enum field constructor",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "init-true",
        "source": 'from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.module\ndef Top() -> {"out": State}:\n    value: State = True\n    return {"out": value}\n',
        "positive": False,
        "category": "explicit Enum init",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "ctor-true",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: State\n@ac.module\ndef Top() -> Entry:\n    return Entry(state=True)\n",
        "positive": False,
        "category": "explicit Enum field constructor",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "default-unused-0",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: State = 0\n",
        "positive": False,
        "category": "unused invalid declared default",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "default-overridden-0",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: State = 0\n@ac.module\ndef Top() -> Entry:\n    return Entry(state=State.ONE)\n",
        "positive": False,
        "category": "overridden invalid declared default",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "default-unused-False",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: State = False\n",
        "positive": False,
        "category": "unused invalid declared default",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "default-overridden-False",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: State = False\n@ac.module\ndef Top() -> Entry:\n    return Entry(state=State.ONE)\n",
        "positive": False,
        "category": "overridden invalid declared default",
        "expected_diagnostic": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    },
    {
        "name": "default-unused-Other-ZERO",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: State = Other.ZERO\n",
        "positive": False,
        "category": "unused invalid declared default",
        "expected_diagnostic": "unsigned boundary hardware type mismatch",
    },
    {
        "name": "default-overridden-Other-ZERO",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: State = Other.ZERO\n@ac.module\ndef Top() -> Entry:\n    return Entry(state=State.ONE)\n",
        "positive": False,
        "category": "overridden invalid declared default",
        "expected_diagnostic": "unsigned boundary hardware type mismatch",
    },
    {
        "name": "control-nozero-declaration",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: NoZero\n",
        "positive": True,
        "category": "nozero declaration legal",
        "expected_diagnostic": None,
    },
    {
        "name": "nozero-omitted",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: NoZero\n@ac.module\ndef Top() -> Entry:\n    return Entry()\n",
        "positive": False,
        "category": "omitted field recursive zero",
        "expected_diagnostic": "recursive Enum zero requires a declared zero-code member",
    },
    {
        "name": "control-nozero-explicit",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: NoZero\n@ac.module\ndef Top() -> Entry:\n    return Entry(state=NoZero.SECOND)\n",
        "positive": True,
        "category": "nozero constructor explicit",
        "expected_diagnostic": None,
    },
    {
        "name": "table-State-zero",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: State = State.ONE\n@ac.module\ndef Top(index: ac.u1) -> Entry:\n    values = ac.table[2, Entry](init=0)\n    return values[index]\n",
        "positive": True,
        "category": "table zero/default distinction",
        "expected_diagnostic": None,
    },
    {
        "name": "table-State-constructor",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: State = State.ONE\n@ac.module\ndef Top(index: ac.u1) -> Entry:\n    values = ac.table[2, Entry](init=Entry())\n    return values[index]\n",
        "positive": True,
        "category": "table zero/default distinction",
        "expected_diagnostic": None,
    },
    {
        "name": "table-NoZero-zero",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: NoZero = NoZero.SECOND\n@ac.module\ndef Top(index: ac.u1) -> Entry:\n    values = ac.table[2, Entry](init=0)\n    return values[index]\n",
        "positive": False,
        "category": "table zero/default distinction",
        "expected_diagnostic": "recursive Enum zero requires a declared zero-code member",
    },
    {
        "name": "table-NoZero-constructor",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Entry:\n    state: NoZero = NoZero.SECOND\n@ac.module\ndef Top(index: ac.u1) -> Entry:\n    values = ac.table[2, Entry](init=Entry())\n    return values[index]\n",
        "positive": True,
        "category": "table zero/default distinction",
        "expected_diagnostic": None,
    },
    {
        "name": "control-local-enum-bool",
        "source": 'from enum import Enum\nimport pycircuit as ac\n@ac.encoding(width=1)\nclass bool(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.module\ndef Top(value: bool) -> {"out": bool}:\n    state: bool = bool.ZERO\n    return {"out": value if ac.enum_to_bits(state) == 0 else state}\n',
        "positive": True,
        "category": "nominal bool binding",
        "expected_diagnostic": None,
    },
    {
        "name": "control-struct-bool",
        "source": "import pycircuit as ac\n@ac.struct\nclass bool:\n    value: ac.bits[2] = 2\n@ac.module\ndef Top() -> bool:\n    result: bool = bool()\n    return result\n",
        "positive": True,
        "category": "nominal bool binding",
        "expected_diagnostic": None,
    },
    {
        "name": "alias-provider",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n",
        "positive": True,
        "category": "import bool provider",
        "expected_diagnostic": None,
    },
    {
        "name": "control-import-enum-bool",
        "source": 'import pycircuit as ac\nfrom independent.alias_provider import State as bool\n@ac.module\ndef Top(value: bool) -> {"out": bool}:\n    state: bool = bool.ONE\n    return {"out": value}\n',
        "positive": True,
        "category": "nominal bool binding",
        "expected_diagnostic": None,
    },
    {
        "name": "rule-if-state",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Result:\n    value: ac.bits[1]\n@ac.rule\ndef evaluate(state: State, boolean: bool) -> Result:\n    if state:\n        out = True\n    else:\n        out = False\n    return Result(value=out)\n\n@ac.module\ndef Top(state: State, boolean: bool) -> Result:\n    return evaluate(state, boolean)\n",
        "positive": False,
        "category": "truthiness rule condition",
        "expected_diagnostic": "conditional requires authoritative Boolean or bits[1]",
    },
    {
        "name": "rule-if-boolean",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Result:\n    value: ac.bits[1]\n@ac.rule\ndef evaluate(state: State, boolean: bool) -> Result:\n    if boolean:\n        out = True\n    else:\n        out = False\n    return Result(value=out)\n\n@ac.module\ndef Top(state: State, boolean: bool) -> Result:\n    return evaluate(state, boolean)\n",
        "positive": True,
        "category": "truthiness rule condition",
        "expected_diagnostic": None,
    },
    {
        "name": "rule-tobits-local-1",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Result:\n    value: ac.bits[1]\n@ac.rule\ndef evaluate(state: State, boolean: bool) -> Result:\n    raw: ac.bits[1] = ac.enum_to_bits(state)\n    return Result(value=raw)\n\n@ac.module\ndef Top(state: State, boolean: bool) -> Result:\n    return evaluate(state, boolean)\n",
        "positive": False,
        "category": "toBits rule local width boundary",
        "expected_diagnostic": "unsigned boundary implicit narrowing is unsupported",
    },
    {
        "name": "rule-tobits-field-1",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Result:\n    value: ac.bits[1]\n@ac.rule\ndef evaluate(state: State, boolean: bool) -> Result:\n    return Result(value=ac.enum_to_bits(state))\n\n@ac.module\ndef Top(state: State, boolean: bool) -> Result:\n    return evaluate(state, boolean)\n",
        "positive": False,
        "category": "toBits struct field width boundary",
        "expected_diagnostic": "unsigned boundary implicit narrowing is unsupported",
    },
    {
        "name": "rule-tobits-local-2",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Result:\n    value: ac.bits[2]\n@ac.rule\ndef evaluate(state: State, boolean: bool) -> Result:\n    raw: ac.bits[2] = ac.enum_to_bits(state)\n    return Result(value=raw)\n\n@ac.module\ndef Top(state: State, boolean: bool) -> Result:\n    return evaluate(state, boolean)\n",
        "positive": True,
        "category": "toBits rule local width boundary",
        "expected_diagnostic": None,
    },
    {
        "name": "rule-tobits-field-2",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Result:\n    value: ac.bits[2]\n@ac.rule\ndef evaluate(state: State, boolean: bool) -> Result:\n    return Result(value=ac.enum_to_bits(state))\n\n@ac.module\ndef Top(state: State, boolean: bool) -> Result:\n    return evaluate(state, boolean)\n",
        "positive": True,
        "category": "toBits struct field width boundary",
        "expected_diagnostic": None,
    },
    {
        "name": "rule-tobits-local-5",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Result:\n    value: ac.bits[5]\n@ac.rule\ndef evaluate(state: State, boolean: bool) -> Result:\n    raw: ac.bits[5] = ac.enum_to_bits(state)\n    return Result(value=raw)\n\n@ac.module\ndef Top(state: State, boolean: bool) -> Result:\n    return evaluate(state, boolean)\n",
        "positive": True,
        "category": "toBits rule local width boundary",
        "expected_diagnostic": None,
    },
    {
        "name": "rule-tobits-field-5",
        "source": "from enum import Enum\nfrom typing import Annotated\nimport pycircuit as ac\n@ac.encoding(width=2)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass Other(Enum):\n    ZERO = 0\n    ONE = 1\n@ac.encoding(width=2)\nclass NoZero(Enum):\n    FIRST = 1\n    SECOND = 2\n@ac.struct\nclass Result:\n    value: ac.bits[5]\n@ac.rule\ndef evaluate(state: State, boolean: bool) -> Result:\n    return Result(value=ac.enum_to_bits(state))\n\n@ac.module\ndef Top(state: State, boolean: bool) -> Result:\n    return evaluate(state, boolean)\n",
        "positive": True,
        "category": "toBits struct field width boundary",
        "expected_diagnostic": None,
    },
]

S3C_GUARDS = {
    "one-name-tuple": "enum_from_bits requires exactly two ordinary Names",
    "three-name-tuple": "enum_from_bits requires exactly two ordinary Names",
    "list-target": "enum_from_bits requires exactly two ordinary Names",
    "star-target": "enum_from_bits targets must be ordinary Names",
    "nested-target": "enum_from_bits targets must be ordinary Names",
    "duplicate-target": "enum_from_bits targets must be distinct Names",
    "reserved-clock": "enum_from_bits target shadows a reserved/source binding: 'pyc_clk'",
    "reserved-reset": "enum_from_bits target shadows a reserved/source binding: 'pyc_rst'",
    "shadow-enum": "enum_from_bits target shadows a reserved/source binding: 'State'",
    "shadow-marker": "enum_from_bits target shadows a reserved/source binding: 'decode'",
    "shadow-module": "enum_from_bits target shadows a reserved/source binding: 'Top'",
    "shadow-rule": "enum_from_bits target shadows a reserved/source binding: 'evaluate'",
    "scalar-call": "enum_from_bits requires exactly two ordinary Names",
    "return-tuple-call": "enum_from_bits requires one two-name Tuple target",
    "field-target": "enum_from_bits targets must be ordinary Names",
    "index-target": "enum_from_bits targets must be ordinary Names",
    "persistent-owner-target": "enum_from_bits target cannot write persistent owner formal 'value'",
    "readonly-owner-target": "enum_from_bits target cannot write persistent owner formal 'value'",
    "nonlocal-target": "nonlocal writes are forbidden",
    "structural-rebind": "enum_from_bits target cannot write an owner or shadow a binding: 'decoded'",
    "structural-input-shadow": "enum_from_bits target cannot write an owner or shadow a binding: 'raw'",
    "partial-carrier-read": "unknown hardware value 'decoded'",
    "partial-member-read": "unknown hardware value 'member'",
    "carrier-nominal-boundary": "binding boundary hardware type mismatch",
    "membership-integer-boundary": "binding boundary requires declared Integer source kind",
    "member-later-kind": "binding boundary requires declared Boolean source kind",
    "carrier-later-kind": "source Integer/Boolean literals cannot be implicitly converted to Enum",
    "both-arms-nominal-mismatch": "branch values require equal hardware types",
    "both-arms-member-kind-mismatch": "conditional branches have different known Boolean and Integer kinds",
    "atomic-second-boundary": "binding boundary hardware type mismatch",
    "raw-too-narrow": "'ac.enum.from_bits' op enum.from_bits input width must equal 2",
    "raw-too-wide": "'ac.enum.from_bits' op enum.from_bits input width must equal 2",
    "raw-integer": "enum_from_bits requires authoritative fixed bits",
    "raw-boolean": "enum_from_bits requires authoritative fixed bits",
    "raw-nominal": "enum_from_bits requires authoritative fixed bits",
    "marker-shadow-formal": "module input shadows a source declaration or namespace",
    "chained-tuple": "enum_from_bits requires one two-name Tuple target",
    "arbitrary-tuple-producer": "tuple/list literal requires an expected Table type",
    "untyped-producer": "enum_from_bits[Enum] requires exactly one positional bits value",
}

S3C = {
    "positive": {
        "tuple-module": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n\n@ac.struct\nclass TupleResult:\n    raw: ac.bits[2]\n    member: ac.u1\n    ignored_member_raw: ac.bits[2]\n    only_member: ac.u1\n    sparse_raw: ac.bits[3]\n    sparse_member: ac.u1\n    wide_raw: ac.bits[65]\n    wide_member: ac.u1\n    huge_raw: ac.bits[130]\n    huge_member: ac.u1\n\n@ac.module\ndef Top(raw: ac.bits[2], sparse: ac.bits[3], wide: ac.bits[65], huge: ac.bits[130]) -> TupleResult:\n    decoded, member = decode[State](raw)\n    kept, unused_member = ac.enum_from_bits[State](raw)\n    unused_carrier, only_member = decode[State](raw)\n    sparse_decoded, sparse_member = decode[Sparse](sparse)\n    wide_decoded, wide_member = decode[Wide](wide)\n    huge_decoded, huge_member = ac.enum_from_bits[Huge](huge)\n    return TupleResult(raw=ac.enum_to_bits(decoded), member=member,\n        ignored_member_raw=ac.enum_to_bits(kept), only_member=only_member,\n        sparse_raw=ac.enum_to_bits(sparse_decoded), sparse_member=sparse_member,\n        wide_raw=ac.enum_to_bits(wide_decoded), wide_member=wide_member,\n        huge_raw=ac.enum_to_bits(huge_decoded), huge_member=huge_member)\n",
        "raw-once": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.module\ndef Top(raw: ac.bits[3]) -> PairResult:\n    decoded, member = decode[State](raw[1:3])\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n",
        "tuple-rule": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    decoded, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
        "both-arms": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    if choose:\n        decoded, member = decode[State](raw)\n    else:\n        decoded, member = decode[State](alternate)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
        "partial-recovered": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    if choose:\n        decoded, member = decode[State](alternate)\n    decoded, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
        "existing-boundaries": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    decoded: State = State.ZERO\n    member: bool = False\n    decoded, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
        "pure-formal": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(value: State, raw: ac.bits[2]) -> PairResult:\n    value, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(value), member=member)\n\n@ac.module\ndef Top(value: State, raw: ac.bits[2]) -> PairResult:\n    return evaluate(value, raw)\n",
        "owner-lifecycle": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n\n@ac.struct\nclass OwnerResult:\n    prior: ac.bits[2]\n    proposed: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(owner, raw: ac.bits[2], enable) -> OwnerResult:\n    old = owner\n    decoded, member = decode[State](raw)\n    if enable:\n        owner = decoded\n    return OwnerResult(prior=ac.enum_to_bits(old), proposed=ac.enum_to_bits(owner), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], enable: ac.u1) -> OwnerResult:\n    state: State = State.ZERO\n    return evaluate(state, raw, enable)\n",
        "field-control": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.struct\nclass Box:\n    value: State = State.ZERO\n\n@ac.rule\ndef evaluate(raw: ac.bits[2]) -> PairResult:\n    box = Box()\n    decoded, member = decode[State](raw)\n    box.value = decoded\n    return PairResult(raw=ac.enum_to_bits(box.value), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2]) -> PairResult:\n    return evaluate(raw)\n",
        "index-control": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(entries, raw: ac.bits[2], index) -> PairResult:\n    decoded, member = decode[State](raw)\n    entries[index] = decoded\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], index: ac.u1) -> PairResult:\n    entries = ac.table[2, State](init=0)\n    return evaluate(entries, raw, index)\n",
        "readonly-control": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(value: State, saved: State, raw: ac.bits[2]) -> PairResult:\n    decoded, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(saved), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2]) -> PairResult:\n    state: State = State.ZERO\n    return evaluate(state, state, raw)\n",
        "nested-rule-control": 'import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.module\ndef Leaf(data: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": data}\n@ac.module\ndef Top(raw: ac.bits[2]) -> {"out_raw": ac.bits[2], "out_member": bool}:\n    decoded, member = decode[State](raw)\n    child = Leaf()\n    @ac.rule\n    def inspect():\n        child(data=ac.enum_to_bits(decoded))\n    inspect()\n    return {"out_raw": child.out, "out_member": member}\n',
        "behavioral-rebind": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.module\ndef Top(raw: ac.bits[3]) -> PairResult:\n    decoded, member = decode[State](raw[1:3])\n    decoded, member = decode[State](raw[1:3])\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n",
        "structural-tuple": 'import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.module\ndef Top(raw: ac.bits[3]) -> {"out_raw": ac.bits[2], "out_member": bool}:\n    decoded, member = decode[State](raw[1:3])\n    return {"out_raw": ac.enum_to_bits(decoded), "out_member": member}\n',
    },
    "negative": {
        "one-name-tuple": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    (decoded,) = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "strict fixed arity: two distinct ordinary local names; preserve reserved/declaration/import boundaries",
        },
        "three-name-tuple": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    (decoded, member, third) = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "strict fixed arity: two distinct ordinary local names; preserve reserved/declaration/import boundaries",
        },
        "list-target": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    [decoded, member] = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "strict fixed arity: two distinct ordinary local names; preserve reserved/declaration/import boundaries",
        },
        "star-target": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    (decoded, *member) = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "strict fixed arity: two distinct ordinary local names; preserve reserved/declaration/import boundaries",
        },
        "nested-target": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    ((decoded, inner), member) = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "strict fixed arity: two distinct ordinary local names; preserve reserved/declaration/import boundaries",
        },
        "duplicate-target": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    (decoded, decoded) = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "strict fixed arity: two distinct ordinary local names; preserve reserved/declaration/import boundaries",
        },
        "reserved-clock": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    (decoded, pyc_clk) = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "strict fixed arity: two distinct ordinary local names; preserve reserved/declaration/import boundaries",
        },
        "reserved-reset": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    (decoded, pyc_rst) = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "strict fixed arity: two distinct ordinary local names; preserve reserved/declaration/import boundaries",
        },
        "shadow-enum": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    (State, member) = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "strict fixed arity: two distinct ordinary local names; preserve reserved/declaration/import boundaries",
        },
        "shadow-marker": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    (decode, member) = ac.enum_from_bits[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "strict fixed arity: two distinct ordinary local names; preserve reserved/declaration/import boundaries",
        },
        "shadow-module": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    (Top, member) = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "strict fixed arity: two distinct ordinary local names; preserve reserved/declaration/import boundaries",
        },
        "shadow-rule": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    (evaluate, member) = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "strict fixed arity: two distinct ordinary local names; preserve reserved/declaration/import boundaries",
        },
        "scalar-call": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    decoded = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "the two-result intrinsic is not a scalar expression",
        },
        "return-tuple-call": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    return decode[State](raw)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "no arbitrary tuple-return or scalar expression admission",
        },
        "field-target": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.struct\nclass Box:\n    value: State = State.ZERO\n\n@ac.rule\ndef evaluate(raw: ac.bits[2]) -> PairResult:\n    box = Box()\n    box.value, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(box.value), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2]) -> PairResult:\n    return evaluate(raw)\n",
            "control": "field-control",
            "guard_intention": "tuple targets cannot be field paths",
        },
        "index-target": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(entries, raw: ac.bits[2], index) -> PairResult:\n    entries[index], member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(entries[index]), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], index: ac.u1) -> PairResult:\n    entries = ac.table[2, State](init=0)\n    return evaluate(entries, raw, index)\n",
            "control": "index-control",
            "guard_intention": "tuple targets cannot be owner table elements",
        },
        "persistent-owner-target": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(value: State, raw: ac.bits[2]) -> PairResult:\n    value, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(value), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2]) -> PairResult:\n    state: State = State.ZERO\n    return evaluate(state, raw)\n",
            "control": "owner-lifecycle",
            "guard_intention": "an actual persistent owner cannot be a tuple target; later ordinary assignment is legal",
        },
        "readonly-owner-target": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(value: State, saved: State, raw: ac.bits[2]) -> PairResult:\n    value, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(saved), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2]) -> PairResult:\n    state: State = State.ZERO\n    return evaluate(state, state, raw)\n",
            "control": "readonly-control",
            "guard_intention": "owner identity is independent of writes/overlap: reject tuple target even when actual duplicates a read-only formal",
        },
        "nonlocal-target": {
            "source": 'import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.module\ndef Leaf(data: ac.bits[2]) -> {"out": ac.bits[2]}:\n    return {"out": data}\n@ac.module\ndef Top(raw: ac.bits[2]) -> {"out_raw": ac.bits[2], "out_member": bool}:\n    decoded, member = decode[State](raw)\n    child = Leaf()\n    @ac.rule\n    def inspect():\n        nonlocal decoded\n        decoded, rule_member = ac.enum_from_bits[State](raw)\n    inspect()\n    return {"out_raw": child.out, "out_member": member}\n',
            "control": "nested-rule-control",
            "guard_intention": "retired nonlocal ownership proposal is forbidden; structural rule-local assignment stays bounded",
        },
        "structural-rebind": {
            "source": 'import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.module\ndef Top(raw: ac.bits[3]) -> {"out_raw": ac.bits[2], "out_member": bool}:\n    decoded, member = decode[State](raw[1:3])\n    decoded, member = decode[State](raw[1:3])\n    return {"out_raw": ac.enum_to_bits(decoded), "out_member": member}\n',
            "control": "structural-tuple",
            "guard_intention": "true mapping-return structural locals remain immutable",
        },
        "structural-input-shadow": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.module\ndef Top(raw: ac.bits[3]) -> PairResult:\n    raw, member = decode[State](raw[1:3])\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n",
            "control": "raw-once",
            "guard_intention": "structural tuple target must not shadow an input",
        },
        "partial-carrier-read": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    if choose:\n        decoded, member = decode[State](raw)\n    else:\n        member = False\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "both-arms",
            "guard_intention": "carrier has no definite assignment on the else path",
        },
        "partial-member-read": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    if choose:\n        decoded, member = decode[State](raw)\n    else:\n        decoded = State.ZERO\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "both-arms",
            "guard_intention": "membership has no definite assignment on the else path",
        },
        "carrier-nominal-boundary": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    decoded: Peer = Peer.ZERO\n    member: bool = False\n    decoded, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "existing-boundaries",
            "guard_intention": "the carrier binding cannot cross distinct nominal identities",
        },
        "membership-integer-boundary": {
            "source": "import pycircuit as ac\nfrom typing import Annotated\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    decoded: State = State.ZERO\n    member: Annotated[int, range(1 << 1)] = 0\n    decoded, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "existing-boundaries",
            "guard_intention": "membership Boolean does not overwrite fixed-bit source-kind contract",
        },
        "member-later-kind": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    decoded: State = State.ZERO\n    member: bool = False\n    decoded, member = decode[State](raw)\n    member = 1\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "existing-boundaries",
            "guard_intention": "Boolean local boundary survives tuple binding",
        },
        "carrier-later-kind": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    decoded: State = State.ZERO\n    member: bool = False\n    decoded, member = decode[State](raw)\n    decoded = 0\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "existing-boundaries",
            "guard_intention": "nominal local boundary survives tuple binding",
        },
        "both-arms-nominal-mismatch": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    if choose:\n        decoded, member = decode[State](raw)\n    else:\n        decoded, member = decode[Peer](alternate)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "both-arms",
            "guard_intention": "branch carrier joins require the same nominal type",
        },
        "both-arms-member-kind-mismatch": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    if choose:\n        decoded, member = decode[State](raw)\n    else:\n        decoded = State.ZERO\n        member = 1\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "both-arms",
            "guard_intention": "branch membership join remains Boolean",
        },
        "atomic-second-boundary": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    decoded: State = State.ZERO\n    member: Peer = Peer.ZERO\n    decoded, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "existing-boundaries",
            "guard_intention": "validate second result before publishing either binding; protected publication proves external atomicity",
        },
        "raw-too-narrow": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.module\ndef Top(raw: ac.bits[1]) -> PairResult:\n    decoded, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n",
            "control": "raw-once",
            "guard_intention": "from-bits requires authoritative fixed bits of exactly declared width",
        },
        "raw-too-wide": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.module\ndef Top(raw: ac.bits[3]) -> PairResult:\n    decoded, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n",
            "control": "raw-once",
            "guard_intention": "from-bits requires authoritative fixed bits of exactly declared width",
        },
        "raw-integer": {
            "source": "import pycircuit as ac\nfrom typing import Annotated\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.module\ndef Top(raw: Annotated[int, range(1 << 2)]) -> PairResult:\n    decoded, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n",
            "control": "raw-once",
            "guard_intention": "from-bits requires authoritative fixed bits of exactly declared width",
        },
        "raw-boolean": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.module\ndef Top(raw: bool) -> PairResult:\n    decoded, member = decode[Singleton](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n",
            "control": "raw-once",
            "guard_intention": "from-bits requires authoritative fixed bits of exactly declared width",
        },
        "raw-nominal": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.module\ndef Top(raw: State) -> PairResult:\n    decoded, member = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n",
            "control": "raw-once",
            "guard_intention": "Enum raw transport requires explicit to-bits; nominal operand is not fixed bits",
        },
        "marker-shadow-formal": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(decode: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    decoded, member = decode[State](decode)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(decode: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "marker resolution must follow the actual parameter binding",
        },
        "chained-tuple": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    decoded, member = other, flag = decode[State](raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "one assignment target only; no chained tuple publication",
        },
        "arbitrary-tuple-producer": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    decoded, member = (State.ZERO, True)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "the admitted two-result producer is typed enum_from_bits only",
        },
        "untyped-producer": {
            "source": "import pycircuit as ac\nfrom pycircuit import enum_from_bits as decode\nfrom enums.facade2 import Exported as State\nfrom enums.types import Singleton, Sparse, Wide, Huge\nfrom enums.peer import State as Peer\n@ac.struct\nclass PairResult:\n    raw: ac.bits[2]\n    member: ac.u1\n\n@ac.rule\ndef evaluate(raw: ac.bits[2], alternate: ac.bits[2], choose) -> PairResult:\n    decoded, member = decode(raw)\n    return PairResult(raw=ac.enum_to_bits(decoded), member=member)\n\n@ac.module\ndef Top(raw: ac.bits[2], alternate: ac.bits[2], choose: ac.u1) -> PairResult:\n    return evaluate(raw, alternate, choose)\n",
            "control": "tuple-rule",
            "guard_intention": "the nominal producer type argument is required",
        },
    },
    "inputs": {"raw": 2, "sparse": 3, "wide": 65, "huge": 130},
    "fields": {
        "raw": 2,
        "member": 1,
        "ignored_member_raw": 2,
        "only_member": 1,
        "sparse_raw": 3,
        "sparse_member": 1,
        "wide_raw": 65,
        "wide_member": 1,
        "huge_raw": 130,
        "huge_member": 1,
    },
    "rows": [
        {
            "raw": "00",
            "sparse": "000",
            "wide": "00000000000000000000000000000000000000000000000000000000000000000",
            "huge": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "01",
            "sparse": "001",
            "wide": "10000000000000000000000000000000000000000000000000000000000000011",
            "huge": "1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "0x",
            "sparse": "00x",
            "wide": "11111111111111111111111111111111111111111111111111111111111111111",
            "huge": "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
        },
        {
            "raw": "0z",
            "sparse": "00z",
            "wide": "00000000000000000000000000000000000000000000000000000000000000001",
            "huge": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001",
        },
        {
            "raw": "10",
            "sparse": "010",
            "wide": "00000000000000000000000000000000000000000000000000000000000000010",
            "huge": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010",
        },
        {
            "raw": "11",
            "sparse": "011",
            "wide": "10000000000000000000000000000000000000000000000000000000000000000",
            "huge": "1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "1x",
            "sparse": "01x",
            "wide": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            "huge": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
        },
        {
            "raw": "1z",
            "sparse": "01z",
            "wide": "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
            "huge": "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
        },
        {
            "raw": "x0",
            "sparse": "0x0",
            "wide": "0000000000000000000000000000000000000000000000000000000000000000x",
            "huge": "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000x",
        },
        {
            "raw": "x1",
            "sparse": "0x1",
            "wide": "0000000000000000000000000000000000000000000000000000000000000000z",
            "huge": "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000z",
        },
        {
            "raw": "xx",
            "sparse": "0xx",
            "wide": "000000000000000000000000000000000000000000000000000000000000000x0",
            "huge": "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000x0",
        },
        {
            "raw": "xz",
            "sparse": "0xz",
            "wide": "000000000000000000000000000000000000000000000000000000000000000z0",
            "huge": "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000z0",
        },
        {
            "raw": "z0",
            "sparse": "0z0",
            "wide": "0x000000000000000000000000000000000000000000000000000000000000000",
            "huge": "000000000000000000000000000000000000000000000000000000000000000000x000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "z1",
            "sparse": "0z1",
            "wide": "0z000000000000000000000000000000000000000000000000000000000000000",
            "huge": "000000000000000000000000000000000000000000000000000000000000000000z000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "zx",
            "sparse": "0zx",
            "wide": "x0000000000000000000000000000000000000000000000000000000000000000",
            "huge": "00000000000000000000000000000000000000000000000000000000000000000x0000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "zz",
            "sparse": "0zz",
            "wide": "z0000000000000000000000000000000000000000000000000000000000000000",
            "huge": "00000000000000000000000000000000000000000000000000000000000000000z0000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "00",
            "sparse": "100",
            "wide": "1000000000000000000000000000000000000000000000000000000000000001x",
            "huge": "x000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "01",
            "sparse": "101",
            "wide": "1000000000000000000000000000000000000000000000000000000000000001z",
            "huge": "z000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "0x",
            "sparse": "10x",
            "wide": "100000000000000000000000000000000000000000000000000000000000000x1",
            "huge": "100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011x",
        },
        {
            "raw": "0z",
            "sparse": "10z",
            "wide": "100000000000000000000000000000000000000000000000000000000000000z1",
            "huge": "100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011z",
        },
        {
            "raw": "10",
            "sparse": "110",
            "wide": "1x000000000000000000000000000000000000000000000000000000000000011",
            "huge": "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001x1",
        },
        {
            "raw": "11",
            "sparse": "111",
            "wide": "1z000000000000000000000000000000000000000000000000000000000000011",
            "huge": "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001z1",
        },
        {
            "raw": "1x",
            "sparse": "11x",
            "wide": "x0000000000000000000000000000000000000000000000000000000000000011",
            "huge": "100000000000000000000000000000000000000000000000000000000000000000x000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "1z",
            "sparse": "11z",
            "wide": "z0000000000000000000000000000000000000000000000000000000000000011",
            "huge": "100000000000000000000000000000000000000000000000000000000000000000z000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "x0",
            "sparse": "1x0",
            "wide": "00000000000000000000000000000000000000000000000000000000000000000",
            "huge": "10000000000000000000000000000000000000000000000000000000000000000x0000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "x1",
            "sparse": "1x1",
            "wide": "10000000000000000000000000000000000000000000000000000000000000011",
            "huge": "10000000000000000000000000000000000000000000000000000000000000000z0000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "xx",
            "sparse": "1xx",
            "wide": "11111111111111111111111111111111111111111111111111111111111111111",
            "huge": "x000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "xz",
            "sparse": "1xz",
            "wide": "00000000000000000000000000000000000000000000000000000000000000001",
            "huge": "z000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "z0",
            "sparse": "1z0",
            "wide": "00000000000000000000000000000000000000000000000000000000000000010",
            "huge": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "z1",
            "sparse": "1z1",
            "wide": "10000000000000000000000000000000000000000000000000000000000000000",
            "huge": "1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "zx",
            "sparse": "1zx",
            "wide": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            "huge": "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
        },
        {
            "raw": "zz",
            "sparse": "1zz",
            "wide": "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
            "huge": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001",
        },
        {
            "raw": "00",
            "sparse": "x00",
            "wide": "0000000000000000000000000000000000000000000000000000000000000000x",
            "huge": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010",
        },
        {
            "raw": "01",
            "sparse": "x01",
            "wide": "0000000000000000000000000000000000000000000000000000000000000000z",
            "huge": "1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "0x",
            "sparse": "x0x",
            "wide": "000000000000000000000000000000000000000000000000000000000000000x0",
            "huge": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
        },
        {
            "raw": "0z",
            "sparse": "x0z",
            "wide": "000000000000000000000000000000000000000000000000000000000000000z0",
            "huge": "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
        },
        {
            "raw": "10",
            "sparse": "x10",
            "wide": "0x000000000000000000000000000000000000000000000000000000000000000",
            "huge": "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000x",
        },
        {
            "raw": "11",
            "sparse": "x11",
            "wide": "0z000000000000000000000000000000000000000000000000000000000000000",
            "huge": "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000z",
        },
        {
            "raw": "1x",
            "sparse": "x1x",
            "wide": "x0000000000000000000000000000000000000000000000000000000000000000",
            "huge": "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000x0",
        },
        {
            "raw": "1z",
            "sparse": "x1z",
            "wide": "z0000000000000000000000000000000000000000000000000000000000000000",
            "huge": "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000z0",
        },
        {
            "raw": "x0",
            "sparse": "xx0",
            "wide": "1000000000000000000000000000000000000000000000000000000000000001x",
            "huge": "000000000000000000000000000000000000000000000000000000000000000000x000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "x1",
            "sparse": "xx1",
            "wide": "1000000000000000000000000000000000000000000000000000000000000001z",
            "huge": "000000000000000000000000000000000000000000000000000000000000000000z000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "xx",
            "sparse": "xxx",
            "wide": "100000000000000000000000000000000000000000000000000000000000000x1",
            "huge": "00000000000000000000000000000000000000000000000000000000000000000x0000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "xz",
            "sparse": "xxz",
            "wide": "100000000000000000000000000000000000000000000000000000000000000z1",
            "huge": "00000000000000000000000000000000000000000000000000000000000000000z0000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "z0",
            "sparse": "xz0",
            "wide": "1x000000000000000000000000000000000000000000000000000000000000011",
            "huge": "x000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "z1",
            "sparse": "xz1",
            "wide": "1z000000000000000000000000000000000000000000000000000000000000011",
            "huge": "z000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "zx",
            "sparse": "xzx",
            "wide": "x0000000000000000000000000000000000000000000000000000000000000011",
            "huge": "100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011x",
        },
        {
            "raw": "zz",
            "sparse": "xzz",
            "wide": "z0000000000000000000000000000000000000000000000000000000000000011",
            "huge": "100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011z",
        },
        {
            "raw": "00",
            "sparse": "z00",
            "wide": "00000000000000000000000000000000000000000000000000000000000000000",
            "huge": "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001x1",
        },
        {
            "raw": "01",
            "sparse": "z01",
            "wide": "10000000000000000000000000000000000000000000000000000000000000011",
            "huge": "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001z1",
        },
        {
            "raw": "0x",
            "sparse": "z0x",
            "wide": "11111111111111111111111111111111111111111111111111111111111111111",
            "huge": "100000000000000000000000000000000000000000000000000000000000000000x000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "0z",
            "sparse": "z0z",
            "wide": "00000000000000000000000000000000000000000000000000000000000000001",
            "huge": "100000000000000000000000000000000000000000000000000000000000000000z000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "10",
            "sparse": "z10",
            "wide": "00000000000000000000000000000000000000000000000000000000000000010",
            "huge": "10000000000000000000000000000000000000000000000000000000000000000x0000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "11",
            "sparse": "z11",
            "wide": "10000000000000000000000000000000000000000000000000000000000000000",
            "huge": "10000000000000000000000000000000000000000000000000000000000000000z0000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "1x",
            "sparse": "z1x",
            "wide": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            "huge": "x000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "1z",
            "sparse": "z1z",
            "wide": "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
            "huge": "z000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "x0",
            "sparse": "zx0",
            "wide": "0000000000000000000000000000000000000000000000000000000000000000x",
            "huge": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "x1",
            "sparse": "zx1",
            "wide": "0000000000000000000000000000000000000000000000000000000000000000z",
            "huge": "1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111",
        },
        {
            "raw": "xx",
            "sparse": "zxx",
            "wide": "000000000000000000000000000000000000000000000000000000000000000x0",
            "huge": "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
        },
        {
            "raw": "xz",
            "sparse": "zxz",
            "wide": "000000000000000000000000000000000000000000000000000000000000000z0",
            "huge": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001",
        },
        {
            "raw": "z0",
            "sparse": "zz0",
            "wide": "0x000000000000000000000000000000000000000000000000000000000000000",
            "huge": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010",
        },
        {
            "raw": "z1",
            "sparse": "zz1",
            "wide": "0z000000000000000000000000000000000000000000000000000000000000000",
            "huge": "1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        },
        {
            "raw": "zx",
            "sparse": "zzx",
            "wide": "x0000000000000000000000000000000000000000000000000000000000000000",
            "huge": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
        },
        {
            "raw": "zz",
            "sparse": "zzz",
            "wide": "z0000000000000000000000000000000000000000000000000000000000000000",
            "huge": "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
        },
    ],
    "gold": [
        {
            "raw": "00",
            "member": "1",
            "ignored_member_raw": "00",
            "only_member": "1",
            "sparse_raw": "000",
            "sparse_member": "1",
            "wide_raw": "00000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "1",
            "huge_raw": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "1",
        },
        {
            "raw": "01",
            "member": "1",
            "ignored_member_raw": "01",
            "only_member": "1",
            "sparse_raw": "001",
            "sparse_member": "0",
            "wide_raw": "10000000000000000000000000000000000000000000000000000000000000011",
            "wide_member": "1",
            "huge_raw": "1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "1",
        },
        {
            "raw": "0x",
            "member": "x",
            "ignored_member_raw": "0x",
            "only_member": "x",
            "sparse_raw": "00x",
            "sparse_member": "x",
            "wide_raw": "11111111111111111111111111111111111111111111111111111111111111111",
            "wide_member": "1",
            "huge_raw": "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
            "huge_member": "1",
        },
        {
            "raw": "0z",
            "member": "x",
            "ignored_member_raw": "0z",
            "only_member": "x",
            "sparse_raw": "00z",
            "sparse_member": "x",
            "wide_raw": "00000000000000000000000000000000000000000000000000000000000000001",
            "wide_member": "0",
            "huge_raw": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001",
            "huge_member": "0",
        },
        {
            "raw": "10",
            "member": "0",
            "ignored_member_raw": "10",
            "only_member": "0",
            "sparse_raw": "010",
            "sparse_member": "0",
            "wide_raw": "00000000000000000000000000000000000000000000000000000000000000010",
            "wide_member": "0",
            "huge_raw": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010",
            "huge_member": "0",
        },
        {
            "raw": "11",
            "member": "0",
            "ignored_member_raw": "11",
            "only_member": "0",
            "sparse_raw": "011",
            "sparse_member": "1",
            "wide_raw": "10000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "0",
            "huge_raw": "1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "0",
        },
        {
            "raw": "1x",
            "member": "0",
            "ignored_member_raw": "1x",
            "only_member": "0",
            "sparse_raw": "01x",
            "sparse_member": "x",
            "wide_raw": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            "wide_member": "x",
            "huge_raw": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            "huge_member": "x",
        },
        {
            "raw": "1z",
            "member": "0",
            "ignored_member_raw": "1z",
            "only_member": "0",
            "sparse_raw": "01z",
            "sparse_member": "x",
            "wide_raw": "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
            "wide_member": "x",
            "huge_raw": "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
            "huge_member": "x",
        },
        {
            "raw": "x0",
            "member": "x",
            "ignored_member_raw": "x0",
            "only_member": "x",
            "sparse_raw": "0x0",
            "sparse_member": "x",
            "wide_raw": "0000000000000000000000000000000000000000000000000000000000000000x",
            "wide_member": "x",
            "huge_raw": "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000x",
            "huge_member": "x",
        },
        {
            "raw": "x1",
            "member": "x",
            "ignored_member_raw": "x1",
            "only_member": "x",
            "sparse_raw": "0x1",
            "sparse_member": "x",
            "wide_raw": "0000000000000000000000000000000000000000000000000000000000000000z",
            "wide_member": "x",
            "huge_raw": "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000z",
            "huge_member": "x",
        },
        {
            "raw": "xx",
            "member": "x",
            "ignored_member_raw": "xx",
            "only_member": "x",
            "sparse_raw": "0xx",
            "sparse_member": "x",
            "wide_raw": "000000000000000000000000000000000000000000000000000000000000000x0",
            "wide_member": "x",
            "huge_raw": "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000x0",
            "huge_member": "x",
        },
        {
            "raw": "xz",
            "member": "x",
            "ignored_member_raw": "xz",
            "only_member": "x",
            "sparse_raw": "0xz",
            "sparse_member": "x",
            "wide_raw": "000000000000000000000000000000000000000000000000000000000000000z0",
            "wide_member": "x",
            "huge_raw": "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000z0",
            "huge_member": "x",
        },
        {
            "raw": "z0",
            "member": "x",
            "ignored_member_raw": "z0",
            "only_member": "x",
            "sparse_raw": "0z0",
            "sparse_member": "x",
            "wide_raw": "0x000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "x",
            "huge_raw": "000000000000000000000000000000000000000000000000000000000000000000x000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "x",
        },
        {
            "raw": "z1",
            "member": "x",
            "ignored_member_raw": "z1",
            "only_member": "x",
            "sparse_raw": "0z1",
            "sparse_member": "x",
            "wide_raw": "0z000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "x",
            "huge_raw": "000000000000000000000000000000000000000000000000000000000000000000z000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "x",
        },
        {
            "raw": "zx",
            "member": "x",
            "ignored_member_raw": "zx",
            "only_member": "x",
            "sparse_raw": "0zx",
            "sparse_member": "x",
            "wide_raw": "x0000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "x",
            "huge_raw": "00000000000000000000000000000000000000000000000000000000000000000x0000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "x",
        },
        {
            "raw": "zz",
            "member": "x",
            "ignored_member_raw": "zz",
            "only_member": "x",
            "sparse_raw": "0zz",
            "sparse_member": "x",
            "wide_raw": "z0000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "x",
            "huge_raw": "00000000000000000000000000000000000000000000000000000000000000000z0000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "x",
        },
        {
            "raw": "00",
            "member": "1",
            "ignored_member_raw": "00",
            "only_member": "1",
            "sparse_raw": "100",
            "sparse_member": "0",
            "wide_raw": "1000000000000000000000000000000000000000000000000000000000000001x",
            "wide_member": "x",
            "huge_raw": "x000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "x",
        },
        {
            "raw": "01",
            "member": "1",
            "ignored_member_raw": "01",
            "only_member": "1",
            "sparse_raw": "101",
            "sparse_member": "1",
            "wide_raw": "1000000000000000000000000000000000000000000000000000000000000001z",
            "wide_member": "x",
            "huge_raw": "z000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "x",
        },
        {
            "raw": "0x",
            "member": "x",
            "ignored_member_raw": "0x",
            "only_member": "x",
            "sparse_raw": "10x",
            "sparse_member": "x",
            "wide_raw": "100000000000000000000000000000000000000000000000000000000000000x1",
            "wide_member": "x",
            "huge_raw": "100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011x",
            "huge_member": "x",
        },
        {
            "raw": "0z",
            "member": "x",
            "ignored_member_raw": "0z",
            "only_member": "x",
            "sparse_raw": "10z",
            "sparse_member": "x",
            "wide_raw": "100000000000000000000000000000000000000000000000000000000000000z1",
            "wide_member": "x",
            "huge_raw": "100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011z",
            "huge_member": "x",
        },
        {
            "raw": "10",
            "member": "0",
            "ignored_member_raw": "10",
            "only_member": "0",
            "sparse_raw": "110",
            "sparse_member": "0",
            "wide_raw": "1x000000000000000000000000000000000000000000000000000000000000011",
            "wide_member": "x",
            "huge_raw": "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001x1",
            "huge_member": "x",
        },
        {
            "raw": "11",
            "member": "0",
            "ignored_member_raw": "11",
            "only_member": "0",
            "sparse_raw": "111",
            "sparse_member": "0",
            "wide_raw": "1z000000000000000000000000000000000000000000000000000000000000011",
            "wide_member": "x",
            "huge_raw": "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001z1",
            "huge_member": "x",
        },
        {
            "raw": "1x",
            "member": "0",
            "ignored_member_raw": "1x",
            "only_member": "0",
            "sparse_raw": "11x",
            "sparse_member": "0",
            "wide_raw": "x0000000000000000000000000000000000000000000000000000000000000011",
            "wide_member": "x",
            "huge_raw": "100000000000000000000000000000000000000000000000000000000000000000x000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "x",
        },
        {
            "raw": "1z",
            "member": "0",
            "ignored_member_raw": "1z",
            "only_member": "0",
            "sparse_raw": "11z",
            "sparse_member": "0",
            "wide_raw": "z0000000000000000000000000000000000000000000000000000000000000011",
            "wide_member": "x",
            "huge_raw": "100000000000000000000000000000000000000000000000000000000000000000z000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "x",
        },
        {
            "raw": "x0",
            "member": "x",
            "ignored_member_raw": "x0",
            "only_member": "x",
            "sparse_raw": "1x0",
            "sparse_member": "0",
            "wide_raw": "00000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "1",
            "huge_raw": "10000000000000000000000000000000000000000000000000000000000000000x0000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "x",
        },
        {
            "raw": "x1",
            "member": "x",
            "ignored_member_raw": "x1",
            "only_member": "x",
            "sparse_raw": "1x1",
            "sparse_member": "x",
            "wide_raw": "10000000000000000000000000000000000000000000000000000000000000011",
            "wide_member": "1",
            "huge_raw": "10000000000000000000000000000000000000000000000000000000000000000z0000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "x",
        },
        {
            "raw": "xx",
            "member": "x",
            "ignored_member_raw": "xx",
            "only_member": "x",
            "sparse_raw": "1xx",
            "sparse_member": "x",
            "wide_raw": "11111111111111111111111111111111111111111111111111111111111111111",
            "wide_member": "1",
            "huge_raw": "x000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "x",
        },
        {
            "raw": "xz",
            "member": "x",
            "ignored_member_raw": "xz",
            "only_member": "x",
            "sparse_raw": "1xz",
            "sparse_member": "x",
            "wide_raw": "00000000000000000000000000000000000000000000000000000000000000001",
            "wide_member": "0",
            "huge_raw": "z000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "x",
        },
        {
            "raw": "z0",
            "member": "x",
            "ignored_member_raw": "z0",
            "only_member": "x",
            "sparse_raw": "1z0",
            "sparse_member": "0",
            "wide_raw": "00000000000000000000000000000000000000000000000000000000000000010",
            "wide_member": "0",
            "huge_raw": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "1",
        },
        {
            "raw": "z1",
            "member": "x",
            "ignored_member_raw": "z1",
            "only_member": "x",
            "sparse_raw": "1z1",
            "sparse_member": "x",
            "wide_raw": "10000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "0",
            "huge_raw": "1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "1",
        },
        {
            "raw": "zx",
            "member": "x",
            "ignored_member_raw": "zx",
            "only_member": "x",
            "sparse_raw": "1zx",
            "sparse_member": "x",
            "wide_raw": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            "wide_member": "x",
            "huge_raw": "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
            "huge_member": "1",
        },
        {
            "raw": "zz",
            "member": "x",
            "ignored_member_raw": "zz",
            "only_member": "x",
            "sparse_raw": "1zz",
            "sparse_member": "x",
            "wide_raw": "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
            "wide_member": "x",
            "huge_raw": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001",
            "huge_member": "0",
        },
        {
            "raw": "00",
            "member": "1",
            "ignored_member_raw": "00",
            "only_member": "1",
            "sparse_raw": "x00",
            "sparse_member": "x",
            "wide_raw": "0000000000000000000000000000000000000000000000000000000000000000x",
            "wide_member": "x",
            "huge_raw": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010",
            "huge_member": "0",
        },
        {
            "raw": "01",
            "member": "1",
            "ignored_member_raw": "01",
            "only_member": "1",
            "sparse_raw": "x01",
            "sparse_member": "x",
            "wide_raw": "0000000000000000000000000000000000000000000000000000000000000000z",
            "wide_member": "x",
            "huge_raw": "1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "0",
        },
        {
            "raw": "0x",
            "member": "x",
            "ignored_member_raw": "0x",
            "only_member": "x",
            "sparse_raw": "x0x",
            "sparse_member": "x",
            "wide_raw": "000000000000000000000000000000000000000000000000000000000000000x0",
            "wide_member": "x",
            "huge_raw": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            "huge_member": "x",
        },
        {
            "raw": "0z",
            "member": "x",
            "ignored_member_raw": "0z",
            "only_member": "x",
            "sparse_raw": "x0z",
            "sparse_member": "x",
            "wide_raw": "000000000000000000000000000000000000000000000000000000000000000z0",
            "wide_member": "x",
            "huge_raw": "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
            "huge_member": "x",
        },
        {
            "raw": "10",
            "member": "0",
            "ignored_member_raw": "10",
            "only_member": "0",
            "sparse_raw": "x10",
            "sparse_member": "0",
            "wide_raw": "0x000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "x",
            "huge_raw": "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000x",
            "huge_member": "x",
        },
        {
            "raw": "11",
            "member": "0",
            "ignored_member_raw": "11",
            "only_member": "0",
            "sparse_raw": "x11",
            "sparse_member": "x",
            "wide_raw": "0z000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "x",
            "huge_raw": "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000z",
            "huge_member": "x",
        },
        {
            "raw": "1x",
            "member": "0",
            "ignored_member_raw": "1x",
            "only_member": "0",
            "sparse_raw": "x1x",
            "sparse_member": "x",
            "wide_raw": "x0000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "x",
            "huge_raw": "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000x0",
            "huge_member": "x",
        },
        {
            "raw": "1z",
            "member": "0",
            "ignored_member_raw": "1z",
            "only_member": "0",
            "sparse_raw": "x1z",
            "sparse_member": "x",
            "wide_raw": "z0000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "x",
            "huge_raw": "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000z0",
            "huge_member": "x",
        },
        {
            "raw": "x0",
            "member": "x",
            "ignored_member_raw": "x0",
            "only_member": "x",
            "sparse_raw": "xx0",
            "sparse_member": "x",
            "wide_raw": "1000000000000000000000000000000000000000000000000000000000000001x",
            "wide_member": "x",
            "huge_raw": "000000000000000000000000000000000000000000000000000000000000000000x000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "x",
        },
        {
            "raw": "x1",
            "member": "x",
            "ignored_member_raw": "x1",
            "only_member": "x",
            "sparse_raw": "xx1",
            "sparse_member": "x",
            "wide_raw": "1000000000000000000000000000000000000000000000000000000000000001z",
            "wide_member": "x",
            "huge_raw": "000000000000000000000000000000000000000000000000000000000000000000z000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "x",
        },
        {
            "raw": "xx",
            "member": "x",
            "ignored_member_raw": "xx",
            "only_member": "x",
            "sparse_raw": "xxx",
            "sparse_member": "x",
            "wide_raw": "100000000000000000000000000000000000000000000000000000000000000x1",
            "wide_member": "x",
            "huge_raw": "00000000000000000000000000000000000000000000000000000000000000000x0000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "x",
        },
        {
            "raw": "xz",
            "member": "x",
            "ignored_member_raw": "xz",
            "only_member": "x",
            "sparse_raw": "xxz",
            "sparse_member": "x",
            "wide_raw": "100000000000000000000000000000000000000000000000000000000000000z1",
            "wide_member": "x",
            "huge_raw": "00000000000000000000000000000000000000000000000000000000000000000z0000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "x",
        },
        {
            "raw": "z0",
            "member": "x",
            "ignored_member_raw": "z0",
            "only_member": "x",
            "sparse_raw": "xz0",
            "sparse_member": "x",
            "wide_raw": "1x000000000000000000000000000000000000000000000000000000000000011",
            "wide_member": "x",
            "huge_raw": "x000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "x",
        },
        {
            "raw": "z1",
            "member": "x",
            "ignored_member_raw": "z1",
            "only_member": "x",
            "sparse_raw": "xz1",
            "sparse_member": "x",
            "wide_raw": "1z000000000000000000000000000000000000000000000000000000000000011",
            "wide_member": "x",
            "huge_raw": "z000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "x",
        },
        {
            "raw": "zx",
            "member": "x",
            "ignored_member_raw": "zx",
            "only_member": "x",
            "sparse_raw": "xzx",
            "sparse_member": "x",
            "wide_raw": "x0000000000000000000000000000000000000000000000000000000000000011",
            "wide_member": "x",
            "huge_raw": "100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011x",
            "huge_member": "x",
        },
        {
            "raw": "zz",
            "member": "x",
            "ignored_member_raw": "zz",
            "only_member": "x",
            "sparse_raw": "xzz",
            "sparse_member": "x",
            "wide_raw": "z0000000000000000000000000000000000000000000000000000000000000011",
            "wide_member": "x",
            "huge_raw": "100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011z",
            "huge_member": "x",
        },
        {
            "raw": "00",
            "member": "1",
            "ignored_member_raw": "00",
            "only_member": "1",
            "sparse_raw": "z00",
            "sparse_member": "x",
            "wide_raw": "00000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "1",
            "huge_raw": "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001x1",
            "huge_member": "x",
        },
        {
            "raw": "01",
            "member": "1",
            "ignored_member_raw": "01",
            "only_member": "1",
            "sparse_raw": "z01",
            "sparse_member": "x",
            "wide_raw": "10000000000000000000000000000000000000000000000000000000000000011",
            "wide_member": "1",
            "huge_raw": "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001z1",
            "huge_member": "x",
        },
        {
            "raw": "0x",
            "member": "x",
            "ignored_member_raw": "0x",
            "only_member": "x",
            "sparse_raw": "z0x",
            "sparse_member": "x",
            "wide_raw": "11111111111111111111111111111111111111111111111111111111111111111",
            "wide_member": "1",
            "huge_raw": "100000000000000000000000000000000000000000000000000000000000000000x000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "x",
        },
        {
            "raw": "0z",
            "member": "x",
            "ignored_member_raw": "0z",
            "only_member": "x",
            "sparse_raw": "z0z",
            "sparse_member": "x",
            "wide_raw": "00000000000000000000000000000000000000000000000000000000000000001",
            "wide_member": "0",
            "huge_raw": "100000000000000000000000000000000000000000000000000000000000000000z000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "x",
        },
        {
            "raw": "10",
            "member": "0",
            "ignored_member_raw": "10",
            "only_member": "0",
            "sparse_raw": "z10",
            "sparse_member": "0",
            "wide_raw": "00000000000000000000000000000000000000000000000000000000000000010",
            "wide_member": "0",
            "huge_raw": "10000000000000000000000000000000000000000000000000000000000000000x0000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "x",
        },
        {
            "raw": "11",
            "member": "0",
            "ignored_member_raw": "11",
            "only_member": "0",
            "sparse_raw": "z11",
            "sparse_member": "x",
            "wide_raw": "10000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "0",
            "huge_raw": "10000000000000000000000000000000000000000000000000000000000000000z0000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "x",
        },
        {
            "raw": "1x",
            "member": "0",
            "ignored_member_raw": "1x",
            "only_member": "0",
            "sparse_raw": "z1x",
            "sparse_member": "x",
            "wide_raw": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            "wide_member": "x",
            "huge_raw": "x000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "x",
        },
        {
            "raw": "1z",
            "member": "0",
            "ignored_member_raw": "1z",
            "only_member": "0",
            "sparse_raw": "z1z",
            "sparse_member": "x",
            "wide_raw": "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
            "wide_member": "x",
            "huge_raw": "z000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "x",
        },
        {
            "raw": "x0",
            "member": "x",
            "ignored_member_raw": "x0",
            "only_member": "x",
            "sparse_raw": "zx0",
            "sparse_member": "x",
            "wide_raw": "0000000000000000000000000000000000000000000000000000000000000000x",
            "wide_member": "x",
            "huge_raw": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "1",
        },
        {
            "raw": "x1",
            "member": "x",
            "ignored_member_raw": "x1",
            "only_member": "x",
            "sparse_raw": "zx1",
            "sparse_member": "x",
            "wide_raw": "0000000000000000000000000000000000000000000000000000000000000000z",
            "wide_member": "x",
            "huge_raw": "1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111",
            "huge_member": "1",
        },
        {
            "raw": "xx",
            "member": "x",
            "ignored_member_raw": "xx",
            "only_member": "x",
            "sparse_raw": "zxx",
            "sparse_member": "x",
            "wide_raw": "000000000000000000000000000000000000000000000000000000000000000x0",
            "wide_member": "x",
            "huge_raw": "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
            "huge_member": "1",
        },
        {
            "raw": "xz",
            "member": "x",
            "ignored_member_raw": "xz",
            "only_member": "x",
            "sparse_raw": "zxz",
            "sparse_member": "x",
            "wide_raw": "000000000000000000000000000000000000000000000000000000000000000z0",
            "wide_member": "x",
            "huge_raw": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001",
            "huge_member": "0",
        },
        {
            "raw": "z0",
            "member": "x",
            "ignored_member_raw": "z0",
            "only_member": "x",
            "sparse_raw": "zz0",
            "sparse_member": "x",
            "wide_raw": "0x000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "x",
            "huge_raw": "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010",
            "huge_member": "0",
        },
        {
            "raw": "z1",
            "member": "x",
            "ignored_member_raw": "z1",
            "only_member": "x",
            "sparse_raw": "zz1",
            "sparse_member": "x",
            "wide_raw": "0z000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "x",
            "huge_raw": "1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "huge_member": "0",
        },
        {
            "raw": "zx",
            "member": "x",
            "ignored_member_raw": "zx",
            "only_member": "x",
            "sparse_raw": "zzx",
            "sparse_member": "x",
            "wide_raw": "x0000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "x",
            "huge_raw": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            "huge_member": "x",
        },
        {
            "raw": "zz",
            "member": "x",
            "ignored_member_raw": "zz",
            "only_member": "x",
            "sparse_raw": "zzz",
            "sparse_member": "x",
            "wide_raw": "z0000000000000000000000000000000000000000000000000000000000000000",
            "wide_member": "x",
            "huge_raw": "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
            "huge_member": "x",
        },
    ],
    "planes": {
        "raw": "raw",
        "ignored_member_raw": "raw",
        "sparse_raw": "sparse",
        "wide_raw": "wide",
        "huge_raw": "huge",
    },
    "raw_once_rows": [
        {"raw": "000"},
        {"raw": "001"},
        {"raw": "00x"},
        {"raw": "00z"},
        {"raw": "010"},
        {"raw": "011"},
        {"raw": "01x"},
        {"raw": "01z"},
        {"raw": "0x0"},
        {"raw": "0x1"},
        {"raw": "0xx"},
        {"raw": "0xz"},
        {"raw": "0z0"},
        {"raw": "0z1"},
        {"raw": "0zx"},
        {"raw": "0zz"},
        {"raw": "100"},
        {"raw": "101"},
        {"raw": "10x"},
        {"raw": "10z"},
        {"raw": "110"},
        {"raw": "111"},
        {"raw": "11x"},
        {"raw": "11z"},
        {"raw": "1x0"},
        {"raw": "1x1"},
        {"raw": "1xx"},
        {"raw": "1xz"},
        {"raw": "1z0"},
        {"raw": "1z1"},
        {"raw": "1zx"},
        {"raw": "1zz"},
        {"raw": "x00"},
        {"raw": "x01"},
        {"raw": "x0x"},
        {"raw": "x0z"},
        {"raw": "x10"},
        {"raw": "x11"},
        {"raw": "x1x"},
        {"raw": "x1z"},
        {"raw": "xx0"},
        {"raw": "xx1"},
        {"raw": "xxx"},
        {"raw": "xxz"},
        {"raw": "xz0"},
        {"raw": "xz1"},
        {"raw": "xzx"},
        {"raw": "xzz"},
        {"raw": "z00"},
        {"raw": "z01"},
        {"raw": "z0x"},
        {"raw": "z0z"},
        {"raw": "z10"},
        {"raw": "z11"},
        {"raw": "z1x"},
        {"raw": "z1z"},
        {"raw": "zx0"},
        {"raw": "zx1"},
        {"raw": "zxx"},
        {"raw": "zxz"},
        {"raw": "zz0"},
        {"raw": "zz1"},
        {"raw": "zzx"},
        {"raw": "zzz"},
    ],
    "raw_once_gold": [
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "0x", "member": "x"},
        {"raw": "0x", "member": "x"},
        {"raw": "0x", "member": "x"},
        {"raw": "0x", "member": "x"},
        {"raw": "0z", "member": "x"},
        {"raw": "0z", "member": "x"},
        {"raw": "0z", "member": "x"},
        {"raw": "0z", "member": "x"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "1x", "member": "0"},
        {"raw": "1x", "member": "0"},
        {"raw": "1x", "member": "0"},
        {"raw": "1x", "member": "0"},
        {"raw": "1z", "member": "0"},
        {"raw": "1z", "member": "0"},
        {"raw": "1z", "member": "0"},
        {"raw": "1z", "member": "0"},
        {"raw": "x0", "member": "x"},
        {"raw": "x0", "member": "x"},
        {"raw": "x0", "member": "x"},
        {"raw": "x0", "member": "x"},
        {"raw": "x1", "member": "x"},
        {"raw": "x1", "member": "x"},
        {"raw": "x1", "member": "x"},
        {"raw": "x1", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xz", "member": "x"},
        {"raw": "xz", "member": "x"},
        {"raw": "xz", "member": "x"},
        {"raw": "xz", "member": "x"},
        {"raw": "z0", "member": "x"},
        {"raw": "z0", "member": "x"},
        {"raw": "z0", "member": "x"},
        {"raw": "z0", "member": "x"},
        {"raw": "z1", "member": "x"},
        {"raw": "z1", "member": "x"},
        {"raw": "z1", "member": "x"},
        {"raw": "z1", "member": "x"},
        {"raw": "zx", "member": "x"},
        {"raw": "zx", "member": "x"},
        {"raw": "zx", "member": "x"},
        {"raw": "zx", "member": "x"},
        {"raw": "zz", "member": "x"},
        {"raw": "zz", "member": "x"},
        {"raw": "zz", "member": "x"},
        {"raw": "zz", "member": "x"},
    ],
    "branch_rows": [
        {"raw": "00", "alternate": "11", "choose": "0"},
        {"raw": "00", "alternate": "11", "choose": "1"},
        {"raw": "00", "alternate": "11", "choose": "x"},
        {"raw": "00", "alternate": "11", "choose": "z"},
        {"raw": "01", "alternate": "1x", "choose": "0"},
        {"raw": "01", "alternate": "1x", "choose": "1"},
        {"raw": "01", "alternate": "1x", "choose": "x"},
        {"raw": "01", "alternate": "1x", "choose": "z"},
        {"raw": "0x", "alternate": "1z", "choose": "0"},
        {"raw": "0x", "alternate": "1z", "choose": "1"},
        {"raw": "0x", "alternate": "1z", "choose": "x"},
        {"raw": "0x", "alternate": "1z", "choose": "z"},
        {"raw": "0z", "alternate": "x0", "choose": "0"},
        {"raw": "0z", "alternate": "x0", "choose": "1"},
        {"raw": "0z", "alternate": "x0", "choose": "x"},
        {"raw": "0z", "alternate": "x0", "choose": "z"},
        {"raw": "10", "alternate": "x1", "choose": "0"},
        {"raw": "10", "alternate": "x1", "choose": "1"},
        {"raw": "10", "alternate": "x1", "choose": "x"},
        {"raw": "10", "alternate": "x1", "choose": "z"},
        {"raw": "11", "alternate": "xx", "choose": "0"},
        {"raw": "11", "alternate": "xx", "choose": "1"},
        {"raw": "11", "alternate": "xx", "choose": "x"},
        {"raw": "11", "alternate": "xx", "choose": "z"},
        {"raw": "1x", "alternate": "xz", "choose": "0"},
        {"raw": "1x", "alternate": "xz", "choose": "1"},
        {"raw": "1x", "alternate": "xz", "choose": "x"},
        {"raw": "1x", "alternate": "xz", "choose": "z"},
        {"raw": "1z", "alternate": "z0", "choose": "0"},
        {"raw": "1z", "alternate": "z0", "choose": "1"},
        {"raw": "1z", "alternate": "z0", "choose": "x"},
        {"raw": "1z", "alternate": "z0", "choose": "z"},
        {"raw": "x0", "alternate": "z1", "choose": "0"},
        {"raw": "x0", "alternate": "z1", "choose": "1"},
        {"raw": "x0", "alternate": "z1", "choose": "x"},
        {"raw": "x0", "alternate": "z1", "choose": "z"},
        {"raw": "x1", "alternate": "zx", "choose": "0"},
        {"raw": "x1", "alternate": "zx", "choose": "1"},
        {"raw": "x1", "alternate": "zx", "choose": "x"},
        {"raw": "x1", "alternate": "zx", "choose": "z"},
        {"raw": "xx", "alternate": "zz", "choose": "0"},
        {"raw": "xx", "alternate": "zz", "choose": "1"},
        {"raw": "xx", "alternate": "zz", "choose": "x"},
        {"raw": "xx", "alternate": "zz", "choose": "z"},
        {"raw": "xz", "alternate": "00", "choose": "0"},
        {"raw": "xz", "alternate": "00", "choose": "1"},
        {"raw": "xz", "alternate": "00", "choose": "x"},
        {"raw": "xz", "alternate": "00", "choose": "z"},
        {"raw": "z0", "alternate": "01", "choose": "0"},
        {"raw": "z0", "alternate": "01", "choose": "1"},
        {"raw": "z0", "alternate": "01", "choose": "x"},
        {"raw": "z0", "alternate": "01", "choose": "z"},
        {"raw": "z1", "alternate": "0x", "choose": "0"},
        {"raw": "z1", "alternate": "0x", "choose": "1"},
        {"raw": "z1", "alternate": "0x", "choose": "x"},
        {"raw": "z1", "alternate": "0x", "choose": "z"},
        {"raw": "zx", "alternate": "0z", "choose": "0"},
        {"raw": "zx", "alternate": "0z", "choose": "1"},
        {"raw": "zx", "alternate": "0z", "choose": "x"},
        {"raw": "zx", "alternate": "0z", "choose": "z"},
        {"raw": "zz", "alternate": "10", "choose": "0"},
        {"raw": "zz", "alternate": "10", "choose": "1"},
        {"raw": "zz", "alternate": "10", "choose": "x"},
        {"raw": "zz", "alternate": "10", "choose": "z"},
        {"raw": "00", "alternate": "00", "choose": "0"},
        {"raw": "00", "alternate": "00", "choose": "1"},
        {"raw": "00", "alternate": "01", "choose": "0"},
        {"raw": "00", "alternate": "01", "choose": "1"},
        {"raw": "00", "alternate": "10", "choose": "0"},
        {"raw": "00", "alternate": "10", "choose": "1"},
        {"raw": "00", "alternate": "11", "choose": "0"},
        {"raw": "00", "alternate": "11", "choose": "1"},
        {"raw": "01", "alternate": "00", "choose": "0"},
        {"raw": "01", "alternate": "00", "choose": "1"},
        {"raw": "01", "alternate": "01", "choose": "0"},
        {"raw": "01", "alternate": "01", "choose": "1"},
        {"raw": "01", "alternate": "10", "choose": "0"},
        {"raw": "01", "alternate": "10", "choose": "1"},
        {"raw": "01", "alternate": "11", "choose": "0"},
        {"raw": "01", "alternate": "11", "choose": "1"},
        {"raw": "10", "alternate": "00", "choose": "0"},
        {"raw": "10", "alternate": "00", "choose": "1"},
        {"raw": "10", "alternate": "01", "choose": "0"},
        {"raw": "10", "alternate": "01", "choose": "1"},
        {"raw": "10", "alternate": "10", "choose": "0"},
        {"raw": "10", "alternate": "10", "choose": "1"},
        {"raw": "10", "alternate": "11", "choose": "0"},
        {"raw": "10", "alternate": "11", "choose": "1"},
        {"raw": "11", "alternate": "00", "choose": "0"},
        {"raw": "11", "alternate": "00", "choose": "1"},
        {"raw": "11", "alternate": "01", "choose": "0"},
        {"raw": "11", "alternate": "01", "choose": "1"},
        {"raw": "11", "alternate": "10", "choose": "0"},
        {"raw": "11", "alternate": "10", "choose": "1"},
        {"raw": "11", "alternate": "11", "choose": "0"},
        {"raw": "11", "alternate": "11", "choose": "1"},
        {"raw": "00", "alternate": "01", "choose": "x"},
    ],
    "branch_gold": [
        {"raw": "11", "member": "0"},
        {"raw": "00", "member": "1"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "1x", "member": "0"},
        {"raw": "01", "member": "1"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "1z", "member": "0"},
        {"raw": "0x", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "x0", "member": "x"},
        {"raw": "0z", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "x1", "member": "x"},
        {"raw": "10", "member": "0"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "11", "member": "0"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xz", "member": "x"},
        {"raw": "1x", "member": "0"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "z0", "member": "x"},
        {"raw": "1z", "member": "0"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "z1", "member": "x"},
        {"raw": "x0", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "zx", "member": "x"},
        {"raw": "x1", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "zz", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "00", "member": "1"},
        {"raw": "xz", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "01", "member": "1"},
        {"raw": "z0", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "0x", "member": "x"},
        {"raw": "z1", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "0z", "member": "x"},
        {"raw": "zx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "10", "member": "0"},
        {"raw": "zz", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "10", "member": "0"},
        {"raw": "00", "member": "1"},
        {"raw": "11", "member": "0"},
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "10", "member": "0"},
        {"raw": "01", "member": "1"},
        {"raw": "11", "member": "0"},
        {"raw": "01", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "10", "member": "0"},
        {"raw": "01", "member": "1"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "00", "member": "1"},
        {"raw": "11", "member": "0"},
        {"raw": "01", "member": "1"},
        {"raw": "11", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "0x", "member": "1"},
    ],
    "rule_gold": [
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "0x", "member": "x"},
        {"raw": "0x", "member": "x"},
        {"raw": "0x", "member": "x"},
        {"raw": "0x", "member": "x"},
        {"raw": "0z", "member": "x"},
        {"raw": "0z", "member": "x"},
        {"raw": "0z", "member": "x"},
        {"raw": "0z", "member": "x"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "1x", "member": "0"},
        {"raw": "1x", "member": "0"},
        {"raw": "1x", "member": "0"},
        {"raw": "1x", "member": "0"},
        {"raw": "1z", "member": "0"},
        {"raw": "1z", "member": "0"},
        {"raw": "1z", "member": "0"},
        {"raw": "1z", "member": "0"},
        {"raw": "x0", "member": "x"},
        {"raw": "x0", "member": "x"},
        {"raw": "x0", "member": "x"},
        {"raw": "x0", "member": "x"},
        {"raw": "x1", "member": "x"},
        {"raw": "x1", "member": "x"},
        {"raw": "x1", "member": "x"},
        {"raw": "x1", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xz", "member": "x"},
        {"raw": "xz", "member": "x"},
        {"raw": "xz", "member": "x"},
        {"raw": "xz", "member": "x"},
        {"raw": "z0", "member": "x"},
        {"raw": "z0", "member": "x"},
        {"raw": "z0", "member": "x"},
        {"raw": "z0", "member": "x"},
        {"raw": "z1", "member": "x"},
        {"raw": "z1", "member": "x"},
        {"raw": "z1", "member": "x"},
        {"raw": "z1", "member": "x"},
        {"raw": "zx", "member": "x"},
        {"raw": "zx", "member": "x"},
        {"raw": "zx", "member": "x"},
        {"raw": "zx", "member": "x"},
        {"raw": "zz", "member": "x"},
        {"raw": "zz", "member": "x"},
        {"raw": "zz", "member": "x"},
        {"raw": "zz", "member": "x"},
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "00", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "10", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "00", "member": "1"},
    ],
    "pure_rows": [
        {"value": "10", "raw": "00"},
        {"value": "10", "raw": "01"},
        {"value": "10", "raw": "0x"},
        {"value": "10", "raw": "0z"},
        {"value": "10", "raw": "10"},
        {"value": "10", "raw": "11"},
        {"value": "10", "raw": "1x"},
        {"value": "10", "raw": "1z"},
        {"value": "10", "raw": "x0"},
        {"value": "10", "raw": "x1"},
        {"value": "10", "raw": "xx"},
        {"value": "10", "raw": "xz"},
        {"value": "10", "raw": "z0"},
        {"value": "10", "raw": "z1"},
        {"value": "10", "raw": "zx"},
        {"value": "10", "raw": "zz"},
    ],
    "pure_gold": [
        {"raw": "00", "member": "1"},
        {"raw": "01", "member": "1"},
        {"raw": "0x", "member": "x"},
        {"raw": "0z", "member": "x"},
        {"raw": "10", "member": "0"},
        {"raw": "11", "member": "0"},
        {"raw": "1x", "member": "0"},
        {"raw": "1z", "member": "0"},
        {"raw": "x0", "member": "x"},
        {"raw": "x1", "member": "x"},
        {"raw": "xx", "member": "x"},
        {"raw": "xz", "member": "x"},
        {"raw": "z0", "member": "x"},
        {"raw": "z1", "member": "x"},
        {"raw": "zx", "member": "x"},
        {"raw": "zz", "member": "x"},
    ],
    "owner_probes": [
        {
            "clock": "0",
            "reset": "0",
            "enable": "1",
            "raw": "11",
            "action": "xfer",
            "gold": {"prior": "00", "proposed": "11", "member": "0"},
        },
        {
            "clock": "1",
            "reset": "0",
            "enable": "1",
            "raw": "11",
            "action": "xfer",
            "gold": {"prior": "00", "proposed": "11", "member": "0"},
        },
        {
            "clock": "0",
            "reset": "0",
            "enable": "0",
            "raw": "01",
            "action": "xfer",
            "gold": {"prior": "11", "proposed": "11", "member": "1"},
        },
        {
            "clock": "1",
            "reset": "0",
            "enable": "0",
            "raw": "01",
            "action": "xfer",
            "gold": {"prior": "11", "proposed": "11", "member": "1"},
        },
        {
            "clock": "0",
            "reset": "0",
            "enable": "1",
            "raw": "0z",
            "action": "xfer",
            "gold": {"prior": "11", "proposed": "0z", "member": "x"},
        },
        {
            "clock": "1",
            "reset": "0",
            "enable": "1",
            "raw": "0z",
            "action": "discard",
            "gold": {"prior": "11", "proposed": "0z", "member": "x"},
        },
        {
            "clock": "1",
            "reset": "0",
            "enable": "1",
            "raw": "1x",
            "action": "xfer",
            "gold": {"prior": "11", "proposed": "1x", "member": "0"},
        },
        {
            "clock": "0",
            "reset": "0",
            "enable": "0",
            "raw": "00",
            "action": "xfer",
            "gold": {"prior": "1x", "proposed": "1x", "member": "1"},
        },
        {
            "clock": "1",
            "reset": "1",
            "enable": "0",
            "raw": "00",
            "action": "xfer",
            "gold": {"prior": "1x", "proposed": "1x", "member": "1"},
        },
        {
            "clock": "0",
            "reset": "0",
            "enable": "0",
            "raw": "00",
            "action": "xfer",
            "gold": {"prior": "00", "proposed": "00", "member": "1"},
        },
    ],
}

STATE_SOURCE = "import pycircuit as ac\nfrom enums.facade2 import Exported as State\nfrom enums.types import Wide, Singleton\n\n@ac.struct\nclass Entry:\n    state: State = State.ONE\n    wide: Wide = Wide.HIGH\n\n@ac.struct\nclass Nested:\n    raw: Entry\n    configured: Entry = Entry()\n\n@ac.struct\nclass NoZeroField:\n    value: Singleton\n\n@ac.struct\nclass StateResult:\n    old: State\n    old_wide: Wide\n    zero_state: State\n    default_state: State\n    zero_wide: Wide\n    default_wide: Wide\n    nested_zero: State\n    nested_default: State\n    no_zero_explicit: Singleton\n\n@ac.rule\ndef advance(owner: State, large: Wide, zero, configured, nested, data: State, wide_data: Wide, enable, index) -> StateResult:\n    # Observe every owner before proposals; no after-write interpretation is required.\n    result = StateResult(old=owner, old_wide=large,\n        zero_state=zero[index].state, default_state=configured[index].state,\n        zero_wide=zero[index].wide, default_wide=configured[index].wide,\n        nested_zero=nested.raw.state, nested_default=nested.configured.state,\n        no_zero_explicit=Singleton.ONLY)\n    if enable:\n        owner = data\n        large = wide_data\n        configured[index] = Entry(state=data, wide=wide_data)\n    return result\n\n@ac.module\ndef Stateful(data: State, wide_data: Wide, enable: ac.u1, index: ac.u1) -> StateResult:\n    owner: State = State.ONE\n    large: Wide = Wide.HIGH\n    zero = ac.table[2, Entry](init=0)\n    configured = ac.table[2, Entry](init=Entry())\n    nested: Nested = Nested()\n    # Declaring NoZeroField is legal; explicitly supply its member when constructing.\n    legal = NoZeroField(value=Singleton.ONLY)\n    return advance(owner, large, zero, configured, nested, data, wide_data, enable, index)\n"

COM_SOURCE = "import pycircuit as ac\nfrom pycircuit import enum_to_bits as raw\nfrom enums.facade2 import Exported as State\nfrom enums.types import Sparse, Singleton, Sequential, Hot, Gray, Wide, Huge\n\n@ac.struct\nclass Converted:\n    payload: ac.bits[9]\n\n@ac.struct\nclass ComResult:\n    left_raw: ac.bits[2]\n    selected: ac.bits[2]\n    selected_member: ac.bits[2]\n    equal: ac.u1\n    unequal: ac.u1\n    equals_member: ac.u1\n    wider_direct: ac.bits[9]\n    wider_alias: ac.bits[9]\n    wider_field: ac.bits[9]\n    sparse_member: ac.bits[3]\n    singleton_member: ac.bits[1]\n    sequential_member: ac.bits[3]\n    hot_member: ac.bits[4]\n    gray_member: ac.bits[3]\n    wide_member: ac.bits[65]\n    huge_member: ac.bits[130]\n    wide_raw: ac.bits[65]\n    huge_raw: ac.bits[130]\n\n@ac.rule\ndef evaluate(left: State, right: State, wide: Wide, huge: Huge, choose) -> ComResult:\n    alias: State = left\n    first_raw = raw(alias)\n    wider_direct: ac.bits[9] = ac.enum_to_bits(alias)\n    wider_alias: ac.bits[9] = first_raw\n    converted = Converted(payload=ac.enum_to_bits(alias))\n    member = State.ONE\n    selected = left if choose else right\n    return ComResult(left_raw=first_raw, selected=raw(selected),\n        selected_member=raw(member if choose else left),\n        equal=left == right, unequal=left != right, equals_member=left == State.ONE,\n        wider_direct=wider_direct, wider_alias=wider_alias, wider_field=converted.payload,\n        sparse_member=raw(Sparse.DONE), singleton_member=raw(Singleton.ONLY),\n        sequential_member=raw(Sequential.END), hot_member=raw(Hot.D), gray_member=raw(Gray.D),\n        wide_member=raw(Wide.HIGH), huge_member=raw(Huge.HIGH),\n        wide_raw=raw(wide), huge_raw=raw(huge))\n\n@ac.module\ndef Com(left: State, right: State, wide: Wide, huge: Huge, choose: ac.u1) -> ComResult:\n    return evaluate(left, right, wide, huge, choose)\n"

# S3b runtime inputs/goldens follow the accepted source contracts, independently
# of the emitted models. S3c public enum_from_bits tuple syntax is not used.
if args.semantics:
    CLOCK, RESET = "pyc_7079635f636c6b", "pyc_7079635f727374"
    fixtures = Path(__file__).resolve().parent
    runtime_root = Path(args.source_compiler).resolve().parent.parent
    runtime = next(
        p
        for p in (
            runtime_root / "runtime/libpyc6_runtime.a",
            runtime_root / "lib/libpyc6_runtime.a",
        )
        if p.is_file()
    )

    def word(value, width):
        return format(value % (1 << width), f"0{width}b")

    def choose(control, yes, no):
        if control == "1":
            return yes
        if control == "0":
            return no
        return "".join(a if a == b else "x" for a, b in zip(yes, no, strict=True))

    def equal(a, b):
        if any(x in "01" and y in "01" and x != y for x, y in zip(a, b, strict=True)):
            return "0"
        return "1" if a == b and all(x in "01" for x in a) else "x"

    def invert(value):
        return "1" if value == "0" else "0" if value == "1" else "x"

    def wide_patterns(width):
        patterns = [
            word(0, width),
            word(1, width),
            word((1 << width) - 1, width),
            word((1 << (width - 1)) + 7, width),
        ]
        for bit in sorted({0, 1, 63, 64, width - 1}):
            for symbol in "xz":
                value = list(word(0, width))
                value[width - bit - 1] = symbol
                patterns.append("".join(value))
        return patterns

    from enum_source_execution import execution_tools

    make_vectors, execute = execution_tools(
        args=args,
        repo=repo,
        fixtures=fixtures,
        build=build,
        source=source,
        units=units,
        declaration_sources=declaration_sources,
        runtime=runtime,
        compile_unit=compile_unit,
        cli=cli,
        run=run,
        payload=payload,
        digest=digest,
        clock=CLOCK,
    )

    com_inputs = {"left": 2, "right": 2, "wide": 65, "huge": 130, "choose": 1}
    com_fields = {
        "left_raw": 2,
        "selected": 2,
        "selected_member": 2,
        "equal": 1,
        "unequal": 1,
        "equals_member": 1,
        "wider_direct": 9,
        "wider_alias": 9,
        "wider_field": 9,
        "sparse_member": 3,
        "singleton_member": 1,
        "sequential_member": 3,
        "hot_member": 4,
        "gray_member": 3,
        "wide_member": 65,
        "huge_member": 130,
        "wide_raw": 65,
        "huge_raw": 130,
    }
    patterns2 = ["".join(bits) for bits in itertools.product("01xz", repeat=2)]
    wide65, wide130 = wide_patterns(65), wide_patterns(130)
    com_rows, com_gold = [], []
    for number, (left, right, control) in enumerate(
        itertools.product(patterns2, patterns2, "01xz")
    ):
        row = {
            "left": left,
            "right": right,
            "wide": wide65[number % len(wide65)],
            "huge": wide130[(number * 5) % len(wide130)],
            "choose": control,
        }
        com_rows.append(row)
    for left, right, control in itertools.product(
        ("00", "01", "10", "11"), ("00", "01", "10", "11"), "01"
    ):
        com_rows.append(
            {
                "left": left,
                "right": right,
                "wide": word(0, 65),
                "huge": word(0, 130),
                "choose": control,
            }
        )
    for row in com_rows:
        left, right, control = row["left"], row["right"], row["choose"]
        com_gold.append(
            {
                "left_raw": left,
                "selected": choose(control, left, right),
                "selected_member": choose(control, "01", left),
                "equal": equal(left, right),
                "unequal": invert(equal(left, right)),
                "equals_member": equal(left, "01"),
                "wider_direct": "0000000" + left,
                "wider_alias": "0000000" + left,
                "wider_field": "0000000" + left,
                "sparse_member": "101",
                "singleton_member": "1",
                "sequential_member": "011",
                "hot_member": "1000",
                "gray_member": "010",
                "wide_member": word((1 << 64) + 3, 65),
                "huge_member": word((1 << 129) + 7, 130),
                "wide_raw": row["wide"],
                "huge_raw": row["huge"],
            }
        )
    semantics_receipts = [
        execute("values", COM_SOURCE, "Com", com_inputs, com_fields, com_rows, com_gold)
    ]
    state_inputs = {
        "data": 2,
        "wide_data": 65,
        "enable": 1,
        "index": 1,
        CLOCK: 1,
        RESET: 1,
    }
    state_fields = {
        "old": 2,
        "old_wide": 65,
        "zero_state": 2,
        "default_state": 2,
        "zero_wide": 65,
        "default_wide": 65,
        "nested_zero": 2,
        "nested_default": 2,
        "no_zero_explicit": 1,
    }

    def state_row(clock="0", reset="0", enable="0", index="0", data="00", wide=None):
        return {
            "data": data,
            "wide_data": word(0, 65) if wide is None else wide,
            "enable": enable,
            "index": index,
            CLOCK: clock,
            RESET: reset,
        }

    class StateOracle:
        def __init__(self):
            self.clock = "0"
            self.initialize()

        def initialize(self):
            self.owner = "01"
            self.wide = word((1 << 64) + 3, 65)
            self.lanes = [(self.owner, self.wide), (self.owner, self.wide)]
            self.computed = [False, False]

        def sample(self, row, action=0):
            clock, reset, enable, index = (
                row[CLOCK],
                row[RESET],
                row["enable"],
                row["index"],
            )
            lane = self.lanes[int(index)] if index in "01" else ("xx", "x" * 65)
            output = {
                "old": self.owner,
                "old_wide": self.wide,
                "zero_state": "00" if index in "01" else "xx",
                "default_state": lane[0],
                "zero_wide": word(0, 65) if index in "01" else "x" * 65,
                "default_wide": lane[1],
                "nested_zero": "00",
                "nested_default": "01",
                "no_zero_explicit": "1",
            }
            self.plane_fields = set(state_fields)
            if index in "xz":
                self.plane_fields -= {
                    "zero_state",
                    "zero_wide",
                    "default_state",
                    "default_wide",
                }
            elif self.computed[int(index)]:
                self.plane_fields -= {"default_state", "default_wide"}
            rising = self.clock == "0" and clock == "1"
            failure = clock in "xz" or (
                rising and (reset in "xz" or (reset == "0" and enable in "xz"))
            )
            assert failure == (action == 2), (row, action)
            if action == 0:
                if rising:
                    if reset == "1":
                        self.initialize()
                    elif enable == "1":
                        self.owner, self.wide = row["data"], row["wide_data"]
                        if index in "01":
                            self.lanes[int(index)] = (self.owner, self.wide)
                            self.computed[int(index)] = False
                        else:
                            self.lanes = [
                                (
                                    choose("x", self.owner, old),
                                    choose("x", self.wide, wide),
                                )
                                for old, wide in self.lanes
                            ]
                            self.computed = [True, True]
                self.clock = clock
            return output

    state_rows = []
    for index in range(8):
        row = state_row(
            enable=str(index % 2),
            index=str((index // 2) % 2),
            data=word(index, 2),
            wide=word((1 << 64) + index, 65),
        )
        state_rows += [row, row | {CLOCK: "1"}, row]
    state_rows += [
        state_row("1", "0", "1", "1", "11"),
        state_row("1", "1"),
        state_row("0", "1"),
        state_row("1", "1"),
        state_row(),
    ]
    state_known_prefix = len(state_rows)
    for index, data_word in enumerate(patterns2):
        row = state_row(
            enable="1",
            index=str(index % 2),
            data=data_word,
            wide=wide65[index % len(wide65)],
        )
        state_rows += [row, row | {CLOCK: "1"}, row]
    for symbol in "xz":
        row = state_row(enable="1", index=symbol, data="zz", wide="z" * 65)
        state_rows += [
            row,
            row | {CLOCK: "1"},
            state_row(index="0"),
            state_row(index="1"),
        ]
    state_rows += [
        state_row("1", "1", "x"),
        state_row(),
        state_row("1", "0", "0", "1", "zz"),
        state_row(),
    ]
    state_oracle = StateOracle()
    state_gold, state_planes = [], []
    for row in state_rows:
        state_gold.append(state_oracle.sample(row))
        state_planes.append(state_oracle.plane_fields)
    probes = [
        (state_row(), 0),
        (state_row("1", enable="1", data="xz", wide="z" + "x" * 64), 1),
        (state_row("1", enable="1", data="xz", wide="z" + "x" * 64), 0),
        (state_row(), 0),
    ]
    failures = []
    for control in ("enable", CLOCK, RESET):
        for symbol in "xz":
            failed = state_row("1", enable="1", data="zx", wide="x" + "z" * 64)
            failed[control] = symbol
            failures.append(failed)
            probes += [
                (failed, 2),
                (state_row("1", enable="1", data="11"), 0),
                (state_row(), 0),
            ]
    probes += [(state_row("1", "1", "x"), 0), (state_row(), 0)]
    probe_oracle = StateOracle()
    probe_gold, probe_planes = [], []
    for row, action in probes:
        probe_gold.append(probe_oracle.sample(row, action))
        probe_planes.append(probe_oracle.plane_fields)
    semantics_receipts.append(
        execute(
            "storage",
            STATE_SOURCE,
            "Stateful",
            state_inputs,
            state_fields,
            state_rows,
            state_gold,
            True,
            probes,
            probe_gold,
            failures,
            state_known_prefix,
            state_planes,
            probe_planes,
        )
    )

    # Independent B3 packet: precise owning diagnostics remain distinct. In
    # particular, a fixed-left arithmetic rejection is the unsigned type guard,
    # not evidence that the dedicated Enum arithmetic guard ran.
    b3_source_archive = build / "b3-source-inputs"
    b3_source_archive.mkdir()
    b3_receipts = []
    alias_provider = None
    for case in B3_CASES:
        if case["name"] != "alias-provider":
            continue
        filename = "alias_provider.py"
        (source / filename).write_text(case["source"])
        (b3_source_archive / filename).write_text(case["source"])
        alias_provider = build / "b3-alias-provider"
        compile_unit(filename, alias_provider)
        assert (
            'ac.unit_kind = "declarations"'
            in payload(alias_provider, "body").read_text()
        )
        (source / filename).unlink()
        b3_receipts.append(
            {
                "case": case["name"],
                "positive": True,
                "exit_status": 0,
                "body_sha256": digest(payload(alias_provider, "body")),
            }
        )
    assert alias_provider is not None
    protected_paths = [
        Path(receipt[key])
        for receipt in semantics_receipts
        for key in ("unit", "final")
    ]
    protected_paths += [
        Path(receipt["output"]) / target
        for receipt in semantics_receipts
        for target in ("cpp", "verilog")
    ]
    protected_outputs = {str(path): managed_snapshot(path) for path in protected_paths}
    for case in B3_CASES:
        name = case["name"]
        if name == "alias-provider":
            continue
        text = case["source"].replace(
            "from independent.alias_provider", "from enums.alias_provider"
        )
        (source / "values.py").write_text(text)
        archived = b3_source_archive / (name + ".py")
        archived.write_text(text)
        supplied = (
            [*units.values(), alias_provider]
            if name == "control-import-enum-bool"
            else list(units.values())
        )
        fresh = build / ("b3-" + name)
        if case["positive"]:
            compile_unit("values.py", fresh, supplied)
            body = payload(fresh, "body").read_text()
            if name in (
                "table-State-zero",
                "table-State-constructor",
                "table-NoZero-constructor",
            ):
                expected_member = {
                    "table-State-zero": "ZERO",
                    "table-State-constructor": "ONE",
                    "table-NoZero-constructor": "SECOND",
                }[name]
                creates = [
                    line for line in body.splitlines() if '"ac.enum.create"' in line
                ]
                assert (
                    len(creates) == 1 and f'member = "{expected_member}"' in creates[0]
                ), (name, creates)
            if name == "control-import-enum-bool":
                assert not (source / "alias_provider.py").exists()
                assert '!ac.enum<"enums.alias_provider.State">' in body, body
                assert (
                    "ac.interfaces = " in body and 'path = "alias_provider.py"' in body
                ), body
            b3_receipts.append(
                {
                    "case": name,
                    "positive": True,
                    "exit_status": 0,
                    "source_sha256": digest(archived),
                    "body_sha256": digest(payload(fresh, "body")),
                }
            )
        else:
            diagnostic = case["expected_diagnostic"]
            compile_unit("values.py", fresh, supplied, code=1, diagnostic=diagnostic)
            assert not fresh.exists(), name
            compile_unit(
                "values.py",
                Path(semantics_receipts[0]["unit"]),
                supplied,
                replace=True,
                code=1,
                diagnostic=diagnostic,
            )
            assert {
                str(path): managed_snapshot(path) for path in protected_paths
            } == protected_outputs, name
            b3_receipts.append(
                {
                    "case": name,
                    "positive": False,
                    "fresh_exit_status": 1,
                    "replacement_exit_status": 1,
                    "diagnostic": diagnostic,
                    "category": case["category"],
                    "source_sha256": digest(archived),
                }
            )

    def tuple_membership(raw, codes):
        comparisons = [equal(raw, word(code, len(raw))) for code in codes]
        return "1" if "1" in comparisons else "x" if "x" in comparisons else "0"

    semantics_receipts.append(
        execute(
            "tuple_module",
            S3C["positive"]["tuple-module"],
            "Top",
            S3C["inputs"],
            S3C["fields"],
            S3C["rows"],
            S3C["gold"],
            plane_inputs=S3C["planes"],
            latent=True,
        )
    )
    pair_fields = {"raw": 2, "member": 1}
    tuple_receipts = [semantics_receipts[-1]]
    tuple_receipts.append(
        execute(
            "tuple_raw_once",
            S3C["positive"]["raw-once"],
            "Top",
            {"raw": 3},
            pair_fields,
            S3C["raw_once_rows"],
            S3C["raw_once_gold"],
            plane_inputs={"raw": ("raw", 1)},
        )
    )
    branch_rows, branch_gold = S3C["branch_rows"], S3C["branch_gold"]
    branch_planes = []
    for row in branch_rows:
        branch_planes.append(
            ""
            if row["choose"] in "xz"
            else (row["raw"] if row["choose"] == "1" else row["alternate"])
        )
    # Dynamic branch selection uses symbol goldens for unknown choose; both
    # original membership results are joined independently from the carriers.
    tuple_receipts.append(
        execute(
            "tuple_both_arms",
            S3C["positive"]["both-arms"],
            "Top",
            {"raw": 2, "alternate": 2, "choose": 1},
            pair_fields,
            branch_rows,
            branch_gold,
            plane_inputs={
                "raw": {"control": "choose", "yes": "raw", "no": "alternate"}
            },
        )
    )
    for name in ("tuple-rule", "partial-recovered", "existing-boundaries"):
        # One bounded raw/control matrix covers definite assignment/boundaries;
        # unlike both-arms, these return the unconditional decode of raw.
        rows = [
            {"raw": row["raw"], "alternate": row["alternate"], "choose": row["choose"]}
            for row in branch_rows[:64]
        ]
        gold = [
            {"raw": row["raw"], "member": tuple_membership(row["raw"], (0, 1))}
            for row in rows
        ]
        rows += [
            {"raw": row["raw"], "alternate": row["alternate"], "choose": row["choose"]}
            for row in branch_rows[64:96]
        ]
        gold += [
            {"raw": row["raw"], "member": tuple_membership(row["raw"], (0, 1))}
            for row in rows[64:]
        ]
        tuple_receipts.append(
            execute(
                "tuple_" + name.replace("-", "_"),
                S3C["positive"][name],
                "Top",
                {"raw": 2, "alternate": 2, "choose": 1},
                pair_fields,
                rows,
                gold,
                plane_inputs={"raw": "raw"},
            )
        )
    tuple_receipts.append(
        execute(
            "tuple_pure_formal",
            S3C["positive"]["pure-formal"],
            "Top",
            {"value": 2, "raw": 2},
            pair_fields,
            S3C["pure_rows"],
            S3C["pure_gold"],
            plane_inputs={"raw": "raw"},
        )
    )

    # Ordinary assignment after decoding is the owner-writing path. Native
    # discard is isolated from RTL, which executes only actual clock/reset inputs.
    def tuple_owner_sample(row, state, clock):
        proposed = choose(row["enable"], row["raw"], state)
        return {
            "prior": state,
            "proposed": proposed,
            "member": tuple_membership(row["raw"], (0, 1)),
        }

    owner_inputs = {"raw": 2, "enable": 1, CLOCK: 1, RESET: 1}
    owner_fields = {"prior": 2, "proposed": 2, "member": 1}
    owner_rows, owner_gold, owner_probes, owner_probe_gold = [], [], [], []
    state, clock = "00", "0"
    for action in S3C["owner_probes"]:
        row = {
            "raw": action["raw"],
            "enable": action["enable"],
            CLOCK: action["clock"],
            RESET: action["reset"],
        }
        owner_probes.append((row, int(action["action"] == "discard")))
        expected = tuple_owner_sample(row, state, clock)
        assert expected == action["gold"], (action, expected)
        owner_probe_gold.append(expected)
        if action["action"] != "discard":
            if clock == "0" and row[CLOCK] == "1":
                state = (
                    "00"
                    if row[RESET] == "1"
                    else (row["raw"] if row["enable"] == "1" else state)
                )
            clock = row[CLOCK]
    state, clock = "00", "0"
    for action in S3C["owner_probes"]:
        if action["action"] == "discard":
            continue
        row = {
            "raw": action["raw"],
            "enable": action["enable"],
            CLOCK: action["clock"],
            RESET: action["reset"],
        }
        owner_rows.append(row)
        owner_gold.append(tuple_owner_sample(row, state, clock))
        if clock == "0" and row[CLOCK] == "1":
            state = (
                "00"
                if row[RESET] == "1"
                else (row["raw"] if row["enable"] == "1" else state)
            )
        clock = row[CLOCK]
    owner_known_prefix = next(
        i
        for i, row in enumerate(owner_rows)
        if any(c in "xz" for value in row.values() for c in value)
    )
    tuple_receipts.append(
        execute(
            "tuple_owner",
            S3C["positive"]["owner-lifecycle"],
            "Top",
            owner_inputs,
            owner_fields,
            owner_rows,
            owner_gold,
            True,
            owner_probes,
            owner_probe_gold,
            [],
            owner_known_prefix,
            [{"prior", "proposed"} for _ in owner_rows],
            [{"prior", "proposed"} for _ in owner_probes],
        )
    )
    semantics_receipts += tuple_receipts[1:]

    guarded_source = S3C["positive"]["owner-lifecycle"].replace(
        "    if enable:\n", "    if enable and member:\n"
    )

    def guarded_row(clock="0", reset="0", enable="0", raw="0x"):
        return {"raw": raw, "enable": enable, CLOCK: clock, RESET: reset}

    def guarded_sample(row, state):
        member = tuple_membership(row["raw"], (0, 1))
        effective = (
            "0"
            if "0" in (row["enable"], member)
            else "1" if row["enable"] == member == "1" else "x"
        )
        return {
            "prior": state,
            "proposed": choose(effective, row["raw"], state),
            "member": member,
        }, effective

    guarded_known_rows = [
        guarded_row(raw="00"),
        guarded_row("1", raw="00"),
        guarded_row(raw="11", enable="1"),
        guarded_row("1", raw="11", enable="1"),
        guarded_row(raw="01", enable="1"),
        guarded_row("1", raw="01", enable="1"),
        guarded_row(raw="00"),
    ]
    guarded_rows = guarded_known_rows + [
        guarded_row(),
        guarded_row("1"),
        guarded_row(),
        guarded_row("1", enable="1", raw="11"),
        guarded_row(),
        guarded_row("1", enable="1", raw="01"),
        guarded_row(),
        guarded_row("1", enable="0", raw="0z"),
        guarded_row(),
        guarded_row("1", "1", "1", "0x"),
        guarded_row(),
    ]
    guarded_gold = []
    state, clock = "00", "0"
    for row in guarded_rows:
        gold, effective = guarded_sample(row, state)
        guarded_gold.append(gold)
        if clock == "0" and row[CLOCK] == "1":
            if row[RESET] == "1":
                state = "00"
            elif effective == "1":
                state = row["raw"]
            else:
                assert effective == "0", row
        clock = row[CLOCK]
    guarded_probes = [
        (guarded_row(), 0),
        (guarded_row("1", enable="1", raw="01"), 1),
        (guarded_row("1", enable="1", raw="01"), 0),
        (guarded_row(), 0),
    ]
    guarded_failures = []
    for raw in ("0x", "0z"):
        bad = guarded_row("1", enable="1", raw=raw)
        guarded_failures.append(bad)
        guarded_probes += [
            (bad, 2),
            (guarded_row("1", enable="1", raw="00"), 0),
            (guarded_row(), 0),
        ]
    guarded_probe_gold = []
    state, clock = "00", "0"
    for row, action in guarded_probes:
        gold, effective = guarded_sample(row, state)
        guarded_probe_gold.append(gold)
        failure = (
            clock == "0"
            and row[CLOCK] == "1"
            and row[RESET] == "0"
            and effective == "x"
        )
        assert failure == (action == 2), (row, action)
        if action == 0:
            if clock == "0" and row[CLOCK] == "1":
                state = (
                    "00"
                    if row[RESET] == "1"
                    else (row["raw"] if effective == "1" else state)
                )
            clock = row[CLOCK]
    semantics_receipts.append(
        execute(
            "tuple_guarded_owner",
            guarded_source,
            "Top",
            owner_inputs,
            owner_fields,
            guarded_rows,
            guarded_gold,
            True,
            guarded_probes,
            guarded_probe_gold,
            guarded_failures,
            len(guarded_known_rows),
            [{"prior"} for _ in guarded_rows],
            [{"prior"} for _ in guarded_probes],
        )
    )

    # Every rejection replaces its actual named nearby positive unit, preserving
    # its whole public closure and all previously emitted runtime products.
    tuple_controls = {
        "tuple-module": tuple_receipts[0],
        "raw-once": tuple_receipts[1],
        "both-arms": tuple_receipts[2],
        "tuple-rule": tuple_receipts[3],
        "partial-recovered": tuple_receipts[4],
        "existing-boundaries": tuple_receipts[5],
        "pure-formal": tuple_receipts[6],
        "owner-lifecycle": tuple_receipts[7],
    }
    tuple_source_archive = build / "tuple-source-inputs"
    tuple_source_archive.mkdir()
    tuple_control_receipts = []
    for name, text in S3C["positive"].items():
        (tuple_source_archive / (name + ".py")).write_text(text)
        if name in tuple_controls:
            continue
        filename = "tuple_control_" + name.replace("-", "_") + ".py"
        (source / filename).write_text(text)
        output = build / ("tuple-control-" + name)
        output.mkdir()
        unit = output / "unit"
        compile_unit(filename, unit, list(units.values()))
        final = output / "control.ac"
        cli(
            "link",
            *[units[n] for n in declaration_sources],
            unit,
            "--top",
            "enums." + filename.removesuffix(".py") + ".Top",
            "-o",
            final,
        )
        run(
            [
                args.optimizer,
                final,
                "--ac-verify-hardware",
                "-o",
                output / "verified.ac",
            ]
        )
        tuple_controls[name] = {
            "unit": str(unit),
            "final": str(final),
            "filename": filename,
        }
        tuple_control_receipts.append(
            {
                "control": name,
                "unit": str(unit),
                "final": str(final),
                "body_sha256": digest(payload(unit, "body")),
                "final_sha256": digest(final),
            }
        )
    tuple_protected_paths = [
        Path(receipt[key])
        for receipt in semantics_receipts
        for key in ("unit", "final")
    ]
    tuple_protected_paths += [
        Path(receipt["output"]) / target
        for receipt in semantics_receipts
        for target in ("cpp", "verilog")
    ]
    tuple_protected_paths += [
        Path(receipt[key])
        for receipt in tuple_controls.values()
        for key in ("unit", "final")
    ]
    tuple_protected_paths = list(dict.fromkeys(tuple_protected_paths))
    tuple_snapshot = {
        str(path): managed_snapshot(path) for path in tuple_protected_paths
    }
    tuple_guard_receipts = []
    for name, case in S3C["negative"].items():
        control = tuple_controls[case["control"]]
        filename = control.get(
            "filename",
            json.loads((Path(control["unit"]) / "unit.json").read_text())["source"][
                "path"
            ],
        )
        (source / filename).write_text(case["source"])
        archived = tuple_source_archive / ("negative-" + name + ".py")
        archived.write_text(case["source"])
        fresh = build / ("tuple-negative-" + name)
        diagnostic = S3C_GUARDS[name]
        compile_unit(
            filename, fresh, list(units.values()), code=1, diagnostic=diagnostic
        )
        assert not fresh.exists(), name
        compile_unit(
            filename,
            Path(control["unit"]),
            list(units.values()),
            replace=True,
            code=1,
            diagnostic=diagnostic,
        )
        assert {
            str(path): managed_snapshot(path) for path in tuple_protected_paths
        } == tuple_snapshot, name
        tuple_guard_receipts.append(
            {
                "case": name,
                "control": case["control"],
                "guard_intention": case["guard_intention"],
                "diagnostic": diagnostic,
                "source": str(archived),
                "source_sha256": digest(archived),
                "fresh_exit_status": 1,
                "replacement_exit_status": 1,
            }
        )

    data = json.loads((evidence / "candidate.json").read_text())
    data["source_semantics"] = semantics_receipts
    data["source_semantic_guards"] = b3_receipts
    data["tuple_guards"] = tuple_guard_receipts
    data["tuple_compile_controls"] = tuple_control_receipts
    data["tuple_branch_witness"] = {
        "inputs": {"raw": "00", "alternate": "01", "choose": "x"},
        "expected": {"raw": "0x", "member": "1"},
    }
    data["native_plane_scope"] = (
        "Raw enum ports/to_bits/widened bits and deterministic selection; storage raw scalar/table transport. Computed table updates assert observable four-state symbols, including unknown-index get/commit; no hidden-X value plane is invented."
    )
    data[
        "scope"
    ] += "; S3b B1/B2 runtime/B3 guards plus S3c typed tuple carrier/membership/runtime/IR witnesses"
    data["deferred"] = "general imported struct default construction"
    data["fixtures"].update(
        {
            str(fixtures / ("enum-source" + suffix)): digest(
                fixtures / ("enum-source" + suffix)
            )
            for suffix in (".cpp", ".sv")
        }
    )
    data["tools"].update(
        {
            str(Path(tool).resolve()): digest(Path(tool))
            for tool in (
                args.emitter,
                args.cxx,
                args.verilator,
                args.iverilog,
                args.vvp,
            )
        }
    )
    data["runtime"] = {str(runtime): digest(runtime)}
    (evidence / "candidate.json").write_text(json.dumps(data, indent=2) + "\n")
    print(
        "Enum source gate passed: declarations/members/tuples/storage, native1/2/genuine RTL and protected owning guards"
    )  # noqa: T201
