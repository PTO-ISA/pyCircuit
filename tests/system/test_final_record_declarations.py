"""Independent declaration-only record profile checks for final nominal record declarations.

These cases intentionally stop at declarations. They exercise source/header
authority, the bounded two-field final projection, and final-only verification;
they do not claim record state or backend execution support.
"""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
from pathlib import Path

import pytest
import test_cpp_source_parts as cpp
import test_driver_compile_link as driver
import test_final_value_declarations as value

pytestmark = pytest.mark.system


PROVIDER = """\
from typing import Annotated

Word = Annotated[int, range(256)]
BitWord = Annotated[int, range(18446744073709551616)]

class Pair:
    lo: Word
    hi: bool

    def __init__(self, hi: bool = False, lo: Word = 3):
        self.lo = lo
        self.hi = hi

class Wide:
    low: BitWord
    high: BitWord

    def __init__(self, high: BitWord = 9, low: BitWord = 5):
        self.low = low
        self.high = high

class _Private:
    first: bool
    second: Word

    def __init__(self, second: Word = 7, first: bool = True):
        self.first = first
        self.second = second
"""

THREE_FIELDS = """\
from typing import Annotated
Word = Annotated[int, range(16)]

class TooWideForThisSlice:
    first: Word
    second: Word
    third: Word

    def __init__(self, first: Word = 1, second: Word = 2, third: Word = 3):
        self.first = first
        self.second = second
        self.third = third
"""

ROOT_SOURCE = """\
from pycircuit import module
from .facade import ExportedPair
from .provider import Word

class LocalPair:
    first: bool
    second: Word

    def __init__(self, second: Word = 4, first: bool = True):
        self.first = first
        self.second = second

@module
def Top():
    value: Word = 11
"""


def _build(
    directory: Path,
    *,
    provider: str = PROVIDER,
    ordinary_helper: bool = False,
    root_source: str = ROOT_SOURCE,
    constructor_mutation: tuple[str, str, str] | None = None,
):
    source_root = directory / "source"
    source_root.mkdir(parents=True)
    source_texts = {
        "provider.py": provider,
        "facade.py": "from .provider import Pair as ExportedPair\n",
        "empty.py": '"""An intentionally empty source unit."""\n',
    }
    if ordinary_helper:
        source_texts["helper.py"] = (
            "from .provider import Pair\ndef unused() -> Pair:\n    return Pair()\n"
        )
    source_texts["root.py"] = root_source
    units = {}
    provider_body = None
    for index, (relative, contents) in enumerate(source_texts.items()):
        if relative != "provider.py" and provider_body is None:
            # Prove consumers compile with the owning source and body absent;
            # the provider interface header is their only declaration source.
            provider_body = units["provider.py"].body.read_bytes()
            (source_root / "provider.py").unlink()
            units["provider.py"].body.unlink()
        source = source_root / relative
        source.write_text(contents, encoding="utf-8")
        units[relative] = cpp._compile_source(
            source,
            source_root=source_root,
            output_dir=directory / "units" / str(index),
            headers=tuple(unit.header for unit in units.values()),
        )
    mutation_count = 0
    if constructor_mutation is not None:
        symbol, before, after = constructor_mutation
        for unit in units.values():
            for artifact in (unit.body, unit.header):
                if not artifact.is_file():
                    continue
                source = artifact.read_text(encoding="utf-8")
                changed, count = _mutate_constructor_body(source, symbol, before, after)
                if count:
                    artifact.write_text(changed, encoding="utf-8")
                    mutation_count += count
        minimum = 3 if symbol.endswith("Pair.__init__") else 1
        assert mutation_count >= minimum, (symbol, mutation_count, before)
    (source_root / "provider.py").write_text(provider, encoding="utf-8")
    units["provider.py"].body.write_bytes(provider_body)
    final = directory / "root.final.ac"
    linked = cpp._link(list(units.values()), final, top="demo.root.Top", role="design")
    return source_root, units, final, linked


def _mutate_constructor_body(
    text: str, symbol: str, before: str, after: str
) -> tuple[str, int]:
    """Edit only a named constructor body while keeping all snapshots aligned."""
    marker = f"func.func @{symbol}"
    count = 0
    offset = 0
    while (start := text.find(marker, offset)) >= 0:
        header_end = text.find("\n", start)
        if header_end < 0 or not text[start:header_end].rstrip().endswith("{"):
            offset = start + len(marker)
            continue
        body_end = text.find("\n  }", header_end)
        if body_end < 0:
            raise AssertionError(f"unclosed constructor function {symbol}")
        body = text[header_end:body_end]
        occurrences = body.count(before)
        if occurrences:
            body = body.replace(before, after, 1)
            text = text[:header_end] + body + text[body_end:]
            count += 1
            offset = header_end + len(body)
        else:
            offset = body_end + 4
    return text, count


def _verify(final: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [cpp._design_harness(), "--design", str(final), "--verify-only"],
        text=True,
        capture_output=True,
        check=False,
    )


def _error_text(result: subprocess.CompletedProcess[str]) -> str:
    return "\n".join(
        line.split("error:", 1)[1]
        for line in result.stderr.splitlines()
        if "error:" in line
    )


def _record_lines(text: str) -> dict[str, str]:
    records = {}
    for line in text.splitlines():
        if '"ac.struct"' not in line and not re.match(r"\s*ac\.struct\b", line):
            continue
        symbol = re.search(r'ac\.struct\s+"([^"]+)"', line)
        if symbol:
            records[symbol.group(1)] = line.strip()
            continue
        symbol = re.search(r'\bsym_name\s*=\s*"([^"]+)"', line)
        if symbol:
            records[symbol.group(1)] = line.strip()
        else:
            custom = re.search(r"ac\.struct\s+@([^\s{]+)", line)
            assert custom, line
            records[custom.group(1)] = line.strip()
    return records


def _expect_invalid(final: Path, text: str, tmp_path: Path):
    mutated = tmp_path / "mutated.final.ac"
    tmp_path.mkdir(parents=True, exist_ok=True)
    mutated.write_text(text, encoding="utf-8")
    checked = _verify(mutated)
    assert checked.returncode == 1, checked.stderr
    for target in ("cpp", "verilog"):
        output = tmp_path / f"mutated.{target}"
        assert not output.exists()
        emitted = subprocess.run(
            [
                cpp._design_harness(),
                "--design",
                str(mutated),
                "--target",
                target,
                "--role",
                "design",
                "--output",
                str(output),
            ],
            text=True,
            capture_output=True,
            check=False,
        )
        assert emitted.returncode == 1, emitted.stderr
        assert not output.exists()


def test_two_field_declarations_are_owned_ordered_and_final_only(tmp_path: Path):
    source_root, units, final, linked = _build(tmp_path)
    assert linked.returncode == 0, linked.stderr
    assert final.is_file()

    text = final.read_text(encoding="utf-8")
    records = _record_lines(text)
    expected = {
        "demo.provider.Pair": ("lo", "hi"),
        "demo.provider.Wide": ("low", "high"),
        "demo.provider._Private": ("first", "second"),
        "demo.root.LocalPair": ("first", "second"),
    }
    assert set(records) == set(expected)
    assert list(records) == sorted(records, key=lambda name: name.encode("utf-8"))
    for symbol, field_names in expected.items():
        line = records[symbol]
        assert f"constructor @{symbol}.__init__" in line
        assert 'ac.declaration_role = "definition"' in line
        positions = [line.index(f'name = "{name}"') for name in field_names]
        assert positions == sorted(positions), (symbol, line)
        assert "func.func" not in line

    for symbol in expected:
        final_fields = re.search(r" fields (.*?) constructor ", records[symbol])
        assert final_fields, records[symbol]
        owner_path = symbol.removeprefix("demo.").rsplit(".", 1)[0] + ".py"
        source_unit = units[owner_path]
        header_line = _record_lines(source_unit.header.read_text())[symbol]
        header_fields = re.search(r" fields (.*?) constructor ", header_line)
        assert header_fields, header_line
        assert final_fields.group(1) == header_fields.group(1), symbol

    provider_header = units["provider.py"].header.read_text(encoding="utf-8")
    assert "demo.provider.Pair" in provider_header
    assert "demo.provider.Wide" in provider_header
    facade_header = units["facade.py"].header.read_text(encoding="utf-8")
    assert "demo.provider.Pair" in facade_header
    assert "demo.facade.ExportedPair" not in records
    assert "demo.facade.Pair" not in records
    assert "demo.facade.ExportedPair" not in text
    assert ("demo", "empty.py") in value._units(text)
    assert "demo.root.Top" in text

    # Constructor defaults and argument order are source-only provenance;
    # the final package keeps the canonical symbol but no executable body.
    assert text.count("constructor @demo.provider.Pair.__init__") == 1
    assert text.count("constructor @demo.provider.Wide.__init__") == 1
    assert "func.func @demo.provider.Pair.__init__" not in text
    assert "func.func @demo.provider.Wide.__init__" not in text

    # The saved package verifies in a fresh process after all Python sources
    # and source transports are unavailable.
    shutil.rmtree(source_root)
    shutil.rmtree(tmp_path / "units")
    checked = _verify(final)
    assert checked.returncode == 0, checked.stderr


def test_source_header_mismatch_and_out_of_slice_declarations_reject_at_link(
    tmp_path: Path,
):
    source_root, units, _final, _linked = _build(tmp_path / "valid")

    # Alter only the supplied provider header after body capture. The link
    # boundary must detect that a consumer cannot redefine provider authority.
    header = units["provider.py"].header
    original = header.read_text(encoding="utf-8")
    changed = re.sub(
        r"constructor @demo\.provider\.Pair\.__init__",
        "constructor @demo.provider.Wide.__init__",
        original,
        count=1,
    )
    assert changed != original
    header.write_text(changed, encoding="utf-8")
    mismatch = cpp._link(
        list(units.values()),
        tmp_path / "mismatch.final.ac",
        top="demo.root.Top",
        role="design",
    )
    assert mismatch.returncode == 1, mismatch.stderr

    # Existing source/header capture can describe wider records, while this
    # bounded final profile must capability-reject even an unused one.
    source_root2 = tmp_path / "wide-source"
    source_root2.mkdir()
    wide_source = source_root2 / "provider.py"
    wide_source.write_text(THREE_FIELDS, encoding="utf-8")
    unit = cpp._compile_source(
        wide_source, source_root=source_root2, output_dir=tmp_path / "wide-unit"
    )
    assert unit.header.is_file()
    root = source_root2 / "root.py"
    root.write_text(
        "from typing import Annotated\nfrom pycircuit import module\n"
        "Word = Annotated[int, range(16)]\n"
        "@module\ndef Top():\n    state: Word = 0\n"
    )
    root_unit = cpp._compile_source(
        root,
        source_root=source_root2,
        output_dir=tmp_path / "wide-root",
        headers=(unit.header,),
    )
    rejected = cpp._link(
        [unit, root_unit],
        tmp_path / "wide.final.ac",
        top="demo.root.Top",
        role="design",
    )
    assert rejected.returncode == 1, rejected.stderr
    assert re.search(
        r"record|struct|field|unsupported|capability", rejected.stderr, re.I
    )


def test_final_record_authority_and_shape_mutations_fail_closed(tmp_path: Path):
    _source_root, _units, final, linked = _build(tmp_path)
    assert linked.returncode == 0, linked.stderr
    text = final.read_text(encoding="utf-8")
    record = _record_lines(text)["demo.provider.Pair"]

    mutations = {
        "constructor": text.replace(
            record,
            record.replace(
                "@demo.provider.Pair.__init__", "@demo.provider.Wide.__init__"
            ),
            1,
        ),
        "unknown-property": text.replace(
            record,
            record.replace(
                "ac.source_owner = {", "ac.unknown = 1, ac.source_owner = {", 1
            ),
            1,
        ),
        "missing-field": text.replace(
            record,
            re.sub(r'name = "hi"', "", record, count=1),
            1,
        ),
        "duplicate-field": text.replace(
            record, record.replace('name = "hi"', 'name = "lo"', 1), 1
        ),
        "field-origin": text.replace(
            record,
            record.replace(
                "definition = @demo.provider.Pair",
                "definition = @demo.provider.Wide",
                1,
            ),
            1,
        ),
        "field-location-path": text.replace(
            record,
            record.replace('path = "provider.py"', 'path = "facade.py"', 1),
            1,
        ),
        "owner": text.replace(
            'ac.source_owner = {package = "demo", path = "provider.py"}',
            'ac.source_owner = {package = "demo", path = "facade.py"}',
            1,
        ),
    }
    for name, mutated in mutations.items():
        assert mutated != text, name
        _expect_invalid(final, mutated, tmp_path / name)
    package = text.index("module attributes {")
    attributes_open = text.index("{", package)
    attributes_end = value._balanced(text, attributes_open)
    body_open = text.index("{", attributes_end + 1)
    body_end = value._balanced(text, body_open)
    _expect_invalid(
        final, text[:body_end] + text[body_end + 1 :], tmp_path / "unclosed"
    )


def test_record_declaration_final_emit_is_explicitly_not_backend_acceptance(
    tmp_path: Path,
):
    _source_root, _units, final, linked = _build(tmp_path)
    assert linked.returncode == 0, linked.stderr
    # declaration-only record profile establishes declaration verification. Until later record storage/backend integration,
    # no final-only record artifact may accidentally claim executable support.
    for target in ("cpp", "verilog"):
        output = tmp_path / f"not-yet-supported.{target}"
        emitted = subprocess.run(
            [
                cpp._design_harness(),
                "--design",
                str(final),
                "--target",
                target,
                "--role",
                "design",
                "--output",
                str(output),
            ],
            text=True,
            capture_output=True,
            check=False,
        )
        assert emitted.returncode == 1, emitted.stderr
        assert not output.exists()


def test_unused_ordinary_helper_is_rejected_during_final_link(tmp_path: Path):
    _source_root, units, _final, linked = _build(tmp_path, ordinary_helper=True)
    assert units["helper.py"].body.is_file()
    assert linked.returncode == 1, linked.stderr
    assert re.search(r"helper|unsupported|capability", linked.stderr, re.I)


def test_nested_record_declaration_compiles_but_final_link_rejects(
    tmp_path: Path,
):
    nested = (
        PROVIDER
        + """\
class Inner:
    value: Word
    def __init__(self, value: Word = 2):
        self.value = value

class Outer:
    inner: Inner
    flag: bool
    def __init__(self, inner: Inner, flag: bool = False):
        self.inner = inner
        self.flag = flag
"""
    )
    _source_root, units, _final, linked = _build(tmp_path, provider=nested)
    assert units["provider.py"].header.is_file()
    assert linked.returncode == 1, linked.stderr
    assert re.search(
        r"nested|record|struct|unsupported|capability", linked.stderr, re.I
    )


@pytest.mark.parametrize(
    ("case", "symbol", "before", "after", "diagnostic"),
    [
        (
            "wrong-validity-path",
            "demo.provider.Pair.__init__",
            "return %0, %arg2",
            "return %0, %arg0",
            "record constructor return/create closure is invalid",
        ),
        (
            "computed-field-value",
            "demo.provider.Wide.__init__",
            "    %0 = ac.struct.create(%arg1, %arg0) : (i64, i64)",
            "    %computed = arith.addi %arg1, %arg1 : i64\n"
            "    %0 = ac.struct.create(%computed, %arg0) : (i64, i64)",
            "record constructor is not a direct two-operation body",
        ),
    ],
)
def test_unused_constructor_requires_direct_complete_projection(
    tmp_path: Path,
    case: str,
    symbol: str,
    before: str,
    after: str,
    diagnostic: str,
):
    _source_root, _units, _final, linked = _build(
        tmp_path, constructor_mutation=(symbol, before, after)
    )
    assert linked.returncode == 1, linked.stderr
    assert diagnostic in linked.stderr
    assert not _final.exists()


def test_record_state_remains_rejected_in_s1(tmp_path: Path):
    stateful = """\
from pycircuit import module
from .provider import Pair

@module
def Top():
    state: Pair = Pair()
"""
    _source_root, units, _final, linked = _build(tmp_path, root_source=stateful)
    assert all(unit.body.is_file() and unit.header.is_file() for unit in units.values())
    assert linked.returncode == 1, linked.stderr
    assert re.search(
        r"value integer state|record|struct|unsupported|capability",
        linked.stderr,
        re.I,
    )


def test_public_compile_link_publishes_final_declarations_and_emit_fails_closed(
    tmp_path: Path,
):
    cli_environment = dict(os.environ)
    cli_environment["PYCIRCUIT_SOURCE_COMPILER"] = cpp._tool(
        "PYCIRCUIT_SOURCE_COMPILER", "pycircuit-source-unit"
    )
    cli_environment["PYCIRCUIT_LINKER"] = cpp._tool(
        "PYCIRCUIT_LINKER", "pycircuit-link"
    )
    roots = (
        cpp.ROOT / "python/pycircuit/src",
        cpp.ROOT / "python/semantic-core/src",
    )
    inherited = cli_environment.get("PYTHONPATH")
    cli_environment["PYTHONPATH"] = os.pathsep.join(
        [*(str(path) for path in roots), *([inherited] if inherited else [])]
    )
    workspace = driver._Workspace(
        root=tmp_path / "cli-source",
        units=tmp_path / "cli-units",
        out=tmp_path / "cli-out",
    )
    workspace.root.mkdir()
    workspace.units.mkdir()
    workspace.out.mkdir()
    sources = {
        "provider.py": PROVIDER,
        "facade.py": "from .provider import Pair as ExportedPair\n",
        "empty.py": '"""Empty declaration source."""\n',
        "root.py": ROOT_SOURCE,
    }
    for relative, content in sources.items():
        (workspace.root / relative).write_text(content, encoding="utf-8")

    assert driver._ok(driver._compile(cli_environment, workspace, "provider.py"))
    provider_unit = workspace.units / "provider"
    receipt = json.loads((provider_unit / "unit.json").read_text())
    assert receipt["files"] == {
        "body": "provider.ac",
        "interface": "provider.interface.ac",
        "depfile": "provider.d",
    }
    provider_body = (provider_unit / receipt["files"]["body"]).read_bytes()
    (workspace.root / "provider.py").unlink()
    (provider_unit / receipt["files"]["body"]).unlink()

    assert driver._ok(
        driver._compile(
            cli_environment,
            workspace,
            "facade.py",
            interface=(provider_unit,),
        )
    )
    facade_unit = workspace.units / "facade"
    assert driver._ok(driver._compile(cli_environment, workspace, "empty.py"))
    empty_unit = workspace.units / "empty"
    assert driver._ok(
        driver._compile(
            cli_environment,
            workspace,
            "root.py",
            interface=(provider_unit, facade_unit),
        )
    )
    root_unit = workspace.units / "root"
    (workspace.root / "provider.py").write_text(PROVIDER, encoding="utf-8")
    (provider_unit / receipt["files"]["body"]).write_bytes(provider_body)

    final = workspace.out / "design_top.ac"
    linked = driver._link(
        cli_environment,
        (provider_unit, facade_unit, empty_unit, root_unit),
        "demo.root.Top",
        final,
    )
    assert linked.returncode == 0, linked.stderr
    final_text = final.read_text(encoding="utf-8")
    assert "demo.provider.Pair" in final_text
    assert "demo.provider._Private" in final_text
    assert "demo.facade.ExportedPair" not in final_text

    for target in ("cpp", "verilog"):
        destination = workspace.out / f"sentinel.{target}"
        destination.write_bytes(b"preserve this existing output\n")
        emitted = driver._cli(
            cli_environment,
            "emit",
            str(final),
            "--target",
            target,
            "-o",
            str(destination),
        )
        driver._diagnostic(emitted, "emit")
        assert destination.read_bytes() == b"preserve this existing output\n"
