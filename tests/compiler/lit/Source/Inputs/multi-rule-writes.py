"""Independent source-intent, old-Q merge and four-state enable acceptance."""

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
for option in ("repo", "source-compiler", "linker", "emitter", "opt", "cxx",
               "verilator", "scratch"):
    parser.add_argument("--" + option, required=True)
for option in ("iverilog", "vvp"):
    parser.add_argument("--" + option, default=shutil.which(option))
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
scratch_root = Path(args.scratch).resolve()
scratch_root.mkdir(parents=True, exist_ok=True)
# Preserve each lit rerun without colliding with prior published units.
scratch = Path(tempfile.mkdtemp(prefix="multi-rule-writes-", dir=scratch_root))
source = scratch / "source"
source.mkdir(exist_ok=True)
sys.path.insert(0, str(repo / "python/pycircuit/src"))
# Load the requested checkout after argument parsing, never an installed copy.
from pycircuit._source_capture import _capture_source_file  # noqa: E402
from pycircuit._source_transport import _emit_source_transport  # noqa: E402

env = dict(os.environ, PYTHONPATH=str(repo / "python/pycircuit/src"),
           PYTHONDONTWRITEBYTECODE="1", PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
           PYCIRCUIT_LINKER=args.linker, PYCIRCUIT_EMITTER=args.emitter)
commands = []


def run(command, success=True):
    command = list(map(str, command))
    result = subprocess.run(command, env=env, cwd=repo, text=True,
                            capture_output=True, timeout=240)
    commands.append({"command": command, "exit_status": result.returncode,
                     "stdout": result.stdout, "stderr": result.stderr})
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == (0 if success else 1), commands[-1]
    assert "Assertion failed" not in result.stderr and "Traceback" not in result.stderr, commands[-1]
    return result


def cli(*options, success=True):
    return run([sys.executable, "-m", "pycircuit.cli", *options], success)


def compile_source(path, output, success=True, replace=False):
    options = ["compile", "-c", path, "--source-root", source,
               "--package-prefix", "multi_rule", "-o", output]
    if replace:
        options.append("--replace")
    return cli(*options, success=success)


def analyze(path, success=True):
    capture = scratch / (path.stem + ".capture.mlir")
    capture.write_text(_emit_source_transport(_capture_source_file(path, source_root=source)))
    return run([args.opt, capture, "--mlir-print-op-on-diagnostic=false", "--ac-analyze-rule-writes", "-o",
                scratch / (path.stem + ".analyzed.mlir")], success)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def snapshot(path):
    return {str(item.relative_to(path)): digest(item) for item in path.rglob("*") if item.is_file()}


text = (fixtures / "multi-rule-writes/design.py").read_text()
design = source / "design.py"
design.write_text(text)
analyze(design)
unit = scratch / "unit"
compile_source(design, unit)
unit_before = snapshot(unit)

# The first textual call is in Multi; Reverse retains a valid independent copy.
replacements = {
    "same-field": ("    WriteB(s, other, en_b)", "    WriteA(s, other, en_b)"),
    "identity-write": ("    WriteB(s, other, en_b)", "    Identity(s, other, en_b)"),
    "parent-child": ("    WriteB(s, other, en_b)", "    Parent(s, other, en_b)"),
    "whole-field": ("    WriteB(s, other, en_b)", "    Whole(s, other, en_b)"),
    "writable-alias": ("    WriteA(s, data, en_a)", "    Aliases(s, s, data)"),
    "same-table-owner": ("second = TableWrite(right, other)", "second = TableWrite(left, other)"),
    "input-shadows-rule": ("def Multi(en_a:", "def Multi(WriteA: ac.u8, en_a:"),
}
negative_sources = {}
for name, (before, after) in replacements.items():
    assert before in text
    negative_sources[name] = text.replace(before, after, 1)
# The former indexed-table-fields rejection now executes as LiteralFields below.
for name, before, after in (
    ("dynamic-same-field", "entries[j].b = old ^ wide", "entries[j].a = other[0:5]"),
    ("table-parent-child", "entries[2].sub.a = other[0:5]", "entries[0].sub.a = other[0:5]"),
    ("table-identity", "entries[2] = entries[0] ^ other[0:5]", "entries[0] = entries[0]"),
):
    assert before in text
    negative_sources[name] = text.replace(before, after, 1)
for name, negative_source in negative_sources.items():
    path = source / (name.replace("-", "_") + ".py")
    path.write_text(negative_source)
    expected = r"shadow|unshadowed|lexical" if name == "input-shadows-rule" else r"overlap|conflict|writable.*alias"
    immediate = name in {"writable-alias", "input-shadows-rule"}
    diagnostic = analyze(path, success=not immediate).stderr
    assert re.search(expected if immediate else r"collected .*overlapping writer pairs; awaiting lowered owner-enable proof",
                     diagnostic, re.I), diagnostic
    absent = scratch / ("invalid-" + name)
    rejected = compile_source(path, absent, success=False)
    assert re.search(expected, rejected.stderr, re.I), rejected.stderr
    assert not absent.exists(), "rejected source published an output unit"
    if name in {"identity-write", "dynamic-same-field", "table-parent-child"}:
        compile_source(path, unit, success=False, replace=True)
        assert snapshot(unit) == unit_before, "rejection changed accepted unit"

# Range evidence is independent of static-selector disjointness. The successful
# non-power-of-two roots below exercise actual read/write, aliases and captures.
range_rejections = {
    "literal-outside": "index = 3",
    "negative-index": "index = 0 - 1",
    "remainder-too-wide": "index = data % 4",
    "runtime-remainder": "index = data % rhs",
    "wrapped-range": "index = data % 3\n    index = index + 255",
}
for name, index in range_rejections.items():
    path = source / (name.replace("-", "_") + ".py")
    path.write_text("""import pycircuit as ac
@ac.struct
class Output:
    value: ac.u8
@ac.rule
def put(entries, index, data):
    entries[index] = data
@ac.module
def Top(data: ac.u8, rhs: ac.u8) -> Output:
    entries = ac.table[3, ac.u8](init=0)
    """ + index + """
    alias = index
    put(entries, alias, data)
    return Output(value=entries[alias])
""")
    analyze(path)
    absent = scratch / ("invalid-range-" + name)
    rejected = compile_source(path, absent, success=False)
    assert "table index" in rejected.stderr, rejected.stderr
    assert not absent.exists()

# Return contracts belong to normal lowering, not overlap analysis. Keep these
# two diagnostics separate from the standalone write-intent rejection matrix.
return_rejections = {
    "void-used-as-value": ("    WriteA(s, data, en_a)", "    missing = WriteA(s, data, en_a)",
                           r"void|no.*result|value.*rule|rule.*value"),
    "none-returned-value": ("def WriteB(s, other, enable) -> None:\n    if enable:\n        s.pair.b = s.pair.a ^ other\n    return",
                            "def WriteB(s, other, enable) -> None:\n    if enable:\n        s.pair.b = s.pair.a ^ other\n    return Ack(value=other)",
                            r"None|void|return.*value|value.*return"),
}
for name, (before, after, diagnostic) in return_rejections.items():
    assert before in text
    path = source / (name.replace("-", "_") + ".py")
    path.write_text(text.replace(before, after, 1))
    absent = scratch / ("invalid-" + name)
    rejected = compile_source(path, absent, success=False)
    assert re.search(diagnostic, rejected.stderr, re.I), rejected.stderr
    assert not absent.exists()

# Shared marker resolution must retain the unsupported-system boundary in
# either decorator order and through an actual imported marker alias.
system_rejections = {
    "module-then-system": "@ac.module\n@ac.system",
    "system-then-module": "@ac.system\n@ac.module",
    "aliased-system": "@sys\n@ac.module",
}
for name, decorators in system_rejections.items():
    path = source / (name.replace("-", "_") + ".py")
    path.write_text("import pycircuit as ac\nfrom pycircuit import system as sys\n" +
                    decorators + '\ndef Top(x: ac.u8) -> {"out": ac.u8}:\n'
                    '    return {"out": x}\n')
    absent = scratch / ("invalid-" + name)
    rejected = compile_source(path, absent, success=False)
    assert "@system is not implemented" in rejected.stderr, rejected.stderr
    assert not absent.exists()

# A stateless mapping return does not itself select behavioral lowering. The
# ordinary expression registration must select that route even for a void rule.
mapping = source / "mapping_void.py"
mapping.write_text('''import pycircuit as ac
@ac.rule
def noop() -> None:
    return
@ac.module
def Top(x: ac.u8) -> {"out": ac.u8}:
    noop()
    return {"out": x}
''')
analyze(mapping)
compile_source(mapping, scratch / "mapping-void-unit")

# Proof facts are necessary known-truth conditions, not Boolean rewriting.
# Each accepted case writes one real scalar owner through distinct captures.
def proof_source(first, second, setup="", third=None, tail=""):
    calls = f"    put(value, data, {first})\n    put(value, other, {second})\n"
    if third is not None:
        calls += f"    put(value, data ^ other, {third})\n"
    return ("import pycircuit as ac\n@ac.struct\nclass Out:\n    value: ac.u8\n"
            "@ac.rule\ndef put(value, data, enable):\n    if enable:\n        value = data\n"
            "@ac.module\ndef Top(p: ac.u1, q: ac.u1, r: ac.u1, data: ac.u8, other: ac.u8) -> Out:\n"
            "    value: ac.u8 = 7\n" + setup + calls + "    return Out(value=value)\n" + tail)

proof_accepts = {
    "truth-and": ("p & q", "~p", ""),
    "truth-not-and-zero": ("~shared", "shared", "    shared = p & q\n"),
    "truth-or-zero": ("~(p | q)", "p", ""),
    "truth-or-common": ("(p & q) | (p & r)", "~p", ""),
    "shared-compound": ("shared", "~shared", "    shared = (p & q) | r\n"),
    "select-x0": ("q if p else zero", "~p", "    zero: ac.u1 = 0\n"),
    "select-x1-zero": ("~(q if p else one)", "~p", "    one: ac.u1 = 1\n"),
    "select-0y": ("zero if p else q", "p", "    zero: ac.u1 = 0\n"),
    "select-1y-zero": ("~(one if p else q)", "p", "    one: ac.u1 = 1\n"),
    "select-x0-zero": ("~(q if p else zero)", "p & q", "    zero: ac.u1 = 0\n"),
    "select-x1": ("q if p else one", "p & ~q", "    one: ac.u1 = 1\n"),
    "select-0y-zero": ("~(zero if p else q)", "~p & q", "    zero: ac.u1 = 0\n"),
    "select-1y": ("one if p else q", "~p & ~q", "    one: ac.u1 = 1\n"),
    "select-identical": ("q if p else q", "~q", ""),
    "select-constant-01": ("False if p else True", "p", ""),
    "select-constant-10": ("True if p else False", "~p", ""),
    "impossible-and": ("p & ~p", "q", ""),
    "impossible-or": ("~(p | ~p)", "q", ""),
    "eq-different": ("data == 17", "data == 33", ""),
    "eq-ne": ("data == 17", "data != 17", ""),
    "ne-zero": ("~(data != 17)", "data == 33", ""),
    "selector-reversed": ("17 == data", "33 == data", ""),
}
# Alternative truth branches can have no common necessary facts. Canonical
# shared SSA still proves the expression against its exact complement.
for name in ("select-x0-zero", "select-x1", "select-0y-zero", "select-1y"):
    first, _, setup = proof_accepts[name]
    expression = first[2:-1] if first.startswith("~(") else first
    proof_accepts[name] = ("~shared" if first.startswith("~(") else "shared",
                           "shared" if first.startswith("~(") else "~shared",
                           setup + f"    shared = {expression}\n")
# Typed module declarations allocate owners. Use actual closed literals in
# select arms so these cases test constant authority rather than reset data.
for name, (first, second, setup) in list(proof_accepts.items()):
    if "zero" in setup or "one" in setup:
        proof_accepts[name] = (first.replace("zero", "0").replace("one", "1"),
                               second, setup[setup.find("    shared"):].replace("zero", "0").replace("one", "1")
                               if "    shared" in setup else "")
for name, (first, second, setup) in proof_accepts.items():
    path = source / (name.replace("-", "_") + ".py")
    path.write_text(proof_source(first, second, setup))
    pending = analyze(path).stderr
    assert "awaiting lowered owner-enable proof" in pending
    compile_source(path, scratch / ("proof-" + name))

# A rule's user output precedes its proposal and source-check suffix. Proof
# endpoint mapping must remain exact when all three result kinds coexist.
checked = source / "checked_grants.py"
checked.write_text(proof_source("p", "~p").replace(
    "def put(value, data, enable):", "def put(value, data, enable) -> Out:").replace(
    "        value = data\n@ac.module", "        value = data\n    assert True, 'grant check'\n"
    "    return Out(value=data)\n@ac.module"))
analyze(checked)
compile_source(checked, scratch / "checked-grants-unit")

# The checked-result layout admits and links, while both public emitters continue
# to reject checked hardware atomically. Prepared native execution is owned by
# the existing seven-case HardwareSourceChecksExecution GTest, not this route.
address_checked = source / "address_grants.py"
address_checked.write_text((fixtures / "source_check_execution/address_grants.py").read_text())
address_checked_unit = scratch / "address-checked-unit"
compile_source(address_checked, address_checked_unit)
address_checked_final = scratch / "address-checked.ac"
cli("link", address_checked_unit, "--top", "multi_rule.address_grants.Top", "-o", address_checked_final)
mapping_final = scratch / "mapping-void.ac"
cli("link", scratch / "mapping-void-unit", "--top", "multi_rule.mapping_void.Top", "-o", mapping_final)
for target in ("cpp", "verilog"):
    protected = scratch / ("checked-protected-" + target)
    cli("emit", mapping_final, "--target", target, "-o", protected)
    before = snapshot(protected)
    for replace in (False, True):
        output = protected if replace else scratch / ("checked-fresh-" + target)
        options = ["emit", address_checked_final, "--target", target, "-o", output]
        if replace:
            options.append("--replace")
        rejected = cli(*options, success=False)
        assert "hardware instrumentation emission is not implemented" in rejected.stderr
        if replace:
            assert snapshot(protected) == before, "failed checked emission changed protected output"
        else:
            assert not output.exists(), "unsupported checked emission published output"


proof_rejects = {
    "independent-captures": proof_source("p", "q"),
    "not-and-counterexample": proof_source("~(p & q)", "p"),
    "or-independent": proof_source("p | q", "~p"),
    "missed-third-pair": proof_source("p", "~p", third="q"),
    "selector-different-input": proof_source("data == 17", "other != 17"),
    "unused-later-module": proof_source("p", "~p", tail=
        "@ac.module\ndef Unused(p: ac.u1, q: ac.u1, data: ac.u8) -> Out:\n"
        "    value: ac.u8 = 0\n    put(value, data, p)\n    put(value, data, q)\n"
        "    return Out(value=value)\n"),
}
for name, candidate in proof_rejects.items():
    path = source / (name.replace("-", "_") + ".py")
    path.write_text(candidate)
    assert "awaiting lowered owner-enable proof" in analyze(path).stderr
    absent = scratch / ("invalid-proof-" + name)
    rejected = compile_source(path, absent, success=False)
    assert re.search(r"overlap|owner-enable|mutual", rejected.stderr), rejected.stderr
    assert not absent.exists()
    compile_source(path, unit, success=False, replace=True)
    assert snapshot(unit) == unit_before

# P0 target binding rejects invalid fields/types/ranges before publication.
field_rejections = {
    "field-missing": ("entries[index].b = other[0:1]", "entries[index].missing = other[0:1]", r"field|member"),
    "field-too-wide": ("entries[index].b = other[0:1]", "entries[index].b = data", r"width|narrow|bits|range"),
    "field-enum-identity": ("entries[index].b = other[0:1]", "entries[index].mark = data[0:2]", r"enum|nominal|type"),
    "field-range-outside": ("def FieldRangePut(entries, data, other, enable, seed) -> FieldViews:\n    i = data % 3", "def FieldRangePut(entries, data, other, enable, seed) -> FieldViews:\n    i = data % 4", r"table index"),
}
for name, (before, after, diagnostic) in field_rejections.items():
    assert before in text
    path = source / (name.replace("-", "_") + ".py")
    path.write_text(text.replace(before, after, 1))
    absent = scratch / ("invalid-" + name)
    rejected = compile_source(path, absent, success=False)
    assert re.search(diagnostic, rejected.stderr, re.I), rejected.stderr
    assert not absent.exists(), "invalid field source published an output"
    rejected = compile_source(path, unit, success=False, replace=True)
    assert snapshot(unit) == unit_before, "invalid field replacement changed accepted unit"

# One conditional field does not authorize an unconditional sibling's owner.
conditional = source / "conditional_sibling.py"
conditional.write_text(text.replace("    WriteB(s, other, en_b)",
    "    WriteBoth(s, data, other, ~en_a)", 1))
assert "awaiting lowered owner-enable proof" in analyze(conditional).stderr
compile_source(conditional, scratch / "invalid-conditional-sibling", success=False)
assert not (scratch / "invalid-conditional-sibling").exists()
compile_source(conditional, unit, success=False, replace=True)
assert snapshot(unit) == unit_before

# Resource admission uses the same public compiler. Success and exhaustion
# cases share source construction; failure must preserve fresh/replaced output.
def balanced_and(terms):
    if len(terms) == 1:
        return terms[0]
    middle = len(terms) // 2
    return "(" + balanced_and(terms[:middle]) + " & " + balanced_and(terms[middle:]) + ")"

budget_cases = []
for depth in (120, 130):
    setup = "    g0 = p & q\n" + "".join(
        f"    g{i} = g{i-1} | g{i-1}\n" for i in range(1, depth))
    budget_cases.append((f"dag-depth-{depth}",
        proof_source(f"g{depth-1}", f"~g{depth-1}", setup),
        None if depth == 120 else "recursion"))
for leaves in (128, 129):
    candidate = proof_source("grant", "~grant", "    grant = " +
        balanced_and([f"p{i}" for i in range(leaves)]) + "\n")
    candidate = candidate.replace("p: ac.u1, q: ac.u1, r: ac.u1",
        ", ".join(f"p{i}: ac.u1" for i in range(leaves)))
    budget_cases.append((f"facts-{leaves}", candidate, None if leaves == 128 else "fact"))
for owners in (31, 32):
    expression = balanced_and(["(p ^ q)"] * 32)
    setup = "".join(f"    v{i}: ac.u8 = 0\n" for i in range(owners))
    setup += "".join(f"    g{i} = {expression}\n    put(v{i}, data, g{i})\n"
                     f"    put(v{i}, other, ~g{i})\n" for i in range(owners))
    candidate = proof_source("p", "~p")
    begin = candidate.index("    value: ac.u8 = 7")
    candidate = candidate[:begin] + setup + "    return Out(value=v0)\n"
    budget_cases.append((f"queries-{owners}", candidate, None if owners == 31 else "query"))
for modules in (1, 2):
    candidate = proof_source("p == 0", "p == 1").replace(
        "p: ac.u1, q: ac.u1, r: ac.u1", "p: ac.bits[1000000]")
    begin = candidate.index("    value: ac.u8 = 7")
    header = candidate[:begin]
    setup = "".join(f"    v{i}: ac.u8 = 0\n    put(v{i}, data, p == 0)\n"
                     f"    put(v{i}, other, p == 1)\n"
                     for i in range(32 if modules == 1 else 20))
    candidate = header + setup + "    return Out(value=v0)\n"
    if modules == 2:
        module_header = header[header.index("@ac.module"):].replace("def Top(", "def Unused(")
        candidate += module_header + setup + "    return Out(value=v0)\n"
    budget_cases.append((f"padded-work-modules-{modules}", candidate,
                         None if modules == 1 else "work"))
# Planning exhaustion occurs before any lowered body exists.
candidate = proof_source("p", "~p")
begin = candidate.index("    value: ac.u8 = 7")
candidate = candidate[:begin] + "    value: ac.u8 = 7\n" + \
    "    put(value, data, p)\n" * 129 + "    return Out(value=value)\n"
budget_cases.append(("pending-pairs", candidate, "pending pair"))
for name, candidate, exhaustion in budget_cases:
    path = source / (name.replace("-", "_") + ".py")
    path.write_text(candidate)
    analyzed = analyze(path, success=exhaustion != "pending pair")
    if exhaustion == "pending pair":
        assert "pending pair budget exhausted" in analyzed.stderr
    output = scratch / ("budget-" + name)
    result = compile_source(path, output, success=exhaustion is None)
    if exhaustion:
        assert f"{exhaustion} budget exhausted" in result.stderr, result.stderr
        assert not output.exists()
        rejected = compile_source(path, unit, success=False, replace=True)
        assert f"{exhaustion} budget exhausted" in rejected.stderr, rejected.stderr
        assert snapshot(unit) == unit_before

# Admission boundaries are source programs, independently of the emitted
# runtime oracle. Address separation only discharges exact actual indices and
# complete paths.
def address_program(first="i", second="j", grant="en_b & (~ga | (i != j))",
               body_a=None, body_b=None, tail="", depth=4, inputs="data: ac.u8, other: ac.u8"):
    body_a = body_a or "if enable:\n        entries[i].a = value"
    body_b = body_b or "if enable:\n        entries[i].a = value"
    return ("import pycircuit as ac\n@ac.struct\nclass Cell:\n    a: ac.u8\n    b: ac.u8\n"
            "@ac.struct\nclass Out:\n    value: ac.u8\n"
            "@ac.rule\ndef A(entries, i, j, value, enable, flag):\n    " + body_a + "\n"
            "@ac.rule\ndef B(entries, i, j, value, enable, flag):\n    " + body_b + "\n"
            "@ac.module\ndef Top(en_a: ac.u1, en_b: ac.u1, " + inputs + ") -> Out:\n"
            f"    entries = ac.table[{depth}, Cell](init=0)\n"
            "    i = data[0:2]\n    j = other[0:2]\n    ga = en_a\n"
            "    gb = " + grant + "\n"
            f"    A(entries, {first}, j, data[0:8], ga, en_a)\n"
            f"    B(entries, {second}, i, other[0:8], gb, en_a)\n"
            "    return Out(value=entries[0].a)\n" + tail)


address_rejects = {
    "independent": address_program(grant="en_b"),
    "same-index": address_program(second="i"),
    "late-conflict": address_program(body_a="if enable:\n        entries[i].a = value\n        entries[j].a = value"),
    "assignment-rebound": address_program(body_a="i = j\n    if enable:\n        entries[i].a = value"),
    "identity-late-conflict": address_program(body_a="if enable:\n        entries[i].a = value\n        entries[j].a = entries[j].a"),
    "whole-element-independent": address_program(grant="en_b", body_a="if enable:\n        entries[i] = Cell(a=value, b=value)"),
    "whole-table-no-index": address_program(body_a="if enable:\n        entries = entries"),
    "conditional-sibling-owner": address_program(body_a="if enable:\n        entries[i].a = value\n    entries[i].b = value"),
    "select-independent-false-arm": address_program(grant="en_b if i != j else en_b & ~other[5:6]"),
    "not-equal-is-not-priority": address_program(grant="en_b & (~ga | (i == j))"),
    "arithmetic-unproved": address_program(second="i + 1", grant="en_b"),
    "range-only": address_program(grant="en_b", depth=256).replace("i = data[0:2]", "i = data % 2").replace("j = other[0:2]", "j = (other % 2) + 2"),
    "dynamic-widths": address_program(grant="en_b & (~ga | (wide.value != j))").replace("@ac.struct\nclass Out:", "@ac.struct\nclass Wide:\n    value: ac.u8\n@ac.struct\nclass Out:").replace("j = other[0:2]", "j = other % 4").replace("    ga = en_a", "    wide = Wide(value=i)\n    ga = en_a"),
    # Same grant SSA must not retain a proof from the first index pair when a
    # later path has a different collision context.
    "cache-context": address_program(body_a="if enable:\n        entries[i].a = value\n        entries[j].b = value", body_b="if enable:\n        entries[i].a = value\n        entries[i].b = value"),
}
address_rejects["later-unused-module"] = address_program() + address_program(grant="en_b").replace("import pycircuit as ac\n", "").replace("class Cell:", "class BadCell:").replace("class Out:", "class BadOut:").replace(" -> Out:", " -> BadOut:").replace("Out(value=", "BadOut(value=").replace("Cell](", "BadCell](").replace("def Top(", "def Unused(").replace("def A(", "def BadA(").replace("def B(", "def BadB(").replace("    A(", "    BadA(").replace("    B(", "    BadB(")
address_accepts = {
    "different-widths-owner-exclusion": address_program(grant="~ga").replace("j = other[0:2]", "j = other % 4"),
    "literal-dynamic": address_program(first="0", grant="en_b & (~ga | (j != 0))"),
}
# Reuse the declared recursion cap with a shared DAG under collision contexts.
for count in (120, 130):
    candidate = address_program(grant="en_b & grant")
    setup = "    g0 = ~ga | (i != j)\n" + "".join(f"    g{k} = g{k - 1} | g{k - 1}\n" for k in range(1, count)) + f"    grant = g{count - 1}\n"
    candidate = candidate.replace("    gb =", setup + "    gb =")
    (address_accepts if count == 120 else address_rejects)[f"shared-dag-{count}"] = candidate
# Large private mask result types must be charged before their construction.
address_rejects["domain-storage-budget"] = address_program(depth=1 << 19, inputs="data: ac.bits[19], other: ac.bits[19]",
    body_a="if enable:\n        entries[i].a = value\n        entries[i].b = value",
    body_b="if enable:\n        entries[i].a = value\n        entries[i].b = value").replace("i = data[0:2]", "i = data").replace("j = other[0:2]", "j = other")

for name, candidate in address_accepts.items():
    path = source / ("address_" + name.replace("-", "_") + ".py")
    path.write_text(candidate)
    compile_source(path, scratch / ("c1-positive-" + name))
for name, candidate in address_rejects.items():
    path = source / ("address_" + name.replace("-", "_") + ".py")
    path.write_text(candidate)
    absent = scratch / ("c1-negative-" + name)
    rejected = compile_source(path, absent, success=False)
    assert re.search(r"overlap|separat|owner|index|width|budget|bits", rejected.stderr, re.I), rejected.stderr
    if "budget" in name or name == "shared-dag-130":
        assert "budget exhausted" in rejected.stderr, rejected.stderr
    assert not absent.exists(), "rejected address-grant source published an output"
    compile_source(path, unit, success=False, replace=True)
    assert snapshot(unit) == unit_before, "rejected address-grant replacement changed accepted unit"


# Independent per-leaf source-assignment oracle for P0. A table is a list of
# cells; only declared target leaves are conditionally updated. No implicit
# element read/rebuild is part of indexed Attribute assignment.
field_modes = {"FieldSingle": 0, "FieldMulti": 0, "FieldMultiReverse": 0,
               "FieldNestedLeaf": 1, "FieldNestedSubtree": 2,
               "FieldExplicitRhs": 3, "FieldWholeConstant": 4,
               "FieldWholeRead": 5, "FieldBranch": 6,
               "FieldRange": 7, "FieldLocal": 8}
field_counts = {}


def field_rows(top, output):
    mode = field_modes[top]
    depth = 3 if mode == 7 else 2
    width = 7 if mode == 8 else 6
    def bit(value):
        return value, 1, 0, True

    def zero():
        return [bit(0) for _ in range(width)]
    state = [zero() for _ in range(depth)]
    last = False
    rows = []
    work = masks = 0

    def port(value, known=255, z=0, size=8):
        return [((value >> b) & 1, (known >> b) & 1, (z >> b) & 1, True)
                for b in range(size - 1, -1, -1)]

    def select(control, new, old):
        if control == 1:
            return new
        if control == 0:
            return old
        if new[:3] == old[:3] and (new[1] or new[2]):
            return new
        return (0, 0, 0, False)

    def index(bits, divisor=None):
        if any(not b[1] or b[2] for b in bits):
            return None
        value = 0
        for b in bits:
            value = value * 2 + b[0]
        return value % divisor if divisor else value

    def get(table, selector):
        return table[selector][:] if selector is not None else [(0, 0, 0, False)] * width

    def update(table, selector, path, value):
        for ordinal, cell in enumerate(table):
            control = None if selector is None else int(ordinal == selector)
            for leaf, replacement in zip(path, value, strict=True):
                cell[leaf] = select(control, replacement, cell[leaf])

    def packed(cells):
        bits = []
        for cell in cells:
            bits.extend([bit(0)] * (8 - len(cell)) + cell)
        planes = [0, 0, 0, 0]
        for b in bits:
            for n in range(3):
                planes[n] = planes[n] * 2 + b[n]
            planes[3] = planes[3] * 2 + int(b[1] or b[2] or b[3])
        return planes

    def row(clock=0, reset=0, ea=1, eb=0, data=0, other=0,
            dk=255, dz=0, ok=255, oz=0, ak=1, az=0,
            action=0, four=False):
        nonlocal state, last, work, masks
        d, o = port(data, dk, dz), port(other, ok, oz)
        ia = None if not ak or az else ea
        i = index(d, 3) if mode == 7 else index(d[-1:])
        j = index(o, 3) if mode == 7 else index(o[-2:-1])
        candidate = [cell[:] for cell in state]
        saved = state[0][:]
        middle = saved[:]
        a_width = 2 if mode == 8 else 1
        keep_path = [a_width + 1, a_width + 2]
        mark_path = [a_width + 3, a_width + 4]

        def seed_table(table):
            for ordinal, cell in enumerate(table):
                cell[:a_width] = ([bit(0), bit(ordinal % 2)] if mode == 8 else [bit(ordinal % 2)])
                cell[keep_path[0]:keep_path[-1] + 1] = o[6 - ordinal * 2:8 - ordinal * 2]
                mark = (2, 1, 3)[ordinal]
                cell[mark_path[0]:mark_path[-1] + 1] = [bit(mark >> 1), bit(mark & 1)]

        if mode == 8:
            if eb:
                seed_table(candidate)
            # Module-local copy reads epoch Q independently of the seed proposal.
            local = [cell[:] for cell in state]
            update(local, j, [0, 1], [bit(0), d[-1]])
            middle = saved[:]
            after, read = local[0][:], get(local, j)
        else:
            if eb and (ia == 1 or mode == 6):
                seed_table(candidate)
                middle = candidate[0][:]
            elif mode == 6:
                updated = [cell[:] for cell in candidate]
                update(updated, i, [0], [bit(1)])
                for ordinal in range(depth):
                    candidate[ordinal][0] = select(ia, updated[ordinal][0], candidate[ordinal][0])
                middle = candidate[0][:]
                update(candidate, j, [1], o[-1:])
            elif ia == 1:
                update(candidate, i, [0], [bit(0)])
                update(candidate, i, [0], [bit(1)])
                middle = candidate[0][:]
                if mode == 2:
                    update(candidate, j, [0, 1], [o[-1], d[-2]])
                elif mode == 3:
                    update(candidate, j, [1], get(candidate, j)[:1])
                elif mode == 4:
                    replacement = [bit(0), o[-1], bit(0), bit(0), bit(0), bit(0)]
                    update(candidate, j, list(range(width)), replacement)
                elif mode == 5:
                    update(candidate, i, list(range(width)), get(candidate, j))
                else:
                    update(candidate, j, [1], o[-1:])
            after, read = candidate[0][:], get(candidate, j)
        observed = state + ([zero()] if depth == 2 else []) + [saved, middle, after, read]
        expected = packed(observed)
        rows.append([clock, reset, ea, ak, az, eb, 1, 0,
                     data, dk, dz, other, ok, oz, action, int(four), *expected])
        if action == 3:
            state = [zero() for _ in range(depth)]
            last = False
        elif action not in (1, 2, 4):
            if clock and not last:
                if reset:
                    state = [zero() for _ in range(depth)]
                else:
                    state = candidate
                    if top.startswith("FieldMulti") and ia == 0:
                        update(state, i, [0], [bit(0)])
            last = bool(clock)
        if action not in (2, 3, 4):
            masks += int(four)
            work += int(not four)

    def initialize(four=False, sibling=None):
        row(action=3, four=four)
        if sibling == 'x':
            row(clock=1, eb=1, other=255, ok=0, four=True)
        elif sibling == 'z':
            row(clock=1, eb=1, other=255, ok=0, oz=255, four=True)
        else:
            row(clock=1, eb=1, other=0x36, four=four)
        row(clock=0, data=0, other=2, four=four)

    # Known source history: sequential local views, both registration orders,
    # false branch, held/falling clocks, physical reset and prepared discard.
    initialize()
    for clock, ea, data, other in ((1, 1, 0, 3), (1, 1, 1, 0),
                                  (0, 0, 0, 2), (1, 0, 0, 2),
                                  (0, 1, 0, 3)):
        row(clock=clock, ea=ea, data=data, other=other)
    row(clock=1, data=0, other=2, action=1)
    row(clock=1, data=0, other=2)
    row(clock=0, data=0, other=2)
    row(clock=1, reset=1, data=0, other=2, action=1)
    row(action=4)
    row(clock=1, data=1, other=3)
    row(clock=0, data=0, other=2)
    row(clock=1, reset=1, data=0, other=2)
    row(clock=0, data=0, other=2)

    # X/Z selector counterexample with independently known RHS0/1. Unknown
    # Table3 modulo retains its full eight-bit index representation.
    for z in (False, True):
        for rhs in (0, 1):
            initialize(True)
            options = {"other": 255 if mode == 7 else (2 | rhs),
                       "ok": 0 if mode == 7 else 253,
                       "oz": 255 if mode == 7 and z else 2 if z else 0,
                       "data": 2 if mode == 2 else 0, "four": True}
            row(clock=1, **options)
            row(clock=0, **options)
        # Untouched keep siblings preserve complete native value/known/Z planes.
        initialize(True, 'z' if z else 'x')
        row(clock=1, data=0, other=2, four=True)
        row(clock=0, data=0, other=2, four=True)
        if mode == 6:
            initialize(True)
            row(clock=1, ea=1, ak=0, az=int(z), data=0, other=2, four=True)
            row(clock=0, ea=1, ak=0, az=int(z), data=0, other=2, four=True)
        elif mode != 8:
            initialize(True)
            row(clock=1, ea=1, ak=0, az=int(z), data=0, other=3, action=2, four=True)
            row(clock=1, data=0, other=3, four=True)
            row(clock=0, data=0, other=3, four=True)
        else:
            # A known index transports a widened X/Z leaf with known-zero high bit.
            initialize(True)
            row(clock=1, data=1, dk=254, dz=int(z), other=0, four=True)
            row(clock=0, data=1, dk=254, dz=int(z), other=0, four=True)
    path = output / "field-rows.txt"
    path.write_text(''.join(' '.join(format(value, 'x') for value in r) + '\n' for r in rows))
    field_counts[top] = (work, masks)
    return path


# Address-grant behavioral oracle: ordinary per-leaf assignments under known grants.
# Legal partial-address cases use disjoint possible sets. Rejected controls
# have no state/clock commit; no automatic collision arbitration is assumed.
address_modes = {"AddressDynamic": 9, "AddressDynamicReverse": 9, "AddressReverseOperands": 10,
                 "AddressEqPolarity": 11, "AddressSelect": 12, "AddressSelectReverse": 12,
                 "AddressSelectEq": 12, "AddressSelectEqReverse": 12,
                 "AddressAssignment": 14, "AddressBranch": 15, "AddressMatch": 16,
                 "AddressIdentity": 17, "AddressSubtree": 18, "AddressWhole": 19,
                 "AddressRange": 20, "AddressMixed": 21, "AddressWholeField": 22}


def address_rows(top, output):
    mode = address_modes[top]
    depth = 3 if mode == 20 else 4

    def bit(v):
        return v, 1, 0, True

    def zeros(width):
        return [bit(0) for _ in range(width)]

    def port(v, k=255, z=0, width=8):
        return [((v >> b) & 1, (k >> b) & 1, (z >> b) & 1, True)
                for b in range(width - 1, -1, -1)]

    def select(c, yes, no):
        if c == 0:
            return no
        if c == 1:
            return yes
        if yes[:3] == no[:3] and (yes[1] or yes[2]):
            return yes
        return 0, 0, 0, False

    def comparison(a, b):
        if any(x[1] and y[1] and x[0] != y[0] for x, y in zip(a, b, strict=True)):
            return 1
        return 0 if all(x[1] and y[1] for x, y in zip(a, b, strict=True)) else None

    def logical_not(a):
        return None if a is None else 1 - a

    def logical_or(a, b):
        return 1 if a == 1 or b == 1 else 0 if a == 0 and b == 0 else None

    def logical_and(a, b):
        return 0 if a == 0 or b == 0 else 1 if a == 1 and b == 1 else None

    def index(raw):
        if mode != 20:
            return raw[-2:]
        if not all(x[1] for x in raw):
            return [(0, 0, 0, False)] * 8
        number = 0
        for x in raw:
            number = number * 2 + x[0]
        return port(number % 3)

    def mask(i, ordinal):
        return logical_not(comparison(i, port(ordinal, width=len(i), k=(1 << len(i)) - 1)))

    def packed(slots):
        planes = [0, 0, 0, 0]
        for slot in slots:
            for b in zeros(8 - len(slot)) + slot:
                for n in range(3):
                    planes[n] = planes[n] * 2 + b[n]
                planes[3] = planes[3] * 2 + int(b[1] or b[2] or b[3])
        return planes

    state = [zeros(6) for _ in range(depth)]
    side = [zeros(8), zeros(8)]
    last = False
    rows = []
    work = masks = 0
    failure_count = check_failures = 0

    def row(clock=0, reset=0, ea=1, eb=1, data=4, other=5, dk=255, dz=0,
            ok=255, oz=0, ak=1, az=0, action=0, four=False):
        nonlocal state, side, last, work, masks, failure_count, check_failures
        d, o = port(data, dk, dz), port(other, ok, oz)
        ia = port(0, width=2, k=3) if mode == 21 else index(d)
        ib = index(o)
        ga = ea if ak else None
        gb = logical_and(eb, logical_or(logical_not(ga), comparison(ia, ib)))
        if action == 2:
            assert ga is None or gb is None, "failure stimulus has no unknown original enable"
            failure_count += 1
        if action == 5:
            assert mode == 13 and ga == 1 and data & 128
            check_failures += 1
        if action not in (2, 3, 4, 5):
            assert ga in (0, 1) and gb in (0, 1)
            if ga and gb:
                assert not any(mask(ia, k) != 0 and mask(ib, k) != 0 for k in range(depth))
        candidate = [cell[:] for cell in state]
        sides = [value[:] for value in side]
        for writer, (raw, address, grant) in enumerate(((d, ia, ga), (o, ib, gb))):
            if grant != 1:
                continue
            for ordinal in range(depth):
                c = mask(address, ordinal)
                if mode == 17:
                    # Explicit unknown-index RHS read keeps conservative all-X.
                    value = state[ordinal][0] if all(x[1] for x in address) and c == 1 else (0, 0, 0, False)
                else:
                    value = raw[-3]
                replacement = [value, raw[-4]]
                if mode == 19 or mode == 22 and writer == 0:
                    replacement += raw[-6:-4] + [bit(0), bit(1)]
                for leaf, rhs in enumerate(replacement):
                    candidate[ordinal][leaf] = select(c, rhs, candidate[ordinal][leaf])
            sides[writer] = raw[:]
        if mode == 14:
            user = [ib, ia]
        elif mode in (15, 16):
            user = [ia, ib]
        else:
            user = [d, o]
        checksum = [bit(a[0] ^ b[0]) if a[1] and b[1] else (0, 0, 0, False)
                    for a, b in zip(side[0], side[1], strict=True)]
        expected = packed(state + ([zeros(6)] if depth == 3 else []) + user + [checksum])
        rows.append([clock, reset, ea, ak, az, eb, 1, 0, data, dk, dz,
                     other, ok, oz, action, int(four), *expected])
        if action == 3:
            state = [zeros(6) for _ in range(depth)]
            side = [zeros(8), zeros(8)]
            last = False
        elif action not in (1, 2, 4, 5):
            if clock and not last:
                if reset:
                    state = [zeros(6) for _ in range(depth)]
                    side = [zeros(8), zeros(8)]
                else:
                    state, side = candidate, sides
            last = bool(clock)
        if action not in (2, 3, 4, 5):
            work += int(not four)
            masks += int(four)

    def initialize(four=False):
        row(action=3, four=four)
        row(clock=1, four=four)
        row(clock=0, four=four)

    initialize()
    for clock, ea, d, o in ((1, 1, 14, 15), (1, 1, 68, 69), (0, 1, 68, 69),
                            (1, 1, 68, 69), (0, 1, 4, 0), (1, 1, 4, 0),
                            (0, 0, 4, 0), (1, 0, 4, 0), (0, 1, 8, 13)):
        row(clock=clock, ea=ea, data=d, other=o)
    row(clock=1, data=8, other=13, action=1)
    row(clock=1, data=8, other=13)
    row(clock=0)
    row(clock=1, reset=1, action=1)
    row(action=4)
    row(clock=1)
    row(clock=0)
    row(clock=1, reset=1)
    row(clock=0)
    if mode == 20:
        row(clock=1, data=4, other=13)
        row(clock=0, ea=0, data=4, other=13)
        row(clock=1, ea=0, data=4, other=13)
        row(clock=0)
    if mode == 13:
        row(clock=1, data=132, action=5)
        row(clock=1)
        row(clock=0)
    for z in (False, True):
        initialize(True)
        if mode == 20:
            # Unknown high physical input bits make full-width modulo unknown;
            # inactive A still permits B, without dropping unknown index bits.
            row(clock=1, ea=0, other=132, ok=127, oz=128 if z else 0, four=True)
            row(clock=0, ea=0, other=132, ok=127, oz=128 if z else 0, four=True)
        else:
            row(clock=1, data=5, dk=254, dz=int(z), other=7, ok=254, oz=int(z), four=True)
            row(clock=0, data=5, dk=254, dz=int(z), other=7, ok=254, oz=int(z), four=True)
            # Unknown branch/match selector cannot change the identical local index.
            row(clock=1, data=68, dk=191, dz=64 if z else 0, four=True)
            row(clock=0, data=68, dk=191, dz=64 if z else 0, four=True)
        if mode != 20:
            # Both nonconstant select arms are known1 when A is inactive,
            # or known0 when B is inactive, despite overlapping X/Z indices.
            initialize(True)
            row(clock=1, ea=0, data=1, dk=254, dz=int(z), other=4, ok=253, oz=2 if z else 0, four=True)
            row(clock=0, ea=0, data=1, dk=254, dz=int(z), other=4, ok=253, oz=2 if z else 0, four=True)
            row(clock=1, eb=0, data=1, dk=254, dz=int(z), other=4, ok=253, oz=2 if z else 0, four=True)
            row(clock=0, eb=0, data=1, dk=254, dz=int(z), other=4, ok=253, oz=2 if z else 0, four=True)
        initialize(True)
        # Overlapping possible addresses, even though some individual masks
        # cannot both equal known1, yield an unknown original B enable.
        if mode == 20:
            row(clock=1, other=132, ok=127, oz=128 if z else 0, action=2, four=True)
        else:
            row(clock=1, data=1, dk=254, dz=int(z), other=4, ok=253, oz=2 if z else 0, action=2, four=True)
        row(clock=1, data=4, other=1, four=True)
        row(clock=0, data=4, other=1, four=True)
        row(clock=1, ea=1, ak=0, az=int(z), action=2, four=True)
        row(clock=1, data=14, other=15, four=True)
        row(clock=0, data=14, other=15, four=True)
    path = output / "field-rows.txt"
    path.write_text(''.join(' '.join(format(value, 'x') for value in r) + '\n' for r in rows))
    field_counts[top] = work, masks
    assert failure_count == 4 and check_failures == int(mode == 13)
    return path


toolroot = Path(args.source_compiler).resolve().parent.parent
runtime = next((path for path in (toolroot / "simulator/gfsim/libpyc6_runtime.a",
                                 toolroot / "lib/libpyc6_runtime.a") if path.is_file()), None)
assert runtime, "candidate Runtime archive missing"
traces = {}
summary = []
cases = [(top, ["-D" + define], masks)
         for top, define, masks in (("Multi", "MULTI", 2), ("Reverse", "MULTI", 2),
                                   ("Solo", "SOLO", 4), ("Tables", "TABLES", 0))]
cases += [(top, [f"-DTABLE_MODE={mode}", f"-DPACKED_WIDTH={width}"], masks)
          for top, mode, width, masks in (("Indexed", 0, 42, 2), ("IndexedReverse", 0, 42, 2),
                                          ("ScalarElements", 1, 15, 2),
                                          ("ScalarClosedExpression", 1, 15, 2), ("ScalarClosedExpressionReverse", 1, 15, 2),
                                          ("ScalarClosedAlias", 1, 15, 2), ("ScalarClosedAliasReverse", 1, 15, 2), ("NestedElements", 2, 42, 2),
                                          ("TableSolo", 3, 42, 4), ("LiteralFields", 4, 32, 2),
                                          ("RangeFive", 5, 40, 2), ("RangeOne", 6, 8, 2),
                                          ("EnumElements", 7, 15, 2))]
grant_names = ("ScalarGrant", "PrefixGrant", "TableGrant", "ComplementGrant", "PoisonGrant", "NeverGrant")
cases += [(name + suffix, [f"-DGRANT_MODE={mode}"], 2)
          for mode, name in enumerate(grant_names) for suffix in ("", "Reverse")]
complement_names = ("PrefixComplement", "TableComplement")
cases += [(name + suffix, [f"-DGRANT_MODE={mode}", "-DGRANT_COMPLEMENT"], 2)
          for mode, name in enumerate(complement_names, 1) for suffix in ("", "Reverse")]
cases += [(top, [f"-DFIELD_MODE={mode}", "-DPACKED_WIDTH=56"], 0)
          for top, mode in {**field_modes, **address_modes}.items()]
for top, defines, masks in cases:
    frames = 20 if any(flag.startswith("-DGRANT_MODE=") for flag in defines) else 17 if any(flag.startswith("-DTABLE_MODE=") for flag in defines) else 14
    output = scratch / top
    output.mkdir(exist_ok=True)
    rows = (address_rows(top, output) if top in address_modes else
            field_rows(top, output) if top in field_modes else None)
    if rows:
        frames, masks = field_counts[top]
    final = output / "design_top.ac"
    cli("link", unit, "--top", "multi_rule.design." + top, "-o", final)
    common = final.read_text()
    # These assert the accepted existing common-IR route, not generated recipes.
    if not any(flag.startswith("-DGRANT_MODE=") for flag in defines):
        assert '"ac.value.merge"' in common, "missing verified common field merge"
    assert re.search(r'"ac.bits.binary"\((%[^, )]+),\s*\1\).*opcode = "xor"', common), \
        "missing common-IR four-state enable poison"
    for target in ("cpp", "verilog"):
        cli("emit", final, "--target", target, "-o", output / target)
    receipt = json.loads((output / "cpp/generated.json").read_text())
    cpp = [output / "cpp" / item["path"] for item in receipt["files"] if item["path"].endswith(".cpp")]
    runner = output / "runner"
    run([args.cxx, "-std=c++20", "-pthread", *defines,
         "-I" + str(repo / "include"), "-I" + str(output / "cpp"),
         fixtures / "multi-rule-writes.cpp", *cpp, runtime, "-o", runner])
    native = []
    for workers in (1, 2):
        trace = run([runner, workers, *([rows] if rows else [])]).stdout
        assert trace.splitlines()[-1] == "PASS", trace
        (output / f"native-{workers}.stdout").write_text(trace)
        native.append([line for line in trace.splitlines() if line.startswith(("WORK ", "MASK "))])
    assert native[0] == native[1] and len(native[0]) == frames + masks
    traces[top] = native[0]
    receipt = json.loads((output / "verilog/generated.json").read_text())
    rtl = [output / "verilog" / item["path"] for item in receipt["files"] if item["role"] == "rtl"]
    rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
    rtl_build = output / "rtl-build"
    library = sorted((repo / "include/verilog").glob("*.v"))
    run([args.verilator, "--binary", "--timing", "--top-module", "tb", "--prefix", "Vmulti",
         "--Mdir", rtl_build, "-j", "2", "-Wno-fatal", *defines,
         *library, *rtl, fixtures / "multi-rule-writes.sv"])
    trace = run([rtl_build / "Vmulti", *([f"+rows={rows}"] if rows else [])]).stdout
    (output / "rtl.stdout").write_text(trace)
    assert [line for line in trace.splitlines() if line.startswith("WORK ")] == \
        [line for line in native[0] if line.startswith("WORK ")]
    if args.iverilog and args.vvp:
        four_runner = output / "rtl-four-state"
        run([args.iverilog, "-g2012", "-DFOUR_STATE", *defines, "-s", "tb", "-o", four_runner,
             *library, *rtl, fixtures / "multi-rule-writes.sv"])
        trace = run([args.vvp, four_runner, *([f"+rows={rows}"] if rows else [])]).stdout
        (output / "rtl-four-state.stdout").write_text(trace)
        assert [line for line in trace.splitlines() if line.startswith(("WORK ", "MASK "))] == native[0]
    summary.append({"top": top, "workers": [1, 2], "work_frames": frames,
                    "mask_frames": masks, "icarus": bool(args.iverilog and args.vvp)})
for root in ("ScalarClosedExpression", "ScalarClosedAlias"):
    assert traces[root] == traces[root + "Reverse"] == traces["ScalarElements"], "closed index changed scalar hardware"
assert traces["AddressSelect"] == traces["AddressSelectReverse"] == traces["AddressSelectEq"] == traces["AddressSelectEqReverse"], "select polarity/order changed address-grant behavior"
assert traces["AddressDynamic"] == traces["AddressDynamicReverse"], "address-grant registration order changed state"
assert traces["FieldMulti"] == traces["FieldMultiReverse"], "field assignment registration order changed behavior"
assert traces["Multi"] == traces["Reverse"], "registration order or owner copy changed hardware"
assert traces["Indexed"] == traces["IndexedReverse"], "table registration order changed old-Q behavior"
for name in (*grant_names, *complement_names):
    assert traces[name] == traces[name + "Reverse"], "grant registration order changed behavior"
inputs = [fixtures / ("multi-rule-writes" + ext) for ext in (".py", ".cpp", ".sv")]
inputs.append(fixtures / "multi-rule-writes/design.py")
(scratch / "candidate.json").write_text(json.dumps({
    "inputs": {str(path.relative_to(repo)): digest(path) for path in inputs},
    "execution": summary, "compile_only": ["mapping-void", "checked-grants"],
    "budget_cases": [name for name, _, _ in budget_cases],
    "address_roots": list(address_modes), "address_accepts": list(address_accepts),
    "address_rejects": list(address_rejects),
    "closed_constant_roots": ["ScalarClosedExpression", "ScalarClosedAlias"],
    "field_roots": list(field_modes), "field_rejections": list(field_rejections),
    "proof_accepts": list(proof_accepts), "proof_rejects": list(proof_rejects),
    "rejections": list(negative_sources) + list(range_rejections) + list(return_rejections) + list(system_rejections)}, indent=2) + "\n")
sys.stdout.write("multi-rule gate passed: source-intent conflicts, old-Q fields, order, tables, rule-owner enables and atomic discard\n")
