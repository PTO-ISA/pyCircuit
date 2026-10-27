"""Void Module signatures retain zero results, hierarchy, and child effects."""

from __future__ import annotations

import hashlib
import json

import pytest
from test_saved_anchor import _tree
from test_scalar_generic_modules import _compile_replace, _link
from test_source_system_execution import (
    _bundle,
    _cli,
    _compile,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system


PURE = """import pycircuit as ac
@ac.module
def Empty() -> None:
    pass
@ac.module
def Explicit() -> None:
    return None
@ac.module
def Bare() -> None:
    return
@ac.module
def Sink(value: ac.u8) -> None:
    pass
@ac.module
def Pair(value: ac.u8) -> None:
    first = Sink(value)
    alias = first
    alias_again = alias
    second = Sink(value)
@ac.module
def Mapping(value: ac.u8) -> {"copy": ac.u8}:
    return {"copy": value}
@ac.module
def Scalar(value: ac.u8) -> ac.u8:
    return value
"""


def _source(tmp_path, text=PURE):
    source = tmp_path / "void module sources"
    source.mkdir()
    (source / "provider.py").write_text(text)
    return source


@pytest.mark.parametrize(
    "top,inputs",
    [
        ("Empty", []),
        ("Explicit", []),
        ("Bare", []),
        ("Sink", ["value"]),
        ("Pair", ["value"]),
    ],
)
def test_pure_void_roots_export_no_payload_or_domain(tmp_path, top, inputs):
    # Compile each admission case independently: a hierarchy regression must
    # not hide whether an empty root or an explicit None return was admitted.
    definitions = {
        "Empty": "def Empty() -> None:\n    pass\n",
        "Explicit": "def Explicit() -> None:\n    return None\n",
        "Bare": "def Bare() -> None:\n    return\n",
        "Sink": "def Sink(value: ac.u8) -> None:\n    pass\n",
        "Pair": "def Pair(value: ac.u8) -> None:\n    first = Sink(value)\n    second = Sink(value)\n",
    }
    text = "import pycircuit as ac\n@ac.module\n" + definitions[top]
    if top == "Pair":
        text += "@ac.module\n" + definitions["Sink"]
    source = _source(tmp_path, text)
    provider = tmp_path / "provider"
    _ok(_compile(source, "provider.py", provider))
    final = tmp_path / "final.ac"
    _ok(_link([provider], "checks.provider." + top, final))
    for target in ("cpp", "verilog"):
        output = tmp_path / target
        _ok(_cli("emit", str(final), "--target", target, "-o", str(output)))
        ports = json.loads((output / "ports.json").read_text())
        assert ports["outputs"] == []
        assert [port["name"] for port in ports["inputs"]] == inputs
        assert ports["domains"] == {"clock": None, "reset": None}


def _published_with_hidden_providers(tmp_path, provider_text, consumer_text, exports):
    source = _source(tmp_path, provider_text)
    (source / "facade.py").write_text(f"from checks.provider import {exports}\n")
    (source / "bench.py").write_text(consumer_text)
    provider, facade, consumer = (
        tmp_path / name for name in ("provider", "facade", "consumer")
    )
    _ok(_compile(source, "provider.py", provider))
    _ok(_compile(source, "facade.py", facade, [provider]))
    snapshots = [_tree(unit) for unit in (provider, facade)]
    hidden_bodies = []
    for unit in (provider, facade):
        receipt = json.loads((unit / "unit.json").read_text())
        body = unit / receipt["files"]["body"]
        hidden = tmp_path / (unit.name + "-hidden.ac")
        body.rename(hidden)
        hidden_bodies.append((body, hidden))
        (source / (unit.name + ".py")).unlink()
    try:
        _ok(_compile(source, "bench.py", consumer, [provider, facade]))
    finally:
        for body, hidden in hidden_bodies:
            hidden.rename(body)
    assert [_tree(unit) for unit in (provider, facade)] == snapshots
    final = tmp_path / "final.ac"
    incomplete = _link([facade, consumer], "checks.bench.Exercise", final)
    assert incomplete.returncode != 0, incomplete.stdout + incomplete.stderr
    assert not final.exists()
    _ok(_link([provider, facade, consumer], "checks.bench.Exercise", final))
    return final


def _execute(final, tmp_path, event, expected):
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    traces = []
    for target in ("cpp", "verilog"):
        root = tmp_path / (target + "-execution")
        root.mkdir()
        binary = _bundle(final, root, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "4", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=4"]
            )
            records = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            events = [
                row
                for row in records
                if row["kind"] == "log" and row["spec"]["event"] == event
            ]
            actual = [
                (
                    int(row["evaluation_epoch"]),
                    tuple(int(value["value"]) for value in row["values"]),
                )
                for row in events
            ]
            assert sorted(actual) == sorted(expected)
            assert all(
                int(row["commit_epoch"]) == int(row["evaluation_epoch"]) + 1
                for row in events
            )
            result = [row for row in records if row["kind"] == "result"]
            assert len(result) == 1 and result[0]["status"] == "TERMINATED"
            traces.append([row for row in records if row["kind"] != "result"])
    assert traces[0] == traces[1] == traces[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


def test_void_reexports_and_legacy_results_execute_from_interface_only_compile(
    tmp_path,
):
    bench = """import pycircuit as ac
from checks.facade import Empty, Explicit, Pair, Scalar
@ac.system
def Exercise():
    phase: ac.u8 = 0
    empty = Empty()
    explicit = Explicit()
    pair = Pair(phase)
    scalar = Scalar(phase)
    @ac.rule
    def step() -> None:
        ac.log("info", "void baseline", phase, scalar)
        phase = phase + 1
"""
    final = _published_with_hidden_providers(
        tmp_path, PURE, bench, "Empty, Explicit, Pair, Mapping, Scalar"
    )
    # Legacy mapping Modules retain their established root interface. They do
    # not acquire the scalar/void direct-call convention through this change.
    mapping = tmp_path / "mapping.ac"
    _ok(_link([tmp_path / "provider"], "checks.provider.Mapping", mapping))
    for target in ("cpp", "verilog"):
        output = tmp_path / ("mapping-" + target)
        _ok(_cli("emit", str(mapping), "--target", target, "-o", str(output)))
        ports = json.loads((output / "ports.json").read_text())
        assert [(port["name"], port["width"]) for port in ports["outputs"]] == [
            ("copy", 8)
        ]
    expected = [(epoch, (epoch // 2, epoch // 2)) for epoch in range(8)]
    _execute(final, tmp_path, "void baseline", expected)


def test_void_parent_preserves_two_effectful_child_instances(tmp_path):
    provider = """import pycircuit as ac
@ac.module
def Counter(delta: ac.u8) -> None:
    count: ac.u8 = 0
    @ac.rule
    def advance() -> None:
        ac.log("info", "void child", delta, count)
        count = count + delta
@ac.module
def Parent() -> None:
    first = Counter(1)
    second = Counter(3)
"""
    bench = """import pycircuit as ac
from checks.facade import Parent
@ac.system
def Exercise():
    parent = Parent()
"""
    final = _published_with_hidden_providers(tmp_path, provider, bench, "Parent")
    expected = [
        (epoch, (delta, delta * (epoch // 2))) for epoch in range(8) for delta in (1, 3)
    ]
    _execute(final, tmp_path, "void child", expected)


@pytest.mark.parametrize("payload", ["value", "False", '{"copy": value}', "(value,)"])
def test_void_payload_rejection_preserves_published_unit(tmp_path, payload):
    source = _source(
        tmp_path,
        """import pycircuit as ac
@ac.module
def Sink(value: ac.u8) -> None:
    pass
""",
    )
    provider = tmp_path / "provider"
    _ok(_compile(source, "provider.py", provider))
    before = _tree(provider)
    path = source / "provider.py"
    path.write_text(path.read_text().replace("    pass", "    return " + payload))
    rejected = _compile_replace(source, "provider.py", provider)
    assert rejected.returncode != 0, rejected.stdout + rejected.stderr
    assert "return" in rejected.stderr.lower(), rejected.stdout + rejected.stderr
    assert _tree(provider) == before


@pytest.mark.parametrize("payload", ["value", '{"copy": value}'])
def test_unannotated_module_results_still_require_an_annotation(tmp_path, payload):
    source = _source(
        tmp_path,
        "import pycircuit as ac\n@ac.module\ndef Legacy(value: ac.u8):\n"
        f"    return {payload}\n",
    )
    output = tmp_path / "unannotated"
    rejected = _compile(source, "provider.py", output)
    assert rejected.returncode != 0, rejected.stdout + rejected.stderr
    assert "requires a return annotation" in rejected.stderr
    assert not output.exists()


@pytest.mark.parametrize(
    "body,result",
    [
        ("    occupied = value\n    occupied = Sink(value)\n", "None"),
        ("    first = Sink(value)\n    first = Sink(value)\n", "None"),
        (
            "    first = Sink(value)\n    alias = first\n    alias = Sink(value)\n",
            "None",
        ),
        ("    first = Sink(value)\n    second = Sink(first)\n", "None"),
        ("    first = Sink(value)\n    return first\n", "ac.u8"),
        ("    first = Sink(value)\n    return first + value\n", "ac.u8"),
        (
            "    first = Empty()\n    alias = first\n"
            "    alias: ac.u8 = 0\n    return alias\n",
            "ac.u8",
        ),
        (
            "    first = Empty()\n    alias = first\n"
            "    alias: ac.table[2, ac.u8] = 0\n    return alias\n",
            "ac.table[2, ac.u8]",
        ),
        (
            "    first = Empty()\n    alias = first\n"
            "    alias = ac.table[2, ac.u8](init=0)\n    return alias\n",
            "ac.table[2, ac.u8]",
        ),
    ],
    ids=[
        "occupied-local",
        "instance-rebind",
        "alias-rebind",
        "argument",
        "return",
        "arithmetic",
        "alias-scalar-state",
        "alias-annotated-table-state",
        "alias-constructor-table-state",
    ],
)
def test_void_handles_are_immutable_and_not_payloads(tmp_path, body, result):
    provider = """import pycircuit as ac
@ac.module
def Empty() -> None:
    pass
@ac.module
def Sink(value: ac.u8) -> None:
    pass
@ac.module
def Parent(value: ac.u8) -> None:
    first = Sink(value)
    alias = first
"""
    source = _source(tmp_path, provider)
    output = tmp_path / "provider"
    _ok(_compile(source, "provider.py", output))
    before = _tree(output)
    prefix = provider.split("@ac.module\ndef Parent", 1)[0]
    (source / "provider.py").write_text(
        prefix + f"@ac.module\ndef Parent(value: ac.u8) -> {result}:\n" + body
    )
    rejected = _compile_replace(source, "provider.py", output)
    assert rejected.returncode != 0, rejected.stdout + rejected.stderr
    assert _tree(output) == before
