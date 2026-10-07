"""Independent lexical-site binding, field scheduling and old-Q feedback oracles."""

import argparse
import ast
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
for name in ("repo", "source-compiler", "linker", "emitter", "cxx", "verilator", "scratch"):
    parser.add_argument("--" + name, required=True)
for name in ("iverilog", "vvp"):
    parser.add_argument("--" + name, default=shutil.which(name))
parser.add_argument("--baseline-rejection", action="store_true")
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
env = dict(os.environ, PYTHONPATH=str(repo / "python/pycircuit/src"),
           PYTHONDONTWRITEBYTECODE="1", PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
           PYCIRCUIT_LINKER=args.linker, PYCIRCUIT_EMITTER=args.emitter)
commands = []


def run(command, accepted=True):
    command = list(map(str, command))
    result = subprocess.run(command, env=env, cwd=repo, capture_output=True,
                            text=True, timeout=240)
    commands.append({"command": command, "exit_status": result.returncode,
                     "stdout": result.stdout, "stderr": result.stderr})
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == (0 if accepted else 1), commands[-1]
    assert "Assertion failed" not in result.stderr and "Traceback" not in result.stderr, commands[-1]
    return result


def cli(*arguments, accepted=True):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], accepted)


def snapshot(directory):
    return {path.relative_to(directory).as_posix(): path.read_bytes()
            for path in directory.rglob("*") if path.is_file()}


DESIGN = """import pycircuit as ac

@ac.struct
class Pair:
    left: ac.u13
    right: ac.u13

@ac.struct
class Node:
    pair: Pair
    wide: ac.bits[73]

@ac.struct
class GraphResult:
    a_left: ac.u13
    a_right: ac.u13
    b_left: ac.u13
    b_right: ac.u13
    a_wide: ac.bits[73]
    b_wide: ac.bits[73]

@ac.struct
class Scalar:
    value: ac.u13

@ac.struct
class Taken:
    early: ac.u13
    forwarded: ac.u13

@ac.struct
class LexicalResult:
    early: ac.u13
    late: ac.u13
    future: ac.u13
    alias: ac.u13
    saved: ac.u13
    mutated: ac.u13
    after: ac.u13
    call_saved: ac.u13
    call_latest: ac.u13

@ac.struct
class Word:
    value: ac.bits[73]

@ac.struct
class CallsResult:
    shared: ac.bits[73]
    fanout: ac.bits[73]
    nested_left: ac.bits[73]
    nested_right: ac.bits[73]
    zero: ac.bits[73]
    zero_again: ac.bits[73]

@ac.struct
class FeedbackResult:
    q: ac.bits[73]
    computed: ac.bits[73]

@ac.module
def Graph(x: ac.u13, y: ac.u13, wide: ac.bits[73]) -> GraphResult:
    a = Side(b.pair.left, x, wide)
    b = Side(a.pair.left, y, wide)
    return GraphResult(a_left=a.pair.left, a_right=a.pair.right,
                       b_left=b.pair.left, b_right=b.pair.right,
                       a_wide=a.wide, b_wide=b.wide)

@ac.module
def Lexical(first: ac.u13, second: ac.u13) -> LexicalResult:
    old_alias = first
    x = first
    a = Take(b.value, x)
    x = second
    b = Single(x)
    local = Pair(left=first, right=first)
    saved = local
    local.left = second
    x = first
    after = Single(x)
    before = Single(first)
    saved_call = before
    before = Single(second)
    return LexicalResult(early=a.early, late=a.forwarded, future=b.value,
                         alias=old_alias, saved=saved.left, mutated=local.left,
                         after=after.value, call_saved=saved_call.value,
                         call_latest=before.value)

@ac.module
def Calls(wide: ac.bits[73]) -> CallsResult:
    shared = Echo(wide)
    fanout = shared
    left = Echo(Echo(wide).value)
    right = Echo(Echo(wide).value)
    zero = Constant()
    return CallsResult(shared=shared.value, fanout=fanout.value,
                       nested_left=left.value, nested_right=right.value,
                       zero=zero.value, zero_again=zero.value)

@ac.module
def Feedback(delta: ac.bits[73], enable: ac.u1) -> FeedbackResult:
    stored = Store(mixed.value, enable)
    mixed = Mix(stored.value, delta)
    return FeedbackResult(q=stored.value, computed=mixed.value)

@ac.module
def Side(incoming: ac.u13, own: ac.u13, wide: ac.bits[73]) -> Node:
    return Node(pair=Pair(left=own, right=incoming), wide=wide)

@ac.module
def Take(forwarded: ac.u13, early: ac.u13) -> Taken:
    return Taken(early=early, forwarded=forwarded)

@ac.module
def Single(value: ac.u13) -> Scalar:
    return Scalar(value=value)

@ac.module
def Echo(value: ac.bits[73]) -> Word:
    return Word(value=value)

@ac.module
def Constant() -> Word:
    return Word(value=17)

@ac.rule
def update(state, value, enable) -> Word:
    old = state
    if enable:
        state = value
    return Word(value=old)

@ac.module
def Store(value: ac.bits[73], enable: ac.u1) -> Word:
    state: ac.bits[73] = 0
    return update(state, value, enable)

@ac.module
def Mix(old: ac.bits[73], delta: ac.bits[73]) -> Word:
    return Word(value=old ^ delta)
"""

# The retained API roots use the same execution loop with independently
# published leaf/wrapper providers. Explicit owner binding replaces retired capture.
HISTORICAL_LEAF = """import pycircuit as ac
@ac.struct
class Byte:
    value: ac.u8
@ac.module
def Increment(value: ac.u8) -> Byte:
    return Byte(value=value + 1)
@ac.rule
def accumulate(total, incoming) -> Byte:
    total = total + incoming
    return Byte(value=total)
@ac.module
def CapturedAccumulator(incoming: ac.u8) -> Byte:
    total: ac.u8 = 0
    return accumulate(total, incoming)
@ac.module
def LexicalAccumulator(incoming: ac.u8) -> Byte:
    total: ac.u8 = 0
    return accumulate(total, incoming)
"""
HISTORICAL_WRAPPER = """import pycircuit as ac
from graph_binding.historical_leaf import Byte, Increment
@ac.module
def Wrapper(value: ac.u8) -> Byte:
    return Increment(value)
"""
HISTORICAL_ROOTS = {
    "InferredModule": ("Increment", "inferred-module", "DIRECT_BYTE_STATIC", 1028, 4),
    "InferredNested": ("Wrapper", "inferred-nested-module", "DIRECT_BYTE_STATIC", 1028, 4),
    "InferredRule": ("CapturedAccumulator", "inferred-nested-rule", "DIRECT_BYTE_STATE", 29, 5),
    "InferredStateful": ("LexicalAccumulator", "inferred-stateful-module", "DIRECT_BYTE_STATE", 29, 5),
}
HISTORICAL_CONSUMER = """import pycircuit as ac
from graph_binding.historical_leaf import Increment, CapturedAccumulator, LexicalAccumulator
from graph_binding.historical_wrapper import Wrapper
@ac.struct
class PairBytes:
    left: ac.u8
    right: ac.u8
"""
for top, (child, *_rest) in HISTORICAL_ROOTS.items():
    HISTORICAL_CONSUMER += f"""
@ac.module
def {top}(left: ac.u8, right: ac.u8) -> PairBytes:
    a = {child}(left)
    b = {child}(right)
    return PairBytes(left=a.value, right=b.value)
"""

# Reorder only declarations and eligible immutable instance-result assignments;
# the lexical snapshots intentionally retain their source order.
tree = ast.parse(DESIGN)
segments = [ast.get_source_segment(DESIGN, node) for node in tree.body]
# Include decorators in the reordered source, not just the def/class bodies.
segments = ["\n".join([*("@" + ast.unparse(decorator) for decorator in getattr(node, "decorator_list", [])), segment])
            for node, segment in zip(tree.body, segments, strict=True)]
reordered = "\n\n".join([segment for node, segment in zip(tree.body, segments, strict=True)
                          if not isinstance(node, ast.FunctionDef)] +
                         [segment for node, segment in reversed(list(zip(tree.body, segments, strict=True)))
                          if isinstance(node, ast.FunctionDef)]) + "\n"
reordered = reordered.replace("    a = Side(b.pair.left, x, wide)\n    b = Side(a.pair.left, y, wide)",
                              "    b = Side(a.pair.left, y, wide)\n    a = Side(b.pair.left, x, wide)")

PREFIX = "import pycircuit as ac\n@ac.struct\nclass Scalar:\n    value: ac.u13\n@ac.module\ndef Single(value: ac.u13) -> Scalar:\n    return Scalar(value=value)\n"


def ordinary(body):
    return PREFIX + "@ac.module\ndef Top(first: ac.u13, second: ac.u13) -> Scalar:\n" + body


cases = {
    "arbitrary-local-forward": ordinary("    a = Single(local)\n    local = first\n    return a\n"),
    "unknown-forward": ordinary("    a = Single(missing.value)\n    return a\n"),
    "rebound-future": ordinary("    a = Single(b.value)\n    b = Single(first)\n    b = Single(second)\n    return a\n"),
    "mutated-future": ordinary("    a = Single(b.value)\n    b = Single(first)\n    b.value = second\n    return a\n"),
    "indexed-mutation-future": ordinary("    a = Single(b.value)\n    b = Single(first)\n    b.value[0] = second\n    return a\n"),
    "annotated-future": ordinary("    a = Single(b.value)\n    b: Scalar = Single(first)\n    return a\n"),
    "definition-shadow-future": PREFIX + "@ac.module\ndef Later(value: ac.u13) -> Scalar:\n    return Scalar(value=value)\n@ac.module\ndef Top(first: ac.u13) -> Scalar:\n    a = Single(Later.value)\n    Later = Single(first)\n    return a\n",
    "struct-shadow-future": PREFIX + "@ac.module\ndef Top(first: ac.u13) -> Scalar:\n    a = Single(Scalar.value)\n    Scalar = Single(first)\n    return a\n",
    "runtime-controlled": ordinary("    if first == second:\n        a = Single(first)\n    return Scalar()\n"),
    "rule-local-call": PREFIX + "@ac.rule\ndef calculate(value) -> Scalar:\n    a = Single(value)\n    return a\n@ac.module\ndef Top(value: ac.u13) -> Scalar:\n    return calculate(value)\n",
    "recursive-definition": PREFIX + "@ac.module\ndef Recurse(value: ac.u13) -> Scalar:\n    return Recurse(value)\n",
    "static-parameter-call": PREFIX + "@ac.module\ndef Static(value: ac.u13, *, extra: int = 1) -> Scalar:\n    return Scalar(value=value)\n@ac.module\ndef Top(value: ac.u13) -> Scalar:\n    return Static(value)\n",
    "missing-actual": ordinary("    a = Single()\n    return a\n"),
    "duplicate-actual": ordinary("    a = Single(first, value=second)\n    return a\n"),
    "wrong-kind-actual": ordinary("    a = Single(first == second)\n    return a\n"),
    "wrong-nominal-actual": "import pycircuit as ac\n@ac.struct\nclass Left:\n    value: ac.u13\n@ac.struct\nclass Right:\n    value: ac.u13\n@ac.module\ndef Child(value: Left) -> Left:\n    return value\n@ac.module\ndef Top(value: ac.u13) -> Left:\n    return Child(Right(value=value))\n",
    "observed-cycle": ordinary("    a = Single(b.value)\n    b = Single(a.value)\n    return a\n"),
    "self-cycle": ordinary("    a = Single(a.value)\n    return a\n"),
    "dead-cycle": ordinary("    a = Single(b.value)\n    b = Single(a.value)\n    return Scalar()\n"),
}

diagnostics = {'arbitrary-local-forward': "unknown hardware value 'local'",
 'unknown-forward': "unknown hardware value 'missing'",
 'rebound-future': "ambiguous forward module result 'b'",
 'mutated-future': "ambiguous forward module result 'b'",
 'indexed-mutation-future': "ambiguous forward module result 'b'",
 'annotated-future': "unknown hardware value 'b'",
 'definition-shadow-future': "ambiguous forward module result 'Later'",
 'struct-shadow-future': "ambiguous forward module result 'Scalar'",
 'runtime-controlled': 'direct module calls under runtime control are unsupported',
 'rule-local-call': 'direct module calls are only admitted at module scope',
 'recursive-definition': 'recursive module instantiation is unsupported',
 'static-parameter-call': 'behavioral module static parameters are unsupported',
 'missing-actual': 'missing direct module argument',
 'duplicate-actual': 'unknown or repeated direct module argument',
 'wrong-kind-actual': 'unsigned boundary hardware type mismatch',
 'wrong-nominal-actual': 'struct constructor nominal type disagrees with destination',
 'observed-cycle': "'ac.module' op combinational cycle in hardware field dependencies",
 'self-cycle': "'ac.module' op combinational cycle in hardware field dependencies",
 'dead-cycle': "'ac.module' op combinational cycle in hardware field dependencies"}
assert cases.keys() == diagnostics.keys()

with tempfile.TemporaryDirectory(prefix="direct-call-graph-", dir=scratch) as temporary:
    build = Path(temporary)
    source = build / "source"
    source.mkdir()
    design = source / "design.py"
    design.write_text(DESIGN)
    (scratch / "design.py").write_text(DESIGN)
    unit = build / "unit"

    def compile_source(path, output, accepted=True, replace=False, interfaces=()):
        arguments = ["compile", "-c", path, "--source-root", source,
                     "--package-prefix", "graph_binding", "-o", output]
        if replace:
            arguments.append("--replace")
        for interface in interfaces:
            arguments.extend(["-I", interface])
        return cli(*arguments, accepted=accepted)

    if args.baseline_rejection:
        result = compile_source(design, unit, accepted=False)
        assert "unknown hardware value 'b'" in result.stderr, result.stderr
        assert not unit.exists()
        sys.stdout.write("direct-call graph baseline rejects forward module-result fields\n")
        sys.exit(0)
    compile_source(design, unit)
    reverse = source / "reverse.py"
    reverse.write_text(reordered)
    reverse_unit = build / "reverse-unit"
    compile_source(reverse, reverse_unit)
    history_leaf, history_wrapper = build / "history-leaf", build / "history-wrapper"
    history_units = {"design": build / "history-unit", "reverse": build / "history-reverse-unit"}
    leaf_source = source / "historical_leaf.py"
    wrapper_source = source / "historical_wrapper.py"
    leaf_source.write_text(HISTORICAL_LEAF)
    wrapper_source.write_text(HISTORICAL_WRAPPER)
    compile_source(leaf_source, history_leaf)
    hidden = build / "hidden-providers"
    hidden.mkdir()
    # Published interfaces are sufficient for every dependent compilation.
    for path in (leaf_source, history_leaf / "historical_leaf.ac", history_leaf / "historical_leaf.d"):
        path.rename(hidden / path.name)
    compile_source(wrapper_source, history_wrapper, interfaces=[history_leaf])
    for path in (wrapper_source, history_wrapper / "historical_wrapper.ac", history_wrapper / "historical_wrapper.d"):
        path.rename(hidden / path.name)
    for variant in ("design", "reverse"):
        filename = "historical" if variant == "design" else "historical_reverse"
        text = HISTORICAL_CONSUMER
        if variant == "reverse":
            for child, *_rest in HISTORICAL_ROOTS.values():
                text = text.replace(f"    a = {child}(left)\n    b = {child}(right)",
                                    f"    b = {child}(right)\n    a = {child}(left)")
        path = source / (filename + ".py")
        path.write_text(text)
        (scratch / path.name).write_text(text)
        compile_source(path, history_units[variant], interfaces=[history_leaf, history_wrapper])
        dependencies = (history_units[variant] / (filename + ".d")).read_text()
        for provider, basename in ((history_leaf, "historical_leaf"), (history_wrapper, "historical_wrapper")):
            assert str(provider / (basename + ".interface.ac")) in dependencies
            for suffix in (".ac", ".d"):
                assert str(provider / (basename + suffix)) not in dependencies
            assert str(source / (basename + ".py")) not in dependencies
    for provider, basename in ((history_leaf, "historical_leaf"), (history_wrapper, "historical_wrapper")):
        for suffix in (".ac", ".d"):
            (hidden / (basename + suffix)).rename(provider / (basename + suffix))

    toolroot = Path(args.source_compiler).resolve().parent.parent
    runtime = next((path for path in (toolroot / "simulator/gfsim/libpyc6_runtime.a",
                                     toolroot / "lib/libpyc6_runtime.a") if path.is_file()), None)
    assert runtime is not None, "Runtime archive missing from this build/install"
    products = []
    summaries = []
    execution_cases = [("Graph", "DIRECT_GRAPH", 12, 4),
                                      ("Lexical", "DIRECT_LEXICAL", 12, 0),
                                      ("Calls", "DIRECT_CALLS", 12, 4),
                                      ("Feedback", "DIRECT_FEEDBACK", 16, 4)]
    execution_cases += [(top, define, frames, masks)
                        for top, (_, _, define, frames, masks) in HISTORICAL_ROOTS.items()]
    for top, define, frames, masks in execution_cases:
        reference = None
        for variant, active_unit in (("design", unit), ("reverse", reverse_unit)):
            output = build / (top + "-" + variant)
            output.mkdir()
            final = output / "design_top.ac"
            if top in HISTORICAL_ROOTS:
                module = "historical" if variant == "design" else "historical_reverse"
                cli("link", history_leaf, history_wrapper, history_units[variant],
                    "--top", "graph_binding." + module + "." + top, "-o", final)
            else:
                cli("link", active_unit, "--top", "graph_binding." + variant + "." + top, "-o", final)
            # Count actual semantic call occurrences within the requested owner.
            text = final.read_text()
            calls = [line for line in text.splitlines() if '"ac.instance"' in line and
                     "definition = @graph_binding." + variant + "." + top in line.split("parameters =")[0]]
            if top == "Calls":
                assert len(calls) == 6, calls  # five Echo sites and one zero-arg Constant
                assert sum(".Echo" in line for line in calls) == 5
                assert sum(".Constant" in line for line in calls) == 1
            if top == "Graph":
                assert len(calls) == 2 and all(".Side" in line for line in calls), calls
            for target in ("cpp", "verilog"):
                cli("emit", final, "--target", target, "-o", output / target)
            receipt = json.loads((output / "cpp/generated.json").read_text())
            cpp = [output / "cpp" / row["path"] for row in receipt["files"] if row["path"].endswith(".cpp")]
            runner = output / "runner"
            run([args.cxx, "-std=c++20", "-pthread", "-D" + define, "-I" + str(repo / "include"),
                 "-I" + str(output / "cpp"), fixtures / "direct-call-graph.cpp", *cpp, runtime, "-o", runner])
            traces = []
            for workers in (1, 2):
                trace = run([runner, str(workers)]).stdout
                (scratch / f"{top}-{variant}-workers-{workers}.stdout").write_text(trace)
                traces.append([row for row in trace.splitlines() if row.startswith(("WORK ", "MASK "))])
            assert len(traces[0]) == frames + masks and traces[0] == traces[1]
            if reference is None:
                reference = traces[0]
            else:
                assert reference == traces[0], "declaration/definition reorder changed hardware"
            receipt = json.loads((output / "verilog/generated.json").read_text())
            rtl = [output / "verilog" / row["path"] for row in receipt["files"] if row["role"] == "rtl"]
            rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
            rtl_build = output / "rtl-build"
            run([args.verilator, "--binary", "--timing", "--top-module", "tb", "--prefix", "Vgraph",
                 "--Mdir", rtl_build, "-j", "2", "-Wno-fatal", "-D" + define,
                 *sorted((repo / "include/verilog").glob("*.v")), *rtl, fixtures / "direct-call-graph.sv"])
            rtl_trace = run([rtl_build / "Vgraph"]).stdout
            (scratch / f"{top}-{variant}-rtl.stdout").write_text(rtl_trace)
            assert [row for row in rtl_trace.splitlines() if row.startswith("WORK ")] == [row for row in traces[0] if row.startswith("WORK ")]
            if args.iverilog and args.vvp:
                rtl_runner = output / "rtl-four-state"
                run([args.iverilog, "-g2012", "-DDIRECT_FOUR_STATE", "-D" + define, "-s", "tb", "-o", rtl_runner,
                     *sorted((repo / "include/verilog").glob("*.v")), *rtl, fixtures / "direct-call-graph.sv"])
                four = run([args.vvp, rtl_runner]).stdout
                (scratch / f"{top}-{variant}-rtl-four-state.stdout").write_text(four)
                assert [row for row in four.splitlines() if row.startswith(("WORK ", "MASK "))] == traces[0]
            products.extend([final, output / "cpp", output / "verilog"])
            summaries.append({"top": top, "variant": variant, "frames": frames, "masks": masks})

    before = snapshot(unit)
    protected = {path: snapshot(path) if path.is_dir() else path.read_bytes() for path in products}
    for name, text in cases.items():
        design.write_text(text)
        (scratch / (name + ".py")).write_text(text)
        absent = build / ("invalid-" + name)
        rejected = compile_source(design, absent, accepted=False)
        assert diagnostics[name] in rejected.stderr, rejected.stderr
        assert not absent.exists()
        replacement = compile_source(design, unit, accepted=False, replace=True)
        assert diagnostics[name] in replacement.stderr, replacement.stderr
        assert snapshot(unit) == before
        assert all((snapshot(path) if path.is_dir() else path.read_bytes()) == value for path, value in protected.items())
    inputs = [fixtures / ("direct-call-graph" + extension) for extension in (".py", ".cpp", ".sv")]
    (scratch / "candidate.json").write_text(json.dumps({
        "fixtures": {str(path.relative_to(repo)): hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs},
        "execution": summaries, "historical_roots": HISTORICAL_ROOTS, "rejected_cases": sorted(cases), "protected_rejections": len(cases) * 2,
    }, indent=2) + "\n")

sys.stdout.write("direct-call graph gate passed: forward field graph, lexical snapshots, AST sites/fanout, old-Q feedback; " + str(len(cases) * 2) + " protected rejections\n")
