"""Literal providers retain source authority and exact, typed numerical values.

Expected values are specified independently below. These tests use the existing
public source/System flow and native source-unit helpers, not a second runner.
"""

from __future__ import annotations

import json
import re
import shutil
from pathlib import Path

import pytest
import test_cpp_source_parts as native
import test_source_namespace_bindings as source_bindings
from test_source_system_execution import (
    _bundle,
    _cli,
    _compile,
    _install,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system

LITERALS = """limit = 19
SAME = 19
WIDTH = +7
NEGATIVE = -13
BIG = 1267650600228229401496703205499
ENABLED = True
DISABLED = False
"""

TYPES = """import pycircuit as ac
@ac.struct
class Packet:
    flag: bool = ENABLED
    payload: ac.bits[WIDTH] = limit
WIDTH = 7
limit = 19
ENABLED = True
"""


def _write(directory: Path, name: str, text: str) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    (directory / name).write_text(text, encoding="utf-8")


def _tree(directory: Path) -> dict[str, bytes]:
    return {
        str(path.relative_to(directory)): path.read_bytes()
        for path in directory.rglob("*")
        if path.is_file()
    }


def _file(unit: Path, role: str) -> Path:
    return unit / json.loads((unit / "unit.json").read_text())["files"][role]


def _link(units, final, *extra, top="checks.bench.Exercise"):
    return _cli("link", *map(str, units), "--top", top, "-o", str(final), *extra)


def _replace(source, unit, providers=()):
    arguments = [
        "compile",
        "-c",
        str(source / "bench.py"),
        "--source-root",
        str(source),
        "--package-prefix",
        "checks",
        "-o",
        str(unit),
        "--replace",
    ]
    for provider in providers:
        arguments.extend(("-I", str(provider)))
    return _cli(*arguments)


def _observed_value(row):
    value = row["values"][0]
    if value["kind"] == "bits4":
        assert int(value["known"], 16) == (1 << value["width"]) - 1
        assert int(value["z"], 16) == 0
        return int(value["value"], 16)
    assert value["kind"] in ("integer", "bool")
    return int(value["value"])


def _execute(final, directory, expected):
    directory.mkdir(parents=True, exist_ok=True)
    traces = []
    for target in ("cpp", "verilog"):
        binary = _bundle(final, directory, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "2", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=2"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            logs = [row for row in rows if row["kind"] == "log"]
            assert len(logs) == len(expected) * 4
            for event, value in expected.items():
                actual = [
                    (
                        int(row["evaluation_epoch"]),
                        int(row["commit_epoch"]),
                        _observed_value(row),
                    )
                    for row in logs
                    if row["spec"]["event"] == event
                ]
                assert actual == [(epoch, epoch + 1, value) for epoch in range(4)]
            results = [row for row in rows if row["kind"] == "result"]
            assert len(results) == 1 and results[0]["status"] == "TERMINATED"
            traces.append(logs)
    assert traces[0] == traces[1] == traces[2]


def test_constant_only_provider_publishes_exact_distinct_declarations(tmp_path):
    source, unit = tmp_path / "source", tmp_path / "unit"
    _write(source, "provider.py", LITERALS)
    _ok(_compile(source, "provider.py", unit))
    for role in ("body", "interface"):
        text = _file(unit, role).read_text()
        for name, kind, value in (
            ("limit", "integer", "#ac.math_int<19>"),
            ("SAME", "integer", "#ac.math_int<19>"),
            ("WIDTH", "integer", "#ac.math_int<7>"),
            ("NEGATIVE", "integer", "#ac.math_int<-13>"),
            ("BIG", "integer", "#ac.math_int<1267650600228229401496703205499>"),
            ("ENABLED", "bool", "true"),
            ("DISABLED", "bool", "false"),
        ):
            declaration = next(
                line
                for line in text.splitlines()
                if '"ac.constant"' in line
                and f'sym_name = "checks.provider.{name}"' in line
            )
            assert f'type = {{kind = "{kind}"}}' in declaration
            assert f'value = {{kind = "{kind}", value = {value}}}' in declaration
            assert 'path = "provider.py"' in declaration


@pytest.mark.parametrize(
    "spelling", ["local", "direct", "alias", "namespace", "reexport"]
)
def test_literal_values_execute_with_widths_defaults_and_source_absent_anchor(
    tmp_path, spelling
):
    source, units = tmp_path / "source", []
    names = ("limit", "SAME", "WIDTH", "NEGATIVE", "BIG", "ENABLED", "DISABLED")
    refs = dict(zip(names, names, strict=True))
    if spelling == "local":
        prefix = "import pycircuit as ac\n" + LITERALS
    else:
        _write(source, "provider.py", LITERALS)
        provider = tmp_path / "provider"
        _ok(_compile(source, "provider.py", provider))
        units.append(provider)
        (source / "provider.py").unlink()
        body = _file(provider, "body")
        saved_body = body.read_bytes()
        body.unlink()
        try:
            if spelling == "reexport":
                _write(
                    source,
                    "facade.py",
                    "from checks.provider import " + ", ".join(names) + "\n",
                )
                facade = tmp_path / "facade"
                _ok(_compile(source, "facade.py", facade, [provider]))
                units.append(facade)
                (source / "facade.py").unlink()
                owner = "facade"
            else:
                owner = "provider"
            if spelling == "namespace":
                prefix = f"import pycircuit as ac\nimport checks.{owner} as values\n"
                refs = {name: f"values.{name}" for name in names}
            elif spelling == "alias":
                refs = {name: f"imported_{name}" for name in names}
                prefix = (
                    "import pycircuit as ac\nfrom checks.provider import "
                    + ", ".join(f"{name} as {refs[name]}" for name in names)
                    + "\n"
                )
            else:
                prefix = (
                    f"import pycircuit as ac\nfrom checks.{owner} import "
                    + ", ".join(names)
                    + "\n"
                )
            _write(source, "bench.py", _system(prefix, refs))
            bench = tmp_path / "bench"
            _ok(_compile(source, "bench.py", bench, units))
            imported = _file(bench, "interface").read_text()
            identities = re.findall(
                r'"ac.constant"[^\n]*sym_name = "([^"]+)"', imported
            )
            assert set(identities) == {f"checks.provider.{name}" for name in names}
            assert len(identities) == len(names)
        finally:
            body.write_bytes(saved_body)
    if spelling == "local":
        _write(source, "bench.py", _system(prefix, refs))
        bench = tmp_path / "bench"
        _ok(_compile(source, "bench.py", bench))
    units.append(bench)
    snapshots = [_tree(unit) for unit in units]
    final, anchor = tmp_path / "literal.ac", tmp_path / "anchor"
    _ok(_link(units, final))
    _ok(_link(units, anchor, "--target", "anchor"))
    assert '"ac.constant"' not in final.read_text()
    assert '"ac.constant"' not in (anchor / "graph.ac").read_text()
    assert any('"ac.constant"' in path.read_text() for path in anchor.rglob("*.ac"))
    assert snapshots == [_tree(unit) for unit in units]
    for directory in (source, *units):
        shutil.rmtree(directory)
    before = _tree(anchor)
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    restored = tmp_path / "restored.ac"
    _ok(
        _cli(
            "link",
            "--from-anchor",
            str(anchor),
            "--target",
            "final",
            "-o",
            str(restored),
        )
    )
    assert '"ac.constant"' not in restored.read_text()
    assert _tree(anchor) == before
    expected = {
        "small": 19,
        "same": 19,
        "wide": 1267650600228229401496703205499,
        "signed": 7,
        "true": 1,
        "false": 0,
        "default": 19,
    }
    # Both saved-final paths must execute; each path feeds both backends unchanged.
    for artifact, name in ((final, "original"), (restored, "reloaded")):
        _execute(artifact, tmp_path / name, expected)


def _system(prefix, ref):
    return (
        prefix
        + f"""
@ac.module
def Identity(value: ac.bits[W], *, W: int = {ref["WIDTH"]}) -> ac.bits[W]:
    return value
@ac.struct
class LocalPacket:
    flag: bool = {ref["ENABLED"]}
    payload: ac.bits[{ref["WIDTH"]}] = {ref["limit"]}
def require_boolean(value: bool) -> bool:
    return value
@ac.system
def Exercise():
    selected = Identity({ref["limit"]})
    @ac.rule
    def observe():
        packet = LocalPacket()
        wide: ac.bits[101] = {ref["BIG"]}
        signed: ac.u8 = {ref["NEGATIVE"]} + 20
        ac.log("info", "small", selected)
        ac.log("info", "same", {ref["SAME"]})
        ac.log("info", "wide", wide)
        ac.log("info", "signed", signed)
        ac.log("info", "true", require_boolean(packet.flag))
        ac.log("info", "false", {ref["DISABLED"]})
        ac.log("info", "default", packet.payload)
"""
    )


def test_mixed_provider_predeclarations_and_boolean_record_binding(tmp_path):
    source, provider, unit = (
        tmp_path / "source",
        tmp_path / "provider",
        tmp_path / "unit",
    )
    _write(source, "provider.py", TYPES)
    _ok(_compile(source, "provider.py", provider))
    (source / "provider.py").unlink()
    _write(
        source,
        "bench.py",
        """import pycircuit as ac
from checks.provider import Packet, WIDTH, limit, ENABLED
@ac.system
def Exercise():
    @ac.rule
    def observe():
        value = Packet(flag=ENABLED, payload=limit)
        ac.log("info", "flag", value.flag)
        ac.log("info", "payload", value.payload)
""",
    )
    _ok(_compile(source, "bench.py", unit, [provider]))
    final = tmp_path / "mixed.ac"
    _ok(_link([provider, unit], final))
    _execute(final, tmp_path, {"flag": 1, "payload": 19})


INVALID_DECLARATIONS = [
    "VALUE = 1\nVALUE = 2",
    "VALUE = 1\nVALUE = 1",
    "VALUE: int = 1",
    "VALUE = OTHER = 1",
    "VALUE, OTHER = (1, 2)",
    "VALUE = [1]",
    "VALUE = {'a': 1}",
    "VALUE = {1}",
    "VALUE = (1,)",
    "VALUE = None",
    "VALUE = 1.25",
    "VALUE = 'text'",
    "VALUE = 1 + 2",
    "VALUE = int(1)",
    "OTHER = 1\nVALUE = OTHER",
    "VALUE = -True",
    "VALUE = --1",
    "VALUE = ~1",
    "VALUE = +False",
    "ac = 1",
    "bool = 1",
    "int = 1",
    "range = 1",
    "VALUE += 1",
    "VALUE = 1\n@ac.module\ndef VALUE() -> ac.u8:\n    return 1",
    "@ac.module\ndef VALUE() -> ac.u8:\n    return 1\nVALUE = 1",
    "VALUE = 1\n@ac.struct\nclass VALUE:\n    a: ac.u8\n    b: bool",
]


@pytest.mark.parametrize("declaration", INVALID_DECLARATIONS)
def test_unsupported_declarations_preserve_published_unit(tmp_path, declaration):
    source, unit = tmp_path / "source", tmp_path / "unit"
    good = "import pycircuit as ac\n@ac.module\ndef Top() -> ac.u8:\n    return 3\n"
    _write(source, "bench.py", good)
    _ok(_compile(source, "bench.py", unit))
    before = _tree(unit)
    _write(source, "bench.py", good + declaration + "\n")
    rejected = _replace(source, unit)
    assert rejected.returncode != 0, rejected.stdout
    assert rejected.stderr.strip()
    assert _tree(unit) == before


@pytest.mark.parametrize("width", ["ENABLED", "DISABLED"])
def test_boolean_literal_is_not_a_static_integer_width(tmp_path, width):
    source, unit = tmp_path / "source", tmp_path / "unit"
    _write(
        source,
        "bench.py",
        "import pycircuit as ac\n"
        + LITERALS
        + f"@ac.module\ndef Top(value: ac.bits[{width}]) -> ac.bits[{width}]:\n    return value\n",
    )
    rejected = _compile(source, "bench.py", unit)
    assert rejected.returncode != 0
    assert re.search(
        r"bool|integer|width|static", rejected.stderr, re.I
    ), rejected.stderr
    assert not unit.exists()


@pytest.mark.parametrize(
    "binding",
    ["module-local", "module-formal", "rule-local", "rule-formal", "static-formal"],
)
def test_constant_collision_rejects_at_protected_hardware_scope(tmp_path, binding):
    source, unit = tmp_path / "source", tmp_path / "unit"
    bodies = {
        "module-local": "@ac.module\ndef Top() -> ac.u8:\n    VALUE = 3\n    return VALUE\n",
        "module-formal": "@ac.module\ndef Top(VALUE: ac.u8) -> ac.u8:\n    return VALUE\n",
        "rule-local": "@ac.module\ndef Top() -> ac.u8:\n    @ac.rule\n    def get() -> ac.u8:\n        VALUE = 3\n        return VALUE\n    return get\n",
        "rule-formal": "@ac.module\ndef Top() -> ac.u8:\n    @ac.rule\n    def get(VALUE=3) -> ac.u8:\n        return VALUE\n    return get\n",
        "static-formal": "@ac.module\ndef Top(value: ac.bits[VALUE], *, VALUE: int = 8) -> ac.bits[VALUE]:\n    return value\n",
    }
    _write(source, "bench.py", "import pycircuit as ac\nVALUE = 19\n" + bodies[binding])
    rejected = _compile(source, "bench.py", unit)
    assert rejected.returncode != 0, rejected.stdout
    assert re.search(
        r"binding|shadow|constant|protected|duplicate|collid", rejected.stderr, re.I
    ), rejected.stderr


def test_pure_helper_formals_and_locals_do_not_capture_global_constant(tmp_path):
    source, unit = tmp_path / "source", tmp_path / "unit"
    _write(
        source,
        "bench.py",
        """import pycircuit as ac
VALUE = 19
def formal(VALUE: ac.u8) -> ac.u8:
    return VALUE
def local(value: ac.u8) -> ac.u8:
    VALUE = value + 1
    return VALUE
@ac.system
def Exercise():
    @ac.rule
    def observe():
        ac.log("info", "formal", formal(4))
        ac.log("info", "local", local(4))
        ac.log("info", "global", VALUE)
""",
    )
    _ok(_compile(source, "bench.py", unit))
    final = tmp_path / "helpers.ac"
    _ok(_link([unit], final))
    _execute(final, tmp_path, {"formal": 4, "local": 5, "global": 19})


def _native_pair(tmp_path):
    source = tmp_path / "source"
    _write(source, "provider.py", "VALUE = 19\nFLAG = True\nUNUSED = 23\n")
    provider = native._compile_source(
        source / "provider.py",
        source_root=source,
        output_dir=tmp_path / "provider",
        package="checks",
    )
    _write(
        source,
        "bench.py",
        """import pycircuit as ac
from checks.provider import VALUE, FLAG
@ac.module
def Top() -> ac.u8:
    return VALUE
""",
    )
    bench = native._compile_source(
        source / "bench.py",
        source_root=source,
        output_dir=tmp_path / "bench",
        package="checks",
        headers=(provider.header,),
    )
    return provider, bench


@pytest.mark.parametrize("failure", ["missing", "duplicate", "stale"])
def test_explicit_provider_closure_preserves_existing_final(tmp_path, failure):
    provider, bench = _native_pair(tmp_path)
    final = tmp_path / "final.ac"
    _ok(native._link([provider, bench], final, top="checks.bench.Top", role=None))
    before = final.read_bytes()
    if failure == "missing":
        units = [bench]
    elif failure == "duplicate":
        units = [provider, bench, provider]
    else:
        provider.source.write_text("VALUE = 20\nFLAG = True\nUNUSED = 23\n")
        changed = native._compile_source(
            provider.source,
            source_root=provider.source.parent,
            output_dir=tmp_path / "changed",
            package="checks",
        )
        units = [changed, bench]
    rejected = native._link(units, final, top="checks.bench.Top", role=None)
    assert rejected.returncode != 0, rejected.stdout
    assert re.search(
        r"provider|closure|definition|duplicate|snapshot|mismatch|interface",
        rejected.stderr,
        re.I,
    ), rejected.stderr
    assert final.read_bytes() == before


@pytest.mark.parametrize("side", ["body", "header"])
@pytest.mark.parametrize("mutation", ["value", "kind", "owner", "role"])
def test_native_constant_snapshot_mutations_reject_and_preserve_final(
    tmp_path, side, mutation
):
    provider, bench = _native_pair(tmp_path)
    final = tmp_path / "final.ac"
    _ok(native._link([provider, bench], final, top="checks.bench.Top", role=None))
    before = final.read_bytes()
    path = getattr(provider, side)
    original = path.read_text()
    line = next(
        row
        for row in original.splitlines()
        if '"ac.constant"' in row and "checks.provider.VALUE" in row
    )
    if mutation == "value":
        changed = line.replace("#ac.math_int<19>", "#ac.math_int<20>")
    elif mutation == "kind":
        changed = line.replace('kind = "integer"', 'kind = "bool"')
    elif mutation == "owner":
        changed = line.replace(
            'ac.source_owner = {package = "checks", path = "provider.py"}',
            'ac.source_owner = {package = "checks", path = "other.py"}',
        )
    else:
        changed = re.sub(
            r'ac.declaration_role = "[^"]+"',
            'ac.declaration_role = "import_snapshot"',
            line,
        )
    assert changed != line, line
    path.write_text(original.replace(line, changed, 1))
    rejected = native._link([provider, bench], final, top="checks.bench.Top", role=None)
    assert rejected.returncode != 0, rejected.stdout
    assert rejected.stderr.strip()
    assert final.read_bytes() == before


@pytest.mark.parametrize(
    "mutation",
    [
        "unsupported-kind",
        "integer-with-bool-value",
        "bool-with-integer-value",
        "missing-type",
        "missing-value",
        "missing-owner",
        "missing-role",
    ],
)
def test_native_constant_schema_rejects_even_with_matching_snapshots(
    tmp_path, mutation
):
    provider, bench = _native_pair(tmp_path)
    paths = (provider.body, provider.header, bench.body, bench.header)
    symbol = (
        "checks.provider.FLAG"
        if mutation == "bool-with-integer-value"
        else "checks.provider.VALUE"
    )
    for path in paths:
        original = path.read_text()
        line = next(
            row
            for row in original.splitlines()
            if '"ac.constant"' in row and f'sym_name = "{symbol}"' in row
        )
        if mutation == "unsupported-kind":
            changed = line.replace('kind = "integer"', 'kind = "float"')
        elif mutation == "integer-with-bool-value":
            changed = line.replace("#ac.math_int<19>", "true")
        elif mutation == "bool-with-integer-value":
            changed = line.replace(
                'value = {kind = "bool", value = true}',
                'value = {kind = "bool", value = #ac.math_int<1>}',
            )
        elif mutation == "missing-type":
            changed = line.replace('type = {kind = "integer"}, ', "")
        elif mutation == "missing-value":
            changed = line.replace(
                ', value = {kind = "integer", value = #ac.math_int<19>}', ""
            )
        elif mutation == "missing-owner":
            changed = re.sub(r", ac.source_owner = \{[^}]*\}", "", line)
        else:
            changed = re.sub(r'ac.declaration_role = "[^"]+", ', "", line)
        assert changed != line, (mutation, line)
        path.write_text(original.replace(line, changed, 1))
    final = tmp_path / "malformed.ac"
    rejected = native._link([provider, bench], final, top="checks.bench.Top", role=None)
    assert rejected.returncode != 0, rejected.stdout
    assert re.search(
        r"constant|value|type|kind|owner|envelope|role", rejected.stderr, re.I
    ), rejected.stderr
    assert not final.exists()


def test_rejected_global_call_is_never_executed(tmp_path):
    source, unit, marker = tmp_path / "source", tmp_path / "unit", tmp_path / "executed"
    _write(
        source,
        "bench.py",
        f"VALUE = __import__('pathlib').Path({str(marker)!r}).write_text('ran')\n",
    )
    rejected = _compile(source, "bench.py", unit)
    assert rejected.returncode != 0
    assert (
        not marker.exists()
    ), "source capture executed the rejected global initializer"
    assert not unit.exists()


@pytest.mark.parametrize("order", ["import-first", "constant-first"])
def test_imported_constant_cannot_be_rebound(tmp_path, order):
    source, provider, unit = (
        tmp_path / "source",
        tmp_path / "provider",
        tmp_path / "unit",
    )
    _write(source, "provider.py", "VALUE = 19\n")
    _ok(_compile(source, "provider.py", provider))
    lines = ["from checks.provider import VALUE", "VALUE = 3"]
    if order == "constant-first":
        lines.reverse()
    _write(source, "bench.py", "\n".join(lines) + "\n")
    rejected = _compile(source, "bench.py", unit, [provider])
    assert rejected.returncode != 0, rejected.stdout
    assert re.search(
        r"binding|shadow|constant|protected|duplicate|reassign|collid",
        rejected.stderr,
        re.I,
    ), rejected.stderr
    assert not unit.exists()


@pytest.mark.parametrize("name", ["VALUE", "UNUSED"], ids=["used", "never-imported"])
def test_owning_body_must_define_every_constant_in_its_header(tmp_path, name):
    provider, bench = _native_pair(tmp_path)
    final = tmp_path / "published.ac"
    _ok(native._link([provider, bench], final, top="checks.bench.Top", role=None))
    before = final.read_bytes()
    original = provider.body.read_text()
    declaration = next(
        row
        for row in original.splitlines()
        if '"ac.constant"' in row and f'sym_name = "checks.provider.{name}"' in row
    )
    provider.body.write_text(original.replace(declaration + "\n", "", 1))
    assert f'sym_name = "checks.provider.{name}"' in provider.header.read_text()
    rejected = native._link([provider, bench], final, top="checks.bench.Top", role=None)
    assert rejected.returncode != 0, rejected.stdout
    assert re.search(
        r"constant|definition|missing|body|header", rejected.stderr, re.I
    ), rejected.stderr
    assert final.read_bytes() == before


@pytest.mark.parametrize("element_kind", ["integer", "bool"])
@pytest.mark.parametrize("spelling", ["named", "namespace"])
def test_valid_native_list_constant_is_not_a_frontend_literal(
    tmp_path, element_kind, spelling
):
    source = tmp_path / "source"
    _write(source, "provider.py", "VALUE = 19\n")
    provider = native._compile_source(
        source / "provider.py",
        source_root=source,
        output_dir=tmp_path / "provider",
        package="checks",
    )
    payload = "#ac.math_int<19>" if element_kind == "integer" else "true"
    static_type = (
        f'{{element = {{kind = "{element_kind}"}}, kind = "list", length = 1 : i64}}'
    )
    static_value = (
        f'{{kind = "list", values = [{{kind = "{element_kind}", value = {payload}}}]}}'
    )
    for path in (provider.body, provider.header):
        original = path.read_text()
        declaration = next(
            row
            for row in original.splitlines()
            if '"ac.constant"' in row and 'sym_name = "checks.provider.VALUE"' in row
        )
        changed = declaration.replace(
            'type = {kind = "integer"}', f"type = {static_type}"
        )
        changed = changed.replace(
            'value = {kind = "integer", value = #ac.math_int<19>}',
            f"value = {static_value}",
        )
        assert changed != declaration
        path.write_text(original.replace(declaration, changed, 1))
        # These are valid native StaticType/StaticValue declarations. Failure to
        # parse or verify the provider is not evidence for frontend rejection.
        _ok(
            _run(
                [
                    str(_install() / "bin/pycircuit-opt"),
                    str(path),
                    "-o",
                    str(path.with_suffix(".verified")),
                ]
            )
        )
    if spelling == "named":
        prefix, expression = "from checks.provider import VALUE\n", "VALUE"
    else:
        prefix, expression = "import checks.provider as provider\n", "provider.VALUE"
    _write(
        source,
        "consumer.py",
        "import pycircuit as ac\n"
        + prefix
        + f"""
@ac.system
def Exercise():
    @ac.rule
    def observe():
        ac.log("info", "value", {expression})
""",
    )
    result = source_bindings._compile(
        source / "consumer.py",
        root=source,
        output=tmp_path / "consumer",
        headers=(provider.header,),
    )
    assert result.completed.returncode == 1, (
        result.completed.stdout + result.completed.stderr
    )
    assert re.search(
        r"unsupported|literal|constant|kind", result.completed.stderr, re.I
    ), result.completed.stderr
    assert "Assertion failed" not in result.completed.stderr
    assert not result.body.exists() and not result.interface.exists()


def test_live_static_call_reference_rejects_before_constant_retirement(tmp_path):
    source = tmp_path / "source"
    _write(
        source,
        "provider.py",
        """import pycircuit as ac
VALUE = 7
@ac.module
def Generic(value: ac.u8, *, UNUSED: int = VALUE) -> ac.u8:
    return value
""",
    )
    unit = native._compile_source(
        source / "provider.py",
        source_root=source,
        output_dir=tmp_path / "provider",
        package="checks",
    )
    final = tmp_path / "published.ac"
    _ok(native._link([unit], final, top="checks.provider.Generic", role=None))
    before = final.read_bytes()
    assert b'"ac.constant"' not in before
    for path in (unit.body, unit.header):
        text = path.read_text()
        match = re.search(
            r'default = (#ac.static_expr<\{kind = "literal",.*?'
            r'value = \{kind = "integer", value = #ac.math_int<7>\}\}>)',
            text,
        )
        assert match, text
        literal = match.group(1)
        # A canonical static call is existing native grammar. Preserve the real
        # declaration occurrence/location and leave every hardware width closed.
        call = literal.replace('kind = "literal"', 'kind = "call"', 1).replace(
            'value = {kind = "integer", value = #ac.math_int<7>}',
            "callee = @checks.provider.VALUE, arguments = []",
        )
        assert call != literal
        path.write_text(text.replace(literal, call))
        _ok(
            _run(
                [
                    str(_install() / "bin/pycircuit-opt"),
                    str(path),
                    "-o",
                    str(path.with_suffix(".verified")),
                ]
            )
        )
    inputs = (unit.body.read_bytes(), unit.header.read_bytes())
    rejected = native._link([unit], final, top="checks.provider.Generic", role=None)
    assert rejected.returncode == 1, rejected.stdout + rejected.stderr
    assert (
        "source constant has a remaining semantic reference after literal materialization"
        in rejected.stderr
    )
    assert final.read_bytes() == before
    assert (unit.body.read_bytes(), unit.header.read_bytes()) == inputs
