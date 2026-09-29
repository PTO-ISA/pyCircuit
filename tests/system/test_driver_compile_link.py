"""End-to-end tests for the public ``pycircuit compile`` and ``pycircuit link``.

These use the real native helpers through the real command line. The CLI is
invoked as a subprocess (not in-process) because the facts under test here are
process-level: the exit status, the single ``pycircuit <command>: ...`` line on
stderr, an empty stdout, and the absence of an interpreter traceback all have to
be observed at the process boundary; a shell wrapper around an in-process call
would not prove them.

Every oracle is derived independently: receipts and control markers are parsed
and compared to the documented object, depfiles are parsed into target and
prerequisites, published programs are compared byte-for-byte against a direct
helper invocation over the same published units, and the replaced program is
checked for the specific semantic change made to the root source.
"""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

import pytest

pytestmark = pytest.mark.system

_REPO = Path(__file__).resolve().parents[2]
_SOURCE_ROOTS = (
    _REPO / "python" / "pycircuit" / "src",
    _REPO / "python" / "semantic-core" / "src",
    _REPO / "python" / "agentic-circuit" / "src",
)
_CLI = "import sys; from pycircuit.cli import main; sys.exit(main(sys.argv[1:]))"

DECLARATION = """\
from typing import Annotated

Word = Annotated[int, range(256)]
"""

COUNTER = """\
from pycircuit import module, rule
from .types import Word

@module
def Counter():
    total: Word = {initial}

    @rule
    def tick():
        nonlocal total
        total = (total + 1) & 255

    tick()
"""

BROKEN = """\
from pycircuit import module, rule

@module
def Broken():

    @rule
    def tick():
        return 1

    tick()
"""


# --------------------------------------------------------------------------
# environment and CLI helpers
# --------------------------------------------------------------------------


def _tool(variable: str, name: str) -> str:
    configured = os.environ.get(variable)
    if configured:
        assert Path(configured).is_file(), f"{variable} does not name a file: {configured}"
        return configured
    found = shutil.which(name)
    if not found:
        raise AssertionError(f"set {variable} or put {name} on PATH")
    return found


@pytest.fixture(scope="session")
def harnesses() -> dict[str, str]:
    return {
        "ACIR_SOURCE_UNIT_HARNESS": _tool(
            "ACIR_SOURCE_UNIT_HARNESS", "acir-source-unit-harness"
        ),
        "ACIR_DESIGN_HARNESS": _tool("ACIR_DESIGN_HARNESS", "acir-design-harness"),
    }


@pytest.fixture(scope="session")
def cli_environment(harnesses: dict[str, str]) -> dict[str, str]:
    environment = dict(os.environ)
    environment.update(harnesses)
    # The subprocess must be able to import the in-tree packages even when the
    # caller relied on pytest's own pythonpath setting.
    roots = os.pathsep.join(str(root) for root in _SOURCE_ROOTS)
    inherited = environment.get("PYTHONPATH")
    environment["PYTHONPATH"] = f"{roots}{os.pathsep}{inherited}" if inherited else roots
    return environment


def _cli(environment: dict[str, str], *argv: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, "-c", _CLI, *argv],
        text=True,
        capture_output=True,
        env=environment,
        cwd=str(_REPO),
        check=False,
    )


def _ok(result: subprocess.CompletedProcess[str]) -> subprocess.CompletedProcess[str]:
    assert result.returncode == 0, result.stderr
    assert result.stderr == "", result.stderr
    return result


def _diagnostic(
    result: subprocess.CompletedProcess[str], command: str
) -> str:
    """The documented failure shape: exit 1 and one prefixed line on stderr."""

    assert result.returncode == 1, (result.returncode, result.stderr)
    assert result.stdout == ""
    lines = result.stderr.splitlines()
    assert len(lines) == 1, f"expected one diagnostic line, got {result.stderr!r}"
    assert lines[0].startswith(f"pycircuit {command}: "), result.stderr
    assert "Traceback (most recent call last)" not in result.stderr
    return lines[0]


def _control(destination: Path) -> Path:
    return destination.parent / f".{destination.name}.pycircuit-publication"


def _assert_nothing_published(destination: Path) -> None:
    """No artifact, and no half-finished transaction left beside it."""

    assert not destination.exists(), destination
    control = _control(destination)
    if control.exists():
        assert sorted(entry.name for entry in control.iterdir()) == [
            "lock",
            "owner.json",
        ], sorted(entry.name for entry in control.iterdir())


def _tree(directory: Path) -> dict[str, bytes]:
    return {
        str(entry.relative_to(directory)): entry.read_bytes()
        for entry in sorted(directory.rglob("*"))
        if entry.is_file()
    }


@dataclass(frozen=True, slots=True)
class _Workspace:
    root: Path
    units: Path
    out: Path


@pytest.fixture
def workspace(tmp_path: Path) -> _Workspace:
    root = tmp_path / "src"
    root.mkdir()
    units = tmp_path / "units"
    units.mkdir()
    out = tmp_path / "out"
    out.mkdir()
    (root / "types.py").write_text(DECLARATION, encoding="utf-8")
    return _Workspace(root=root, units=units, out=out)


def _compile(
    environment: dict[str, str],
    workspace: _Workspace,
    name: str,
    *,
    prefix: str | None = "demo",
    interface: tuple[Path, ...] = (),
    replace: bool = False,
    source: Path | None = None,
    output: Path | None = None,
) -> subprocess.CompletedProcess[str]:
    argv = [
        "compile",
        "-c",
        str(source or workspace.root / name),
        "--source-root",
        str(workspace.root),
    ]
    if prefix is not None:
        argv += ["--package-prefix", prefix]
    for unit in interface:
        argv += ["-I", str(unit)]
    argv += ["-o", str(output or workspace.units / Path(name).stem)]
    if replace:
        argv.append("--replace")
    return _cli(environment, *argv)


def _link(
    environment: dict[str, str],
    units: tuple[Path, ...],
    top: str,
    output: Path,
    *,
    parameters: Path | None = None,
    replace: bool = False,
) -> subprocess.CompletedProcess[str]:
    argv = ["link", *(str(unit) for unit in units), "--top", top]
    if parameters is not None:
        argv += ["--parameters", str(parameters)]
    argv += ["-o", str(output)]
    if replace:
        argv.append("--replace")
    return _cli(environment, *argv)


def _depfile(result: str) -> tuple[str, list[str]]:
    target, separator, prerequisites = result.rstrip("\n").partition(": ")
    assert separator == ": ", result
    return target, prerequisites.split(" ")


def _two_units(
    environment: dict[str, str],
    workspace: _Workspace,
    *,
    initial: int = 0,
) -> tuple[Path, Path]:
    (workspace.root / "counter.py").write_text(
        COUNTER.format(initial=initial), encoding="utf-8"
    )
    _ok(_compile(environment, workspace, "types.py"))
    _ok(
        _compile(
            environment, workspace, "counter.py", interface=(workspace.units / "types",)
        )
    )
    return workspace.units / "types", workspace.units / "counter"


def _direct_program(
    harnesses: dict[str, str],
    workspace: _Workspace,
    top: str,
    scratch: Path,
    stems: tuple[str, ...] = ("counter", "types"),
) -> tuple[Path, dict[str, object]]:
    """Run the documented helper invocation over the published units."""

    scratch.mkdir(parents=True, exist_ok=True)
    program = scratch / "direct.ac"
    report = scratch / "direct-owner.json"
    command = [harnesses["ACIR_DESIGN_HARNESS"]]
    for stem in stems:
        command += [
            "--body",
            str(workspace.units / stem / f"{stem}.ac"),
            "--header",
            str(workspace.units / stem / f"{stem}.interface.ac"),
        ]
    command += [
        "--top",
        top,
        "--target",
        "final",
        "--output",
        str(program),
        "--entry-owner-out",
        str(report),
    ]
    completed = subprocess.run(command, text=True, capture_output=True, check=False)
    assert completed.returncode == 0, completed.stderr
    return program, json.loads(report.read_text(encoding="utf-8"))


# --------------------------------------------------------------------------
# compile
# --------------------------------------------------------------------------


def test_compile_publishes_the_four_file_set_and_receipt(
    workspace: _Workspace,
    cli_environment: dict[str, str],
    harnesses: dict[str, str],
) -> None:
    result = _compile(cli_environment, workspace, "types.py")

    _ok(result)
    unit = workspace.units / "types"
    assert sorted(entry.name for entry in unit.iterdir()) == [
        "types.ac",
        "types.d",
        "types.interface.ac",
        "unit.json",
    ]
    assert json.loads((unit / "unit.json").read_text(encoding="utf-8")) == {
        "kind": "pycircuit-source-unit",
        "source": {"package": "demo", "path": "types.py"},
        "files": {
            "body": "types.ac",
            "interface": "types.interface.ac",
            "depfile": "types.d",
        },
    }
    target, prerequisites = _depfile((unit / "types.d").read_text(encoding="utf-8"))
    assert target == str(unit / "types.ac")
    assert prerequisites == [
        str(workspace.root / "types.py"),
        harnesses["ACIR_SOURCE_UNIT_HARNESS"],
    ]
    # A declaration-only source is published as declarations: it gains no
    # fabricated module, and it exports the declared name.
    body = (unit / "types.ac").read_text(encoding="utf-8")
    assert 'ac.unit_kind = "declarations"' in body
    assert '"ac.module"' not in body
    assert 'name = "Word"' in (unit / "types.interface.ac").read_text(encoding="utf-8")
    # Nothing but the unit and its publication control directory is created.
    assert sorted(entry.name for entry in workspace.units.iterdir()) == [
        ".types.pycircuit-publication",
        "types",
    ]


def test_compile_default_package_prefix_is_empty(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    _ok(_compile(cli_environment, workspace, "types.py", prefix=None))

    receipt = json.loads(
        (workspace.units / "types" / "unit.json").read_text(encoding="utf-8")
    )
    assert receipt["source"] == {"package": "", "path": "types.py"}


def test_compile_consumes_a_declaration_provider_through_interface_unit(
    workspace: _Workspace,
    cli_environment: dict[str, str],
    harnesses: dict[str, str],
) -> None:
    _ok(_compile(cli_environment, workspace, "types.py"))
    (workspace.root / "counter.py").write_text(
        COUNTER.format(initial=0), encoding="utf-8"
    )

    result = _compile(
        cli_environment, workspace, "counter.py", interface=(workspace.units / "types",)
    )

    _ok(result)
    unit = workspace.units / "counter"
    assert sorted(entry.name for entry in unit.iterdir()) == [
        "counter.ac",
        "counter.d",
        "counter.interface.ac",
        "unit.json",
    ]
    assert json.loads((unit / "unit.json").read_text(encoding="utf-8"))["source"] == {
        "package": "demo",
        "path": "counter.py",
    }
    target, prerequisites = _depfile((unit / "counter.d").read_text(encoding="utf-8"))
    assert target == str(unit / "counter.ac")
    assert prerequisites == [
        str(workspace.root / "counter.py"),
        str(workspace.units / "types" / "types.interface.ac"),
        str(workspace.units / "types" / "unit.json"),
        harnesses["ACIR_SOURCE_UNIT_HARNESS"],
    ]
    # The consumed provider is the managed interface; its body is not a
    # prerequisite and is never opened by this compile.
    assert str(workspace.units / "types" / "types.ac") not in prerequisites
    body = (unit / "counter.ac").read_text(encoding="utf-8")
    assert 'ac.source_owner = {package = "demo", path = "counter.py"}' in body
    assert 'name = "Counter"' in body
    assert 'name = "Counter"' in (unit / "counter.interface.ac").read_text(
        encoding="utf-8"
    )


def test_compile_rejects_an_interface_unit_that_is_not_published(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    (workspace.root / "counter.py").write_text(
        COUNTER.format(initial=0), encoding="utf-8"
    )
    unmanaged = workspace.units / "unmanaged"
    unmanaged.mkdir()

    result = _compile(
        cli_environment, workspace, "counter.py", interface=(unmanaged,)
    )

    _diagnostic(result, "compile")
    _assert_nothing_published(workspace.units / "counter")


def test_compile_existing_destination_requires_replace(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    _ok(_compile(cli_environment, workspace, "types.py"))
    unit = workspace.units / "types"
    before = {entry.name: entry.read_bytes() for entry in unit.iterdir()}

    rejected = _compile(cli_environment, workspace, "types.py")

    assert "already exists" in _diagnostic(rejected, "compile")
    assert {entry.name: entry.read_bytes() for entry in unit.iterdir()} == before

    # The same owner republishes; the interface really carries the new content.
    (workspace.root / "types.py").write_text(
        DECLARATION + "\nAlias = Word\n", encoding="utf-8"
    )
    replaced = _compile(cli_environment, workspace, "types.py", replace=True)

    _ok(replaced)
    after = {entry.name: entry.read_bytes() for entry in unit.iterdir()}
    assert after["types.interface.ac"] != before["types.interface.ac"]
    assert "Alias" in (unit / "types.interface.ac").read_text(encoding="utf-8")
    assert json.loads((unit / "unit.json").read_text(encoding="utf-8"))["source"] == {
        "package": "demo",
        "path": "types.py",
    }

    # A different owner is not an acceptable replace target and changes nothing.
    other = _compile(cli_environment, workspace, "types.py", prefix="other", replace=True)
    assert "owner" in _diagnostic(other, "compile")
    assert {entry.name: entry.read_bytes() for entry in unit.iterdir()} == after


def test_compile_rejects_an_invalid_source_with_a_single_line_diagnostic(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    (workspace.root / "broken.py").write_text(BROKEN, encoding="utf-8")

    result = _compile(cli_environment, workspace, "broken.py")

    diagnostic = _diagnostic(result, "compile")
    assert "rejected the source" in diagnostic
    # The rejection is the intended frontend rule, not an incidental failure.
    assert "lexical rules cannot return data" in diagnostic
    _assert_nothing_published(workspace.units / "broken")

    absent = _compile(
        cli_environment, workspace, "absent.py", source=workspace.root / "absent.py"
    )
    assert "absent.py" in _diagnostic(absent, "compile")
    _assert_nothing_published(workspace.units / "absent")


def test_compile_names_the_env_override_when_the_helper_is_missing(
    workspace: _Workspace, cli_environment: dict[str, str], tmp_path: Path
) -> None:
    environment = dict(cli_environment)
    environment["ACIR_SOURCE_UNIT_HARNESS"] = str(tmp_path / "absent-harness")

    result = _compile(environment, workspace, "types.py")

    diagnostic = _diagnostic(result, "compile")
    assert "ACIR_SOURCE_UNIT_HARNESS" in diagnostic
    assert str(tmp_path / "absent-harness") in diagnostic
    _assert_nothing_published(workspace.units / "types")


def test_compile_rejects_a_source_that_escapes_the_source_root(
    workspace: _Workspace, cli_environment: dict[str, str], tmp_path: Path
) -> None:
    outside = tmp_path / "outside.py"
    outside.write_text(DECLARATION, encoding="utf-8")

    result = _compile(cli_environment, workspace, "outside.py", source=outside)

    assert "escapes source root" in _diagnostic(result, "compile")
    _assert_nothing_published(workspace.units / "outside")


# --------------------------------------------------------------------------
# link: published program and control directory
# --------------------------------------------------------------------------


def test_link_publishes_one_program_file_and_the_control_directory(
    workspace: _Workspace,
    cli_environment: dict[str, str],
    harnesses: dict[str, str],
    tmp_path: Path,
) -> None:
    _two_units(cli_environment, workspace)
    units_before = _tree(workspace.units)
    destination = workspace.out / "counter.program.ac"

    result = _link(
        cli_environment,
        (workspace.units / "types", workspace.units / "counter"),
        "demo.counter.Counter",
        destination,
    )

    _ok(result)
    assert sorted(entry.name for entry in workspace.out.iterdir()) == [
        ".counter.program.ac.pycircuit-publication",
        "counter.program.ac",
    ]
    control = _control(destination)
    assert sorted(entry.name for entry in control.iterdir()) == ["lock", "owner.json"]
    assert json.loads((control / "owner.json").read_text(encoding="utf-8")) == {
        "kind": "pycircuit-publication-control",
        "destination": "counter.program.ac",
    }
    # The published bytes are exactly what the documented helper invocation
    # produces from the same published units.
    direct, _ = _direct_program(
        harnesses, workspace, "demo.counter.Counter", tmp_path / "direct"
    )
    assert destination.read_bytes() == direct.read_bytes()
    text = destination.read_text(encoding="utf-8")
    assert "ac.initial_value = 0 : i8" in text
    assert 'ac.source_owner = {package = "demo", path = "counter.py"}' in text
    # Linking did not modify, add, or remove anything among its inputs.
    assert _tree(workspace.units) == units_before


def test_published_program_is_admitted_by_both_backends(
    workspace: _Workspace,
    cli_environment: dict[str, str],
    harnesses: dict[str, str],
) -> None:
    _two_units(cli_environment, workspace)
    destination = workspace.out / "program.ac"
    _ok(
        _link(
            cli_environment,
            (workspace.units / "types", workspace.units / "counter"),
            "demo.counter.Counter",
            destination,
        )
    )

    cpp = workspace.out / "design.cpp"
    emitted = subprocess.run(
        [
            harnesses["ACIR_DESIGN_HARNESS"],
            "--design",
            str(destination),
            "--target",
            "cpp",
            "--output",
            str(cpp),
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    assert emitted.returncode == 0, emitted.stderr
    cpp_text = cpp.read_text(encoding="utf-8")
    assert '#include "gfsim/SimSystem.h"' in cpp_text
    assert '#include "gfsim/SimDFF.h"' in cpp_text

    verilog = workspace.out / "design.sv"
    emitted = subprocess.run(
        [
            harnesses["ACIR_DESIGN_HARNESS"],
            "--design",
            str(destination),
            "--target",
            "verilog",
            "--output",
            str(verilog),
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    assert emitted.returncode == 0, emitted.stderr
    verilog_text = verilog.read_text(encoding="utf-8")
    assert re.search(r"^module\s+\S+\s*\(", verilog_text, re.MULTILINE), verilog_text
    assert re.search(r"\balways\b", verilog_text), verilog_text


@pytest.mark.parametrize(
    "top", ["demo.types.Word", "Counter", "demo.other.Counter"]
)
def test_link_top_must_name_the_linked_graph_root(
    workspace: _Workspace,
    cli_environment: dict[str, str],
    tmp_path: Path,
    top: str,
) -> None:
    _two_units(cli_environment, workspace)
    destination = tmp_path / "wrong-top.ac"

    result = _link(
        cli_environment,
        (workspace.units / "types", workspace.units / "counter"),
        top,
        destination,
    )

    diagnostic = _diagnostic(result, "link")
    assert "does not match the selected source graph root" in diagnostic
    assert "'demo.counter.Counter'" in diagnostic
    _assert_nothing_published(destination)


def test_link_requires_the_destination_parent_to_exist(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    _two_units(cli_environment, workspace)
    destination = workspace.out / "absent" / "program.ac"

    result = _link(
        cli_environment,
        (workspace.units / "types", workspace.units / "counter"),
        "demo.counter.Counter",
        destination,
    )

    assert "parent does not exist" in _diagnostic(result, "link")
    assert not (workspace.out / "absent").exists()
    assert list(workspace.out.iterdir()) == []


def test_link_replace_republishes_a_genuinely_different_program(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    types, counter = _two_units(cli_environment, workspace)
    destination = workspace.out / "program.ac"
    _ok(_link(cli_environment, (types, counter), "demo.counter.Counter", destination))
    before = destination.read_bytes()
    assert "ac.initial_value = 0 : i8" in before.decode("utf-8")

    rejected = _link(cli_environment, (types, counter), "demo.counter.Counter", destination)
    assert "already exists" in _diagnostic(rejected, "link")
    assert destination.read_bytes() == before

    # A genuine change to the root source produces a genuinely different
    # program, and --replace installs it under the same owner.
    (workspace.root / "counter.py").write_text(
        COUNTER.format(initial=9), encoding="utf-8"
    )
    _ok(_compile(cli_environment, workspace, "counter.py", interface=(types,), replace=True))
    replaced = _link(
        cli_environment,
        (types, counter),
        "demo.counter.Counter",
        destination,
        replace=True,
    )

    _ok(replaced)
    after = destination.read_bytes()
    assert after != before
    text = after.decode("utf-8")
    assert "ac.initial_value = 9 : i8" in text
    assert "ac.initial_value = 0 : i8" not in text
    control = _control(destination)
    assert sorted(entry.name for entry in control.iterdir()) == ["lock", "owner.json"]
    assert json.loads((control / "owner.json").read_text(encoding="utf-8")) == {
        "kind": "pycircuit-publication-control",
        "destination": "program.ac",
    }

    # Republishing the same program again is repeatable byte for byte.
    again = _link(
        cli_environment,
        (types, counter),
        "demo.counter.Counter",
        destination,
        replace=True,
    )
    _ok(again)
    assert destination.read_bytes() == after


def test_link_accepts_replace_for_a_first_publication(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    types, counter = _two_units(cli_environment, workspace)
    destination = workspace.out / "fresh.ac"

    _ok(
        _link(
            cli_environment,
            (types, counter),
            "demo.counter.Counter",
            destination,
            replace=True,
        )
    )
    assert destination.is_file()


def test_link_replace_refuses_a_path_this_driver_did_not_publish(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """--replace may only republish a path with a publication control directory."""

    types, counter = _two_units(cli_environment, workspace)
    foreign = workspace.out / "foreign.ac"
    foreign.write_text("user data\n", encoding="utf-8")

    result = _link(
        cli_environment,
        (types, counter),
        "demo.counter.Counter",
        foreign,
        replace=True,
    )

    diagnostic = _diagnostic(result, "link")
    assert "refusing to replace a path with no publication control directory" in (
        diagnostic
    )
    assert str(foreign) in diagnostic
    # The user file is untouched and no publication control directory was
    # created beside it.
    assert foreign.read_text(encoding="utf-8") == "user data\n"
    assert not _control(foreign).exists()
    assert sorted(entry.name for entry in workspace.out.iterdir()) == ["foreign.ac"]

    # A dangling symlink is the same case: exists() cannot see its target, but
    # --replace must still refuse before the lock set bootstraps a control
    # directory on this path.
    dangling = workspace.out / "dangling.ac"
    dangling.symlink_to(workspace.out / "missing-target.ac")
    result = _link(
        cli_environment,
        (types, counter),
        "demo.counter.Counter",
        dangling,
        replace=True,
    )
    diagnostic = _diagnostic(result, "link")
    assert "refusing to replace a path with no publication control directory" in (
        diagnostic
    )
    assert str(dangling) in diagnostic
    assert dangling.is_symlink()
    assert not (workspace.out / "missing-target.ac").exists()
    assert not _control(dangling).exists()

    # Without --replace the same foreign file is still never overwritten.
    rejected = _link(cli_environment, (types, counter), "demo.counter.Counter", foreign)
    assert "already exists" in _diagnostic(rejected, "link")
    assert foreign.read_text(encoding="utf-8") == "user data\n"


# --------------------------------------------------------------------------
# link: --parameters
# --------------------------------------------------------------------------


def test_link_parameter_binding_rules(
    workspace: _Workspace, cli_environment: dict[str, str], tmp_path: Path
) -> None:
    types, counter = _two_units(cli_environment, workspace)
    units = (types, counter)

    nonempty = tmp_path / "nonempty.json"
    nonempty.write_text('[{"name": "Width", "value": 8}]', encoding="utf-8")
    result = _link(
        cli_environment,
        units,
        "demo.counter.Counter",
        workspace.out / "nonempty.ac",
        parameters=nonempty,
    )
    assert "static parameter bindings are not implemented yet" in _diagnostic(
        result, "link"
    )
    _assert_nothing_published(workspace.out / "nonempty.ac")

    not_an_array = tmp_path / "object.json"
    not_an_array.write_text('{"Width": 8}', encoding="utf-8")
    result = _link(
        cli_environment,
        units,
        "demo.counter.Counter",
        workspace.out / "object.ac",
        parameters=not_an_array,
    )
    assert "must be an ordered JSON array" in _diagnostic(result, "link")
    _assert_nothing_published(workspace.out / "object.ac")

    malformed = tmp_path / "malformed.json"
    malformed.write_text("[1,", encoding="utf-8")
    result = _link(
        cli_environment,
        units,
        "demo.counter.Counter",
        workspace.out / "malformed.ac",
        parameters=malformed,
    )
    assert "not valid JSON" in _diagnostic(result, "link")
    _assert_nothing_published(workspace.out / "malformed.ac")

    absent = tmp_path / "absent.json"
    result = _link(
        cli_environment,
        units,
        "demo.counter.Counter",
        workspace.out / "absent.ac",
        parameters=absent,
    )
    assert str(absent) in _diagnostic(result, "link")
    _assert_nothing_published(workspace.out / "absent.ac")

    # An omitted --parameters and an explicit empty array are the same request.
    omitted = workspace.out / "omitted.ac"
    explicit = workspace.out / "explicit.ac"
    _ok(_link(cli_environment, units, "demo.counter.Counter", omitted))
    empty = tmp_path / "empty.json"
    empty.write_text("[]", encoding="utf-8")
    _ok(
        _link(
            cli_environment,
            units,
            "demo.counter.Counter",
            explicit,
            parameters=empty,
        )
    )
    assert omitted.read_bytes() == explicit.read_bytes()

    # The rejected binding did not disturb the program that was already there.
    before = explicit.read_bytes()
    result = _link(
        cli_environment,
        units,
        "demo.counter.Counter",
        explicit,
        parameters=nonempty,
        replace=True,
    )
    assert "static parameter bindings are not implemented yet" in _diagnostic(
        result, "link"
    )
    assert explicit.read_bytes() == before


# --------------------------------------------------------------------------
# link: unit arguments
# --------------------------------------------------------------------------


@pytest.mark.parametrize("unit_kind", ["file", "absent"])
def test_link_unit_argument_must_be_a_directory(
    workspace: _Workspace,
    cli_environment: dict[str, str],
    tmp_path: Path,
    unit_kind: str,
) -> None:
    _two_units(cli_environment, workspace)
    unit = tmp_path / "unit"
    if unit_kind == "file":
        unit.write_text("not a directory\n", encoding="utf-8")
    destination = workspace.out / "program.ac"

    result = _link(cli_environment, (unit,), "demo.counter.Counter", destination)

    diagnostic = _diagnostic(result, "link")
    assert "is not a directory" in diagnostic
    # The private receipt reader reports any read failure as an encoding
    # problem; the public command must name the real cause instead.
    assert "UTF-8" not in diagnostic and "encoding" not in diagnostic
    _assert_nothing_published(destination)


def test_link_rejects_two_units_declaring_the_same_source(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    _two_units(cli_environment, workspace)
    _ok(
        _compile(
            cli_environment,
            workspace,
            "types.py",
            output=workspace.units / "types-again",
        )
    )
    destination = workspace.out / "program.ac"

    result = _link(
        cli_environment,
        (workspace.units / "types", workspace.units / "types-again"),
        "demo.counter.Counter",
        destination,
    )

    assert "same source" in _diagnostic(result, "link")
    _assert_nothing_published(destination)


# --------------------------------------------------------------------------
# link: body closure
# --------------------------------------------------------------------------


def test_link_requires_the_full_body_closure_that_compile_does_not(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """compile is satisfied by a managed interface; link is not."""

    types, counter = _two_units(cli_environment, workspace)
    destination = workspace.out / "program.ac"
    _ok(_link(cli_environment, (types, counter), "demo.counter.Counter", destination))
    published = destination.read_bytes()

    body = types / "types.ac"
    body.unlink()
    assert sorted(entry.name for entry in types.iterdir()) == [
        "types.d",
        "types.interface.ac",
        "unit.json",
    ]

    # The header-only provider still satisfies a dependent compile...
    _ok(
        _compile(
            cli_environment,
            workspace,
            "counter.py",
            interface=(types,),
            output=workspace.units / "counter-header-only",
        )
    )

    # ...but the same provider cannot be linked, because link consumes the full
    # body closure.
    replaced = _link(
        cli_environment,
        (types, counter),
        "demo.counter.Counter",
        destination,
        replace=True,
    )
    assert "file set is not closed" in _diagnostic(replaced, "link")
    assert destination.read_bytes() == published

    fresh = workspace.out / "fresh.ac"
    rejected = _link(
        cli_environment, (types, counter), "demo.counter.Counter", fresh
    )
    assert "file set is not closed" in _diagnostic(rejected, "link")
    _assert_nothing_published(fresh)


# --------------------------------------------------------------------------
# link: entry owner
# --------------------------------------------------------------------------


def test_link_entry_owner_report_names_the_root_source(
    workspace: _Workspace,
    cli_environment: dict[str, str],
    harnesses: dict[str, str],
    tmp_path: Path,
) -> None:
    """The owner the driver consumes is the root source, not a provider.

    The program owner is not persisted in any Python-readable form after a
    committed publication, so this test observes the report itself by running
    the same documented helper invocation the driver builds; the driver's
    report-to-owner mapping is pinned separately in
    ``tests/unit/test_driver_commands.py``.
    """

    _two_units(cli_environment, workspace)

    _, report = _direct_program(
        harnesses, workspace, "demo.counter.Counter", tmp_path / "direct"
    )

    assert report == {
        "source": {"package": "demo", "path": "counter.py"},
        "definition": '@"demo.counter.Counter"',
    }
    # The closure really contained the declaration provider as well.
    assert (workspace.units / "types" / "unit.json").is_file()
