"""Static reference admission and generated cross-hierarchy System acceptance."""

import argparse
import hashlib
import importlib.util
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path


parser = argparse.ArgumentParser()
for option in ("repo", "source-compiler", "linker", "emitter", "scratch"):
    parser.add_argument("--" + option, required=True)
for option in ("cxx", "iverilog", "vvp"):
    parser.add_argument("--" + option)
parser.add_argument("--optimizer")
parser.add_argument("--phase", choices=("source", "link", "all"), default="all")
parser.add_argument("--case", action="append", help="Run only named cases; repeat for several")
args = parser.parse_args()
repo = Path(args.repo).resolve()
retained = Path(args.scratch).resolve()
retained.mkdir(parents=True, exist_ok=True)
scratch = Path(tempfile.mkdtemp(prefix="run-", dir=retained))
(retained / "latest-run.txt").write_text(str(scratch) + "\n")
fixture_dir = Path(__file__).resolve().parent / "static-routing"
spec = importlib.util.spec_from_file_location("static_routing_cases", fixture_dir / "cases.py")
cases = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cases)
execution_spec = importlib.util.spec_from_file_location("static_routing_execution", fixture_dir / "execution.py")
execution = importlib.util.module_from_spec(execution_spec)
execution_spec.loader.exec_module(execution)
env = dict(os.environ, PYTHONPATH=str(repo / "python"), PYTHONDONTWRITEBYTECODE="1",
           PYCIRCUIT_SOURCE_COMPILER=str(Path(args.source_compiler).resolve()),
           PYCIRCUIT_LINKER=str(Path(args.linker).resolve()),
           PYCIRCUIT_EMITTER=str(Path(args.emitter).resolve()))
commands, outcomes = [], []


def run(command, expected=0):
    command = list(map(str, command))
    result = subprocess.run(command, cwd=repo, env=env, capture_output=True,
                            text=True, timeout=240)
    commands.append({"command": command, "exit_status": result.returncode,
                     "expected_exit": expected, "stdout": result.stdout,
                     "stderr": result.stderr})
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == expected, commands[-1]
    assert "Assertion failed" not in result.stderr and "Traceback" not in result.stderr, commands[-1]
    if expected:
        assert result.stderr.strip() or result.stdout.strip(), commands[-1]
    return result


def cli(*arguments, expected=0):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], expected)


def tree(directory):
    return {p.relative_to(directory).as_posix(): p.read_bytes()
            for p in directory.rglob("*") if p.is_file()}


def payload(unit, kind):
    return unit / json.loads((unit / "unit.json").read_text())["files"][kind]


def delimited(text, start):
    """Read one printed attribute container, respecting nested quoted strings."""
    pairs = {"{": "}", "[": "]", "(": ")"}
    assert text[start] in pairs
    stack, quoted, escaped = [], False, False
    for index in range(start, len(text)):
        char = text[index]
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
        elif char in pairs:
            stack.append(pairs[char])
        elif char in pairs.values():
            assert stack and stack.pop() == char, "unbalanced printed attribute"
            if not stack:
                return text[start:index + 1], index + 1
    raise AssertionError("unterminated printed attribute")


def attribute_items(text):
    """Split only top-level commas in the closed metadata containers used here."""
    assert text[0] in "{[" and text[-1] in "}]"
    items, start, index = [], 1, 1
    while index < len(text) - 1:
        if text[index] in "{[(":
            _, index = delimited(text, index)
        elif text[index] == '"':
            match = re.match(r'"(?:[^"\\]|\\.)*"', text[index:])
            assert match, "unterminated printed string"
            index += len(match[0])
        elif text[index] == ",":
            items.append(text[start:index].strip())
            start = index = index + 1
        else:
            index += 1
    if text[start:-1].strip():
        items.append(text[start:-1].strip())
    return items


def attribute_fields(text):
    assert text.startswith("{")
    pairs = [item.split(" = ", 1) for item in attribute_items(text)]
    assert all(len(pair) == 2 for pair in pairs)
    result = dict(pairs)
    assert len(result) == len(pairs), "duplicate metadata key"
    return result


def integer_attribute(text):
    assert re.fullmatch(r"[0-9]+ : i64", text), text
    return int(text.split()[0])


def indexed_requirement_mutation(text):
    """Find the actual indexed use through its sampled control/yield witness.

    No SSA ordinal, source line, definition name or retained management output
    selects the target. The fixture must contain exactly one state_write operand
    2 requirement whose payload is a captured (nonconstant) selector.
    """
    lines = text.splitlines(keepends=True)
    candidates = []
    operation = re.compile(r'^(\s*)(%[\w.$-]+)(?::\d+)? = "([\w.]+)"\(([^)]*)\)')
    for start, opening in enumerate(lines):
        if not (match := operation.match(opening)) or match[3] != "ac.rule":
            continue
        indent = match[1]
        ends = [i for i in range(start + 1, len(lines)) if lines[i].startswith(indent + "})")]
        assert ends, "Rule region has no matching printed close"
        end = ends[0]
        marker = "ac.sampled_admission = "
        if marker not in lines[end]:
            continue
        offset = lines[end].index(marker) + len(marker)
        metadata = attribute_fields(delimited(lines[end], offset)[0])
        requirements = [attribute_fields(item) for item in attribute_items(metadata.get("requirements", "[]"))]
        controls = [attribute_fields(item) for item in attribute_items(metadata["controls"])]
        observations = [attribute_fields(item) for item in attribute_items(metadata["observations"])]
        check_base = integer_attribute(metadata["fire_result"]) + 1 + sum(
            1 + integer_attribute(item["value_count"]) for item in observations)
        block_headers = [line for line in lines[start + 1:end] if line.startswith(indent + "^bb0(")]
        assert len(block_headers) == 1, "ambiguous Rule entry block"
        arguments = set(re.findall(r"(%[\w.$-]+): ", block_headers[0]))
        definitions = {}
        for index in range(start + 1, end):
            found = operation.match(lines[index])
            if found and found[1] == indent + "  ":
                assert found[2] not in definitions
                definitions[found[2]] = (index, found)
        yields = [re.search(r'"ac.yield"\(([^)]*)\)', line) for line in lines[start + 1:end]
                  if line.startswith(indent + '  "ac.yield"(')]
        assert len(yields) == 1
        yielded = [item.strip() for item in yields[0][1].split(",")]
        for row in requirements:
            if row["kind"] != '"state_write"' or integer_attribute(row["operand"]) != 2:
                continue
            control = integer_attribute(row["control"])
            assert controls[control]["kind"] == '"value_available"'
            result = yielded[check_base + 2 * control]
            assert result in definitions, "required-use witness is not a direct Rule operation"
            index, found = definitions[result]
            assert found[3] == "ac.value.require"
            operands = [item.strip() for item in found[4].split(",")]
            assert len(operands) == 3
            attributes = attribute_fields(delimited(lines[index], lines[index].index("<{") + 1)[0])
            assert attributes["occurrence"] == row["occurrence"]
            assert attributes["source_location"] == row["location"]
            if operands[0] not in arguments:
                continue  # The earlier constant entries[0] index is a separate use.
            result_type = lines[index].rsplit(" -> ", 1)[1].split(" loc(", 1)[0]
            zeros = []
            for reference, (constant_index, constant) in definitions.items():
                if constant_index >= index or constant[3] != "ac.bits.constant":
                    continue
                constant_type = lines[constant_index].rsplit(" -> ", 1)[1].split(" loc(", 1)[0]
                if constant_type != result_type:
                    continue
                properties = attribute_fields(delimited(lines[constant_index], lines[constant_index].index("<{") + 1)[0])
                expression = properties["value"]
                if not expression.startswith("#ac.static_expr<{"):
                    continue
                literal = attribute_fields(delimited(expression, expression.index("{"))[0])
                if literal.get("kind") != '"literal"':
                    continue
                value = attribute_fields(literal["value"])
                if value == {"kind": '"integer"', "value": "#ac.math_int<0>"}:
                    zeros.append((constant_index, reference))
            assert zeros, "no dominating exact-type literal zero for path mutation"
            zero_line, zero = min(zeros)  # All candidates have identical literal meaning.
            assert operands[2] != zero
            candidates.append((index, found, zero_line, zero, control))
    assert len(candidates) == 1, ("ambiguous indexed state_write mutation", len(candidates))
    index, found, zero_line, zero, control = candidates[0]
    operands = list(re.finditer(r"%[\w.$#-]+", found[4]))
    assert len(operands) == 3
    path = operands[2]
    begin, end = found.start(4) + path.start(), found.start(4) + path.end()
    original = lines[index]
    lines[index] = original[:begin] + zero + original[end:]
    mutated = "".join(lines)
    assert sum(a != b for a, b in zip(text.splitlines(), mutated.splitlines(), strict=True)) == 1
    return mutated, {"line": index + 1, "zero_line": zero_line + 1,
                     "control": control, "kind": "state_write", "operand": 2,
                     "before": original.split(" <{", 1)[0].strip(),
                     "after": lines[index].split(" <{", 1)[0].strip(),
                     "only_path_operand_changed": True}


def compile_source(source, path, output, imports=(), expected=0, replace=False):
    arguments = ["compile", "-c", path, "--source-root", source,
                 "--package-prefix", "static_routes", "-o", output]
    for unit in imports:
        arguments.extend(("-I", unit))
    if replace:
        arguments.append("--replace")
    return cli(*arguments, expected=expected)


def check(name, operation):
    if args.case and name not in args.case:
        return
    start = len(commands)
    try:
        evidence = operation()
        outcomes.append({"case": name, "status": "PASS", "evidence": evidence,
                         "commands": [start, len(commands)]})
    except (AssertionError, subprocess.TimeoutExpired) as failure:
        outcomes.append({"case": name, "status": "FAIL", "diagnostic": str(failure),
                         "commands": [start, len(commands)]})
    (scratch / "results.json").write_text(json.dumps(outcomes, indent=2) + "\n")
    print(name + ": " + outcomes[-1]["status"], flush=True)


def publish_case(name, sources):
    case = scratch / name
    case.mkdir()
    source = case / "source"
    source.mkdir()
    units, hidden = [], []
    for filename, text in sources:
        path = source / filename
        path.write_text(text)
        output = case / Path(filename).stem
        compile_source(source, path, output, units)
        if units:
            dependencies = payload(output, "depfile").read_text()
            for provider in units:
                assert str(payload(provider, "body")) not in dependencies
            assert any(str(payload(provider, "interface")) in dependencies for provider in units)
        units.append(output)
        # Later consumers have only authenticated headers/receipts. Neither
        # provider source nor body can accidentally supply their semantics.
        if len(units) < len(sources):
            unavailable = case / (Path(filename).stem + "-unavailable")
            unavailable.mkdir()
            for item in (path, payload(output, "body"), payload(output, "depfile")):
                moved = unavailable / item.name
                item.rename(moved)
                hidden.append((item, moved))
    for original, moved in hidden:
        moved.rename(original)
    return case, units


def positive(name, sources, top):
    case, units = publish_case(name, sources)
    result = {"source": "PASS", "link": "NOT RUN", "execution": "NOT RUN",
              "interface_only_consumers": len(units) - 1}
    if args.phase == "source":
        return result
    before = [tree(unit) for unit in units]
    final = case / "final.ac"
    cli("link", *units, "--top", "static_routes." + top, "-o", final)
    assert [tree(unit) for unit in units] == before
    result["link"] = "PASS"
    result["final_sha256"] = hashlib.sha256(final.read_bytes()).hexdigest()
    if args.phase == "all":
        result["execution"] = execute(case, final, name)
    return result


def rejected_source(name, text, diagnostic):
    case, units = publish_case(name, [("design.py", cases.IMPORT + cases.PEER + cases.TOP)])
    unit = units[0]
    original = tree(unit)
    source = case / "source"
    path = source / "design.py"
    path.write_text(text)
    for replace in (False, True):
        output = unit if replace else case / "absent"
        rejected = compile_source(source, path, output, expected=1, replace=replace)
        assert diagnostic.lower() in rejected.stderr.lower(), (name, diagnostic, rejected.stderr)
        assert tree(unit) == original, name
        assert not (case / "absent").exists(), name
    return {"source_rejected": True, "diagnostic": diagnostic,
            "fresh_and_replacement_publications_preserved": True}


def rejected_link(name, text):
    case, units = publish_case(name, [("design.py", text)])
    before = [tree(unit) for unit in units]
    output = case / "rejected.ac"
    cli("link", *units, "--top", "static_routes.design.Top", "-o", output, expected=1)
    assert not output.exists() and [tree(unit) for unit in units] == before
    return {"source": "PASS", "link_rejected": True, "publications_preserved": True}


def rejected_indexed_requirement_path():
    name = "indexed-demand-path-mutation"
    source = (fixture_dir / "indexed_demand.py").read_text()
    case, units = publish_case(name, [("design.py", source)])
    final = case / "final.ac"
    cli("link", *units, "--top", "static_routes.design.Exercise", "-o", final)
    optimizer = (Path(args.optimizer).resolve() if args.optimizer else
                 Path(args.source_compiler).resolve().with_name("pycircuit-opt"))
    # This must be a real verified hardware artifact, never split anchor
    # fragments, a wrong stage, or a package rejected only for stale hashes.
    run([optimizer, final, "--ac-verify-hardware", "-o", case / "baseline-verified.ac"])
    original = final.read_bytes()
    original_units = [tree(unit) for unit in units]
    protected = {}
    for target in ("cpp", "verilog"):
        output = case / ("protected-" + target)
        cli("emit", final, "--target", target, "-o", output)
        protected[target] = tree(output)
    mutated, description = indexed_requirement_mutation(original.decode())
    mutant = case / "mutated-path-zero.ac"
    mutant.write_text(mutated)
    description.update(baseline_sha256=hashlib.sha256(original).hexdigest(),
                       mutant_sha256=hashlib.sha256(mutant.read_bytes()).hexdigest())
    (case / "mutation.json").write_text(json.dumps(description, indent=2) + "\n")
    checks = []

    def rejection(label, command, protection):
        command_start = len(commands)
        try:
            result = run(command, expected=1)
            assert result.stderr.strip(), "rejection requires a semantic diagnostic"
            assert not any(token in result.stderr for token in (
                "error: expected", "use of undeclared SSA value", "unsupported anchor schema",
                "digest disagrees", "requires final stage", "requires routed stage")), result.stderr
            assert protection(), "rejection changed or published output"
            checks.append({"entrance": label, "status": "PASS", "stderr": result.stderr})
        except AssertionError as failure:
            checks.append({"entrance": label, "status": "FAIL", "diagnostic": str(failure),
                           "output_protected": protection()})
        checks[-1]["commands"] = [command_start, len(commands)]
        assert final.read_bytes() == original
        assert [tree(unit) for unit in units] == original_units
        (case / "mutation-checks.json").write_text(json.dumps(checks, indent=2) + "\n")

    verified = case / "mutant-verified.ac"
    rejection("standalone-hardware-verifier",
              [optimizer, mutant, "--ac-verify-hardware", "-o", verified],
              lambda: not verified.exists())
    for target in ("cpp", "verilog"):
        absent = case / ("absent-" + target)
        output = case / ("protected-" + target)
        base = [sys.executable, "-m", "pycircuit.cli", "emit", mutant, "--target", target]
        rejection(target + "-fresh", [*base, "-o", absent],
                  lambda absent=absent, output=output, target=target:
                  not absent.exists() and tree(output) == protected[target])
        rejection(target + "-replace", [*base, "-o", output, "--replace"],
                  lambda output=output, target=target: tree(output) == protected[target])
    failed = [row["entrance"] for row in checks if row["status"] != "PASS"]
    assert not failed, {"accepted_unsafe_mutation_or_changed_output": failed,
                        "evidence": str(case / "mutation-checks.json")}
    return {"baseline_standalone_verified": True, "mutation": description,
            "checks": checks, "source_and_original_final_preserved": True}


def execute(case, final, name):
    return execution.execute(args, repo, fixture_dir, case, final, name, cli, run, tree)


for name, sources, top in cases.source_cases():
    check(name, lambda name=name, sources=sources, top=top: positive(name, sources, top))
for name, source, diagnostic in cases.rejection_cases():
    check(name, lambda name=name, source=source, diagnostic=diagnostic:
          rejected_source(name, source, diagnostic))
if args.phase != "source":
    for name, source in cases.link_rejection_cases():
        # Retired relative receivers reject during capture, even in unused
        # definitions. Preserve publication protection at the current stage.
        if name in {"ancestor-escape", "unused-contextual-definition",
                    "unused-parent-instantiation", "unused-zero-depth-transitive-context"}:
            check(name, lambda name=name, source=source: rejected_source(
                name, source,
                "Rule receiver must be a direct child or declared Module-reference interface"))
        else:
            check(name, lambda name=name, source=source: rejected_link(name, source))
    check("indexed-demand-path-mutation", rejected_indexed_requirement_path)
failed = [row["case"] for row in outcomes if row["status"] != "PASS"]
assert outcomes, "case selection ran no checks"
assert not args.case or set(args.case) <= {row["case"] for row in outcomes}, "unknown case selection"
print(json.dumps({"phase": args.phase, "cases": len(outcomes), "failures": failed,
                  "evidence": str(scratch)}, indent=2))
raise SystemExit(bool(failed))
