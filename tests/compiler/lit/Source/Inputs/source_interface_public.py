"""Actual registered extraction versus public authority and protected publication."""

import argparse
import ast
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
for name in ("repo", "source-compiler", "linker", "emitter", "optimizer", "scratch"):
    parser.add_argument("--" + name, required=True)
parser.add_argument("--invalid", action="store_true")
parser.add_argument("--query-checks", choices=("syntax", "admission", "resources"))
parser.add_argument("--method-checks", choices=("publication", "admission"))
parser.add_argument("--helper-checks", action="store_true")
args = parser.parse_args()
repo = Path(args.repo).resolve()
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
private = tempfile.TemporaryDirectory(prefix="interface-", dir=scratch)
root = Path(private.name)
source = root / "source"
source.mkdir()
fixtures = Path(__file__).resolve().parent
for name in ("source_interface_child.py", "source_interface_parent.py"):
    shutil.copyfile(fixtures / name, source / name)
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
    PYCIRCUIT_EMITTER=args.emitter,
)
commands = []


def run(command, accepted=True):
    result = subprocess.run(
        list(map(str, command)),
        cwd=repo,
        env=env,
        capture_output=True,
        text=True,
        timeout=30,
    )
    row = {
        "command": list(map(str, command)),
        "exit_status": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
    }
    commands.append(row)
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert (
        "Assertion failed" not in result.stderr and "Traceback" not in result.stderr
    ), row
    assert result.returncode == (0 if accepted else 1), row
    if not accepted:
        assert result.stderr.strip(), row
    return result


def cli(*arguments, accepted=True):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], accepted)


def compile_unit(name, output, imports=(), replace=False, accepted=True):
    command = [
        "compile",
        "-c",
        source / name,
        "--source-root",
        source,
        "--package-prefix",
        "interfaces",
        "-o",
        output,
    ]
    for unit in imports:
        command.extend(("-I", unit))
    if replace:
        command.append("--replace")
    return cli(*command, accepted=accepted)


def payload(unit, kind):
    receipt = json.loads((unit / "unit.json").read_text())
    return unit / receipt["files"][kind]


def snapshot(directory):
    return {
        path.relative_to(directory).as_posix(): path.read_bytes()
        for path in directory.rglob("*")
        if path.is_file()
    }


def normalize(path, output):
    run([args.optimizer, path, "--mlir-print-debuginfo", "-o", output])
    return output.read_text()


def extract(path, output, accepted=True):
    return run(
        [
            args.optimizer,
            path,
            "--ac-extract-source-interface",
            "--mlir-print-debuginfo",
            "-o",
            output,
        ],
        accepted,
    )


child, parent = root / "child", root / "parent"
compile_unit("source_interface_child.py", child)
compile_unit("source_interface_parent.py", parent, [child])
for unit in (child, parent):
    extracted = root / (unit.name + "-extracted.ac")
    extract(payload(unit, "body"), extracted)
    actual = normalize(extracted, root / (unit.name + "-actual.ac"))
    expected = normalize(
        payload(unit, "interface"), root / (unit.name + "-expected.ac")
    )
    assert actual == expected, (actual, expected)
    assert '"ac.module"' not in actual and '"ac.system"' not in actual
    assert 'ac.unit_kind = "interface"' in actual

if args.helper_checks:
    provider_text = """import pycircuit as ac
@ac.struct
class Pair:
    left: ac.u8
    right: ac.u8
def bump(value: ac.u8) -> ac.u8:
    result: ac.u8 = value + 1
    return result
def chain(value: ac.u8) -> ac.u8:
    saved = bump(value)
    return bump(saved)
def swap(value: Pair) -> Pair:
    rebuilt = Pair(left=value.right, right=value.left)
    return rebuilt
def consume(value: ac.u8) -> None:
    temporary: ac.u8 = value + 1
    return
"""
    consumer_text = """import pycircuit as ac
from interfaces.helper_provider import bump, chain, swap, consume, Pair
@ac.module
def Consumer(left: ac.u8, right: ac.u8) -> Pair:
    snapshot = left + 1
    def captured(value: ac.u8) -> ac.u8:
        return value ^ snapshot
    snapshot = right + 1
    consume(left)
    return swap(Pair(left=chain(left), right=captured(right)))
"""
    stages = root / "helper-publication"
    stages.mkdir()
    retained = scratch / "helper-publication-artifacts"
    retained.mkdir(exist_ok=True)
    snapshots = retained / "sources"
    snapshots.mkdir(exist_ok=True)
    provider_source = source / "helper_provider.py"
    consumer_source = source / "helper_consumer.py"
    for path, text in (
        (provider_source, provider_text),
        (consumer_source, consumer_text),
    ):
        path.write_text(text)
        (snapshots / path.name).write_text(text)
    provider_unit, consumer_unit = stages / "provider", stages / "consumer"
    compile_unit(provider_source.name, provider_unit)
    provider_body = payload(provider_unit, "body").read_text()
    assert '"ac.module"(' not in provider_body and "func.func" in provider_body
    provider_source.unlink()
    compile_unit(consumer_source.name, consumer_unit, [provider_unit])
    assert not provider_source.exists()
    for unit in (provider_unit, consumer_unit):
        extracted = stages / (unit.name + "-extracted.ac")
        extract(payload(unit, "body"), extracted)
        actual = normalize(extracted, stages / (unit.name + "-actual.ac"))
        expected = normalize(
            payload(unit, "interface"), stages / (unit.name + "-expected.ac")
        )
        assert actual == expected, unit
        assert 'ac.helper_kind = "value"' in expected
        assert "ac.capture_types =" in expected
    final = stages / "final.ac"
    cli(
        "link",
        provider_unit,
        consumer_unit,
        "--top",
        "interfaces.helper_consumer.Consumer",
        "-o",
        final,
    )
    final_text = final.read_text()
    assert "func.func" not in final_text and "func.call" not in final_text, final_text
    emitted = {}
    for target in ("cpp", "verilog"):
        output = stages / target
        cli("emit", final, "--target", target, "-o", output)
        emitted[target] = snapshot(output)
    protected_unit, protected_final = snapshot(consumer_unit), final.read_bytes()
    negatives = {
        "missing-provider": (consumer_text, []),
        "wrong-helper-kind": (
            consumer_text.replace("chain(left)", "chain(True)"),
            [provider_unit],
        ),
        "wrong-nominal": (
            consumer_text.replace(
                "swap(Pair(left=chain(left), right=captured(right)))", "swap(left)"
            ),
            [provider_unit],
        ),
        "none-as-value": (
            consumer_text.replace("chain(left)", "consume(left)"),
            [provider_unit],
        ),
    }
    for name, (text, imports) in negatives.items():
        consumer_source.write_text(text)
        absent = stages / ("invalid-" + name)
        compile_unit(consumer_source.name, absent, imports, accepted=False)
        assert not absent.exists(), name
        compile_unit(
            consumer_source.name, consumer_unit, imports, replace=True, accepted=False
        )
        assert snapshot(consumer_unit) == protected_unit, name
        assert final.read_bytes() == protected_final, name
        assert all(
            snapshot(stages / target) == expected
            for target, expected in emitted.items()
        ), name
    consumer_source.write_text(consumer_text)
    destination = retained / "stages"
    if destination.exists():
        shutil.rmtree(destination)
    shutil.copytree(stages, destination)
    (scratch / "helper-publication-receipt.json").write_text(
        json.dumps(
            {
                "scope": "ordinary helper-only provider, unavailable provider Python, consumer source publication/link/dual emit",
                "provider_python_unavailable": not provider_source.exists(),
                "artifact_sha256": {
                    path.relative_to(retained)
                    .as_posix(): hashlib.sha256(path.read_bytes())
                    .hexdigest()
                    for path in sorted(retained.rglob("*"))
                    if path.is_file()
                },
                "tools": {
                    str(Path(tool).resolve()): hashlib.sha256(
                        Path(tool).read_bytes()
                    ).hexdigest()
                    for tool in (
                        args.source_compiler,
                        args.linker,
                        args.emitter,
                        args.optimizer,
                    )
                },
                "managed_publications_preserved": True,
            },
            indent=2,
        )
        + "\n"
    )
    print("Ordinary helper publication checks passed")
    sys.exit(0)

if args.method_checks:
    provider_text = """import pycircuit as ac
from typing import Annotated
@ac.struct
class Packet:
    value: ac.u8
@ac.module
def Counter():
    count: ac.u8 = 0
    @ac.method
    def read() -> ac.u8:
        return count
    @ac.method
    def add(delta: ac.u8) -> ac.u8:
        before = count
        assert before == count
        ac.log("info", "add", "before", before, delta)
        assert before == count, ""
        ac.log("debug", "literal", "fixed")
        count = count + delta
        ac.report("count", count)
        return count
    @ac.method
    def chain(delta: ac.u8) -> ac.u8:
        saved = add(delta)
        return add(saved)
    @ac.method
    def boolean(value: bool) -> bool:
        return value
    @ac.method
    def integer(value: Annotated[int, range(1 << 1)]) -> Annotated[int, range(1 << 1)]:
        return value
    @ac.method
    def fixed(value: ac.u1) -> ac.u1:
        return value
    @ac.method
    def packet(value: Packet) -> Packet:
        return value
    @ac.method(guard=lambda allow: allow)
    def guarded(allow: bool) -> None:
        count = count
    @ac.rule
    def inspect() -> None:
        saved = count
"""
    relay_text = """import pycircuit as ac
from typing import Annotated
from interfaces.method_provider import Counter, Packet
@ac.module
def Relay():
    left = Counter()
    alias = left
    right = Counter()
    @ac.method
    def chain(delta: ac.u8) -> ac.u8:
        before = alias.read()
        saved = left.chain(delta)
        return right.add(saved)
    @ac.method
    def boolean(value: bool) -> bool:
        return left.boolean(value)
    @ac.method
    def integer(value: Annotated[int, range(1 << 1)]) -> Annotated[int, range(1 << 1)]:
        return left.integer(value)
    @ac.method
    def fixed(value: ac.u1) -> ac.u1:
        return left.fixed(value)
    @ac.method
    def packet(value: Packet) -> Packet:
        return left.packet(value)
    @ac.method
    def guarded(allow: bool) -> None:
        alias.guarded(allow)
"""
    consumer_text = """import pycircuit as ac
from typing import Annotated
from interfaces.method_provider import Counter, Packet
from interfaces.method_relay import Relay
@ac.struct
class Result:
    before: ac.u8
    after: ac.u8
    fixed: ac.u1
    packet: Packet
@ac.module
def Consumer(delta: ac.u8, permitted: bool, logical: Annotated[int, range(1 << 1)], fixed: ac.u1, payload: Packet) -> Result:
    owner = Relay()
    other = Counter()
    alias = owner
    @ac.rule
    def work() -> Result:
        owner.guarded(True)
        before = alias.chain(delta)
        after = owner.chain(before)
        unused = other.add(delta)
        flag = owner.boolean(permitted)
        number = owner.integer(logical)
        bit = owner.fixed(fixed)
        packet = owner.packet(payload)
        return Result(before=before, after=after, fixed=bit, packet=packet)
    return work
"""
    state_plan_text = """import pycircuit as ac
@ac.module
def StatePlan():
    count: ac.u8 = 0
    @ac.method
    def instrument(delta: ac.u8) -> ac.u8:
        before = count
        assert before == count
        ac.report("before", before)
        count = count + delta
        ac.log("info", "after", count, before, count)
        return count
    @ac.rule
    def run() -> None:
        unused = instrument(1)
"""
    execution_text = """import pycircuit as ac
from interfaces.method_provider import Counter
@ac.system
def MethodEffectsSystem():
    owner = Counter()
    @ac.rule
    def drive() -> None:
        unused = owner.add(1)
"""
    archive = root / "method-publication"
    archive.mkdir()
    retained = scratch / ("method-" + args.method_checks + "-artifacts")
    retained.mkdir(exist_ok=True)
    source_snapshots = retained / "sources"
    source_snapshots.mkdir(exist_ok=True)
    provider_source = source / "method_provider.py"
    relay_source = source / "method_relay.py"
    consumer_source = source / "method_consumer.py"
    state_plan_source = source / "method_state_plan.py"
    execution_source = source / "method_execution.py"
    for path, text in (
        (provider_source, provider_text),
        (relay_source, relay_text),
        (consumer_source, consumer_text),
        (state_plan_source, state_plan_text),
        (execution_source, execution_text),
    ):
        path.write_text(text)
        # This evidence copy is outside the declared --source-root. It cannot
        # satisfy a provider Python read after the active source is removed.
        (source_snapshots / path.name).write_text(text)
    for name in ("source_interface_child.py", "source_interface_parent.py"):
        shutil.copyfile(source / name, source_snapshots / name)
    provider_unit, relay_unit, consumer_unit = (
        archive / "provider",
        archive / "relay",
        archive / "consumer",
    )
    state_plan_unit = archive / "state-plan"
    execution_unit = archive / "execution"
    compile_unit(provider_source.name, provider_unit)
    provider_source.unlink()
    compile_unit(execution_source.name, execution_unit, [provider_unit])
    compile_unit(relay_source.name, relay_unit, [provider_unit])
    relay_source.unlink()
    compile_unit(consumer_source.name, consumer_unit, [provider_unit, relay_unit])
    compile_unit(state_plan_source.name, state_plan_unit)
    state_plan_source.unlink()
    assert not provider_source.exists() and not relay_source.exists()
    state_plan_before = archive / "state-plan-before.ac"
    state_plan_after = archive / "state-plan-after.ac"
    run([args.optimizer, payload(state_plan_unit, "body"), "-o", state_plan_before])
    run(
        [
            args.optimizer,
            payload(state_plan_unit, "body"),
            "--ac-analyze-transaction-state-ssa",
            "-o",
            state_plan_after,
        ]
    )
    assert state_plan_before.read_bytes() == state_plan_after.read_bytes()
    state_plan_body = state_plan_after.read_text()
    assert '"ac.expect"' in state_plan_body
    assert state_plan_body.count('"ac.observe"') == 2
    for unit in (provider_unit, relay_unit, consumer_unit, execution_unit):
        extracted = archive / (unit.name + "-extracted.ac")
        extract(payload(unit, "body"), extracted)
        actual = normalize(extracted, archive / (unit.name + "-actual.ac"))
        expected = normalize(
            payload(unit, "interface"), archive / (unit.name + "-expected.ac")
        )
        assert actual == expected, unit
        assert 'semantic_kind = "transactions"' in expected, expected
        assert "transaction_summary =" in expected, expected
        assert '"ac.method"(' not in expected and '"ac.transaction"(' not in expected
    provider_header = payload(provider_unit, "interface").read_text()
    for kind in ("boolean", "integer", "fixed_bits", "nominal"):
        assert f'source_kind = "{kind}"' in provider_header, provider_header
    assert provider_header.count('kind = "check"') == 2, provider_header
    assert provider_header.count('kind = "observe"') == 3, provider_header
    assert 'check_kind = "assert"' in provider_header, provider_header
    assert 'observation_kind = "log"' in provider_header, provider_header
    assert 'observation_kind = "report"' in provider_header, provider_header
    assert "ac.check_id" not in provider_header, provider_header
    assert "ac.observation_id" not in provider_header, provider_header
    provider_body = payload(provider_unit, "body").read_text()
    assert provider_body.count('"ac.expect"') == 2, provider_body
    assert provider_body.count('"ac.observe"') == 3, provider_body
    assert "ac.check_id" not in provider_body, provider_body
    assert "ac.observation_id" not in provider_body, provider_body
    consumer_body = payload(consumer_unit, "body").read_text()
    relay_body = payload(relay_unit, "body").read_text()
    assert consumer_body.count('"ac.instance"(') == 2, consumer_body
    assert relay_body.count('"ac.instance"(') == 2, relay_body
    assert '"ac.method.call"(' in consumer_body and '"ac.transaction"(' in consumer_body

    # Retain a separately linked ordinary program as the publication control.
    control_final = archive / "control.ac"
    cli(
        "link",
        child,
        parent,
        "--top",
        "interfaces.source_interface_parent.Parent",
        "-o",
        control_final,
    )
    control_before = control_final.read_bytes()
    transaction_input = archive / "method-transaction-input.ac"
    transaction_output = archive / "method-transaction-output.ac"
    final = archive / "method-final.ac"
    # Materialize the bounded cross-source Method System. The larger
    # relay/consumer graph above remains publication and admission coverage;
    # its deliberately broad expansion is outside the fixed native work limit.
    command = [args.linker]
    for unit in (provider_unit, execution_unit):
        command.extend(("--body", payload(unit, "body")))
        command.extend(("--header", payload(unit, "interface")))
    command.extend(
        (
            "--top",
            "interfaces.method_execution.MethodEffectsSystem",
            "--target",
            "final",
            "--output",
            final,
            "--transaction-input-out",
            transaction_input,
            "--transaction-output-out",
            transaction_output,
        )
    )
    run(command)
    assert control_final.read_bytes() == control_before
    assert transaction_input.is_file() and transaction_output.is_file()
    before_final = final.read_bytes()
    assert b'"ac.method.call"' in transaction_input.read_bytes()
    assert b"ac.sampled_admission" in transaction_output.read_bytes()
    assert b'"ac.method.call"' not in transaction_output.read_bytes()
    assert b"ac.observation_id" in before_final and b"ac.check_id" in before_final
    emitted = {}
    for target in ("cpp", "verilog"):
        output = archive / target
        cli("emit", final, "--target", target, "-o", output)
        emitted[target] = snapshot(output)
    assert final.read_bytes() == before_final
    assert control_final.read_bytes() == control_before

    if args.method_checks == "admission":
        before_unit = snapshot(consumer_unit)
        missing = compile_unit(
            consumer_source.name,
            archive / "missing-provider",
            [relay_unit],
            accepted=False,
        )
        assert any(
            word in missing.stderr.lower()
            for word in ("dependency", "provider", "interface")
        ), missing.stderr
        assert not (archive / "missing-provider").exists()
        # A dynamic Method guard now gates the enclosing value-returning Rule.
        # Preserve the original source case as positive publication coverage;
        # the compiler must route its implicit output validity through link and
        # both emitters, with the previously published control artifacts intact.
        guarded_text = consumer_text.replace(
            "        before = alias.chain(delta)",
            "        owner.guarded(permitted)\n        before = alias.chain(delta)",
        )
        assert guarded_text != consumer_text
        consumer_source.write_text(guarded_text)
        guarded_unit = archive / "guarded-valued-rule"
        compile_unit(consumer_source.name, guarded_unit, [provider_unit, relay_unit])
        guarded_extracted = archive / "guarded-valued-extracted.ac"
        extract(payload(guarded_unit, "body"), guarded_extracted)
        assert normalize(guarded_extracted, archive / "guarded-actual.ac") == normalize(
            payload(guarded_unit, "interface"), archive / "guarded-expected.ac"
        )
        # The broad publication graph above deliberately repeats named reports
        # on one owner; MethodEffectsSystem separately covers instrumentation.
        # Use the same published owner for a hardware value and guarded proposal.
        guarded_source = source / "guarded_value.py"
        guarded_source.write_text(
            """import pycircuit as ac
from interfaces.method_provider import Counter
@ac.module
def GuardedValue(delta: ac.u8, permitted: bool) -> ac.u8:
    owner = Counter()
    @ac.rule
    def work() -> ac.u8:
        owner.guarded(permitted)
        return owner.read() + delta
    return work
"""
        )
        guarded_execution = archive / "guarded-value-execution"
        compile_unit(guarded_source.name, guarded_execution, [provider_unit])
        guarded_final = archive / "guarded-valued-rule.ac"
        cli(
            "link",
            provider_unit,
            guarded_execution,
            "--top",
            "interfaces.guarded_value.GuardedValue",
            "-o",
            guarded_final,
        )
        guarded_body = guarded_final.read_text()
        assert "ac.output_validity" in guarded_body
        assert "ac.sampled_admission" in guarded_body
        assert '"ac.method.call"' not in guarded_body
        for target in ("cpp", "verilog"):
            cli(
                "emit",
                guarded_final,
                "--target",
                target,
                "-o",
                archive / ("guarded-valued-" + target),
            )
        assert snapshot(consumer_unit) == before_unit
        assert final.read_bytes() == before_final
        assert all(
            snapshot(archive / target) == expected
            for target, expected in emitted.items()
        )
        consumer_source.write_text(consumer_text)
        cases = {
            "unknown-method": consumer_text.replace(
                "alias.chain(delta)", "alias.missing(delta)"
            ),
            "arity": consumer_text.replace(
                "alias.chain(delta)", "alias.chain(delta, delta)"
            ),
            "integer-is-not-boolean": consumer_text.replace(
                "owner.boolean(permitted)", "owner.boolean(logical)"
            ),
            "boolean-is-not-integer": consumer_text.replace(
                "owner.integer(logical)", "owner.integer(permitted)"
            ),
            "fixed-is-not-logical-integer": consumer_text.replace(
                "owner.integer(logical)", "owner.integer(fixed)"
            ),
            "fixed-is-not-boolean": consumer_text.replace(
                "owner.boolean(permitted)", "owner.boolean(fixed)"
            ),
            "boolean-result-is-not-integer": consumer_text.replace(
                "owner.integer(logical)", "owner.integer(owner.boolean(permitted))"
            ),
            "integer-result-is-not-boolean": consumer_text.replace(
                "owner.boolean(permitted)", "owner.boolean(owner.integer(logical))"
            ),
            "fixed-width": consumer_text.replace(
                "owner.fixed(fixed)", "owner.fixed(delta)"
            ),
            "fixed-width-integer-constant": consumer_text.replace(
                "other.add(delta)", "other.add(256)"
            ),
            "fixed-width-captured-integer": consumer_text.replace(
                "    owner = Relay()", "    too_large = 256\n    owner = Relay()"
            ).replace("other.add(delta)", "other.add(too_large)"),
            "logical-integer-interval": consumer_text.replace(
                "owner.integer(logical)", "owner.integer(2)"
            ),
            "nominal": consumer_text.replace(
                "@ac.module\ndef Consumer",
                "@ac.struct\nclass Other:\n    value: ac.u8\n@ac.module\ndef Consumer",
            ).replace("owner.packet(payload)", "owner.packet(Other(value=delta))"),
        }
        for name, text in cases.items():
            assert text != consumer_text, name
            consumer_source.write_text(text)
            absent = archive / ("invalid-" + name)
            fresh = compile_unit(
                consumer_source.name,
                absent,
                [provider_unit, relay_unit],
                accepted=False,
            )
            assert not absent.exists(), name
            replacement = compile_unit(
                consumer_source.name,
                consumer_unit,
                [provider_unit, relay_unit],
                replace=True,
                accepted=False,
            )
            if name in (
                "boolean-result-is-not-integer",
                "integer-result-is-not-boolean",
            ):
                expected_kind = (
                    "Integer" if name == "boolean-result-is-not-integer" else "Boolean"
                )
                diagnostic = (
                    "binding boundary requires declared "
                    + expected_kind
                    + " source kind"
                )
                assert diagnostic in fresh.stderr, fresh.stderr
                assert diagnostic in replacement.stderr, replacement.stderr
            assert snapshot(consumer_unit) == before_unit, name
            assert final.read_bytes() == before_final, name
            assert all(
                snapshot(archive / target) == expected
                for target, expected in emitted.items()
            ), name
        consumer_source.write_text(consumer_text)
    # Keep complete actual publication and extraction artifacts beyond the
    # TemporaryDirectory lifetime. Tests do not read these evidence copies.
    stages = retained / "stages"
    if stages.exists():
        shutil.rmtree(stages)
    shutil.copytree(archive, stages)
    controls = retained / "controls"
    if controls.exists():
        shutil.rmtree(controls)
    controls.mkdir()
    for unit in (child, parent):
        shutil.copytree(unit, controls / unit.name)
    (scratch / ("method-" + args.method_checks + "-receipt.json")).write_text(
        json.dumps(
            {
                "scope": "source publication and header authority plus bounded cross-source Method System effect materialization, actual stage retention and dual emission",
                "provider_python_unavailable": not provider_source.exists(),
                "relay_python_unavailable": not relay_source.exists(),
                "units": {
                    unit.name: hashlib.sha256(
                        payload(unit, "interface").read_bytes()
                    ).hexdigest()
                    for unit in (provider_unit, relay_unit, consumer_unit)
                },
                "managed_hardware_outputs_preserved": True,
                "transaction_stages": {
                    "input": hashlib.sha256(transaction_input.read_bytes()).hexdigest(),
                    "output": hashlib.sha256(
                        transaction_output.read_bytes()
                    ).hexdigest(),
                    "final": hashlib.sha256(final.read_bytes()).hexdigest(),
                },
                "mode": args.method_checks,
                "artifacts": str(retained),
                "artifact_sha256": {
                    path.relative_to(retained)
                    .as_posix(): hashlib.sha256(path.read_bytes())
                    .hexdigest()
                    for path in sorted(retained.rglob("*"))
                    if path.is_file()
                },
                "tools": {
                    str(Path(tool).resolve()): hashlib.sha256(
                        Path(tool).read_bytes()
                    ).hexdigest()
                    for tool in (
                        args.source_compiler,
                        args.linker,
                        args.emitter,
                        args.optimizer,
                    )
                },
            },
            indent=2,
        )
        + "\n"
    )
    print("Method " + args.method_checks + " checks passed")
    sys.exit(0)

if args.query_checks:
    sys.path.insert(0, str(repo / "python"))
    sys.path.insert(0, str(fixtures / "table-queries"))
    from pycircuit._source_capture import _capture_source_file
    from pycircuit._source_transport import _emit_source_transport
    from table_query_admission import (
        CONTROL,
        admission_cases,
        resource_cases,
        structure_cases,
        syntax_cases,
    )

    receipts = []
    archive = root / ("query-" + args.query_checks)
    archive.mkdir(exist_ok=True)
    if args.query_checks == "syntax":
        positives, negatives = syntax_cases()
        # Capture transports ordinary lambda parameter lists independently of a
        # typed receiver. first/argmin/map enforce their arity during compile.
        for name in ("zero-args", "two-args"):
            positives[name] = negatives.pop(name)
        positives["multi-input-map"] = positives["ordinary"].replace(
            "entries.first(where=lambda row: predicate(state))",
            "entries.map(lambda lane, other: lane, entries)",
        )
        for accepted, cases in ((True, positives), (False, negatives)):
            for name, text in cases.items():
                filename = "query_" + name.replace("-", "_") + ".py"
                path = source / filename
                path.write_text(text)
                capture = archive / (name + ".capture.mlir")
                capture.write_text(
                    _emit_source_transport(
                        _capture_source_file(path, source_root=source)
                    )
                )
                before = capture.read_bytes()
                output = archive / (name + ".analyzed.mlir")
                result = run(
                    [args.optimizer, capture, "--ac-analyze-rule-writes", "-o", output],
                    accepted,
                )
                assert capture.read_bytes() == before
                if accepted:
                    assert (
                        "overlapping writer pairs" not in result.stderr
                    ), result.stderr
                else:
                    assert (
                        "Lambda" in result.stderr or "captured" in result.stderr
                    ), result.stderr
                    assert not output.exists()
                receipts.append(
                    {
                        "case": name,
                        "accepted": accepted,
                        "capture_sha256": hashlib.sha256(before).hexdigest(),
                    }
                )
    else:
        path = source / "query_control.py"
        path.write_text(CONTROL)
        unit = archive / "control-unit"
        compile_unit(path.name, unit)
        final = archive / "control.ac"
        cli("link", unit, "--top", "interfaces.query_control.Top", "-o", final)
        protected_outputs = {}
        for target in ("cpp", "verilog"):
            output = archive / target
            cli("emit", final, "--target", target, "-o", output)
            protected_outputs[target] = snapshot(output)
        before_unit, before_final = snapshot(unit), final.read_bytes()
        cases = (
            admission_cases() if args.query_checks == "admission" else resource_cases()
        )
        if args.query_checks == "admission":
            original = "entries.first(where=lambda row: row.valid)"
            for label, predicate, key in (
                ("zero", "lambda: True", "lambda: 0"),
                ("two", "lambda row, other: row.valid", "lambda row, other: row.key"),
                ("default", "lambda row=0: row.valid", "lambda row=0: row.key"),
                ("varargs", "lambda *row: True", "lambda *row: 0"),
            ):
                cases["typed-first-" + label] = CONTROL.replace(
                    original, f"entries.first(where={predicate})"
                )
                cases["typed-argmin-where-" + label] = CONTROL.replace(
                    original,
                    f"entries.argmin(where={predicate}, key=lambda row: row.key)",
                )
                cases["typed-argmin-key-" + label] = CONTROL.replace(
                    original, f"entries.argmin(where=lambda row: row.valid, key={key})"
                )
            map_control = CONTROL.replace(
                "class Result:\n    index: ac.u3\n    valid: ac.u1",
                "class Result:\n    value: ac.table[3, Entry]",
            ).replace(
                "    index, valid = "
                + original
                + "\n    return Result(index=index, valid=valid)",
                "    return Result(value=entries.map(lambda row: row))",
            )
            for label, call in (
                ("zero", "entries.map(lambda: 0)"),
                ("two", "entries.map(lambda row, other: row)"),
                ("missing-zip-parameter", "entries.map(lambda row: row, entries)"),
            ):
                cases["typed-map-" + label] = map_control.replace(
                    "entries.map(lambda row: row)", call
                )
        for name, text in cases.items():
            path.write_text(text)
            (archive / (name + ".py")).write_text(text)
            absent = archive / ("invalid-" + name)
            result = compile_unit(path.name, absent, accepted=False)
            assert not absent.exists()
            if args.query_checks == "resources":
                assert "budget" in result.stderr.lower(), result.stderr
            if name.startswith("typed-"):
                assert (
                    "Lambda" in result.stderr or "captured" in result.stderr
                ), result.stderr
            compile_unit(path.name, unit, replace=True, accepted=False)
            assert snapshot(unit) == before_unit and final.read_bytes() == before_final
            assert all(
                snapshot(archive / target) == expected
                for target, expected in protected_outputs.items()
            )
            receipts.append(
                {
                    "case": name,
                    "fresh_exit_status": 1,
                    "replacement_exit_status": 1,
                    "publication_unchanged": True,
                    "source_sha256": hashlib.sha256(text.encode()).hexdigest(),
                }
            )
        if args.query_checks == "resources":
            for name, text in structure_cases().items():
                path.write_text(text)
                output = archive / ("structure-" + name)
                compile_unit(path.name, output)
                body = payload(output, "body").read_text()
                locations = {
                    alias: (int(line), int(column))
                    for alias, line, column in re.findall(
                        r'(#loc\d+) = loc\("[^\"]+":(\d+):(\d+)\)', body
                    )
                }
                callbacks = [
                    node
                    for node in ast.walk(ast.parse(text))
                    if isinstance(node, ast.Lambda)
                ]
                witness = []
                for callback in callbacks:
                    for attribute in (
                        node
                        for node in ast.walk(callback.body)
                        if isinstance(node, ast.Attribute)
                    ):
                        site = attribute.lineno, attribute.col_offset + 1
                        actual = sum(
                            "ac.struct.get" in line
                            and any(
                                f"loc({alias})" in line
                                for alias, location in locations.items()
                                if location == site
                            )
                            for line in body.splitlines()
                        )
                        assert actual == 1, (
                            name,
                            site,
                            actual,
                            "callback projection was cloned or lost",
                        )
                        witness.append(
                            {
                                "line": site[0],
                                "column": site[1],
                                "logical_copies": actual,
                            }
                        )
                receipts.append(
                    {
                        "case": name,
                        "structure": "one actual callback projection per source expression",
                        "callback_witnesses": witness,
                        "body_sha256": hashlib.sha256(body.encode()).hexdigest(),
                    }
                )
    (archive / "receipt.json").write_text(
        json.dumps(
            {
                "scope": args.query_checks,
                "cases": receipts,
                "source_profile": (
                    "AST/scope only"
                    if args.query_checks == "syntax"
                    else "public compile/link/emit protected query publication"
                ),
                "tools": {
                    str(Path(tool).resolve()): hashlib.sha256(
                        Path(tool).read_bytes()
                    ).hexdigest()
                    for tool in (
                        args.source_compiler,
                        args.linker,
                        args.emitter,
                        args.optimizer,
                    )
                },
            },
            indent=2,
        )
        + "\n"
    )
    print(
        "Table query " + args.query_checks + " checks passed"
    )  # noqa: T201 - standalone gate evidence
    sys.exit(0)

if args.invalid:
    body = payload(parent, "body").read_text()
    malformed = root / "malformed.ac"
    mutations = {
        "stage": body.replace('ac.stage = "source"', 'ac.stage = "final"', 1),
        "kind": body.replace(
            'ac.unit_kind = "implementation"', 'ac.unit_kind = "interface"', 1
        ),
        "missing_exports": body.replace("ac.exports =", "ac.other_exports =", 1),
    }
    for name, text in mutations.items():
        assert text != body, name
        malformed.write_text(text)
        absent = root / ("invalid-" + name + ".ac")
        extract(malformed, absent, accepted=False)
        assert not absent.exists(), name
    # A valid final design and both managed emitted bundles are protected when
    # a later source/interface pass rejects malformed replacement source.
    final = root / "parent.ac"
    cli(
        "link",
        child,
        parent,
        "--top",
        "interfaces.source_interface_parent.Parent",
        "-o",
        final,
    )
    outputs = {}
    for target in ("cpp", "verilog"):
        directory = root / target
        cli("emit", final, "--target", target, "-o", directory)
        outputs[target] = snapshot(directory)
    before_unit, before_final = snapshot(parent), final.read_bytes()
    bad = source / "source_interface_parent.py"
    # The passthrough child feeds its own x from y. Capture/import can form
    # valid graph SSA; actual HardwareAnalysis must reject this combination
    # cycle in the registered source-interface extraction pass.
    bad.write_text(bad.read_text().replace("child(x=x)", "child(x=child.y)"))
    fresh = compile_unit(
        "source_interface_parent.py", root / "absent-unit", [child], accepted=False
    )
    assert any(
        word in fresh.stderr.lower() for word in ("cycle", "cyclic")
    ), fresh.stderr
    assert not (root / "absent-unit").exists()
    replaced = compile_unit(
        "source_interface_parent.py", parent, [child], replace=True, accepted=False
    )
    assert any(
        word in replaced.stderr.lower() for word in ("cycle", "cyclic")
    ), replaced.stderr
    assert snapshot(parent) == before_unit and final.read_bytes() == before_final
    for target in outputs:
        assert snapshot(root / target) == outputs[target]
