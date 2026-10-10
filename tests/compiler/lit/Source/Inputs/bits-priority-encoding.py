"""Independent priority/one-hot source, generated-model and protection gate.

Expected behavior comes from scalar ternary folding in the native/RTL fixtures;
this host harness only authors source fixtures and checks schemas/publication.
"""

import argparse
import hashlib
import json
import os
import re
import shutil
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
    "optimizer",
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
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
    PYCIRCUIT_EMITTER=args.emitter,
)
commands = []


def run(command, code=0, diagnostic=None):
    command = list(map(str, command))
    result = subprocess.run(
        command, env=env, cwd=repo, capture_output=True, text=True, timeout=300
    )
    row = {
        "command": command,
        "exit_status": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
    }
    commands.append(row)
    (build / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == code, row
    assert (
        "Assertion failed" not in result.stderr and "Traceback" not in result.stderr
    ), row
    if diagnostic:
        assert diagnostic in result.stderr, row
    return result


def cli(*arguments, **options):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], **options)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def snapshot(path):
    return {
        p.relative_to(path).as_posix(): p.read_bytes()
        for p in path.rglob("*")
        if p.is_file()
    }


def managed(path):
    control = path.parent / ("." + path.name + ".pycircuit-publication")
    return {
        "product": snapshot(path) if path.is_dir() else path.read_bytes(),
        "publication": snapshot(control),
    }


def assert_absent(path):
    assert not path.exists(), path
    control = path.parent / ("." + path.name + ".pycircuit-publication")
    assert not (control / "stage").exists(), control
    assert not (control / "previous").exists(), control


def payload(unit, kind):
    return unit / json.loads((unit / "unit.json").read_text())["files"][kind]


WIDTHS = (1, 2, 3, 4, 5, 8, 9, 13, 31, 32, 33, 63, 64, 65, 73, 127, 128, 129)
HELPERS = ("priority_encode", "onehot_encode")
PREFIX = "from typing import Annotated\nfrom enum import Enum\nimport pycircuit as ac\n"
ENUM = "@ac.encoding(width=3)\nclass State(Enum):\n    ZERO = 0\n    ONE = 1\n"
PROVIDER = """from typing import Annotated
import pycircuit as ac
@ac.struct
class FixedResult:
    value: ac.u5
@ac.module
def Fixed(value: ac.u5) -> FixedResult:
    return FixedResult(value=value)
@ac.module
def Logical(value: Annotated[int, range(1 << 5)], flag: bool) -> {"integer": Annotated[int, range(1 << 5)], "boolean": bool}:
    return {"integer": value, "boolean": flag}
"""
FIELDS = [
    ("pLIndex1", 1),
    ("pLValid1", 1),
    ("pHIndex1", 1),
    ("pHValid1", 1),
    ("oLIndex1", 1),
    ("oLValid1", 1),
    ("oLConflict1", 1),
    ("oHIndex1", 1),
    ("oHValid1", 1),
    ("oHConflict1", 1),
    ("pLIndex2", 1),
    ("pLValid2", 1),
    ("pHIndex2", 1),
    ("pHValid2", 1),
    ("oLIndex2", 1),
    ("oLValid2", 1),
    ("oLConflict2", 1),
    ("oHIndex2", 1),
    ("oHValid2", 1),
    ("oHConflict2", 1),
    ("pLIndex3", 2),
    ("pLValid3", 1),
    ("pHIndex3", 2),
    ("pHValid3", 1),
    ("oLIndex3", 2),
    ("oLValid3", 1),
    ("oLConflict3", 1),
    ("oHIndex3", 2),
    ("oHValid3", 1),
    ("oHConflict3", 1),
    ("pLIndex4", 2),
    ("pLValid4", 1),
    ("pHIndex4", 2),
    ("pHValid4", 1),
    ("oLIndex4", 2),
    ("oLValid4", 1),
    ("oLConflict4", 1),
    ("oHIndex4", 2),
    ("oHValid4", 1),
    ("oHConflict4", 1),
    ("pLIndex5", 3),
    ("pLValid5", 1),
    ("pHIndex5", 3),
    ("pHValid5", 1),
    ("oLIndex5", 3),
    ("oLValid5", 1),
    ("oLConflict5", 1),
    ("oHIndex5", 3),
    ("oHValid5", 1),
    ("oHConflict5", 1),
    ("pLIndex8", 3),
    ("pLValid8", 1),
    ("pHIndex8", 3),
    ("pHValid8", 1),
    ("oLIndex8", 3),
    ("oLValid8", 1),
    ("oLConflict8", 1),
    ("oHIndex8", 3),
    ("oHValid8", 1),
    ("oHConflict8", 1),
    ("pLIndex9", 4),
    ("pLValid9", 1),
    ("pHIndex9", 4),
    ("pHValid9", 1),
    ("oLIndex9", 4),
    ("oLValid9", 1),
    ("oLConflict9", 1),
    ("oHIndex9", 4),
    ("oHValid9", 1),
    ("oHConflict9", 1),
    ("pLIndex13", 4),
    ("pLValid13", 1),
    ("pHIndex13", 4),
    ("pHValid13", 1),
    ("oLIndex13", 4),
    ("oLValid13", 1),
    ("oLConflict13", 1),
    ("oHIndex13", 4),
    ("oHValid13", 1),
    ("oHConflict13", 1),
    ("pLIndex31", 5),
    ("pLValid31", 1),
    ("pHIndex31", 5),
    ("pHValid31", 1),
    ("oLIndex31", 5),
    ("oLValid31", 1),
    ("oLConflict31", 1),
    ("oHIndex31", 5),
    ("oHValid31", 1),
    ("oHConflict31", 1),
    ("pLIndex32", 5),
    ("pLValid32", 1),
    ("pHIndex32", 5),
    ("pHValid32", 1),
    ("oLIndex32", 5),
    ("oLValid32", 1),
    ("oLConflict32", 1),
    ("oHIndex32", 5),
    ("oHValid32", 1),
    ("oHConflict32", 1),
    ("pLIndex33", 6),
    ("pLValid33", 1),
    ("pHIndex33", 6),
    ("pHValid33", 1),
    ("oLIndex33", 6),
    ("oLValid33", 1),
    ("oLConflict33", 1),
    ("oHIndex33", 6),
    ("oHValid33", 1),
    ("oHConflict33", 1),
    ("pLIndex63", 6),
    ("pLValid63", 1),
    ("pHIndex63", 6),
    ("pHValid63", 1),
    ("oLIndex63", 6),
    ("oLValid63", 1),
    ("oLConflict63", 1),
    ("oHIndex63", 6),
    ("oHValid63", 1),
    ("oHConflict63", 1),
    ("pLIndex64", 6),
    ("pLValid64", 1),
    ("pHIndex64", 6),
    ("pHValid64", 1),
    ("oLIndex64", 6),
    ("oLValid64", 1),
    ("oLConflict64", 1),
    ("oHIndex64", 6),
    ("oHValid64", 1),
    ("oHConflict64", 1),
    ("pLIndex65", 7),
    ("pLValid65", 1),
    ("pHIndex65", 7),
    ("pHValid65", 1),
    ("oLIndex65", 7),
    ("oLValid65", 1),
    ("oLConflict65", 1),
    ("oHIndex65", 7),
    ("oHValid65", 1),
    ("oHConflict65", 1),
    ("pLIndex73", 7),
    ("pLValid73", 1),
    ("pHIndex73", 7),
    ("pHValid73", 1),
    ("oLIndex73", 7),
    ("oLValid73", 1),
    ("oLConflict73", 1),
    ("oHIndex73", 7),
    ("oHValid73", 1),
    ("oHConflict73", 1),
    ("pLIndex127", 7),
    ("pLValid127", 1),
    ("pHIndex127", 7),
    ("pHValid127", 1),
    ("oLIndex127", 7),
    ("oLValid127", 1),
    ("oLConflict127", 1),
    ("oHIndex127", 7),
    ("oHValid127", 1),
    ("oHConflict127", 1),
    ("pLIndex128", 7),
    ("pLValid128", 1),
    ("pHIndex128", 7),
    ("pHValid128", 1),
    ("oLIndex128", 7),
    ("oLValid128", 1),
    ("oLConflict128", 1),
    ("oHIndex128", 7),
    ("oHValid128", 1),
    ("oHConflict128", 1),
    ("pLIndex129", 8),
    ("pLValid129", 1),
    ("pHIndex129", 8),
    ("pHValid129", 1),
    ("oLIndex129", 8),
    ("oLValid129", 1),
    ("oLConflict129", 1),
    ("oHIndex129", 8),
    ("oHValid129", 1),
    ("oHConflict129", 1),
    ("Widen1Index", 8),
    ("Widen1Valid", 1),
    ("Widen1Conflict", 1),
    ("Slice73Index", 4),
    ("Slice73Valid", 1),
    ("Slice73Conflict", 1),
    ("Arithmetic5Index", 3),
    ("Arithmetic5Valid", 1),
    ("Arithmetic5Conflict", 1),
    ("Enum3Index", 2),
    ("Enum3Valid", 1),
    ("Enum3Conflict", 1),
    ("Child5Index", 3),
    ("Child5Valid", 1),
    ("Child5Conflict", 1),
    ("pLIndexSlice6", 3),
    ("pLValidSlice6", 1),
    ("pHIndexSlice6", 3),
    ("pHValidSlice6", 1),
    ("oLIndexSlice6", 3),
    ("oLValidSlice6", 1),
    ("oLConflictSlice6", 1),
    ("oHIndexSlice6", 3),
    ("oHValidSlice6", 1),
    ("oHConflictSlice6", 1),
    ("pLIndexSlice7", 3),
    ("pLValidSlice7", 1),
    ("pHIndexSlice7", 3),
    ("pHValidSlice7", 1),
    ("oLIndexSlice7", 3),
    ("oLValidSlice7", 1),
    ("oLConflictSlice7", 1),
    ("oHIndexSlice7", 3),
    ("oHValidSlice7", 1),
    ("oHConflictSlice7", 1),
]
TRACE_WIDTH = 510

DESIGN = (
    PREFIX
    + "from pycircuit import priority_encode as priority, onehot_encode as onehot\nfrom priority_probe.provider import Fixed\n"
    + ENUM
)
DESIGN += "@ac.struct\nclass ProbeResult:\n" + "".join(
    f"    {name}: ac.bits[{width}]\n" for name, width in FIELDS
)
DESIGN += (
    "@ac.rule\ndef evaluate("
    + ", ".join(f"w{w}" for w in WIDTHS)
    + ", child) -> ProbeResult:\n    result = ProbeResult()\n"
)
for width in WIDTHS:
    for prefix, helper, order in (
        ("pL", "priority", "low"),
        ("pH", "ac.priority_encode", "high"),
        ("oL", "onehot", "low"),
        ("oH", "ac.onehot_encode", "high"),
    ):
        names = (
            ("index", "valid", "conflict")
            if prefix.startswith("o")
            else ("index", "valid")
        )
        DESIGN += f"    {', '.join(prefix + name + str(width) for name in names)} = {helper}(w{width}, order={order!r})\n"
        for name in names:
            DESIGN += (
                f"    result.{prefix}{name.title()}{width} = {prefix}{name}{width}\n"
            )
DESIGN += "    alias = w5\n    arithmetic = alias + 1\n    decoded, member = ac.enum_from_bits[State](w3)\n"
for name, operand in (
    ("Widen1", "w1"),
    ("Slice73", "w73[61:70]"),
    ("Arithmetic5", "arithmetic"),
    ("Enum3", "ac.enum_to_bits(decoded)"),
    ("Child5", "child.value"),
):
    DESIGN += f"    {name}index, {name}valid, {name}conflict = ac.onehot_encode({operand}, order='high')\n"
    for suffix in ("index", "valid", "conflict"):
        DESIGN += f"    result.{name}{suffix.title()} = {name}{suffix}\n"
for width in (6, 7):
    for prefix, helper, order in (
        ("pL", "priority", "low"),
        ("pH", "ac.priority_encode", "high"),
        ("oL", "onehot", "low"),
        ("oH", "ac.onehot_encode", "high"),
    ):
        names = (
            ("index", "valid", "conflict")
            if prefix.startswith("o")
            else ("index", "valid")
        )
        DESIGN += f"    {', '.join(prefix + name + 'Slice' + str(width) for name in names)} = {helper}(w8[:{width}], order={order!r})\n"
        for name in names:
            DESIGN += f"    result.{prefix}{name.title()}Slice{width} = {prefix}{name}Slice{width}\n"
DESIGN += (
    "    return result\n@ac.module\ndef Top("
    + ", ".join(f"w{w}: ac.bits[{w}]" for w in WIDTHS)
    + ") -> ProbeResult:\n    child = Fixed(w5)\n    return evaluate("
    + ", ".join(f"w{w}" for w in WIDTHS)
    + ", child)\n"
)


def scalar(
    statements,
    expression="index",
    base="ac.u5",
    extra="",
    prelude="",
    width=80,
    behavioral=False,
):
    text = PREFIX + prelude + f"@ac.struct\nclass Result:\n    out: ac.bits[{width}]\n"
    if behavioral:
        text += (
            "@ac.rule\ndef evaluate(value"
            + extra
            + ") -> Result:\n"
            + statements
            + f"    return Result(out={expression})\n"
        )
        text += f"@ac.module\ndef Top(value: {base}) -> Result:\n    return evaluate(value)\n"
    else:
        text += (
            f"@ac.module\ndef Top(value: {base}{extra}) -> Result:\n"
            + statements
            + f"    return Result(out={expression})\n"
        )
    return text


def observation(statements, expression):
    # Pure structural rules capture already unpacked module wires. Their
    # existing report form is an imported bare call, not rule-local assignment.
    return (
        PREFIX
        + "from pycircuit import report\n"
        + "@ac.module\ndef Top(value: ac.u5) -> {'out': ac.u5}:\n"
        + statements
        + f"    @ac.rule\n    def observe():\n        report('encoding', {expression})\n    observe()\n    return {{'out': 0}}\n"
    )


cases = {}
controls = {}
for helper in HELPERS:
    triple = helper == "onehot_encode"
    targets = "index, valid, conflict" if triple else "index, valid"
    call = f"ac.{helper}(value)"
    bind = f"    {targets} = {call}\n"

    def add(name, statements, expression="index", *, helper=helper, **kw):
        cases[helper + "-" + name] = scalar(statements, expression, **kw)

    for name, expression in {
        "empty": f"ac.{helper}()",
        "extra-positional": f"ac.{helper}(value, 'high')",
        "keyword-operand": f"ac.{helper}(value=value)",
        "unknown-keyword": f"ac.{helper}(value, other='low')",
        "duplicate-keyword": f"ac.{helper}(value, order='low', order='high')",
        "starred": f"ac.{helper}(*value)",
        "spread": f"ac.{helper}(value, **value)",
        "subscribed": f"ac.{helper}[5](value)",
        "dynamic-order": f"ac.{helper}(value, order=value)",
        "upper-order": f"ac.{helper}(value, order='LOW')",
        "space-order": f"ac.{helper}(value, order=' low ')",
        "invalid-order": f"ac.{helper}(value, order='middle')",
        "bool-order": f"ac.{helper}(value, order=True)",
        "int-literal": f"ac.{helper}(17)",
        "bool-literal": f"ac.{helper}(True)",
        "closed-int": f"ac.{helper}((3 * 7) + 1)",
    }.items():
        add(name, f"    {targets} = {expression}\n")
    for name, base in (
        ("integer", "Annotated[int, range(1 << 5)]"),
        ("one-bit-integer", "Annotated[int, range(1 << 1)]"),
        ("boolean", "bool"),
        ("raw-enum", "State"),
        ("aggregate", "Record"),
    ):
        prelude = (
            ENUM
            if base == "State"
            else (
                "@ac.struct\nclass Record:\n    data: ac.u5\n"
                if base == "Record"
                else ""
            )
        )
        add(name, bind, base=base, prelude=prelude)
    for width in (0, -1, 1 << 64):
        add("invalid-width-" + str(width), bind, base=f"ac.bits[{width}]")
    for name, line in {
        "arity-short": "index" if not triple else "index, valid",
        "arity-long": (
            "index, valid, extra" if not triple else "index, valid, conflict, extra"
        ),
        "duplicate-target": "index, index" if not triple else "index, valid, valid",
        "aggregate-target": (
            "result.out, valid" if not triple else "result.out, valid, conflict"
        ),
        "chained": "other = " + targets,
        "input-target": "value, valid" if not triple else "value, valid, conflict",
        "reserved-target": "Top, valid" if not triple else "Top, valid, conflict",
    }.items():
        add(name, "    result = Result()\n" + f"    {line} = {call}\n")
    for name, expr in (
        ("whole-value", call),
        ("projection", call + ".index"),
        ("valid-projection", call + ".valid"),
        ("indexing", call + "[0]"),
    ):
        add(name, "    index = " + expr + "\n")
    add("nested-return-producer", "", call)
    cases[helper + "-direct-return-producer"] = (
        PREFIX
        + f'@ac.module\ndef Top(value: ac.u5) -> {{"out": ac.u3}}:\n    return {call}\n'
    )
    cases[helper + "-observe-producer"] = observation("", call)
    add("shadow-marker", "    ac = value\n" + bind)
    cases[helper + "-immutable-rebind"] = (
        PREFIX
        + "@ac.module\ndef Top(value: ac.u5) -> {'out': ac.u3}:\n"
        + bind
        + bind
        + "    return {'out': index}\n"
    )
    add(
        "second-boundary",
        "    valid: Annotated[int, range(1 << 1)] = 0\n" + bind,
        behavioral=True,
    )
    if triple:
        add(
            "third-boundary",
            "    conflict: Annotated[int, range(1 << 1)] = 0\n" + bind,
            behavioral=True,
        )
    add("index-narrowing", "    index: ac.u1 = 0\n" + bind, behavioral=True)
    add(
        "partial-index",
        "    if value[:1]:\n    " + bind + "    else:\n        valid = False\n",
        behavioral=True,
    )
    controls[helper + "-structural"] = scalar(bind, "index", width=3)
    controls[helper + "-once"] = scalar(bind.replace("(value)", "(value + 1)"), width=3)
    controls[helper + "-widen"] = scalar(bind, base="ac.bits[65]", width=13)
    controls[helper + "-slice"] = scalar(bind, "index[:2]", base="ac.bits[65]", width=2)
    controls[helper + "-singleton-fixed"] = scalar(
        bind, "ac.popcount(index)", base="ac.u1", width=1
    )
    controls[helper + "-explicit-flag-copy"] = scalar(
        bind + "    copied: ac.u1 = valid\n",
        "ac.popcount(copied)",
        width=1,
        behavioral=True,
    )
    # Each fixed-bit helper independently refuses Boolean producers, aliases,
    # captures and originals after an explicit fresh u1 storage boundary.
    for flag in ("valid", "conflict") if triple else ("valid",):
        for sink in (
            "ac.popcount({})",
            "ac.count_leading_zeros({})",
            "ac.count_trailing_zeros({})",
            "ac.concat({})",
            "ac.priority_encode({})",
            "ac.onehot_encode({})",
        ):
            sink_name = sink.split("(")[0].split(".")[-1]
            expression = sink.format("alias")
            statement = bind + f"    alias = {flag}\n    copied: ac.u1 = {flag}\n"
            if sink_name in HELPERS:
                second = "discard, result_valid" + (
                    ", result_conflict" if sink_name == "onehot_encode" else ""
                )
                add(
                    f"{flag}-{sink_name}-launder",
                    statement + f"    {second} = {expression}\n",
                    "discard",
                    behavioral=True,
                )
            else:
                add(
                    f"{flag}-{sink_name}-launder",
                    statement,
                    expression,
                    behavioral=True,
                )
        cases[helper + "-" + flag + "-capture-launder"] = (
            PREFIX
            + "from pycircuit import report\n"
            + "@ac.module\ndef Top(value: ac.u5) -> {'out': ac.u3}:\n"
            + bind
            + f"    alias = {flag}\n    @ac.rule\n    def inspect():\n        report('flag', ac.popcount(alias))\n    inspect()\n    return {{'out': index}}\n"
        )
    controls[helper + "-captured"] = (
        PREFIX
        + f"""@ac.module
def Child(value: ac.u3, flag: bool) -> {{"out": ac.u3}}:
    return {{"out": value}}
@ac.module
def Top(value: ac.u5) -> {{"out": ac.u3}}:
{bind}    child = Child()
    @ac.rule
    def bind_child():
        child(value=index, flag=valid)
    bind_child()
    return {{"out": child.out}}
"""
    )

# Shared binder joins two- and three-result producers beside Enum conversion.
BRANCH = (
    PREFIX
    + ENUM
    + """@ac.struct
class Result:
    index: ac.u3
    valid: ac.u1
    conflict: ac.u1
    code: ac.u3
    member: ac.u1
@ac.rule
def evaluate(value, choose) -> Result:
    if choose:
        pi, pv = ac.priority_encode(value)
        index, valid, conflict = ac.onehot_encode(value)
        code, member = ac.enum_from_bits[State](value[:3])
    else:
        pi, pv = ac.priority_encode(value, order="high")
        index, valid, conflict = ac.onehot_encode(value, order="high")
        code, member = ac.enum_from_bits[State](value[:3])
    match choose:
        case 0:
            pi, pv = ac.priority_encode(value)
            index, valid, conflict = ac.onehot_encode(value)
            code, member = ac.enum_from_bits[State](value[:3])
        case _:
            pi, pv = ac.priority_encode(value, order="high")
            index, valid, conflict = ac.onehot_encode(value, order="high")
            code, member = ac.enum_from_bits[State](value[:3])
    return Result(index=index, valid=valid, conflict=conflict, code=ac.enum_to_bits(code), member=member)
@ac.module
def Top(value: ac.u5, choose: ac.u1) -> Result:
    return evaluate(value, choose)
"""
)
controls["shared-if-match-enum"] = BRANCH
cases["third-partial-match"] = (
    BRANCH[0 : BRANCH.index("    if choose:")]
    + """    match choose:
        case 0:
            index, valid, conflict = ac.onehot_encode(value)
        case _:
            index, valid = ac.priority_encode(value, order="high")
    return Result(index=index, valid=valid, conflict=conflict, code=value[:3], member=False)
@ac.module
def Top(value: ac.u5, choose: ac.u1) -> Result:
    return evaluate(value, choose)
"""
)
for helper in HELPERS:
    targets = "index, valid, conflict" if helper == "onehot_encode" else "index, valid"
    cases[helper + "-owner-target"] = (
        PREFIX
        + f"""@ac.struct
class Result:
    out: ac.u3
@ac.rule
def evaluate(owner, value) -> Result:
    {targets.replace('index', 'owner')} = ac.{helper}(value)
    return Result(out=owner)
@ac.module
def Top(value: ac.u5) -> Result:
    owner: ac.u3 = 0
    return evaluate(owner, value)
"""
    )


def split_fields(text):
    """Split MLIR dictionaries/lists only at their own nesting level."""
    stack, quoted, escaped, start, parts = [], False, False, 0, []
    for index, char in enumerate(text):
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
        elif char == '"':
            quoted = True
        elif char in "{[(":
            stack.append(char)
        elif char in "}])":
            stack.pop()
        elif char == "," and not stack:
            parts.append(text[start:index].strip())
            start = index + 1
    parts.append(text[start:].strip())
    return parts


def attribute(text, key):
    start = text.index(key + " = ") + len(key) + 3
    opening = text[start]
    assert opening in "{[", (key, text)
    closing = "}" if opening == "{" else "]"
    depth, quoted, escaped = 0, False, False
    for index in range(start, len(text)):
        char = text[index]
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
        elif char == '"':
            quoted = True
        elif char == opening:
            depth += 1
        elif char == closing:
            depth -= 1
            if depth == 0:
                return text[start + 1 : index]
    raise AssertionError((key, text))


def dictionary(text):
    return dict(part.split(" = ", 1) for part in split_fields(text))


def module_header(header, symbol):
    return next(
        line for line in header.splitlines() if f'sym_name = "{symbol}"' in line
    )


def parameter_kinds(header, symbol):
    return {
        json.loads(fields["name"]): dictionary(fields["constraint"][1:-1])[
            "source_kind"
        ]
        for item in split_fields(
            attribute(module_header(header, symbol), "ac.parameters")
        )
        for fields in [dictionary(item[1:-1])]
    }


def dependency_ports(header, symbol):
    return {
        json.loads(dictionary(output[1:-1])["path"])[0]: {
            int(dictionary(item[1:-1])["port"].split()[0])
            for item in split_fields(inputs[1:-1])
        }
        for entry in split_fields(
            attribute(module_header(header, symbol), "dependency_summary")
        )
        for fields in [dictionary(entry[1:-1])]
        for inputs, output in [(fields["inputs"], fields["output"])]
    }


def assert_negative_diagnostic(name, diagnostic):
    """Reject at the intended source contract, rather than an earlier profile gap."""
    helper = next((value for value in HELPERS if name.startswith(value + "-")), "")
    case = name[len(helper) + 1 :] if helper else name
    if case.endswith("launder") or case in (
        "int-literal",
        "bool-literal",
        "closed-int",
        "integer",
        "one-bit-integer",
        "boolean",
        "raw-enum",
        "aggregate",
        "imported-integer",
        "imported-boolean",
    ):
        assert "requires explicitly unsigned fixed bits" in diagnostic, diagnostic
    if case in (
        "whole-value",
        "projection",
        "valid-projection",
        "indexing",
        "nested-return-producer",
        "direct-return-producer",
        "observe-producer",
        "arity-short",
        "arity-long",
        "chained",
    ):
        assert "unpack first" in diagnostic, diagnostic
    if case in ("second-boundary", "third-boundary"):
        assert (
            "binding boundary requires declared Integer source kind" in diagnostic
        ), diagnostic
    if case == "index-narrowing":
        assert "implicit narrowing" in diagnostic, diagnostic
    if case in ("partial-index", "third-partial-match"):
        assert "unknown hardware value" in diagnostic, diagnostic
    if case == "owner-target":
        assert "persistent owner formal" in diagnostic, diagnostic
    if case == "immutable-rebind":
        assert "shadow a binding" in diagnostic, diagnostic
    if case == "observe-producer":
        assert (
            "produces fixed results; unpack first into ordinary local Names"
            in diagnostic
        ), diagnostic
    if case.endswith("capture-launder"):
        assert (
            "popcount operand requires explicitly unsigned fixed bits" in diagnostic
        ), diagnostic


def assert_serialized_graph_bound(body, operation_bound, logarithm):
    """Bound printed source facts by graph nodes and SSA edges, not input bits.

    Core operations have at most two operands; terminal index/population concat
    contributes O(log W) operands. Header/declaration slack is constant. Printed
    type/origin facts can repeat once per operand and result, including variadic
    concat, so each such fact receives a fixed 8 KiB representation envelope.
    """
    lines = body.splitlines()
    arities = []
    for line in lines:
        operation = re.search(r'"ac\.[A-Za-z0-9_.]+"\(([^)]*)\)', line)
        arity = (
            len(re.findall(r"%[A-Za-z0-9_]+", operation.group(1))) if operation else 0
        )
        arities.append(arity)
        assert len(line.encode()) <= 8192 * (arity + 1), (arity, len(line.encode()))
    edge_bound = 3 * operation_bound + 4 * logarithm + 256
    byte_bound = 8192 * (4 * operation_bound + 4 * logarithm + 512)
    assert len(lines) <= operation_bound + 256 and sum(arities) <= edge_bound
    assert len(body.encode()) <= byte_bound
    return {
        "body_byte_bound": byte_bound,
        "source_lines": len(lines),
        "ssa_operands": sum(arities),
        "ssa_operand_bound": edge_bound,
        "largest_line_bytes": max(len(line.encode()) for line in lines),
    }


def assert_operand_once(body):
    """Identify the source argument producer by SSA, independent of helper ops.

    This fixture has one input and operand ``value + 1``. Only that producer
    may directly consume the input; every helper component must share its SSA.
    The source-stage artifact precedes backend optimization and CSE.
    """
    argument = re.search(r"^\s*\^bb[0-9]+\((%[A-Za-z0-9_]+):", body, re.M)
    assert argument, body
    argument = argument.group(1)
    operations = []
    for line in body.splitlines():
        operation = re.match(
            r'^\s*(%[A-Za-z0-9_]+) = "(ac\.[A-Za-z0-9_.]+)"\(([^)]*)\)(.*)$', line
        )
        if operation:
            result, name, operands, attributes = operation.groups()
            operations.append(
                (
                    result,
                    name,
                    [x.strip() for x in operands.split(",") if x.strip()],
                    attributes,
                )
            )
    direct_users = [op for op in operations if argument in op[2]]
    assert len(direct_users) == 1, direct_users
    producer, name, operands, attributes = direct_users[0]
    assert name == "ac.bits.binary" and 'opcode = "add"' in attributes, direct_users
    assert operands.count(argument) == 1 and len(operands) == 2, direct_users
    constant = next(
        op for op in operations if op[0] == next(x for x in operands if x != argument)
    )
    assert (
        constant[1] == "ac.bits.constant"
        and "value = #ac.math_int<1>" in constant[3].split(" -> ", 1)[0]
    ), constant
    shared_users = [op for op in operations if producer in op[2]]
    assert shared_users and all(
        argument not in op[2] for op in shared_users
    ), shared_users
    return {
        "input_ssa": argument,
        "argument_producer_ssa": producer,
        "direct_input_uses": len(direct_users),
        "shared_result_users": len(shared_users),
    }


for tool in (args.cxx, args.verilator, args.iverilog, args.vvp):
    assert shutil.which(tool), f"required executable is unavailable: {tool}"
PROVIDER += """@ac.struct
class Selection:
    index: ac.u3
    valid: ac.u1
    conflict: ac.u1
@ac.module
def Encoded(value: ac.u5) -> Selection:
    index, valid, conflict = ac.onehot_encode(value)
    return Selection(index=index, valid=valid, conflict=conflict)
"""
build = Path(tempfile.mkdtemp(prefix="execution-", dir=scratch))
(scratch / "latest-execution.json").write_text(
    json.dumps({"artifact_directory": str(build)}, indent=2) + "\n"
)
source = build / "source"
source.mkdir(exist_ok=True)


def compile_source(name, output, imports=(), replace=False, **options):
    command = [
        "compile",
        "-c",
        source / name,
        "--source-root",
        source,
        "--package-prefix",
        "priority_probe",
        "-o",
        output,
    ]
    for unit in imports:
        command.extend(("-I", unit))
    if replace:
        command.append("--replace")
    return cli(*command, **options)


for helper in HELPERS:
    run(
        [
            sys.executable,
            "-c",
            f"from pycircuit import {helper}; "
            f"\ntry: {helper}(1)\nexcept RuntimeError: pass\nelse: raise AssertionError('{helper} executed')",
        ]
    )
(source / "provider.py").write_text(PROVIDER)
(build / "provider.py").write_text(PROVIDER)
provider = build / "provider-unit"
compile_source("provider.py", provider)
provider_header = payload(provider, "interface").read_text()
assert parameter_kinds(provider_header, "priority_probe.provider.Fixed") == {
    "value": '"fixed_bits"'
}
(source / "provider.py").unlink()  # Header-only consumer must not execute/read it.
(source / "design.py").write_text(DESIGN)
(build / "design.py").write_text(DESIGN)
unit = build / "unit"
compile_source("design.py", unit, (provider,))
body = payload(unit, "body").read_text()
header = payload(unit, "interface").read_text()
assert parameter_kinds(header, "priority_probe.design.Top") == {
    f"w{w}": '"fixed_bits"' for w in WIDTHS
}
result_line = next(
    line
    for line in header.splitlines()
    if 'ac.struct "priority_probe.design.ProbeResult" fields [' in line
)
result_fields = [
    dictionary(item[1:-1])
    for item in split_fields(
        attribute(result_line.replace(" fields [", " fields = [", 1), "fields")
    )
]
assert [
    (
        json.loads(field["name"]),
        int(re.findall(r"#ac.math_int<([0-9]+)>", field["type"])[-1]),
    )
    for field in result_fields
] == FIELDS
assert '"ac.bits.extract"' in body and '"ac.bits.binary"' in body
assert all(f'"ac.{name}"' not in body for name in ("reg", "variable", "table"))
assert (
    attribute(module_header(header, "priority_probe.design.Top"), "ac.domain_inputs")
    == ""
)
(build / "source-body.ac").write_text(body)
(build / "consumer.interface.ac").write_text(header)
(build / "provider.interface.ac").write_text(provider_header)
control_paths = []
once_receipts = {}
controls["imported-record-fixed-flags"] = (
    PREFIX
    + "from priority_probe.provider import Encoded\n"
    + """@ac.module
def Top(value: ac.u5) -> {"out": ac.u4}:
    selection = Encoded(value)
    index, valid = ac.priority_encode(selection.valid)
    return {"out": ac.concat(selection.index, ac.popcount(selection.conflict))}
"""
)
for name, text in controls.items():
    filename = name.replace("-", "_") + ".py"
    (source / filename).write_text(text)
    (build / filename).write_text(text)
    control = build / name
    compile_source(filename, control, (provider,))
    control_body = payload(control, "body").read_text()
    if name.endswith("-once"):
        once_receipts[name] = assert_operand_once(control_body)
    if name.endswith("-widen"):
        resizes = [
            line
            for line in control_body.splitlines()
            if '"ac.bits.resize"' in line
            and 'mode = "zext"' in line
            and re.findall(r"#ac.math_int<([0-9]+)>", line.rsplit("->", 1)[1])[-1]
            == "13"
        ]
        assert resizes and any(
            re.findall(r"#ac.math_int<([0-9]+)>", line.split("->")[0])[-1] == "7"
            for line in resizes
        ), resizes
    linked = build / (name + ".ac")
    cli(
        "link",
        provider,
        control,
        "--top",
        "priority_probe." + filename[:-3] + ".Top",
        "-o",
        linked,
    )
    control_paths.extend((control, linked))
output = build / "products"
output.mkdir()
final = output / "design.ac"
cli("link", provider, unit, "--top", "priority_probe.design.Top", "-o", final)
final_hash = digest(final)
for target in ("cpp", "verilog"):
    cli("emit", final, "--target", target, "-o", output / target)
    assert digest(final) == final_hash
receipt = json.loads((output / "cpp/generated.json").read_text())
cpp = [
    output / "cpp" / row["path"]
    for row in receipt["files"]
    if row["path"].endswith(".cpp")
]
assert cpp
root = Path(args.source_compiler).resolve().parent.parent
runtime = next(
    (
        p
        for p in (
            root / "runtime/libpyc6_runtime.a",
            root / "lib/libpyc6_runtime.a",
        )
        if p.is_file()
    ),
    None,
)
assert runtime, "Runtime archive missing from this build/install"
runner = output / "runner"
run(
    [
        args.cxx,
        "-std=c++20",
        "-pthread",
        "-I" + str(repo / "include"),
        "-I" + str(output / "cpp"),
        fixtures / "bits-priority-encoding.cpp",
        *cpp,
        runtime,
        "-o",
        runner,
    ]
)
traces = []
for workers in (1, 2):
    trace = run([runner, str(workers)]).stdout
    (build / f"native-workers-{workers}.stdout").write_text(trace)
    traces.append(
        [row for row in trace.splitlines() if row.startswith(("WORK ", "MASK "))]
    )
assert traces[0] == traces[1]
assert all(
    re.fullmatch(rf"(?:WORK|MASK) [01x]{{{TRACE_WIDTH}}}", row) for row in traces[0]
)
known_frames = sum(row.startswith("WORK ") for row in traces[0])
masked_frames = sum(row.startswith("MASK ") for row in traces[0])
assert (known_frames, masked_frames) == (420, 2612), (known_frames, masked_frames)
receipt = json.loads((output / "verilog/generated.json").read_text())
rtl = [
    output / "verilog" / row["path"] for row in receipt["files"] if row["role"] == "rtl"
]
rtl.sort(key=lambda p: (p.name != "design_top.sv", str(p)))
rtl_runner = output / "four-state"
run(
    [
        args.iverilog,
        "-g2012",
        "-DPRIORITY_FOUR_STATE",
        "-s",
        "tb",
        "-o",
        rtl_runner,
        *sorted((repo / "include/verilog").glob("*.v")),
        *rtl,
        fixtures / "bits-priority-encoding.sv",
    ]
)
trace = run([args.vvp, rtl_runner]).stdout
(build / "icarus.stdout").write_text(trace)
assert [
    row for row in trace.splitlines() if row.startswith(("WORK ", "MASK "))
] == traces[0]
rtl_build = output / "rtl-build"
# Verilator 5.044's default optimizer faults on this complete emitted graph.
# This configuration executes the same RTL and all known corpus frames.
run(
    [
        args.verilator,
        "-O0",
        "--output-split",
        "10000",
        "--binary",
        "--timing",
        "--top-module",
        "tb",
        "--prefix",
        "Vpriority",
        "--Mdir",
        rtl_build,
        "-j",
        "2",
        "-Wno-fatal",
        *sorted((repo / "include/verilog").glob("*.v")),
        *rtl,
        fixtures / "bits-priority-encoding.sv",
    ]
)
trace = run([rtl_build / "Vpriority"]).stdout
(build / "verilator.stdout").write_text(trace)
assert [row for row in trace.splitlines() if row.startswith("WORK ")] == [
    row for row in traces[0] if row.startswith("WORK ")
]
protected_paths = [
    provider,
    unit,
    final,
    output / "cpp",
    output / "verilog",
    *control_paths,
]
before = {str(p): managed(p) for p in protected_paths}


def protect():
    assert {str(p): managed(p) for p in protected_paths} == before


# Unpacked observations compile/link; cpp now publishes the observation surface
# while verilog still rejects instrumentation, and neither may displace a
# successful backend product owned by a different design. A producer itself is
# never an observation.
for helper in HELPERS:
    names = "index, valid, conflict" if helper == "onehot_encode" else "index, valid"
    name = "observe_" + helper
    filename = name + ".py"
    text = observation(f"    {names} = ac.{helper}(value)\n", "index")
    (source / filename).write_text(text)
    observed = build / name
    compile_source(filename, observed)
    assert '"ac.observe"' in payload(observed, "body").read_text()
    linked = build / (name + ".ac")
    cli("link", observed, "--top", "priority_probe." + name + ".Top", "-o", linked)
    protected_paths.extend((observed, linked))
    before.update({str(p): managed(p) for p in (observed, linked)})
    for target in ("cpp", "verilog"):
        absent = build / (name + "-" + target)
        if target == "cpp":
            # C++ lowers `ac.observe` into a wired descriptor table.
            cli("emit", linked, "--target", target, "-o", absent)
            observe_receipt = json.loads((absent / "generated.json").read_text())
            observe_header = next(
                row["path"]
                for row in observe_receipt["files"]
                if row["path"].endswith("pycircuit_system.hpp")
            )
            observe_system = (absent / observe_header).read_text()
            assert "pyc_observation_descriptors" in observe_system
            assert "Configure(pyc_observation_descriptors" in observe_system
            assert "Configure({}, 0)" not in observe_system
        else:
            rejected = cli("emit", linked, "--target", target, "-o", absent, code=1)
            assert any(
                word in rejected.stderr.lower()
                for word in ("instrument", "observation", "observe")
            ), rejected.stderr
            assert_absent(absent)
        cli(
            "emit",
            linked,
            "--target",
            target,
            "-o",
            output / target,
            "--replace",
            code=1,
            # The published cpp product comes from the unobserved `design.py`, so
            # its owner differs and the publisher refuses the replacement;
            # verilog still rejects the instrumentation itself.
            diagnostic=(
                "generated.json entry does not match its owner"
                if target == "cpp"
                else "ac.observe"
            ),
        )
        protect()

# Unproven imported logical carriers remain outside fixed-bit admission.
for helper in HELPERS:
    names = "index, valid, conflict" if helper == "onehot_encode" else "index, valid"
    for field in ("integer", "boolean"):
        cases[helper + "-imported-" + field] = (
            PREFIX
            + "from priority_probe.provider import Logical\n"
            + f"""@ac.module
def Top(value: Annotated[int, range(1 << 5)], flag: bool) -> {{"out": ac.u3}}:
    child = Logical()
    @ac.rule
    def bind_child():
        child(value=value, flag=flag)
    bind_child()
    {names} = ac.{helper}(child.{field})
    return {{"out": index}}
"""
        )
negative_receipts = []
for name, text in cases.items():
    (source / "design.py").write_text(text)
    (build / (name + ".py")).write_text(text)
    absent = build / ("invalid-" + name)
    result = compile_source("design.py", absent, (provider,), code=1)
    assert_absent(absent)
    assert "error:" in result.stderr, result.stderr
    assert_negative_diagnostic(name, result.stderr)
    assert any(
        word in result.stderr.lower()
        for word in (
            "operand",
            "fixed",
            "result",
            "target",
            "bind",
            "tuple",
            "unpack",
            "order",
            "width",
            "shadow",
            "immutable",
            "boolean",
            "argument",
            "keyword",
            "syntax",
            "subscription",
            "local",
            "source",
            "unsupported",
            "reserved",
            "assign",
            "stored",
            "nonlocal",
            "rebind",
            "declaration",
            "declared",
            "positional",
            "expects",
            "input",
            "call",
            "struct",
        )
    ), result.stderr
    compile_source("design.py", unit, (provider,), replace=True, code=1)
    protect()
    negative_receipts.append({"case": name, "stderr": result.stderr})

huge_receipts = []
for width in (1 << 32, (1 << 32) + 1, 1 << 63, (1 << 64) - 1):
    for helper in HELPERS:
        for order in ("low", "high"):
            natural = max(1, (width - 1).bit_length())
            name = f"huge_{helper}_{order}_{width}"
            filename = name + ".py"
            targets = (
                "index, valid, conflict"
                if helper == "onehot_encode"
                else "index, valid"
            )
            text = scalar(
                f"    {targets} = ac.{helper}(value, order={order!r})\n",
                base=f"ac.bits[{width}]",
                width=natural,
            )
            (source / filename).write_text(text)
            (build / filename).write_text(text)
            huge = build / name
            compile_source(filename, huge)
            huge_body = payload(huge, "body").read_text()
            operations = re.findall(r'"(ac\.bits\.[a-z]+)"', huge_body)
            logarithm = width.bit_length()
            bound = 16 * logarithm * logarithm + 64 * logarithm + 128
            assert len(operations) <= bound, (width, len(operations), bound)
            literals = list(map(int, re.findall(r"#ac.math_int<([0-9]+)>", huge_body)))
            assert literals and max(x.bit_length() for x in literals) <= 64
            representation = assert_serialized_graph_bound(huge_body, bound, logarithm)
            linked = build / (name + ".ac")
            cli("link", huge, "--top", "priority_probe." + name + ".Top", "-o", linked)
            protected_paths.extend((huge, linked))
            before.update({str(p): managed(p) for p in (huge, linked)})
            failures = {}
            for target in ("cpp", "verilog"):
                absent = build / (name + "-" + target)
                rejected = cli("emit", linked, "--target", target, "-o", absent, code=1)
                assert (
                    "exceeds the current Runtime bit-width capacity" in rejected.stderr
                ), rejected.stderr
                assert_absent(absent)
                cli(
                    "emit",
                    linked,
                    "--target",
                    target,
                    "-o",
                    output / target,
                    "--replace",
                    code=1,
                )
                protect()
                failures[target] = rejected.stderr
            huge_receipts.append(
                {
                    "helper": helper,
                    "order": order,
                    "width": width,
                    "natural_width": natural,
                    "operations": len(operations),
                    "operation_bound": bound,
                    "body_bytes": len(huge_body.encode()),
                    **representation,
                    "largest_literal_bits": max(x.bit_length() for x in literals),
                    "emission_diagnostics": failures,
                    "scope": "compile/capacity only, never Runtime allocation",
                }
            )

# Missing provider declarations reject compilation, and incomplete/mismatched
# explicit closures reject link without replacing prior successful products.
(source / "design.py").write_text(DESIGN)
missing = build / "missing-header"
compile_source("design.py", missing, code=1)
assert_absent(missing)
compile_source("design.py", unit, replace=True, code=1)
protect()
for destination, replace in ((build / "missing-provider.ac", False), (final, True)):
    result = cli(
        "link",
        unit,
        "--top",
        "priority_probe.design.Top",
        "-o",
        destination,
        *(("--replace",) if replace else ()),
        code=1,
    )
    assert "dependency" in result.stderr.lower(), result.stderr
    if not replace:
        assert_absent(destination)
    protect()
(source / "provider.py").write_text(PROVIDER.replace("value: ac.u5", "value: ac.u7"))
mismatch = build / "mismatched-provider"
compile_source("provider.py", mismatch)
for destination, replace in ((build / "mismatched.ac", False), (final, True)):
    result = cli(
        "link",
        mismatch,
        unit,
        "--top",
        "priority_probe.design.Top",
        "-o",
        destination,
        *(("--replace",) if replace else ()),
        code=1,
    )
    assert any(
        word in result.stderr.lower()
        for word in ("match", "interface", "header", "hash")
    ), result.stderr
    if not replace:
        assert_absent(destination)
    protect()

# Copy the managed publication control alongside each forged unit; otherwise a
# publication-owner rejection would fail to exercise interface/body authority.
forged_dir = build / "forged-units"
forged_dir.mkdir()


def copy_managed(original, name):
    destination = forged_dir / name
    shutil.copytree(original, destination)
    control = original.parent / ("." + original.name + ".pycircuit-publication")
    copied = forged_dir / ("." + name + ".pycircuit-publication")
    shutil.copytree(control, copied)
    (copied / "owner.json").write_text(
        json.dumps(
            {"destination": name, "kind": "pycircuit-publication-control"},
            separators=(",", ":"),
        )
    )
    return destination


def forge_header(path):
    lines = path.read_text().splitlines(keepends=True)
    selected = [
        i
        for i, line in enumerate(lines)
        if 'sym_name = "priority_probe.provider.Fixed"' in line
        and 'source_kind = "fixed_bits"' in line
    ]
    assert len(selected) == 1, selected
    lines[selected[0]] = lines[selected[0]].replace(
        'source_kind = "fixed_bits"',
        'source_kind = "integer", domain = #ac.source_domain<{kind = "integer", lower = #ac.math_int<0>, upper = #ac.math_int<32>}>',
        1,
    )
    path.write_text("".join(lines))


forged_provider = copy_managed(provider, "forged-provider")
forged_consumer = copy_managed(unit, "forged-consumer")
forge_header(payload(forged_provider, "interface"))
mutations = 0
for kind in ("body", "interface"):
    path = payload(forged_consumer, kind)
    if 'sym_name = "priority_probe.provider.Fixed"' in path.read_text():
        forge_header(path)
        mutations += 1
assert mutations
forged_before = {str(p): managed(p) for p in (forged_provider, forged_consumer)}
for destination, replace in ((build / "forged-authority.ac", False), (final, True)):
    result = cli(
        "link",
        forged_provider,
        forged_consumer,
        "--top",
        "priority_probe.design.Top",
        "-o",
        destination,
        *(("--replace",) if replace else ()),
        code=1,
    )
    assert any(
        word in result.stderr.lower()
        for word in ("published interface", "contract", "source", "constraint")
    ), result.stderr
    if not replace:
        assert_absent(destination)
    protect()
assert {str(p): managed(p) for p in (forged_provider, forged_consumer)} == forged_before
remaining = TRACE_WIDTH
layout = []
for name, width in FIELDS:
    remaining -= width
    layout.append({"name": name, "width": width, "low": remaining})
(build / "candidate.json").write_text(
    json.dumps(
        {
            "artifact_directory": str(build),
            "fixture_contract": {
                "trace_width": TRACE_WIDTH,
                "fields": layout,
                "first_declared_field": "most significant",
                "widths": WIDTHS,
                "additional_slice_operands": {"6": "w8[:6]", "7": "w8[:7]"},
                "top": "priority_probe.design.Top",
            },
            "fixtures": {
                str(p.relative_to(repo)): digest(p)
                for p in [
                    fixtures / ("bits-priority-encoding" + extension)
                    for extension in (".py", ".cpp", ".sv")
                ]
            },
            "tools": {
                str(Path(tool).resolve()): digest(Path(tool))
                for tool in (
                    args.source_compiler,
                    args.linker,
                    args.emitter,
                    args.optimizer,
                    args.iverilog,
                    args.vvp,
                    args.verilator,
                )
            },
            "runtime": {str(runtime): digest(runtime)},
            "same_final_for_both_targets": final_hash,
            "units": {
                str(p): {
                    kind: digest(payload(p, kind)) for kind in ("body", "interface")
                }
                for p in (provider, unit)
            },
            "known_frames": known_frames,
            "native_four_state_frames": masked_frames,
            "icarus_four_state_frames": masked_frames,
            "native_workers": [1, 2],
            "protected_compile_rejections": 2 * len(cases),
            "negative_receipts": negative_receipts,
            "explicit_closure_guards": [
                "missing-header",
                "missing-provider",
                "mismatched-provider",
                "forged-fixed-authority",
            ],
            "authority_tamper": {
                "snapshot_mutations": mutations,
                "diagnostic": result.stderr,
            },
            "compile_link_controls": sorted(controls),
            "exactly_once_receipts": once_receipts,
            "observation_scope": "unpacked compile/link; both emission targets reject/protect",
            "huge_width_guards": huge_receipts,
            "oracle": "independent scalar conditional fold, OR reduction and known popcount; original seven table vectors checked against explicit outputs",
            "historical_roots_closed": 0,
        },
        indent=2,
    )
    + "\n"
)
print(f"priority/one-hot artifacts: {build}")
print(
    f"priority/one-hot gate passed: {known_frames} known/{masked_frames} four-state frames; native workers 1/2, genuine Icarus and Verilator; {2 * len(cases)} protected compile rejections"
)
