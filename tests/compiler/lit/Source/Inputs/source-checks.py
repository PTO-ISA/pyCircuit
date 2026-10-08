"""Independent source-check SSA, authority, and public managed lifecycle gates."""

import argparse
import ast
import hashlib
import itertools
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def balanced(text, start, opening, closing):
    depth, quoted, escaped = 0, False, False
    for end in range(start, len(text)):
        char = text[end]
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
            continue
        if char == '"':
            quoted = True
        elif char == opening:
            depth += 1
        elif char == closing:
            depth -= 1
            if depth == 0:
                return text[start : end + 1]
    raise AssertionError("unclosed IR attribute")


def attribute(text, key, opening="{", closing="}"):
    match = re.search(
        r"(?<![\w.])" + re.escape(key) + r"\s*=\s*" + re.escape(opening), text
    )
    assert match, (key, text)
    return balanced(text, match.end() - 1, opening, closing)


def top_level_keys(dictionary):
    assert dictionary.startswith("{") and dictionary.endswith("}")
    fields, start, depth, quoted, escaped = [], 1, 0, False, False
    for index, char in enumerate(dictionary[1:-1], 1):
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
        elif char == '"':
            quoted = True
        elif char in "{[":
            depth += 1
        elif char in "}]":
            depth -= 1
        elif char == "," and depth == 0:
            fields.append(dictionary[start:index])
            start = index + 1
    fields.append(dictionary[start:-1])
    keys = []
    for field in fields:
        match = re.match(r"\s*([\w.]+)\s*=", field)
        assert match, field
        keys.append(match[1])
    return keys


def occurrence(text):
    definition = re.search(r"definition = @([\w.]+)", text)
    assert definition, text
    path = attribute(text, "ast_path", "[", "]")
    parts = re.findall(
        r'kind = "(field|index)", (?:name = "([^"]+)"|value = (\d+) : i64)', path
    )
    return definition[1], tuple(
        name if kind == "field" else int(index) for kind, name, index in parts
    )


def ast_paths(node, path=()):
    yield node, path
    for name, value in ast.iter_fields(node):
        if isinstance(value, ast.AST):
            yield from ast_paths(value, path + (name,))
        elif isinstance(value, list):
            for index, child in enumerate(value):
                if isinstance(child, ast.AST):
                    yield from ast_paths(child, path + (name, index))


def operands(line):
    return re.findall(
        r"%[\w]+(?:#\d+)?", re.search(r'"ac\.[\w.]+"\(([^)]*)\)', line)[1]
    )


def results(line):
    left = line.split('"ac.', 1)[0].strip().rstrip("= ")
    grouped = re.fullmatch(r"(%\w+):(\d+)", left)
    if grouped:
        return [grouped[1] + "#" + str(i) for i in range(int(grouped[2]))]
    return re.findall(r"%\w+", left)


def parse_ir(text):
    aliases = dict(re.findall(r"^(#\w+) = (.*)$", text, re.M))
    for _ in range(len(aliases) + 1):
        expanded = re.sub(r"#\w+(?![\w.])", lambda m: aliases.get(m[0], m[0]), text)
        if expanded == text:
            break
        text = expanded
    else:
        raise AssertionError("IR alias cycle")
    modules, stack = [], []
    for line in text.splitlines():
        if '"ac.module"()' in line:
            item = {"text": line, "rules": [], "expects": [], "ops": {}, "args": []}
            modules.append(item)
            stack.append(item)
        elif '"ac.rule"(' in line:
            item = {
                "text": line,
                "ops": {},
                "args": [],
                "results": results(line),
                "captures": operands(line),
            }
            stack[0]["rules"].append(item)
            stack.append(item)
        elif stack and line.lstrip().startswith("^bb"):
            stack[-1]["args"] = re.findall(r"(%\w+)\s*:", line)
        elif stack and line.lstrip().startswith("})"):
            stack[-1]["text"] += "\n" + line
            stack.pop()
        elif stack and re.search(r'"ac\.[\w.]+"\(', line):
            name = re.search(r'"(ac\.[\w.]+)"', line)[1]
            if name == "ac.yield":
                stack[-1]["yield"] = operands(line)
            elif name == "ac.expect":
                stack[0]["expects"].append(line)
            else:
                for value in results(line):
                    stack[-1]["ops"][value] = (name, operands(line), line)
    assert not stack and modules, "no complete hardware module in emitted IR"
    return modules


def check_source_ir(text, path):
    syntax = ast.parse(path.read_text())
    nodes = list(ast_paths(syntax))
    checked_rules = {
        node.name: node
        for node, _ in nodes
        if isinstance(node, ast.FunctionDef)
        and any(isinstance(child, ast.Assert) for child in ast.walk(node))
        and any(
            isinstance(dec, ast.Name | ast.Attribute)
            and (dec.id if isinstance(dec, ast.Name) else dec.attr) == "rule"
            for dec in node.decorator_list
        )
    }
    checked = []
    for module in parse_ir(text):
        for rule in module["rules"]:
            name = re.search(r'name = "([^"]+)"', rule["text"])[1]
            if name not in checked_rules:
                assert "ac.required_checks" not in rule["text"]
                continue
            source_rule = checked_rules[name]
            checks = [
                (node, site)
                for node, site in nodes
                if isinstance(node, ast.Assert) and node in list(ast.walk(source_rule))
            ]
            registration = occurrence(
                attribute(rule["text"].split("\n", 1)[0], "occurrence")
            )
            assert (
                registration[0] == re.search(r'sym_name = "([^"]+)"', module["text"])[1]
            )
            assert registration[0].startswith("checks." + path.stem + ".")
            assert any(
                isinstance(node, ast.Call)
                and isinstance(node.func, ast.Name)
                and node.func.id == name
                and site == registration[1]
                for node, site in nodes
            ), registration
            required = attribute(rule["text"], "ac.required_checks", "[", "]")
            entries, offset = [], 1
            while offset < len(required) - 1:
                start = required.find("{", offset)
                if start < 0:
                    break
                entry = balanced(required, start, "{", "}")
                entries.append(entry)
                offset = start + len(entry)
            assert len(entries) == len(checks), (name, entries, checks)
            assert len(rule["results"]) == len(rule["yield"])
            first = len(rule["results"]) - 2 * len(entries)
            assert first >= 0

            def associated(value, ops=module["ops"]):
                while value in ops:
                    op_name, used, line = ops[value]
                    if op_name != "ac.bits.extract":
                        break
                    # Native verification already validates the operation. This
                    # fixture's one-bit carrier can forward only low-zero,
                    # equally wide extraction, not a cast/select substitution.
                    integers = list(
                        map(int, re.findall(r"#ac.math_int<(-?\d+)>", line))
                    )
                    assert integers[0] == 0 and set(integers[1:]) == {1}, line
                    value = used[0]
                return value

            attached = []
            for index, ((node, site), entry) in enumerate(
                zip(checks, entries, strict=True)
            ):
                identity = attribute(entry, "id")
                assert sorted(top_level_keys(identity)) == [
                    "check",
                    "obligation",
                    "registration",
                ], identity
                assert occurrence(attribute(identity, "registration")) == registration
                assert occurrence(attribute(identity, "check")) == (
                    registration[0],
                    site,
                )
                assert re.search(r"obligation = 0 : i64", identity), identity
                pair = rule["results"][first + 2 * index : first + 2 * index + 2]
                matches = [
                    expect
                    for expect in module["expects"]
                    if list(map(associated, operands(expect))) == pair
                ]
                assert len(matches) == 1, (pair, matches)
                expect = matches[0]
                assert attribute(expect, "ac.check_id") == identity
                assert attribute(expect, "location") == attribute(entry, "location")
                span = attribute(expect, "location")
                assert 'path = "' + path.name + '"' in span
                for field, value in (
                    ("line", node.lineno),
                    ("column", node.col_offset + 1),
                    ("end_line", node.end_lineno),
                    ("end_column", node.end_col_offset + 1),
                ):
                    assert re.search(
                        r"(?<![\w_])" + field + " = " + str(value) + " : i64", span
                    )
                assert 'kind = "assert"' in expect and 'kind = "assert"' in entry
                if node.msg is None:
                    assert "ac.message" not in expect
                else:
                    assert "ac.message = " + json.dumps(node.msg.value) in expect
                attached.append(
                    (
                        node.msg.value if node.msg else None,
                        *rule["yield"][first + 2 * index : first + 2 * index + 2],
                    )
                )
            rule["checks"] = attached
            checked.append((module, rule))
    assert sum(len(rule["checks"]) for _, rule in checked) == sum(
        1 for module in parse_ir(text) for _ in module["expects"]
    )
    assert checked, path
    return checked


# These one-bit truth operations are independent test oracles for actual SSA
# check carriers. They neither lower sources nor simulate storage or a DUT.
def inv(value):
    return "1" if value == "0" else "0" if value == "1" else "x"


def and_bit(a, b):
    return "0" if "0" in (a, b) else "1" if a == b == "1" else "x"


def choose(control, yes, no):
    return (
        yes if control == "1" else no if control == "0" else yes if yes == no else "x"
    )


def evaluate(rule, value, inputs):
    if value in rule["args"]:
        return inputs[rule["args"].index(value)]
    name, used, line = rule["ops"][value]
    values = [evaluate(rule, operand, inputs) for operand in used]
    if name == "ac.bits.constant":
        number = int(re.search(r"#ac.math_int<(-?\d+)>", line)[1])
        assert number in (0, 1), line
        return str(number)
    if name == "ac.bits.binary" and 'opcode = "and"' in line:
        return and_bit(*values)
    if name == "ac.bits.unary" and 'opcode = "not"' in line:
        return inv(values[0])
    if name == "ac.bits.select":
        return choose(*values)
    if name == "ac.bits.compare" and 'predicate = "eq"' in line:
        return (
            "1"
            if values[0] == values[1] and values[0] in "01"
            else "0" if set(values) == {"0", "1"} else "x"
        )
    if name in {"ac.bits.extract", "ac.bits.resize"}:
        return values[0]
    raise AssertionError(("unexpected check SSA producer", name, line))


def main():
    parser = argparse.ArgumentParser()
    for name in (
        "repo",
        "source-compiler",
        "linker",
        "optimizer",
        "emitter",
        "cxx",
        "verilator",
        "scratch",
    ):
        parser.add_argument("--" + name, required=True)
    args = parser.parse_args()
    repo, evidence = Path(args.repo).resolve(), Path(args.scratch).resolve()
    evidence.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="source-checks-", dir=evidence))
    source, units = root / "source", root / "units"
    source.mkdir()
    units.mkdir()
    fixtures = Path(__file__).resolve().parent / "source_checks"
    for fixture in fixtures.glob("*.py"):
        shutil.copyfile(fixture, source / fixture.name)
    env = dict(
        os.environ,
        PYTHONPATH=str(repo / "python/pycircuit/src"),
        PYTHONDONTWRITEBYTECODE="1",
        PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
        PYCIRCUIT_LINKER=args.linker,
        PYCIRCUIT_EMITTER=args.emitter,
    )
    commands, receipts = [], []

    def run(command, code=0, diagnostic=None):
        command = list(map(str, command))
        result = subprocess.run(
            command, cwd=repo, env=env, capture_output=True, text=True, timeout=45
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
            "Traceback" not in result.stderr and "Assertion failed" not in result.stderr
        ), row
        if diagnostic:
            assert diagnostic in result.stderr, row
        return result

    def cli(*arguments, code=0, diagnostic=None, optimized=False):
        return run(
            [
                sys.executable,
                *(["-O"] if optimized else []),
                "-m",
                "pycircuit.cli",
                *arguments,
            ],
            code,
            diagnostic,
        )

    def compile_unit(
        name,
        output,
        imports=(),
        replace=False,
        code=0,
        diagnostic=None,
        optimized=False,
    ):
        command = [
            "compile",
            "-c",
            source / name,
            "--source-root",
            source,
            "--package-prefix",
            "checks",
            "-o",
            output,
        ]
        for unit in imports:
            command.extend(("-I", unit))
        if replace:
            command.append("--replace")
        return cli(*command, code=code, diagnostic=diagnostic, optimized=optimized)

    def payload(unit, kind):
        return unit / json.loads((unit / "unit.json").read_text())["files"][kind]

    def snapshot(path):
        return {
            p.relative_to(path).as_posix(): p.read_bytes()
            for p in path.rglob("*")
            if p.is_file()
        }

    def protected(path):
        return {
            "payload": snapshot(path) if path.is_dir() else path.read_bytes(),
            "control": snapshot(
                path.parent / ("." + path.name + ".pycircuit-publication")
            ),
        }

    def generic(path, destination):
        run([args.optimizer, path, "--mlir-print-op-generic", "-o", destination])
        return destination.read_text()

    lowered = {}
    for name in ("snapshots", "continuation", "matrix", "provider", "unchecked"):
        unit, optimized = units / name, units / (name + "-optimized")
        compile_unit(name + ".py", unit)
        compile_unit(name + ".py", optimized, optimized=True)
        for kind in ("body", "interface"):
            assert (
                payload(unit, kind).read_bytes()
                == payload(optimized, kind).read_bytes()
            ), (name, kind, "-O changed obligations")
        text = generic(payload(unit, "body"), root / (name + ".generic.ac"))
        if name != "unchecked":
            lowered[name] = check_source_ir(text, source / (name + ".py"))
        final = root / (name + ".ac")
        cli(
            "link",
            unit,
            "--top",
            "checks." + name + (".Checked" if name == "provider" else ".Top"),
            "-o",
            final,
        )
        if name != "unchecked":
            check_source_ir(
                generic(final, root / (name + ".final.generic.ac")),
                source / (name + ".py"),
            )
        receipts.append(
            {
                "name": name,
                "body_sha256": hashlib.sha256(
                    payload(unit, "body").read_bytes()
                ).hexdigest(),
                "final_sha256": hashlib.sha256(final.read_bytes()).hexdigest(),
                "optimized_equal": True,
            }
        )

    _, snapshot_rule = lowered["snapshots"][0]
    assert len(snapshot_rule["checks"]) == 4
    # Formal order is declared by source; SSA numbering is read from actual IR.
    expected = {"entry": 0, "between": 1, "after": 2, "saved": 0}
    for message, condition, _ in snapshot_rule["checks"]:
        for inputs in itertools.product("01xz", repeat=len(snapshot_rule["args"])):
            assert (
                evaluate(snapshot_rule, condition, inputs) == inputs[expected[message]]
            ), (message, condition, inputs)
    assert snapshot_rule["yield"][0] in snapshot_rule["ops"]
    output_op = snapshot_rule["ops"][snapshot_rule["yield"][0]]
    assert (
        output_op[0] == "ac.struct.create"
        and output_op[1][0] == snapshot_rule["args"][2]
    )

    _, rule = lowered["continuation"][0]
    for gate, inner, selector, ok in itertools.product("01xz", repeat=4):
        # Each assert updates L with logical AND, and each captured path is
        # A & L. In particular, 1 & Z is X even when a selector chooses Z.
        asserted_continuation = and_bit("1", ok)
        after_if = and_bit("1", choose(gate, asserted_continuation, "1"))
        equal_zero = inv(selector)
        gold = {
            "if-yes": (ok, and_bit("1", gate)),
            "nested": ("1", and_bit(and_bit(gate, ok), inner)),
            "if-join": ("1", and_bit("1", after_if)),
            "match-first": (ok, and_bit(after_if, equal_zero)),
            "match-join": (
                "1",
                and_bit(after_if, choose(equal_zero, asserted_continuation, "1")),
            ),
        }
        for message, condition, path in rule["checks"]:
            actual = (
                evaluate(rule, condition, [gate, inner, selector, ok]),
                evaluate(rule, path, [gate, inner, selector, ok]),
            )
            assert actual == gold[message], (
                message,
                gate,
                inner,
                selector,
                ok,
                actual,
                gold[message],
            )

    matrix_module, matrix_rule = lowered["matrix"][0]
    names = json.loads(attribute(matrix_module["text"], "input_names", "[", "]"))
    parent_args = matrix_module["args"]
    _, condition, path = matrix_rule["checks"][0]
    for p, c in itertools.product("01xz", repeat=2):
        actuals = [
            {"path": p, "condition": c}[names[parent_args.index(value)]]
            for value in matrix_rule["captures"]
        ]
        assert evaluate(matrix_rule, condition, actuals) == c
        assert evaluate(matrix_rule, path, actuals) == ("x" if p == "z" else p)

    provider_unit = units / "provider"
    provider_source = source / "provider.py"
    hidden = root / "hidden"
    hidden.mkdir()
    provider_source.rename(hidden / "provider.py")
    for kind in ("body", "dependencies"):
        # Depfile key is optional in older receipts; hide the published basename.
        path = provider_unit / ("provider.ac" if kind == "body" else "provider.d")
        if path.exists():
            path.rename(hidden / path.name)
    parent_unit = units / "parent"
    compile_unit("parent.py", parent_unit, [provider_unit])
    depfile = (parent_unit / "parent.d").read_text()
    assert str(provider_unit / "provider.interface.ac") in depfile
    assert (
        str(provider_source) not in depfile
        and str(provider_unit / "provider.ac") not in depfile
    )
    for basename in ("provider.ac", "provider.d"):
        (hidden / basename).rename(provider_unit / basename)
    final = root / "closure.ac"
    cli("link", provider_unit, parent_unit, "--top", "checks.parent.Top", "-o", final)
    final_text = generic(final, root / "closure.generic.ac")
    assert len(check_source_ir(final_text, hidden / "provider.py")) == 2
    assert (
        final_text.count('"ac.instance"(') == 2
    ), "unused checked provider occurrence disappeared"
    before_final = protected(final)
    cli(
        "link",
        parent_unit,
        "--top",
        "checks.parent.Top",
        "-o",
        root / "missing.ac",
        code=1,
    )
    assert not (root / "missing.ac").exists()
    cli(
        "link",
        parent_unit,
        "--top",
        "checks.parent.Top",
        "-o",
        final,
        "--replace",
        code=1,
    )
    assert protected(final) == before_final

    checked_products = []
    # Checked ordinary roots now publish through the verified reachable plan.
    # Preserve cross-owner output protection independently of check admission.
    for target in ("cpp", "verilog"):
        checked = root / ("checked-parent-" + target)
        cli("emit", final, "--target", target, "-o", checked)
        checked_receipt = json.loads((checked / "generated.json").read_text())
        assert checked_receipt["entry"] == {
            "definition": '@"checks.parent.Top"',
            "arguments": [],
        }
        names = {row["path"] for row in checked_receipt["files"]}
        assert ("pycircuit_system.hpp" if target == "cpp" else "design_top.sv") in names
        checked_products.append(checked)
        published = root / ("published-" + target)
        cli("emit", root / "unchecked.ac", "--target", target, "-o", published)
        before = protected(published)
        cli(
            "emit",
            final,
            "--target",
            target,
            "-o",
            published,
            "--replace",
            code=1,
            diagnostic="generated.json entry does not match its owner",
        )
        assert protected(published) == before

    observe_unit = units / "observe"
    compile_unit("observe.py", observe_unit)
    observe_final = root / "observe.ac"
    cli("link", observe_unit, "--top", "checks.observe.Top", "-o", observe_final)
    assert '"ac.observe"' in observe_final.read_text()
    # The C++ backend lowers `ac.observe` into a wired descriptor
    # table, so the cpp product must be published carrying its observation
    # surface. Verilog still rejects instrumentation by name. Neither backend
    # may displace a published product that belongs to a different owner.
    observe_cpp = root / "observe-cpp"
    cli("emit", observe_final, "--target", "cpp", "-o", observe_cpp)
    observe_receipt = json.loads((observe_cpp / "generated.json").read_text())
    observe_header = next(
        row["path"]
        for row in observe_receipt["files"]
        if row["path"].endswith("pycircuit_system.hpp")
    )
    observe_system = (observe_cpp / observe_header).read_text()
    assert "pyc_observation_descriptors" in observe_system
    assert "Configure(pyc_observation_descriptors" in observe_system
    assert "Configure({}, 0)" not in observe_system
    for target in ("cpp", "verilog"):
        published = root / ("published-" + target)
        before = protected(published)
        # The cpp case now reaches the publisher, which refuses to replace the
        # `checks.unchecked.Top` product with this `checks.observe.Top` design.
        diagnostic = (
            "generated.json entry does not match its owner"
            if target == "cpp"
            else "ac.observe"
        )
        cli(
            "emit",
            observe_final,
            "--target",
            target,
            "-o",
            published,
            "--replace",
            code=1,
            diagnostic=diagnostic,
        )
        assert protected(published) == before

    baseline = (source / "matrix.py").read_text()
    negatives = {
        "integer": (
            baseline.replace("assert condition", "assert 1"),
            "Boolean source condition, not Integer",
        ),
        "wide": (
            baseline.replace("condition: bool", "condition: ac.u2").replace(
                "from pycircuit import module, rule",
                "import pycircuit as ac\nfrom pycircuit import module, rule",
            ),
            "authoritative Boolean or bits[1]",
        ),
        "message": (
            baseline.replace('"matrix"', "condition"),
            "assert message must be a static string",
        ),
        "retired-nonlocal": (
            baseline.replace(
                'assert condition, "matrix"',
                'nonlocal condition\n            assert condition, "matrix"',
            ),
            "unsupported behavioral rule statement 'Nonlocal'",
        ),
    }
    protected_unit = units / "matrix"
    original = protected(protected_unit)
    for label, (text, diagnostic) in negatives.items():
        (source / "matrix.py").write_text(text)
        absent = units / ("bad-" + label)
        compile_unit("matrix.py", absent, code=1, diagnostic=diagnostic)
        assert not absent.exists(), label
        compile_unit(
            "matrix.py", protected_unit, replace=True, code=1, diagnostic=diagnostic
        )
        assert protected(protected_unit) == original, label
    (source / "matrix.py").write_text(baseline)
    # Runtime division is now legal. Preserve the assertion obligation and the
    # public checked-emission/output-protection boundary, not the old arithmetic diagnostic.
    (source / "matrix.py").write_text(
        "import pycircuit as ac\n@ac.struct\nclass Result:\n    value: ac.u1\n@ac.rule\ndef inspect(condition)->Result:\n    assert condition != 0\n    quotient = condition // condition\n    return Result(value=quotient)\n@ac.module\ndef Top(condition: ac.u1)->Result:\n    return inspect(condition)\n"
    )
    checked_divisor = units / "checked-runtime-divisor"
    compile_unit("matrix.py", checked_divisor)
    checked_final = root / "checked-runtime-divisor.ac"
    cli("link", checked_divisor, "--top", "checks.matrix.Top", "-o", checked_final)
    assert '"ac.expect"' in checked_final.read_text()
    assert 'opcode = "udiv"' in checked_final.read_text()
    for target in ("cpp", "verilog"):
        admitted = root / ("checked-divisor-" + target)
        cli("emit", checked_final, "--target", target, "-o", admitted)
        admitted_receipt = json.loads((admitted / "generated.json").read_text())
        assert admitted_receipt["entry"] == {
            "definition": '@"checks.matrix.Top"',
            "arguments": [],
        }
        checked_products.append(admitted)
        published = root / ("published-" + target)
        before = protected(published)
        cli(
            "emit",
            checked_final,
            "--target",
            target,
            "-o",
            published,
            "--replace",
            code=1,
            diagnostic="generated.json entry does not match its owner",
        )
        assert protected(published) == before
    (source / "matrix.py").write_text(baseline)

    # Native IR validation is a separate entrance from publication hashes.
    # Every mutation changes an actual emitted check, never a handwritten DUT.
    matrix_final = (root / "matrix.final.generic.ac").read_text()
    matrix_expect = next(
        line for line in matrix_final.splitlines() if '"ac.expect"' in line
    )
    pair = operands(matrix_expect)
    id_text = attribute(matrix_expect, "ac.check_id")
    requirements = attribute(matrix_final, "ac.required_checks", "[", "]")
    check_span = attribute(matrix_expect, "location")
    foreign = id_text.replace("@checks.matrix.Top", "@foreign.Other")
    redirected = matrix_expect.replace(", ".join(pair), ", ".join(reversed(pair)), 1)
    altered_span = re.sub(
        r"(?<![\w_])line = (\d+) : i64", "line = 999 : i64", check_span, count=1
    )
    mutants = {
        "deleted": matrix_final.replace(matrix_expect + "\n", "", 1),
        "duplicate": matrix_final.replace(
            matrix_expect, matrix_expect + "\n" + matrix_expect, 1
        ),
        "orphan": matrix_final.replace(
            "ac.required_checks = " + requirements, "ac.required_checks = []", 1
        ),
        "cross-owner": matrix_final.replace(
            matrix_expect, matrix_expect.replace(id_text, foreign, 1), 1
        ),
        "suffix-swap": matrix_final.replace(matrix_expect, redirected, 1),
        "wrong-kind": matrix_final.replace(
            matrix_expect,
            matrix_expect.replace('kind = "assert"', 'kind = "range"', 1),
            1,
        ),
        "wrong-span": matrix_final.replace(
            matrix_expect, matrix_expect.replace(check_span, altered_span, 1), 1
        ),
        "wrong-message": matrix_final.replace(
            matrix_expect,
            matrix_expect.replace('ac.message = "matrix"', "ac.message = 7 : i64", 1),
            1,
        ),
    }
    for name, text in mutants.items():
        assert text != matrix_final, name
        path = root / ("invalid-ir-" + name + ".ac")
        path.write_text(text)
        output = root / ("invalid-ir-" + name + ".out.ac")
        run([args.optimizer, path, "--ac-verify-hardware", "-o", output], code=1)
        assert not output.exists(), name
    range_final = root / "existing-range-kind.ac"
    range_final.write_text(matrix_final.replace('kind = "assert"', 'kind = "range"'))
    run(
        [
            args.optimizer,
            range_final,
            "--ac-verify-hardware",
            "-o",
            root / "range-verified.ac",
        ]
    )
    assert (root / "range-verified.ac").read_text().count('"ac.expect"') == 1

    # Inventories contain paths, not hashes. Semantic violations must still
    # reject without damaging either a fresh or an existing publication.
    body = payload(provider_unit, "body")
    original_body = body.read_bytes()
    body_lines = original_body.decode().splitlines(keepends=True)
    expect_index = next(i for i, line in enumerate(body_lines) if '"ac.expect"' in line)
    del body_lines[expect_index]
    body.write_text("".join(body_lines))
    assert (
        body.read_bytes() != original_body
        and b"ac.required_checks" in body.read_bytes()
    )
    bad_final = root / "tampered-body.ac"
    cli(
        "link",
        provider_unit,
        parent_unit,
        "--top",
        "checks.parent.Top",
        "-o",
        bad_final,
        code=1,
        diagnostic="RequiredCheck has no matching expect",
    )
    assert not bad_final.exists()
    cli(
        "link",
        provider_unit,
        parent_unit,
        "--top",
        "checks.parent.Top",
        "-o",
        final,
        "--replace",
        code=1,
        diagnostic="RequiredCheck has no matching expect",
    )
    assert protected(final) == before_final
    body.write_bytes(original_body)
    header = payload(provider_unit, "interface")
    original_header = header.read_bytes()
    output_names = 'output_names = ["value"]'
    assert output_names in original_header.decode()
    header.write_text(
        original_header.decode().replace(
            output_names, 'output_names = ["value", "missing"]', 1
        )
    )
    compile_unit(
        "parent.py",
        units / "tampered-parent",
        [provider_unit],
        code=1,
        diagnostic="port name counts must match function signature",
    )
    assert not (units / "tampered-parent").exists()
    parent_before = protected(parent_unit)
    compile_unit(
        "parent.py",
        parent_unit,
        [provider_unit],
        replace=True,
        code=1,
        diagnostic="port name counts must match function signature",
    )
    assert protected(parent_unit) == parent_before
    header.write_bytes(original_header)

    # Reuse the existing independent lifecycle caller against the public
    # emitter, rather than the test-only prepared backend entrance.
    lifecycle = root / "public-managed-lifecycle"
    lifecycle.mkdir()
    for target in ("cpp", "verilog"):
        cli("emit", root / "matrix.ac", "--target", target, "-o", lifecycle / target)
    cpp = lifecycle / "cpp"
    emitted = json.loads((cpp / "generated.json").read_text())
    sources = [
        cpp / row["path"] for row in emitted["files"] if row["path"].endswith(".cpp")
    ]
    runtime_root = Path(args.source_compiler).resolve().parent.parent
    runtime = next(
        path
        for path in (
            runtime_root / "simulator/gfsim/libpyc6_runtime.a",
            runtime_root / "lib/libpyc6_runtime.a",
        )
        if path.is_file()
    )
    native = lifecycle / "native"
    run(
        [
            args.cxx,
            "-std=c++20",
            "-pthread",
            "-DT3_CASE=0",
            "-I" + str(repo / "include"),
            "-I" + str(cpp),
            Path(__file__).with_name("source-check-execution.cpp"),
            *sources,
            runtime,
            "-o",
            native,
        ]
    )
    syntax = ast.parse((source / "matrix.py").read_text())
    nodes = list(ast_paths(syntax))
    check, check_path = next(
        (node, path) for node, path in nodes if isinstance(node, ast.Assert)
    )
    _, registration_path = next(
        (node, path)
        for node, path in nodes
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Name)
        and node.func.id == "inspect"
    )

    def source_occurrence(path):
        return {
            "site": {
                "definition": "checks.matrix.Top",
                "ast_path": [
                    (
                        {"kind": "index", "value": item}
                        if isinstance(item, int)
                        else {"kind": "field", "name": item}
                    )
                    for item in path
                ],
            },
            "expansion": [],
        }

    expected_id = {
        "registration": source_occurrence(registration_path),
        "check": source_occurrence(check_path),
        "obligation": 0,
    }
    expected_span = {
        "path": "matrix.py",
        "line": check.lineno,
        "column": check.col_offset + 1,
        "end_line": check.end_lineno,
        "end_column": check.end_col_offset + 1,
    }

    def decode(token):
        return "" if token == "-" else bytes.fromhex(token).decode()

    def rows(stdout):
        parsed = {}
        for line in stdout.splitlines():
            if not line.startswith("ROW "):
                continue
            parts = line.split()
            assert len(parts) == 12 and parts[1] not in parsed, line
            parsed[parts[1]] = {
                "status": int(parts[2]),
                "epoch": int(parts[3]),
                "phase": int(parts[4]),
                "code": decode(parts[5]),
                "message": decode(parts[6]),
                "instance": decode(parts[7]),
                "source": json.loads(decode(parts[8])) if parts[8] != "-" else None,
                "id": json.loads(decode(parts[9])) if parts[9] != "-" else None,
                "available": bool(int(parts[10])),
                "output": parts[11],
            }
        return parsed

    def success(row, output):
        assert row["status"] == 1 and row["epoch"] == 1 and row["phase"] == 0, row
        assert row["code"] == "" and row["available"] and row["output"] == output, row

    def failure(row, epoch=0):
        assert row["status"] == 3 and row["epoch"] == epoch and row["phase"] == 3, row
        assert row["code"] == "source_check_failed" and not row["available"], row
        assert row["message"] == check.msg.value and row["instance"] == "root", row
        assert row["id"] == expected_id and row["source"] == expected_span, row

    config = lifecycle / "runner-config.json"
    config.write_text(
        json.dumps(
            {
                "schema": "pycircuit-model-config",
                "version": "1",
                "max_ticks": 8,
                "max_domain_cycles": {},
                "deadlock_window": None,
            },
            sort_keys=True,
            separators=(",", ":"),
        )
        + "\n"
    )
    native_traces, runner_traces = [], []
    for workers in (1, 2):
        stdout = run([native, workers]).stdout
        (lifecycle / f"native-{workers}.stdout").write_text(stdout)
        native_traces.append(stdout)
        actual = rows(stdout)
        for path, condition in itertools.product("01xz", repeat=2):
            label = "matrix-" + path + condition
            if and_bit(path, inv(condition)) == "0":
                success(actual[label], condition)
            else:
                failure(actual[label])
                failure(actual[label + "-retry"])
                success(actual[label + "-reset"], "1")
        events = lifecycle / f"runner-{workers}.jsonl"
        stdout = run([native, workers, config, events]).stdout
        (lifecycle / f"runner-{workers}.stdout").write_text(stdout)
        runner_traces.append(stdout)
        actual = rows(stdout)
        failure(actual["runner-failure"], 1)
        failure(actual["runner-retry"], 1)
        assert "RUNNER initialized=1 drives=2 samples=1 bounded=8" in stdout
        records = [json.loads(line) for line in events.read_text().splitlines()]
        assert len(records) == 1
        result = records[0]
        assert (
            result["kind"] == "result"
            and result["status"] == "FAILED"
            and result["epoch_time"] == "1"
        ), result
        assert result["error"] == {
            "phase": "check",
            "code": "source_check_failed",
            "message": check.msg.value,
            "instance": "root",
            "source": expected_span,
            "check_id": expected_id,
        }, result
    assert native_traces[0] == native_traces[1]
    assert runner_traces[0] == runner_traces[1]
    rtl = lifecycle / "verilog"
    emitted = json.loads((rtl / "generated.json").read_text())
    sources = [rtl / row["path"] for row in emitted["files"] if row["role"] == "rtl"]
    rtl_build = lifecycle / "verilated"
    run(
        [
            args.verilator,
            "--binary",
            "--timing",
            "--top-module",
            "tb",
            "--Mdir",
            rtl_build,
            "-j",
            "2",
            "-Wno-fatal",
            "-CFLAGS",
            "-std=c++20",
            *sources,
            Path(__file__).with_name("source-checks-managed.sv"),
        ]
    )
    stdout = run([rtl_build / "Vtb"]).stdout
    (lifecycle / "rtl.stdout").write_text(stdout)
    assert [line for line in stdout.splitlines() if line.startswith("MATRIX ")] == [
        "MATRIX 00 0",
        "MATRIX 01 0",
        "MATRIX 10 1",
        "MATRIX 11 0",
    ]
    assert "MANAGED_CHECKED_MODULE_OK" in stdout.splitlines()

    results_record = {
        "scope": "source capture/SSA/source-unit and public checked ordinary-module lifecycle",
        "artifact_directory": str(root),
        "fixtures": {
            p.name: hashlib.sha256(p.read_bytes()).hexdigest()
            for p in fixtures.glob("*.py")
        },
        "products": receipts,
        "continuation_frames": 256,
        "matrix_frames": 16,
        "source_negatives": list(negatives),
        "ir_negatives": list(mutants),
        "existing_range_binding_verified": True,
        "explicit_provider_closure": True,
        "backend_checked_admission": ["cpp", "verilog"],
        "checked_products": [str(path) for path in checked_products],
        "native_public_matrix_frames": 16,
        "native_public_workers": [1, 2],
        "managed_lifecycle_directory": str(lifecycle),
        "rtl_public_known_matrix_frames": 4,
        "observation_fresh_admission": ["cpp"],
        "observation_emission_rejected": ["verilog"],
        "cross_owner_replacement_rejected": ["cpp", "verilog"],
    }
    (evidence / "results.json").write_text(json.dumps(results_record, indent=2) + "\n")
    print(
        "PASS: source-check IDs/SSA/-O/provider closure/protected rejections; public checked native lifecycle workers1/2 and known RTL matrix"
    )  # noqa: T201 - standalone gate PASS receipt


if __name__ == "__main__":
    main()
