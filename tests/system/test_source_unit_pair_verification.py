"""Independent regressions for full source-unit pair verification.

The saved body and header are one compile-time unit: matching owners alone do
not prove that they describe the same source snapshot. These tests ask the
private native verifier to read both artifacts, then exercise the public
replacement path against the exact same malformed pairs.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

import pytest
from pycircuit._native_verify import native_helper
from pycircuit._source_compile import _compile_source_unit
from pycircuit._source_unit_files import _load_full_source_unit

pytestmark = pytest.mark.system

ROOT = Path(__file__).resolve().parents[2]
DECLARATION = """\
from typing import Annotated

Word = Annotated[int, range(256)]
"""
COUNTER = """\
from pycircuit import module, rule
from .types import Word

@module
def Counter():
    total: Word = 0

    @rule
    def tick():
        nonlocal total
        total = (total + 1) & 255

    tick()
"""
OTHER = COUNTER.replace("Counter", "Other").replace("total", "count")
COUNTER_PORT_A = """\
from pycircuit import module, rule
from .types import Word

@module
def Counter(enabled: bool, incoming: Word, outgoing: Word):
    @rule
    def update():
        nonlocal outgoing
        if enabled and incoming != 0:
            outgoing = (incoming + 1) & 255

    update()
"""
COUNTER_PORT_B = COUNTER_PORT_A.replace("incoming", "amount")


def _tool(variable: str, name: str) -> str:
    configured = os.environ.get(variable)
    if configured:
        assert Path(
            configured
        ).is_file(), f"{variable} does not name a file: {configured}"
        return configured
    found = shutil.which(name)
    if not found:
        raise AssertionError(f"set {variable} or put {name} on PATH")
    return found


def _compile_native(
    root: Path, output: Path, name: str, *, interfaces=(), replace=False
):
    return _compile_source_unit(
        root / name,
        source_root=root,
        package="demo",
        native_compiler=_tool("PYCIRCUIT_SOURCE_COMPILER", "pycircuit-source-unit"),
        output=output,
        interface_units=list(interfaces),
        replace=replace,
    )


def _compile_cli_replace(
    workspace: _Workspace,
    unit: Path,
    *,
    source_name: str = "counter.py",
    interfaces: tuple[Path, ...] | None = None,
) -> subprocess.CompletedProcess[str]:
    environment = dict(os.environ)
    source_roots = [
        ROOT / "python",
        ROOT / "python" / "semantic-core" / "src",
    ]
    inherited = environment.get("PYTHONPATH")
    environment["PYTHONPATH"] = os.pathsep.join(
        [*(str(path) for path in source_roots), *([inherited] if inherited else [])]
    )
    environment["PYCIRCUIT_SOURCE_COMPILER"] = _tool(
        "PYCIRCUIT_SOURCE_COMPILER", "pycircuit-source-unit"
    )
    environment["PYCIRCUIT_LINKER"] = _tool("PYCIRCUIT_LINKER", "pycircuit-link")
    argv = [
        sys.executable,
        "-c",
        "import sys; from pycircuit.cli import main; sys.exit(main(sys.argv[1:]))",
        "compile",
        "-c",
        str(workspace.root / source_name),
        "--source-root",
        str(workspace.root),
        "--package-prefix",
        "demo",
    ]
    if interfaces is None:
        interfaces = (workspace.units / "types",) if source_name == "counter.py" else ()
    for interface in interfaces:
        argv.extend(("-I", str(interface)))
    argv.extend(("-o", str(unit), "--replace"))
    return subprocess.run(
        argv,
        cwd=str(ROOT),
        env=environment,
        text=True,
        capture_output=True,
        check=False,
    )


def _tree(path: Path) -> dict[str, bytes]:
    return {
        item.name: item.read_bytes()
        for item in sorted(path.iterdir())
        if item.is_file()
    }


def _pair_result(
    body: Path, header: Path, report: Path
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            str(native_helper("design")),
            "--body",
            str(body),
            "--header",
            str(header),
            "--verify-only",
            "--unit-owner-out",
            str(report),
        ],
        text=True,
        capture_output=True,
        check=False,
    )


def _assert_private_pair_rejected(body: Path, header: Path, scratch: Path) -> None:
    report = scratch / "unit-owner.json"
    result = _pair_result(body, header, report)
    assert result.returncode == 1, (result.returncode, result.stderr)
    assert "error:" in result.stderr, "native verifier returned no semantic diagnostic"
    assert (
        not report.exists()
    ), "native verifier wrote owner evidence for a rejected pair"


@dataclass(frozen=True)
class _Workspace:
    root: Path
    units: Path


@pytest.fixture
def workspace(tmp_path: Path) -> _Workspace:
    root = tmp_path / "src"
    units = tmp_path / "units"
    root.mkdir()
    units.mkdir()
    (root / "types.py").write_text(DECLARATION, encoding="utf-8")
    (root / "counter.py").write_text(COUNTER, encoding="utf-8")
    (root / "other.py").write_text(OTHER, encoding="utf-8")
    return _Workspace(root, units)


def _publish_counter(workspace: _Workspace) -> tuple[object, object]:
    types = _compile_native(workspace.root, workspace.units / "types", "types.py")
    counter = _compile_native(
        workspace.root,
        workspace.units / "counter",
        "counter.py",
        interfaces=(workspace.units / "types",),
    )
    return types, counter


def _same_owner_other_header(
    workspace: _Workspace, tmp_path: Path
) -> tuple[Path, Path]:
    _, counter = _publish_counter(workspace)
    source = workspace.root / "counter.py"
    source.write_text(OTHER, encoding="utf-8")
    other = _compile_native(
        workspace.root,
        workspace.units / "other-snapshot",
        "counter.py",
        interfaces=(workspace.units / "types",),
    )
    body = workspace.units / "counter" / counter.body
    header_text = (workspace.units / "other-snapshot" / other.interface).read_text(
        encoding="utf-8"
    )
    header = tmp_path / "counter.interface.ac"
    header.write_text(header_text, encoding="utf-8")
    assert (
        'ac.source_owner = {package = "demo", path = "counter.py"}'
        in body.read_text(encoding="utf-8")
    )
    assert 'ac.source_owner = {package = "demo", path = "counter.py"}' in header_text
    return body, header


def test_private_verify_only_reads_body_and_header_with_the_same_owner(
    workspace: _Workspace,
    tmp_path: Path,
) -> None:
    """Counter and Other snapshots compiled from counter.py still disagree."""
    body, header = _same_owner_other_header(workspace, tmp_path)
    _assert_private_pair_rejected(body, header, tmp_path)


def test_private_verify_only_rejects_same_symbol_changed_port_snapshot(
    workspace: _Workspace,
    tmp_path: Path,
) -> None:
    source = workspace.root / "counter.py"
    source.write_text(COUNTER_PORT_A, encoding="utf-8")
    _compile_native(workspace.root, workspace.units / "types", "types.py")
    interfaces = (workspace.units / "types",)
    old = _compile_native(
        workspace.root, workspace.units / "old", "counter.py", interfaces=interfaces
    )
    source.write_text(COUNTER_PORT_B, encoding="utf-8")
    new = _compile_native(
        workspace.root, workspace.units / "new", "counter.py", interfaces=interfaces
    )
    body = workspace.units / "old" / old.body
    header = workspace.units / "new" / new.interface
    old_text = body.read_text(encoding="utf-8")
    new_text = header.read_text(encoding="utf-8")
    assert "incoming" in old_text and "amount" in new_text
    _assert_private_pair_rejected(body, header, tmp_path)


@pytest.mark.parametrize("swap", ["interface-as-body", "body-as-interface"])
def test_private_verify_only_rejects_stage_swaps(
    workspace: _Workspace,
    tmp_path: Path,
    swap: str,
) -> None:
    _, counter = _publish_counter(workspace)
    unit = workspace.units / "counter"
    body = unit / counter.body
    header = unit / counter.interface
    wrong_stage = tmp_path / f"{swap}.ac"
    if swap == "interface-as-body":
        wrong_stage.write_bytes(header.read_bytes())
    else:
        wrong_stage.write_bytes(body.read_bytes())
    _assert_private_pair_rejected(wrong_stage, wrong_stage, tmp_path)


@pytest.mark.parametrize(
    "mutation",
    [
        'module attributes {ac.source_owner = {package = "demo", path = "counter.py"}} {}',
        'module attributes {ac.source_owner = {package = "demo", path = "counter.py"}, ac.stage = "body"} {}',
        'module attributes {ac.source_owner = {package = "demo", path = "counter.py"}, ac.unit_kind = "implementation"} {}',
    ],
    ids=["owner-only-empty-builtin", "missing-unit-kind", "missing-stage"],
)
def test_private_verify_only_rejects_empty_builtin_body_metadata(
    workspace: _Workspace,
    tmp_path: Path,
    mutation: str,
) -> None:
    _compile_native(workspace.root, workspace.units / "types", "types.py")
    _compile_native(
        workspace.root,
        workspace.units / "counter",
        "counter.py",
        interfaces=(workspace.units / "types",),
    )
    body = tmp_path / "counter.ac"
    body.write_text(mutation + "\n", encoding="utf-8")
    header = workspace.units / "counter" / "counter.interface.ac"
    _assert_private_pair_rejected(body, header, tmp_path)


def test_private_verify_only_rejects_changed_retained_export_snapshot(
    workspace: _Workspace,
    tmp_path: Path,
) -> None:
    _, counter = _publish_counter(workspace)
    unit = workspace.units / "counter"
    body = unit / counter.body
    header = tmp_path / "counter.interface.ac"
    text = (unit / counter.interface).read_text(encoding="utf-8")
    assert "ac.exports" in text
    changed = text.replace('name = "Counter"', 'name = "Unexpected"', 1)
    assert changed != text
    header.write_text(changed, encoding="utf-8")
    _assert_private_pair_rejected(body, header, tmp_path)


def test_private_verify_only_rejects_changed_retained_import_binding_snapshot(
    workspace: _Workspace,
    tmp_path: Path,
) -> None:
    _, counter = _publish_counter(workspace)
    unit = workspace.units / "counter"
    body = unit / counter.body
    header = tmp_path / "counter.interface.ac"
    text = (unit / counter.interface).read_text(encoding="utf-8")
    marker = "ac.import_bindings = "
    start = text.index(marker) + len(marker)
    end = text.index("\n", start)
    original = text[start:end]
    changed = original.replace(
        "target = @demo.types.Word", "target = @demo.types.OtherWord"
    )
    assert changed != original
    header.write_text(text[:start] + changed + text[end:], encoding="utf-8")
    _assert_private_pair_rejected(body, header, tmp_path)


def test_full_load_accepts_ordinary_implementation_and_declaration_units(
    workspace: _Workspace,
) -> None:
    types, counter = _publish_counter(workspace)
    loaded_counter = _load_full_source_unit(
        workspace.units / "counter", owner=counter.owner
    )
    loaded_types = _load_full_source_unit(workspace.units / "types", owner=types.owner)
    assert 'ac.unit_kind = "implementation"' in loaded_counter.body
    assert 'ac.unit_kind = "declarations"' in loaded_types.body


def test_full_load_accepts_consistent_stale_dependency_snapshot_and_header_only_provider(
    workspace: _Workspace,
) -> None:
    types, counter = _publish_counter(workspace)
    counter_before = _tree(workspace.units / "counter")
    (workspace.root / "types.py").write_text(
        DECLARATION + "\nAlias = Word\n", encoding="utf-8"
    )
    _compile_native(workspace.root, workspace.units / "types", "types.py", replace=True)

    # A dependent unit remains valid against the provider snapshot it captured.
    loaded = _load_full_source_unit(workspace.units / "counter", owner=counter.owner)
    assert loaded.body
    assert _tree(workspace.units / "counter") == counter_before

    # A stable declaration provider is consumed through its interface alone.
    shutil.rmtree(workspace.units / "counter")
    (workspace.units / "types" / types.body).unlink()
    (workspace.units / "types" / types.depfile).unlink()
    result = _compile_native(
        workspace.root,
        workspace.units / "counter",
        "counter.py",
        interfaces=(workspace.units / "types",),
    )
    assert (workspace.units / "counter" / result.body).is_file()
    assert types.owner == {
        "kind": "source-unit",
        "source": {"package": "demo", "path": "types.py"},
    }


def test_compile_replace_preserves_corrupted_pair_and_publication_state(
    workspace: _Workspace,
) -> None:
    types, counter = _publish_counter(workspace)
    unit = workspace.units / "counter"
    header = unit / counter.interface
    original = header.read_bytes()
    header.write_bytes(original.replace(b"demo.counter.Counter", b"demo.counter.Other"))
    before = _tree(unit)
    assert set(before) == {
        "counter.ac",
        "counter.d",
        "counter.interface.ac",
        "unit.json",
    }
    control = unit.parent / ".counter.pycircuit-publication"
    control_before = {
        item.name: item.read_bytes() for item in control.iterdir() if item.is_file()
    }
    assert set(control_before) == {"lock", "owner.json"}

    result = _compile_cli_replace(workspace, unit)
    assert result.returncode == 1, result.stderr
    assert result.stdout == ""
    assert "Traceback" not in result.stderr

    assert _tree(unit) == before
    assert sorted(item.name for item in control.iterdir()) == ["lock", "owner.json"]
    assert {
        item.name: item.read_bytes() for item in control.iterdir() if item.is_file()
    } == control_before
    assert types.owner["source"]["path"] == "types.py"


def _corrupt_declaration_role(workspace: _Workspace):
    types = _compile_native(workspace.root, workspace.units / "types", "types.py")
    unit = workspace.units / "types"
    header = unit / types.interface
    original = header.read_bytes()
    definition_role = b'ac.declaration_role = "definition"'
    import_role = b'ac.declaration_role = "import_snapshot"'
    assert definition_role in original
    corrupted = original.replace(definition_role, import_role, 1)
    assert corrupted != original
    header.write_bytes(corrupted)
    return types, unit, header


def test_private_verify_rejects_a_definition_role_snapshot_as_import_snapshot(
    workspace: _Workspace, tmp_path: Path
) -> None:
    types, unit, header = _corrupt_declaration_role(workspace)
    _assert_private_pair_rejected(unit / types.body, header, tmp_path)


def test_declaration_replace_preserves_definition_role_snapshot_as_import_snapshot(
    workspace: _Workspace,
) -> None:
    types, unit, _ = _corrupt_declaration_role(workspace)
    before = _tree(unit)
    control = unit.parent / ".types.pycircuit-publication"
    control_before = {
        item.name: item.read_bytes() for item in control.iterdir() if item.is_file()
    }
    assert set(before) == {
        "types.ac",
        "types.d",
        "types.interface.ac",
        "unit.json",
    }
    assert set(control_before) == {"lock", "owner.json"}

    result = _compile_cli_replace(
        workspace, unit, source_name="types.py", interfaces=()
    )
    assert result.returncode == 1, result.stderr
    assert result.stdout == ""
    assert "Traceback" not in result.stderr
    assert _tree(unit) == before
    assert sorted(item.name for item in control.iterdir()) == ["lock", "owner.json"]
    assert {
        item.name: item.read_bytes() for item in control.iterdir() if item.is_file()
    } == control_before


def test_full_load_accepts_type_alias_through_a_native_facade_reexport(
    workspace: _Workspace,
) -> None:
    types = _compile_native(workspace.root, workspace.units / "types", "types.py")
    (workspace.root / "facade.py").write_text(
        "from .types import Word as PublicWord\n", encoding="utf-8"
    )
    facade = _compile_native(
        workspace.root,
        workspace.units / "facade",
        "facade.py",
        interfaces=(workspace.units / "types",),
    )
    (workspace.root / "consumer.py").write_text(
        "from pycircuit import module, rule\n"
        "from .facade import PublicWord\n\n"
        "@module\n"
        "def Consumer(incoming: PublicWord, outgoing: PublicWord):\n"
        "    @rule\n"
        "    def update():\n"
        "        nonlocal outgoing\n"
        "        outgoing = incoming\n\n"
        "    update()\n",
        encoding="utf-8",
    )
    consumer = _compile_native(
        workspace.root,
        workspace.units / "consumer",
        "consumer.py",
        interfaces=(workspace.units / "facade", workspace.units / "types"),
    )

    facade_loaded = _load_full_source_unit(
        workspace.units / "facade", owner=facade.owner
    )
    consumer_loaded = _load_full_source_unit(
        workspace.units / "consumer", owner=consumer.owner
    )
    assert "PublicWord" in facade_loaded.interface
    assert "PublicWord" in consumer_loaded.interface
    assert "target = @demo.types.Word" in consumer_loaded.interface
    assert 'path = "facade.py"' in consumer_loaded.interface
    assert types.owner["source"]["path"] == "types.py"


def _retarget_local_attribute(
    path: Path, attribute: str, old_target: bytes, new_target: bytes
) -> bytes:
    original = path.read_bytes()
    marker = f"{attribute} = ".encode()
    start = original.index(marker) + len(marker)
    end = original.index(b", ac.", start)
    value = original[start:end]
    assert value.count(old_target) == 1, (path, attribute, value)
    changed = (
        original[:start] + value.replace(old_target, new_target, 1) + original[end:]
    )
    assert changed != original
    path.write_bytes(changed)
    return changed


def _assert_replace_rejected_without_touching_unit(
    workspace: _Workspace, unit: Path, source_name: str = "counter.py"
) -> None:
    before = _tree(unit)
    assert set(before) == {
        "counter.ac",
        "counter.d",
        "counter.interface.ac",
        "unit.json",
    }
    control = unit.parent / f".{unit.name}.pycircuit-publication"
    control_before = {
        item.name: item.read_bytes() for item in control.iterdir() if item.is_file()
    }
    assert set(control_before) == {"lock", "owner.json"}

    result = _compile_cli_replace(workspace, unit, source_name=source_name)
    assert result.returncode == 1, result.stderr
    assert result.stdout == ""
    assert "Traceback" not in result.stderr
    assert _tree(unit) == before
    assert sorted(item.name for item in control.iterdir()) == ["lock", "owner.json"]
    assert {
        item.name: item.read_bytes() for item in control.iterdir() if item.is_file()
    } == control_before


def _published_counter_with_local_target_mutation(
    workspace: _Workspace,
    attribute: str,
    old_target: bytes,
    new_target: bytes,
) -> tuple[object, object, Path]:
    types, counter = _publish_counter(workspace)
    unit = workspace.units / "counter"
    for name in (counter.body, counter.interface):
        _retarget_local_attribute(unit / name, attribute, old_target, new_target)
    return types, counter, unit


def test_private_verify_rejects_both_exports_retargeted_to_missing_definition(
    workspace: _Workspace, tmp_path: Path
) -> None:
    _, counter, unit = _published_counter_with_local_target_mutation(
        workspace,
        "ac.exports",
        b"target = @demo.counter.Counter",
        b"target = @demo.counter.Missing",
    )
    _assert_private_pair_rejected(
        unit / counter.body, unit / counter.interface, tmp_path
    )


def test_replace_preserves_both_exports_retargeted_to_missing_definition(
    workspace: _Workspace,
) -> None:
    _, _, unit = _published_counter_with_local_target_mutation(
        workspace,
        "ac.exports",
        b"target = @demo.counter.Counter",
        b"target = @demo.counter.Missing",
    )
    _assert_replace_rejected_without_touching_unit(workspace, unit)


def test_private_verify_rejects_both_import_bindings_retargeted_to_missing_type(
    workspace: _Workspace, tmp_path: Path
) -> None:
    _, counter, unit = _published_counter_with_local_target_mutation(
        workspace,
        "ac.import_bindings",
        b"target = @demo.types.Word",
        b"target = @demo.types.MissingWord",
    )
    _assert_private_pair_rejected(
        unit / counter.body, unit / counter.interface, tmp_path
    )


def test_replace_preserves_both_import_bindings_retargeted_to_missing_type(
    workspace: _Workspace,
) -> None:
    _, _, unit = _published_counter_with_local_target_mutation(
        workspace,
        "ac.import_bindings",
        b"target = @demo.types.Word",
        b"target = @demo.types.MissingWord",
    )
    _assert_replace_rejected_without_touching_unit(workspace, unit)
