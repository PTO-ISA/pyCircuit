"""Independent adversarial gate for dependent-width literal normalization.

Written by an independent reviewer.  It shares no oracle with the change under
review and does not read the implementer's tests.  Every case states the
behaviour the source language requires; the harness fails when the compiler
disagrees.

Categories
----------
positive   a matching computed/dependent width must still bind, on the standard
           storage leaves, on a user module and through a forwarded parameter;
width      a real width disagreement must still reject, wider and narrower, for
           a literal and for a computed declared width, on an input port and on
           an output port;
kind       Boolean and mathematical Integer must not leak into a destination
           that declares the other kind;
nominal    equal packed width must not merge struct, enum or table identity;
unclosed   a width that cannot be evaluated must never compare equal to a
           concrete width;
suffix     a rule that binds an instance *and* carries source checks must keep
           the condition/path suffix positions, counts and types, and the
           published ac.expect obligations must still name them.

Unbound parameter controls
--------------------------
A module's default W is not proof that W matches a concrete port width for all
instantiations. These cases must reject, even when the defaults happen to match.
They are ordinary negative tests, not known gaps or xfails.

The harness compiles every case in a private scratch directory so lit's
persistent %t.dir cannot collide with a previous publication.
"""

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
for name in ("repo", "source-compiler", "linker", "emitter", "scratch"):
    parser.add_argument("--" + name, required=True)
parser.add_argument("--optimizer")
parser.add_argument("--dump", help="write every case to this directory and exit")
args = parser.parse_args()
repo = Path(args.repo).resolve()
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)

HEADER = "import pycircuit as ac\n"
ANN = "from typing import Annotated\n"
BOUNDARY = (
    "mathematical boundary requires known Integer source and " "destination kinds"
)


def memory(
    addr="ac.u2",
    data="ac.u13",
    strobe="ac.u2",
    result=None,
    t="ac.u13",
    addr_width=2,
    depth=3,
):
    """A standard storage leaf whose wstrb port is
    bits(floordiv(add(type_width(T), 7), 8)): a closed dependent width."""
    result = data if result is None else result
    return (
        HEADER + "from pycircuit import sync_mem\n"
        "@ac.module\n"
        f"def Explicit(clk: bool, rst: bool, ren: bool, addr: {addr},\n"
        f"             valid: bool, data: {data}, strobe: {strobe}) -> "
        f'{{"rdata": {result}}}:\n'
        f"    memory = sync_mem(T={t}, ADDR_WIDTH={addr_width}, DEPTH={depth})\n"
        "    @ac.rule\n"
        "    def bind():\n"
        "        memory(clk=clk, rst=rst, ren=ren, raddr=addr,\n"
        "               wvalid=valid, waddr=addr, wdata=data, wstrb=strobe)\n"
        "    bind()\n"
        '    return {"rdata": memory.rdata}\n'
    )


def latch(dtype="ac.u8", ttype="ac.u8", result=None, init="0"):
    """A standard dff leaf; T is the declared port type of d/init (input) and of
    q (output).  The declared return repeats T so that a dependent output width
    is not compared against an unrelated concrete spelling."""
    result = ttype if result is None else result
    return (
        ANN + HEADER + "from pycircuit import dff\n"
        "@ac.module\n"
        f'def Holder(clk: bool, rst: bool, d: {dtype}) -> {{"q": {result}}}:\n'
        f"    state = dff(T={ttype})\n"
        "    @ac.rule\n"
        "    def bind():\n"
        f"        state(clk=clk, rst=rst, d=d, init={init})\n"
        "    bind()\n"
        '    return {"q": state.q}\n'
    )


# A leaf whose declared T is closed but still symbolic: 1 << (4 + 4) is only
# concrete after evaluation, exactly like the storage leaves' computed strobe.
COMPUTED = "Annotated[int, range(1 << (4 + 4))]"

# A leaf whose declared T forwards the enclosing module's own static parameter:
# the declared width is a parameter reference at the binding site.
DEPENDENT_INPUT = (
    ANN + HEADER + "from pycircuit import dff\n"
    "@ac.module\n"
    "def Top(clk: bool, rst: bool, x: ac.u8,\n"
    '        *, W: int = 12) -> {"y": ac.u8}:\n'
    "    state = dff(T=Annotated[int, range(1 << (W - 4))])\n"
    "    @ac.rule\n"
    "    def bind():\n"
    "        state(clk=clk, rst=rst, d=x, init=0)\n"
    "    bind()\n"
    '    return {"y": state.q}\n'
)

# The same closed dependent declared width compared against a concrete equal
# width on the *output* port.
DEPENDENT_OUTPUT = (
    ANN + HEADER + "from pycircuit import dff\n"
    "@ac.module\n"
    "def Top(clk: bool, rst: bool, x: Annotated[int, range(1 << (W - 4))],\n"
    '        *, W: int = 12) -> {"y": ac.u8}:\n'
    "    state = dff(T=Annotated[int, range(1 << (W - 4))])\n"
    "    @ac.rule\n"
    "    def bind():\n"
    "        state(clk=clk, rst=rst, d=x, init=0)\n"
    "    bind()\n"
    '    return {"y": state.q}\n'
)

# A concrete child proves that returning an instance output is admissible at
# all; the dependent variant isolates the width comparison.
CONCRETE_CHILD = (
    HEADER + "from pycircuit import dff\n"
    "@ac.module\n"
    'def Child(clk: bool, rst: bool, x: ac.u8) -> {"y": ac.u8}:\n'
    "    state = dff(T=ac.u8)\n"
    "    @ac.rule\n"
    "    def bind():\n"
    "        state(clk=clk, rst=rst, d=x, init=0)\n"
    "    bind()\n"
    '    return {"y": state.q}\n'
    "@ac.module\n"
    'def Top(clk: bool, rst: bool, v: ac.u8) -> {"y": ac.u8}:\n'
    "    child = Child()\n"
    "    @ac.rule\n"
    "    def bind():\n"
    "        child(clk=clk, rst=rst, x=v)\n"
    "    bind()\n"
    '    return {"y": child.y}\n'
)

DEPENDENT_CHILD = (
    ANN + HEADER + "from pycircuit import dff\n"
    "@ac.module\n"
    "def Child(clk: bool, rst: bool, x: Annotated[int, range(1 << (W - 4))],\n"
    "          *, W: int = 12) -> "
    '{"y": Annotated[int, range(1 << (W - 4))]}:\n'
    "    state = dff(T=Annotated[int, range(1 << (W - 4))])\n"
    "    @ac.rule\n"
    "    def bind():\n"
    "        state(clk=clk, rst=rst, d=x, init=0)\n"
    "    bind()\n"
    '    return {"y": state.q}\n'
    "@ac.module\n"
    'def Top(clk: bool, rst: bool, v: ac.u8) -> {"y": ac.u8}:\n'
    "    child = Child(W=12)\n"
    "    @ac.rule\n"
    "    def bind():\n"
    "        child(clk=clk, rst=rst, x=v)\n"
    "    bind()\n"
    '    return {"y": child.y}\n'
)

# A user module that forwards its own static parameter into a leaf's T: the
# declared width stays a parameter reference at the binding site.
FORWARDED = (
    ANN + HEADER + "from pycircuit import dffe\n"
    "@ac.module\n"
    "def WordLatch(clk: bool, rst: bool, en: bool,\n"
    "              d: Annotated[int, range(1 << W)], *, W: int = 9\n"
    '              ) -> {"q": Annotated[int, range(1 << W)]}:\n'
    "    state = dffe(T=Annotated[int, range(1 << W)])\n"
    "    @ac.rule\n"
    "    def bind():\n"
    "        state(clk=clk, rst=rst, en=en, d=d, init=0)\n"
    "    bind()\n"
    '    return {"q": state.q}\n'
    "@ac.module\n"
    "def Parent(clk: bool, rst: bool, en: bool,\n"
    "           data: Annotated[int, range(1 << W)], *, W: int = 9\n"
    '           ) -> {"q": Annotated[int, range(1 << W)]}:\n'
    "    word = WordLatch(W=W)\n"
    "    @ac.rule\n"
    "    def bind_word():\n"
    "        word(clk=clk, rst=rst, en=en, d=data)\n"
    "    bind_word()\n"
    '    return {"q": word.q}\n'
)

NOMINAL = (
    HEADER + "@ac.struct\n"
    "class Left:\n    value: ac.u8\n"
    "@ac.struct\n"
    "class Right:\n    value: ac.u8\n"
    "@ac.module\n"
    'def Take(x: Left) -> {"y": ac.u8}:\n'
    '    return {"y": x.value}\n'
    "@ac.module\n"
    'def Top(p: Left, q: Right) -> {"y": ac.u8}:\n'
    "    first = Take()\n"
    "    second = Take()\n"
    "    @ac.rule\n"
    "    def bind_first():\n"
    "        first(x=p)\n"
    "    @ac.rule\n"
    "    def bind_second():\n"
    "        second(x=__PEER__)\n"
    "    bind_first()\n"
    "    bind_second()\n"
    '    return {"y": first.y ^ second.y}\n'
)

ENUM_NOMINAL = (
    "from enum import Enum\n"
    + HEADER
    + "@ac.encoding(width=8)\nclass First(Enum):\n    A = 0\n"
    "@ac.encoding(width=8)\nclass Second(Enum):\n    A = 0\n"
    "@ac.module\n"
    'def Take(x: First) -> {"y": ac.u8}:\n'
    '    return {"y": ac.enum_to_bits(x)}\n'
    "@ac.module\n"
    'def Top(p: First, q: Second) -> {"y": ac.u8}:\n'
    "    first = Take()\n"
    "    second = Take()\n"
    "    @ac.rule\n"
    "    def bind_first():\n"
    "        first(x=p)\n"
    "    @ac.rule\n"
    "    def bind_second():\n"
    "        second(x=__PEER__)\n"
    "    bind_first()\n"
    "    bind_second()\n"
    '    return {"y": first.y ^ second.y}\n'
)

TABLE_NOMINAL = (
    HEADER + "@ac.struct\n"
    "class Left:\n    value: ac.u8\n"
    "@ac.struct\n"
    "class Right:\n    value: ac.u8\n"
    "@ac.module\n"
    'def Take(x: ac.table[4, Left]) -> {"y": ac.u8}:\n'
    '    return {"y": x[0].value}\n'
    "@ac.module\n"
    "def Top(p: ac.table[4, Left], q: ac.table[4, Right]) -> "
    '{"y": ac.u8}:\n'
    "    first = Take()\n"
    "    second = Take()\n"
    "    @ac.rule\n"
    "    def bind_first():\n"
    "        first(x=p)\n"
    "    @ac.rule\n"
    "    def bind_second():\n"
    "        second(x=__PEER__)\n"
    "    bind_first()\n"
    "    bind_second()\n"
    '    return {"y": first.y ^ second.y}\n'
)

TABLE_EXTENT = (
    HEADER + "@ac.module\n"
    'def Take(x: ac.table[2 + 2, ac.u8]) -> {"y": ac.u8}:\n'
    '    return {"y": x[0]}\n'
    "@ac.module\n"
    'def Top(p: ac.table[4, ac.u8]) -> {"a": ac.u8}:\n'
    "    first = Take()\n"
    "    @ac.rule\n"
    "    def bind():\n"
    "        first(x=p)\n"
    "    bind()\n"
    '    return {"a": first.y}\n'
)

UNCLOSED = (
    HEADER + "@ac.module\n"
    'def Take(x: ac.bits[W]) -> {"y": ac.bits[W]}:\n'
    '    return {"y": x}\n'
    "@ac.module\n"
    'def Top(p: ac.u8) -> {"a": ac.u8}:\n'
    "    first = Take()\n"
    "    @ac.rule\n"
    "    def bind():\n"
    "        first(x=p)\n"
    "    bind()\n"
    '    return {"a": first.y}\n'
)

CHECKED_BINDING = (
    HEADER + "from pycircuit import sync_mem\n"
    "@ac.module\n"
    "def Checked(clk: bool, rst: bool, addr: ac.u2, data: ac.u13,\n"
    '            strobe: ac.u2, ok: bool) -> {"rdata": ac.u13}:\n'
    "    memory = sync_mem(T=ac.u13, ADDR_WIDTH=2, DEPTH=3)\n"
    "    @ac.rule\n"
    "    def bind():\n"
    '        assert ok, "__GATE__"\n'
    "        memory(clk=clk, rst=rst, ren=ok, raddr=addr,\n"
    "               wvalid=ok, waddr=addr, wdata=data, wstrb=strobe)\n"
    "    bind()\n"
    '    return {"rdata": memory.rdata}\n'
)

# name -> (source, expected acceptance, structural expectation)
# structural expectation is None or (rule name, bound inputs, source checks).
CASES = {
    # -- positive controls -------------------------------------------------
    "memory-dependent-width-baseline": (memory(), True, ("bind", 8, 0)),
    "memory-address-width-three": (memory(addr="ac.u3", addr_width=3), True, None),
    "memory-payload-width-fourteen": (
        memory(data="ac.u14", result="ac.u14", t="ac.u14"),
        True,
        None,
    ),
    "latch-computed-declared-width": (
        latch(dtype="ac.u8", ttype=COMPUTED),
        True,
        ("bind", 4, 0),
    ),
    "child-concrete-output": (CONCRETE_CHILD, True, ("bind", 4, 0)),
    "forwarded-dependent-parameter": (FORWARDED, True, ("bind_word", 4, 0)),
    "nominal-struct-identity": (NOMINAL.replace("__PEER__", "p"), True, None),
    "nominal-enum-identity": (ENUM_NOMINAL.replace("__PEER__", "p"), True, None),
    "nominal-table-element-identity": (
        TABLE_NOMINAL.replace("__PEER__", "p"),
        True,
        None,
    ),
    # -- width authority ---------------------------------------------------
    "width-mismatch-strobe-wider": (memory(strobe="ac.u3"), False, None),
    "width-mismatch-strobe-narrower": (memory(strobe="ac.u1"), False, None),
    "width-mismatch-payload-wider": (memory(data="ac.u14"), False, None),
    "width-mismatch-address-wider": (memory(addr="ac.u3"), False, None),
    "width-mismatch-address-width-three": (
        memory(addr="ac.u2", addr_width=3),
        False,
        None,
    ),
    "width-mismatch-computed-narrower": (
        latch(dtype="ac.u7", ttype=COMPUTED, result="ac.u7"),
        False,
        None,
    ),
    "width-mismatch-computed-wider": (
        latch(dtype="ac.u9", ttype=COMPUTED, result="ac.u9"),
        False,
        None,
    ),
    "width-mismatch-t-is-not-widened": (
        latch(dtype="ac.u14", ttype="ac.u13", result="ac.u14"),
        False,
        None,
    ),
    # -- output direction --------------------------------------------------
    "child-explicit-parameter-output": (DEPENDENT_CHILD, True, None),
    "child-explicit-parameter-nine-bit-output": (
        DEPENDENT_CHILD.replace("Child(W=12)", "Child(W=13)").replace("ac.u8", "ac.u9"),
        True,
        None,
    ),
    "width-mismatch-dependent-output": (
        DEPENDENT_CHILD.replace("Child(W=12)", "Child(W=13)"),
        False,
        None,
    ),
    # -- kind authority ----------------------------------------------------
    "kind-boolean-into-two-bit-leaf": (memory(strobe="bool"), False, None),
    "kind-boolean-into-one-bit-integer-leaf": (
        latch(
            dtype="bool",
            ttype="Annotated[int, range(1 << 1)]",
            result="ac.u1",
            init="False",
        ),
        False,
        None,
    ),
    "kind-boolean-into-one-bit-fixed-leaf": (
        latch(dtype="bool", ttype="ac.u1", result="ac.u1", init="False"),
        True,
        None,
    ),
    "kind-integer-into-fixed-width-analogue": (
        latch(dtype="Annotated[int, range(1 << 8)]", ttype="ac.u8"),
        True,
        None,
    ),
    "kind-integer-into-two-bit-computed-leaf": (
        memory(strobe="Annotated[int, range(1 << 2)]"),
        True,
        None,
    ),
    # -- nominal authority -------------------------------------------------
    "nominal-struct-mismatch": (NOMINAL.replace("__PEER__", "q"), False, None),
    "nominal-enum-mismatch": (ENUM_NOMINAL.replace("__PEER__", "q"), False, None),
    "nominal-table-element-mismatch": (
        TABLE_NOMINAL.replace("__PEER__", "q"),
        False,
        None,
    ),
    # -- unclosed widths ---------------------------------------------------
    "unclosed-width-is-not-concrete": (UNCLOSED, False, None),
    "table-computed-extent": (TABLE_EXTENT, True, None),
    "table-computed-extent-mismatch": (
        TABLE_EXTENT.replace("2 + 2", "2 + 3"),
        False,
        None,
    ),
    # -- binding suffix ----------------------------------------------------
    "binding-with-source-check": (
        CHECKED_BINDING.replace("__GATE__", "strobe gate"),
        True,
        ("bind", 8, 1),
    ),
    "binding-with-source-check-mismatch": (
        CHECKED_BINDING.replace("strobe: ac.u2", "strobe: ac.u3").replace(
            "__GATE__", "strobe gate"
        ),
        False,
        None,
    ),
    "binding-with-integer-check-condition": (
        CHECKED_BINDING.replace("assert ok", "assert data").replace(
            "__GATE__", "strobe gate"
        ),
        False,
        None,
    ),
}

# name -> (source, required acceptance, diagnostic the current compiler emits)
PARAMETER_MISMATCHES = {
    "dependent-input-width-not-resolved": (DEPENDENT_INPUT, False, BOUNDARY),
    "dependent-output-width-not-resolved": (DEPENDENT_OUTPUT, False, BOUNDARY),
}

if args.dump:
    target = Path(args.dump)
    target.mkdir(parents=True, exist_ok=True)
    for name, (text, _, _) in list(CASES.items()) + list(PARAMETER_MISMATCHES.items()):
        (target / (name.replace("-", "_") + ".py")).write_text(text)
    sys.stdout.write(
        f"{len(CASES) + len(PARAMETER_MISMATCHES)} cases written to {target}\n"
    )
    sys.exit(0)


def run(command, accepted):
    environment = dict(
        os.environ,
        PYTHONPATH=str(repo / "python"),
        PYTHONDONTWRITEBYTECODE="1",
        PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
        PYCIRCUIT_LINKER=args.linker,
        PYCIRCUIT_EMITTER=args.emitter,
    )
    result = subprocess.run(
        list(map(str, command)),
        env=environment,
        cwd=repo,
        text=True,
        capture_output=True,
        timeout=300,
    )
    assert "Assertion failed" not in result.stderr, result.stderr
    assert "Traceback" not in result.stderr, result.stderr
    assert result.returncode == (0 if accepted else 1), {
        "command": list(map(str, command)),
        "exit_status": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
    }
    return result


# The IR helpers below follow the published technique of the existing
# source-checks fixture: read the printed operation line instead of trusting a
# claim about the suffix.
def operands(line):
    match = re.search(r'"ac\.[\w.]+"\(([^)]*)\)', line)
    return re.findall(r"%[\w]+(?:#\d+)?", match[1]) if match else []


def results(line):
    left = line.split('"ac.', 1)[0].strip().rstrip("= ")
    grouped = re.fullmatch(r"(%\w+):(\d+)", left)
    if grouped:
        return [grouped[1] + "#" + str(i) for i in range(int(grouped[2]))]
    return re.findall(r"%\w+", left)


def result_type_list(text, rule_index):
    """Split the rule operation's printed `-> (...)` result type list."""
    lines = text.splitlines()
    closing = next(
        line for line in lines[rule_index:] if line.lstrip().startswith("})")
    )
    marker = closing.rfind("-> (")
    assert marker >= 0, closing[:200]
    start = closing.index("(", marker)
    depth, end = 0, None
    for index in range(start, len(closing)):
        if closing[index] == "(":
            depth += 1
        elif closing[index] == ")":
            depth -= 1
            if depth == 0:
                end = index
                break
    assert end is not None, closing[:200]
    parts, depth, current = [], 0, ""
    for character in closing[start + 1 : end]:
        if character in "([{":
            depth += 1
        elif character in ")]}":
            depth -= 1
        if character == "," and depth == 0:
            parts.append(current.strip())
            current = ""
        else:
            current += character
    if current.strip():
        parts.append(current.strip())
    return parts


def verify_rule_suffix(text, rule_name, inputs, checks):
    """The rule must publish exactly inputs + 2*checks results, its yield must
    cover every result in order, every published ac.expect obligation must
    consume the condition/path pair at the suffix position the rule declares,
    and the check suffix must keep its own shared one-bit carrier type."""
    lines = text.splitlines()
    rule_line = next(
        line
        for line in lines
        if '"ac.rule"(' in line and f'name = "{rule_name}"' in line
    )
    published = results(rule_line)
    assert len(published) == inputs + 2 * checks, (rule_name, published)
    if checks:
        assert "ac.required_checks" in text, "no published source-check obligation"
    else:
        assert "ac.required_checks" not in text, "unexpected check obligation"
    start = lines.index(rule_line)
    yield_line = next(line for line in lines[start:] if '"ac.yield"(' in line)
    yielded = operands(yield_line)
    # The yield is printed with the rule's block arguments, so only its arity is
    # comparable to the published result list; the positions are what matter.
    assert len(yielded) == len(published), (yielded, published)
    types = result_type_list(text, start)
    assert len(types) == len(published), (types, published)
    if checks:
        # Both suffix carriers share one one-bit type, distinct from every
        # binding-prefix type: the republish must not reach into the suffix.
        assert types[-1] == types[-2], types
        assert "#ac.math_int<1>" in types[-1], types[-1]
        assert types[-1] not in types[:inputs], types
    expectations = [line for line in lines[start:] if '"ac.expect"(' in line]
    assert len(expectations) == checks, expectations
    for index, expectation in enumerate(expectations):
        pair = published[
            len(published)
            - 2 * checks
            + 2 * index : len(published)
            - 2 * checks
            + 2 * index
            + 2
        ]
        assert operands(expectation) == pair, (operands(expectation), pair)
    return published


def compile_case(name, text, accepted, build):
    design = build / "source" / (name.replace("-", "_") + ".py")
    design.write_text(text)
    unit = build / ("unit-" + name)
    command = [
        sys.executable,
        "-m",
        "pycircuit.cli",
        "compile",
        "-c",
        design,
        "--source-root",
        build / "source",
        "--package-prefix",
        "adversarial",
        "-o",
        unit,
    ]
    return unit, run(command, accepted)


build = Path(tempfile.mkdtemp(prefix="literal-normalization-", dir=scratch))
(build / "source").mkdir()
log = {"cases": [], "parameter_mismatches": []}
mismatches = []
for name, (text, accepted, structure) in CASES.items():
    unit, result = compile_case(name, text, accepted, build)
    observed = result.returncode == 0
    log["cases"].append(
        {
            "name": name,
            "expected": accepted,
            "observed": observed,
            "stderr": result.stderr,
            "source": text,
        }
    )
    if observed != accepted:
        mismatches.append((name, accepted, result.stderr))
    if observed and structure:
        receipt = json.loads((unit / "unit.json").read_text())
        body = (unit / receipt["files"]["body"]).read_text()
        log["cases"][-1]["rule_results"] = len(verify_rule_suffix(body, *structure))
    sys.stdout.write(
        f"{'ACCEPT' if observed else 'REJECT'} "
        f"{'as-required' if observed == accepted else 'UNEXPECTED'} "
        f"{name}\n"
    )
for name, (text, _required, diagnostic) in PARAMETER_MISMATCHES.items():
    unit, result = compile_case(name, text, False, build)
    observed = result.returncode == 0
    if observed or diagnostic not in result.stderr:
        mismatches.append((name, False, result.stderr))
    assert not unit.exists(), name + " published a rejected unit"
    sys.stdout.write(f"REJECT parameter-default-not-a-proof {name}\n")
    log["parameter_mismatches"].append(
        {
            "name": name,
            "expected": False,
            "observed": observed,
            "stderr": result.stderr,
            "source": text,
        }
    )
(build / "adversarial-results.json").write_text(json.dumps(log, indent=2) + "\n")
assert not mismatches, mismatches
sys.stdout.write(
    f"literal normalization adversarial gate: {len(CASES)} required "
    f"cases hold, {len(PARAMETER_MISMATCHES)} strict parameter negatives; "
    f"scratch {build}\n"
)
