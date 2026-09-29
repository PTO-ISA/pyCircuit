"""Failing-first regression tests for C3 artifact verification and recovery.

These tests pin three defects of the public ``pycircuit compile`` / ``pycircuit
link`` drivers plus the behaviour that must not regress while they are fixed:

* an existing linked artifact must be natively verified and its verified root
  owner must match the owner the publication was given before ``--replace``
  installs over it, so a plain user file, a corrupt file, or another root's
  program can never be replaced;
* an existing source unit must be natively verified (body and header) and both
  internal owners must agree with the receipt before ``--replace`` installs over
  it, so a corrupted body or interface, or a unit whose internal owner was
  rewritten, is refused with its bytes untouched;
* a managed input whose publication did not finish (destination absent because
  the old target was moved aside, ``phase = prepared``, ``previous`` and
  ``stage`` present) is a legitimate state: the public commands must recover it
  through the existing state machine, not report it as a missing input.

Every oracle is stated independently of the implementation: bytes are compared
before and after a refusal, the publication control directory is enumerated
exactly, the interrupted transaction is inspected file by file, and the
recovered unit is compared with the tree that existed before the interruption.
A refusal never counts as a pass by itself: the concrete artifact, the exact
diagnostic family, and the untouched evidence are all asserted.
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

import pytest

from pycircuit._publication_fs import _PublicationFileSystem
from pycircuit._source_compile import _compile_source_unit

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
DECLARATION_UPDATED = DECLARATION + "\nAlias = Word\n"

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

OTHER = """\
from pycircuit import module, rule
from .types import Word

@module
def Other():
    total: Word = {initial}

    @rule
    def tick():
        nonlocal total
        total = (total + 1) & 255

    tick()
"""

# The three refusal families the driver must keep apart. Each family is named by
# the product's own distinct diagnostic for it, taken from the stable surfaces
# of the same corruption: the native verifier rejects the artifact, the verified
# owner disagrees with the request, or the managed path carries no evidence.
_CORRUPT_IR = "failed native verification"
_CORRUPT_TEXT = "not UTF-8 text"
_OWNER_MISMATCH = "owner does not match"
_MISSING_CONTROL = "without a publication control directory"
_MISSING_INPUT = ("not a directory", "no unit.json")
# The protocol's wrapper for a validator that raised something other than a
# publication error. A refusal that lands here has lost its reason, so no
# corruption or owner refusal may be reported through it.
_GENERIC_WRAPPER = "published artifact validation failed"


def _expected_journal(name: str = "types", *, phase: str = "prepared") -> dict[str, object]:
    return {
        "kind": "pycircuit-publication",
        "artifact": "source-unit",
        "destination": name,
        "owner": {
            "kind": "source-unit",
            "source": {"package": "demo", "path": "types.py"},
        },
        "had_previous": True,
        "phase": phase,
    }


class _Abort(BaseException):
    """Abrupt termination.

    Only a ``BaseException`` models a process that died mid-publication: an
    ``Exception`` is rolled back immediately by ``_publish_artifact`` and leaves
    no durable transaction to recover.
    """


def _fault_at(point: str):
    def fault(current: str) -> None:
        if current == point:
            raise _Abort(f"injected fault at {current}")

    return fault


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
        "ACIR_DESIGN_HARNESS": _tool(
            "ACIR_DESIGN_HARNESS", "acir-design-harness"
        ),
    }


@pytest.fixture(scope="session")
def cli_environment(harnesses: dict[str, str]) -> dict[str, str]:
    environment = dict(os.environ)
    environment.update(harnesses)
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


def _diagnostic(result: subprocess.CompletedProcess[str], command: str) -> str:
    """The documented failure shape: exit 1 and one prefixed line on stderr."""

    assert result.returncode == 1, (result.returncode, result.stderr)
    assert result.stdout == ""
    lines = result.stderr.splitlines()
    assert len(lines) == 1, f"expected one diagnostic line, got {result.stderr!r}"
    assert lines[0].startswith(f"pycircuit {command}: "), result.stderr
    assert "Traceback (most recent call last)" not in result.stderr
    return lines[0]


def _corruption(result: subprocess.CompletedProcess[str], command: str) -> str:
    """A refusal that blames the artifact's content, not its owner or its path."""

    diagnostic = _diagnostic(result, command)
    assert _CORRUPT_IR in diagnostic, diagnostic
    assert _OWNER_MISMATCH not in diagnostic, diagnostic
    assert _MISSING_CONTROL not in diagnostic, diagnostic
    assert _GENERIC_WRAPPER not in diagnostic, diagnostic
    return diagnostic


def _owner_mismatch(result: subprocess.CompletedProcess[str], command: str) -> str:
    """A refusal that blames the declared owner, not the artifact's content."""

    diagnostic = _diagnostic(result, command)
    assert _OWNER_MISMATCH in diagnostic, diagnostic
    assert _CORRUPT_IR not in diagnostic, diagnostic
    assert _GENERIC_WRAPPER not in diagnostic, diagnostic
    return diagnostic


def _control(destination: Path) -> Path:
    return destination.parent / f".{destination.name}.pycircuit-publication"


def _control_names(destination: Path) -> list[str]:
    control = _control(destination)
    assert control.is_dir(), control
    return sorted(entry.name for entry in control.iterdir())


def _committed_control(destination: Path) -> None:
    """A committed publication leaves exactly the initialized control state."""

    assert _control_names(destination) == ["lock", "owner.json"]
    assert json.loads(
        (_control(destination) / "owner.json").read_text(encoding="utf-8")
    ) == {
        "kind": "pycircuit-publication-control",
        "destination": destination.name,
    }


def _nothing_published(destination: Path) -> None:
    """No artifact, and at most the bootstrapped publication control."""

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


def _closed_unit_tree(unit: Path) -> dict[str, bytes]:
    """The four-file source unit, named from its own receipt."""

    receipt = json.loads((unit / "unit.json").read_text(encoding="utf-8"))
    stem = Path(receipt["source"]["path"]).stem
    tree = _tree(unit)
    assert sorted(tree) == [
        f"{stem}.ac",
        f"{stem}.d",
        f"{stem}.interface.ac",
        "unit.json",
    ]
    return tree


def _depfile(text: str) -> tuple[str, list[str]]:
    target, separator, prerequisites = text.rstrip("\n").partition(": ")
    assert separator == ": ", text
    return target, prerequisites.split(" ")


@dataclass(frozen=True, slots=True)
class _Workspace:
    base: Path
    root: Path
    units: Path
    out: Path


@pytest.fixture
def workspace(tmp_path: Path, cli_environment: dict[str, str]) -> _Workspace:
    # ``/var`` is a symlink on macOS and the publication layer rejects symlinked
    # path chains, so every temporary path is resolved before use.
    base = tmp_path.resolve()
    root = base / "src"
    units = base / "units"
    out = base / "out"
    for directory in (root, units, out):
        directory.mkdir()
    (root / "types.py").write_text(DECLARATION, encoding="utf-8")
    (root / "counter.py").write_text(COUNTER.format(initial=0), encoding="utf-8")
    (root / "other.py").write_text(OTHER.format(initial=5), encoding="utf-8")
    return _Workspace(base=base, root=root, units=units, out=out)


def _compile(
    environment: dict[str, str],
    workspace: _Workspace,
    name: str,
    *,
    interface: tuple[Path, ...] = (),
    replace: bool = False,
    output: Path | None = None,
) -> subprocess.CompletedProcess[str]:
    argv = [
        "compile",
        "-c",
        str(workspace.root / name),
        "--source-root",
        str(workspace.root),
        "--package-prefix",
        "demo",
    ]
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
    replace: bool = False,
) -> subprocess.CompletedProcess[str]:
    argv = ["link", *(str(unit) for unit in units), "--top", top]
    argv += ["-o", str(output)]
    if replace:
        argv.append("--replace")
    return _cli(environment, *argv)


def _published(environment: dict[str, str], workspace: _Workspace) -> tuple[Path, Path]:
    """Publish the declaration provider and the dependent implementation unit."""

    types = workspace.units / "types"
    counter = workspace.units / "counter"
    _ok(_compile(environment, workspace, "types.py"))
    _ok(_compile(environment, workspace, "counter.py", interface=(types,)))
    return types, counter


def _published_other(
    environment: dict[str, str], workspace: _Workspace, types: Path
) -> Path:
    """Publish the second, genuinely different root as its own unit."""

    other = workspace.units / "other"
    _ok(_compile(environment, workspace, "other.py", interface=(types,)))
    return other


def _linked_program(
    environment: dict[str, str], workspace: _Workspace, units: tuple[Path, ...], top: str
) -> Path:
    destination = workspace.out / "program.ac"
    _ok(_link(environment, units, top, destination))
    return destination


def _interrupt_provider_replace(
    workspace: _Workspace, *, point: str, unit: str = "types"
) -> dict[str, bytes]:
    """Replace the published unit ``unit`` and die at ``point``.

    Returns the unit tree exactly as it was before the interruption.
    """

    destination = workspace.units / unit
    before = _closed_unit_tree(destination)
    (workspace.root / "types.py").write_text(DECLARATION_UPDATED, encoding="utf-8")
    with pytest.raises(_Abort):
        _compile_source_unit(
            workspace.root / "types.py",
            source_root=workspace.root,
            package="demo",
            native_compiler=_tool(
                "ACIR_SOURCE_UNIT_HARNESS", "acir-source-unit-harness"
            ),
            output=destination,
            replace=True,
            filesystem=_PublicationFileSystem(_fault_at(point)),
        )
    return before


def _assert_interrupted_state(
    workspace: _Workspace, *, point: str, unit: str = "types"
) -> None:
    """The state the real fault hook must have left on disk."""

    destination = workspace.units / unit
    control = _control(destination)
    expected_names = {
        "after_previous_saved": ["journal.json", "lock", "owner.json", "previous", "stage"],
        "after_journal_prepared": ["journal.json", "lock", "owner.json", "stage"],
        "after_stage_complete": ["journal.json", "lock", "owner.json", "stage"],
        "after_journal_committed": ["journal.json", "lock", "owner.json", "previous"],
    }
    assert _control_names(destination) == expected_names[point]
    journal = json.loads((control / "journal.json").read_text(encoding="utf-8"))
    if point == "after_previous_saved":
        assert not destination.exists()
        assert journal == _expected_journal(unit)
        assert (control / "previous").is_dir()
        assert (control / "stage").is_dir()
        return
    if point == "after_journal_prepared":
        # The replacement was fully prepared but the old target was not moved.
        assert destination.is_dir()
        assert journal == _expected_journal(unit)
        assert not (control / "previous").exists()
        assert (control / "stage").is_dir()
        return
    if point == "after_stage_complete":
        assert destination.is_dir()
        assert journal == _expected_journal(unit, phase="preparing")
        assert not (control / "previous").exists()
        assert (control / "stage").is_dir()
        return
    if point == "after_journal_committed":
        assert destination.is_dir()
        assert journal == _expected_journal(unit, phase="committed")
        assert (control / "previous").is_dir()
        assert not (control / "stage").exists()
        return
    raise AssertionError(f"unhandled fault point: {point}")


def _assert_recovered(workspace: _Workspace, before: dict[str, bytes]) -> None:
    """The transaction was rolled back to the pre-interruption unit."""

    types = workspace.units / "types"
    assert _closed_unit_tree(types) == before
    _committed_control(types)


def _assert_consumed_committed(workspace: _Workspace, committed: dict[str, bytes]) -> None:
    """A committed-but-cleanup-pending unit is used as-is, never rolled back."""

    types = workspace.units / "types"
    assert _closed_unit_tree(types) == committed
    assert _control_names(types) == ["journal.json", "lock", "owner.json", "previous"]
    assert json.loads(
        (_control(types) / "journal.json").read_text(encoding="utf-8")
    )["phase"] == "committed"


# --------------------------------------------------------------------------
# linked artifact: --replace must verify IR and owner (defect A)
# --------------------------------------------------------------------------


def test_link_replace_republishes_a_changed_same_owner_program(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """A legitimate same-owner --replace still succeeds and installs the change.

    This is the positive control for the strengthened program validator: a
    genuinely different but valid program from the same root must remain
    replaceable, and the published bytes must be exactly what a first-time link
    of the same units produces.
    """

    types, counter = _published(cli_environment, workspace)
    destination = _linked_program(
        cli_environment, workspace, (types, counter), "demo.counter.Counter"
    )
    before = destination.read_bytes()
    assert b"ac.initial_value = 0 : i8" in before

    (workspace.root / "counter.py").write_text(
        COUNTER.format(initial=9), encoding="utf-8"
    )
    _ok(_compile(cli_environment, workspace, "counter.py", interface=(types,), replace=True))
    _ok(
        _link(
            cli_environment,
            (types, counter),
            "demo.counter.Counter",
            destination,
            replace=True,
        )
    )

    after = destination.read_bytes()
    assert after != before
    assert b"ac.initial_value = 9 : i8" in after
    assert b"ac.initial_value = 0 : i8" not in after
    _committed_control(destination)

    # The replaced artifact is the very program a fresh link of the same units
    # publishes: a byte-for-byte oracle independent of the replace path.
    fresh = workspace.out / "fresh.ac"
    _ok(_link(cli_environment, (types, counter), "demo.counter.Counter", fresh))
    assert destination.read_bytes() == fresh.read_bytes()


def test_link_replace_creates_a_missing_destination_normally(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """--replace is not required to find an existing artifact."""

    types, counter = _published(cli_environment, workspace)
    fresh = workspace.out / "fresh.ac"
    assert not fresh.exists()
    assert not _control(fresh).exists()

    _ok(
        _link(
            cli_environment,
            (types, counter),
            "demo.counter.Counter",
            fresh,
            replace=True,
        )
    )

    assert fresh.is_file()
    assert b"ac.initial_value = 0 : i8" in fresh.read_bytes()
    _committed_control(fresh)
    plain = workspace.out / "plain.ac"
    _ok(_link(cli_environment, (types, counter), "demo.counter.Counter", plain))
    assert fresh.read_bytes() == plain.read_bytes()


def test_link_replace_refuses_a_different_owner_and_keeps_the_old_bytes(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """A valid program from another root is an owner mismatch, not a target."""

    types, counter = _published(cli_environment, workspace)
    other = _published_other(cli_environment, workspace, types)
    destination = _linked_program(
        cli_environment, workspace, (types, counter), "demo.counter.Counter"
    )
    before = destination.read_bytes()

    result = _link(
        cli_environment,
        (types, other),
        "demo.other.Other",
        destination,
        replace=True,
    )

    _owner_mismatch(result, "link")
    assert destination.read_bytes() == before
    _committed_control(destination)


def test_link_replace_refuses_plain_non_ir_text(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """An ordinary user file is never replaced, even with a control marker."""

    types, counter = _published(cli_environment, workspace)
    destination = _linked_program(
        cli_environment, workspace, (types, counter), "demo.counter.Counter"
    )
    text = b"this is not MLIR at all\n"
    destination.write_bytes(text)

    result = _link(
        cli_environment,
        (types, counter),
        "demo.counter.Counter",
        destination,
        replace=True,
    )

    _corruption(result, "link")
    assert destination.read_bytes() == text
    _committed_control(destination)


def test_link_replace_refuses_a_binary_non_utf8_target(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """An undecodable target is corruption, with its own diagnostic.

    A binary file at a published program path must be refused in the corruption
    family. Reporting it through the protocol's generic
    ``published artifact validation failed`` wrapper is the regression pinned
    here: that wrapper erases the reason and would stop the three families from
    being distinguishable.
    """

    types, counter = _published(cli_environment, workspace)
    destination = _linked_program(
        cli_environment, workspace, (types, counter), "demo.counter.Counter"
    )
    binary = b"\x00\x01\xff\xfe\x00binary"
    destination.write_bytes(binary)

    result = _link(
        cli_environment,
        (types, counter),
        "demo.counter.Counter",
        destination,
        replace=True,
    )

    diagnostic = _diagnostic(result, "link")
    assert _CORRUPT_TEXT in diagnostic, diagnostic
    assert _GENERIC_WRAPPER not in diagnostic, diagnostic
    assert _OWNER_MISMATCH not in diagnostic, diagnostic
    assert _MISSING_CONTROL not in diagnostic, diagnostic
    assert destination.read_bytes() == binary
    _committed_control(destination)


@pytest.mark.parametrize(
    "payload",
    [
        "module {\n}\n",
        'module attributes {ac.stage = "final"} {\n}\n',
    ],
)
def test_link_replace_refuses_mlir_that_is_not_a_final_program(
    workspace: _Workspace, cli_environment: dict[str, str], payload: str
) -> None:
    """Parsing is not enough: the artifact must be the linked final program."""

    types, counter = _published(cli_environment, workspace)
    destination = _linked_program(
        cli_environment, workspace, (types, counter), "demo.counter.Counter"
    )
    corrupted = payload.encode("utf-8")
    destination.write_bytes(corrupted)

    result = _link(
        cli_environment,
        (types, counter),
        "demo.counter.Counter",
        destination,
        replace=True,
    )

    _corruption(result, "link")
    assert destination.read_bytes() == corrupted
    _committed_control(destination)


def test_link_replace_refuses_user_text_behind_a_failed_link_marker(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """A control directory left by a failed link is a path guard, not proof.

    The failing link reaches the publication lock set, so it bootstraps the
    control directory at the fresh destination and then fails. Whatever is
    placed there afterwards must still be verified before it is replaced.
    """

    types, counter = _published(cli_environment, workspace)
    destination = workspace.out / "fresh.ac"

    failed = _link(
        cli_environment, (types, counter), "demo.wrong.Wrong", destination
    )
    assert "does not match the selected source graph root" in _diagnostic(
        failed, "link"
    )
    assert not destination.exists()
    assert _control_names(destination) == ["lock", "owner.json"]

    text = b"user replacement text\n"
    destination.write_bytes(text)

    result = _link(
        cli_environment,
        (types, counter),
        "demo.counter.Counter",
        destination,
        replace=True,
    )

    diagnostic = _diagnostic(result, "link")
    assert _MISSING_CONTROL not in diagnostic, diagnostic
    assert _OWNER_MISMATCH not in diagnostic, diagnostic
    assert _CORRUPT_IR in diagnostic, diagnostic
    assert destination.read_bytes() == text
    assert _control_names(destination) == ["lock", "owner.json"]


def test_hand_written_control_marker_does_not_make_plain_text_replaceable(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """A hand-written marker naming the path must not authorize a replacement."""

    types, counter = _published(cli_environment, workspace)
    destination = workspace.out / "hostile.ac"
    control = _control(destination)
    control.mkdir()
    (control / "owner.json").write_text(
        json.dumps(
            {
                "kind": "pycircuit-publication-control",
                "destination": destination.name,
            }
        ),
        encoding="utf-8",
    )
    (control / "lock").write_text("", encoding="utf-8")
    text = b"hand written payload\n"
    destination.write_bytes(text)

    result = _link(
        cli_environment,
        (types, counter),
        "demo.counter.Counter",
        destination,
        replace=True,
    )

    diagnostic = _diagnostic(result, "link")
    assert _MISSING_CONTROL not in diagnostic, diagnostic
    assert _OWNER_MISMATCH not in diagnostic, diagnostic
    assert _CORRUPT_IR in diagnostic, diagnostic
    assert destination.read_bytes() == text
    assert _control_names(destination) == ["lock", "owner.json"]


def test_copied_control_marker_does_not_make_a_foreign_owner_program_replaceable(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """A copied marker plus a genuine program from another root is still foreign."""

    types, counter = _published(cli_environment, workspace)
    other = _published_other(cli_environment, workspace, types)
    foreign_source = workspace.out / "foreign.ac"
    _ok(_link(cli_environment, (types, other), "demo.other.Other", foreign_source))
    victim = workspace.out / "victim.ac"
    victim.write_bytes(foreign_source.read_bytes())
    marker = json.loads(
        (_control(foreign_source) / "owner.json").read_text(encoding="utf-8")
    )
    assert marker["destination"] == foreign_source.name
    control = _control(victim)
    control.mkdir()
    (control / "owner.json").write_text(
        json.dumps(
            {
                "kind": "pycircuit-publication-control",
                "destination": victim.name,
            }
        ),
        encoding="utf-8",
    )
    (control / "lock").write_text("", encoding="utf-8")
    before = victim.read_bytes()

    result = _link(
        cli_environment,
        (types, counter),
        "demo.counter.Counter",
        victim,
        replace=True,
    )

    _owner_mismatch(result, "link")
    assert victim.read_bytes() == before
    assert _control_names(victim) == ["lock", "owner.json"]


def test_link_replace_still_refuses_symlink_dangling_and_directory_destinations(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """The pre-existing destination rejections must survive the new validation."""

    types, counter = _published(cli_environment, workspace)
    top = "demo.counter.Counter"

    # (a) A symlink alias of a managed program cannot be replaced through, even
    # when a marker names the alias itself.
    real = _linked_program(cli_environment, workspace, (types, counter), top)
    real_before = real.read_bytes()
    alias = workspace.out / "alias.ac"
    alias.symlink_to(real)
    alias_control = _control(alias)
    alias_control.mkdir()
    (alias_control / "owner.json").write_text(
        json.dumps(
            {
                "kind": "pycircuit-publication-control",
                "destination": alias.name,
            }
        ),
        encoding="utf-8",
    )
    (alias_control / "lock").write_text("", encoding="utf-8")

    result = _link(cli_environment, (types, counter), top, alias, replace=True)

    _diagnostic(result, "link")
    assert alias.is_symlink()
    assert real.read_bytes() == real_before

    # (b) A dangling symlink whose target exists() cannot see.
    dangling = workspace.out / "dangling.ac"
    dangling.symlink_to(workspace.out / "missing-target.ac")

    result = _link(cli_environment, (types, counter), top, dangling, replace=True)

    _diagnostic(result, "link")
    assert dangling.is_symlink()
    assert not (workspace.out / "missing-target.ac").exists()

    # (c) A directory destination, with and without a marker naming it.
    directory = workspace.out / "adirectory"
    directory.mkdir()
    result = _link(cli_environment, (types, counter), top, directory, replace=True)
    _diagnostic(result, "link")
    assert directory.is_dir() and list(directory.iterdir()) == []

    named = workspace.out / "adirectory-named"
    named.mkdir()
    named_control = _control(named)
    named_control.mkdir()
    (named_control / "owner.json").write_text(
        json.dumps(
            {
                "kind": "pycircuit-publication-control",
                "destination": named.name,
            }
        ),
        encoding="utf-8",
    )
    (named_control / "lock").write_text("", encoding="utf-8")
    result = _link(cli_environment, (types, counter), top, named, replace=True)
    _diagnostic(result, "link")
    assert named.is_dir() and list(named.iterdir()) == []

    assert real.read_bytes() == real_before


# --------------------------------------------------------------------------
# source unit: --replace must verify IR and internal owners (defect B)
# --------------------------------------------------------------------------


def test_compile_replace_refuses_a_corrupted_unit_body(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    types, counter = _published(cli_environment, workspace)
    corrupted = "CORRUPTED UNIT BODY"
    (counter / "counter.ac").write_text(corrupted, encoding="utf-8")
    before = _closed_unit_tree(counter)

    result = _compile(
        cli_environment, workspace, "counter.py", interface=(types,), replace=True
    )

    diagnostic = _diagnostic(result, "compile")
    assert _CORRUPT_IR in diagnostic, diagnostic
    assert _OWNER_MISMATCH not in diagnostic, diagnostic
    assert _closed_unit_tree(counter) == before
    assert (counter / "counter.ac").read_text(encoding="utf-8") == corrupted


def test_compile_replace_refuses_a_corrupted_unit_interface(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    types, counter = _published(cli_environment, workspace)
    corrupted = "CORRUPTED UNIT BODY"
    (counter / "counter.interface.ac").write_text(corrupted, encoding="utf-8")
    before = _closed_unit_tree(counter)

    result = _compile(
        cli_environment, workspace, "counter.py", interface=(types,), replace=True
    )

    diagnostic = _diagnostic(result, "compile")
    assert _CORRUPT_IR in diagnostic, diagnostic
    assert _OWNER_MISMATCH not in diagnostic, diagnostic
    assert _closed_unit_tree(counter) == before
    assert (counter / "counter.interface.ac").read_text(encoding="utf-8") == corrupted


def test_compile_replace_refuses_a_unit_whose_owner_disagrees(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """Receipt, body owner, and expected owner must all agree.

    Both directions of the disagreement are pinned: a rewritten receipt source
    and a rewritten body owner. Neither may be replaced.
    """

    types, counter = _published(cli_environment, workspace)
    body_path = counter / "counter.ac"
    receipt_path = counter / "unit.json"
    original_body = body_path.read_text(encoding="utf-8")
    original_receipt = receipt_path.read_text(encoding="utf-8")
    internal = 'ac.source_owner = {package = "demo", path = "counter.py"}'
    assert internal in original_body, original_body[:400]

    # (a) the receipt and the expected owner disagree: already refused today.
    receipt = json.loads(original_receipt)
    receipt["source"] = {"package": "elsewhere", "path": "counter.py"}
    receipt_path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")

    result = _compile(
        cli_environment, workspace, "counter.py", interface=(types,), replace=True
    )

    _owner_mismatch(result, "compile")
    assert json.loads(receipt_path.read_text(encoding="utf-8"))["source"] == {
        "package": "elsewhere",
        "path": "counter.py",
    }

    # (b) the receipt is intact but the body's internal owner was rewritten.
    receipt_path.write_text(original_receipt, encoding="utf-8")
    rewritten = original_body.replace(
        internal, 'ac.source_owner = {package = "elsewhere", path = "counter.py"}'
    )
    assert rewritten != original_body
    body_path.write_text(rewritten, encoding="utf-8")

    result = _compile(
        cli_environment, workspace, "counter.py", interface=(types,), replace=True
    )

    _owner_mismatch(result, "compile")
    assert body_path.read_text(encoding="utf-8") == rewritten


def test_compile_replace_still_republishes_a_legitimate_same_owner_update(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """The strengthened validator must not reject a legitimate update."""

    types, counter = _published(cli_environment, workspace)
    before = _closed_unit_tree(counter)

    (workspace.root / "counter.py").write_text(
        COUNTER.format(initial=3), encoding="utf-8"
    )
    _ok(
        _compile(
            cli_environment, workspace, "counter.py", interface=(types,), replace=True
        )
    )

    after = _closed_unit_tree(counter)
    assert after["counter.ac"] != before["counter.ac"]
    # The body carries the new initializer literal, and carried a different one
    # before the update.
    assert b"#ac.math_int<3>" in after["counter.ac"]
    assert b"#ac.math_int<3>" not in before["counter.ac"]
    _committed_control(counter)

    # The replaced unit is a real unit: the existing link path still consumes it.
    destination = workspace.out / "program.ac"
    _ok(_link(cli_environment, (types, counter), "demo.counter.Counter", destination))
    assert b"ac.initial_value = 3 : i8" in destination.read_bytes()


# --------------------------------------------------------------------------
# managed input recovery (defect C)
# --------------------------------------------------------------------------


def test_interrupted_provider_replace_leaves_a_prepared_transaction(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """Item 13/14: the real fault hook must leave the real durable state."""

    _published(cli_environment, workspace)
    before = _interrupt_provider_replace(workspace, point="after_previous_saved")
    _assert_interrupted_state(workspace, point="after_previous_saved")

    types = workspace.units / "types"
    control = _control(types)
    assert not types.exists()
    assert json.loads((control / "journal.json").read_text(encoding="utf-8")) == (
        _expected_journal()
    )
    assert _tree(control / "previous") == before
    stage = _tree(control / "stage")
    assert sorted(stage) == ["types.ac", "types.d", "types.interface.ac", "unit.json"]
    assert stage["types.interface.ac"] != before["types.interface.ac"]
    assert b"Alias" in stage["types.interface.ac"]


def test_public_compile_recovers_a_provider_in_a_prepared_transaction(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """Item 15: compile -I recovers the unfinished provider and succeeds."""

    types, counter = _published(cli_environment, workspace)
    before = _interrupt_provider_replace(workspace, point="after_previous_saved")

    result = _compile(
        cli_environment,
        workspace,
        "counter.py",
        interface=(types,),
        output=workspace.units / "counter-recovered",
    )

    _ok(result)
    next_unit = workspace.units / "counter-recovered"
    assert sorted(entry.name for entry in next_unit.iterdir()) == [
        "counter.ac",
        "counter.d",
        "counter.interface.ac",
        "unit.json",
    ]
    target, prerequisites = _depfile(
        (next_unit / "counter.d").read_text(encoding="utf-8")
    )
    assert target == str(next_unit / "counter.ac")
    assert str(types / "types.interface.ac") in prerequisites
    assert str(types / "unit.json") in prerequisites
    assert str(types / "types.ac") not in prerequisites
    # The provider itself was recovered, not copied or left half-published.
    _assert_recovered(workspace, before)


def test_public_link_recovers_a_provider_in_a_prepared_transaction(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """Item 15: link recovers the unfinished provider and succeeds."""

    types, counter = _published(cli_environment, workspace)
    before = _interrupt_provider_replace(workspace, point="after_previous_saved")

    destination = workspace.out / "program.ac"
    _ok(_link(cli_environment, (types, counter), "demo.counter.Counter", destination))

    assert b"ac.initial_value = 0 : i8" in destination.read_bytes()
    _assert_recovered(workspace, before)

    # The recovered closure links exactly like a never-interrupted one.
    reference = workspace.out / "reference.ac"
    _ok(_link(cli_environment, (types, counter), "demo.counter.Counter", reference))
    assert destination.read_bytes() == reference.read_bytes()


@pytest.mark.parametrize("point", ["after_journal_prepared", "after_stage_complete"])
def test_public_commands_recover_the_other_precommit_rollback_points(
    workspace: _Workspace, cli_environment: dict[str, str], point: str
) -> None:
    """Item 16: the equivalent rollback states recover through both commands."""

    types, counter = _published(cli_environment, workspace)
    before = _interrupt_provider_replace(workspace, point=point)
    _assert_interrupted_state(workspace, point=point)

    _ok(
        _compile(
            cli_environment,
            workspace,
            "counter.py",
            interface=(types,),
            output=workspace.units / "counter-recovered",
        )
    )
    _assert_recovered(workspace, before)

    destination = workspace.out / "program.ac"
    _ok(_link(cli_environment, (types, counter), "demo.counter.Counter", destination))
    assert b"ac.initial_value = 0 : i8" in destination.read_bytes()
    _assert_recovered(workspace, before)


def test_public_commands_consume_a_committed_cleanup_pending_provider(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """Item 17: a committed publication whose cleanup is pending is valid."""

    types, counter = _published(cli_environment, workspace)
    _interrupt_provider_replace(workspace, point="after_journal_committed")
    _assert_interrupted_state(workspace, point="after_journal_committed")
    committed = _closed_unit_tree(types)
    # The committed artifact is the new unit, not the pre-replacement one.
    assert b"Alias" in committed["types.interface.ac"]

    _ok(
        _compile(
            cli_environment,
            workspace,
            "counter.py",
            interface=(types,),
            output=workspace.units / "counter-recovered",
        )
    )
    _assert_consumed_committed(workspace, committed)

    destination = workspace.out / "program.ac"
    _ok(_link(cli_environment, (types, counter), "demo.counter.Counter", destination))
    assert b"ac.initial_value = 0 : i8" in destination.read_bytes()
    _assert_consumed_committed(workspace, committed)


def test_header_only_provider_is_consumed_by_compile_but_not_by_link(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """Item 18: only the full validator is strengthened, never the header view."""

    types, counter = _published(cli_environment, workspace)
    (types / "types.ac").unlink()
    (types / "types.d").unlink()
    header_only = _tree(types)
    assert sorted(header_only) == ["types.interface.ac", "unit.json"]

    # compile is satisfied by the managed interface and never opens the body.
    result = _compile(
        cli_environment,
        workspace,
        "counter.py",
        interface=(types,),
        output=workspace.units / "counter-header-only",
    )
    _ok(result)
    next_unit = workspace.units / "counter-header-only"
    target, prerequisites = _depfile(
        (next_unit / "counter.d").read_text(encoding="utf-8")
    )
    assert target == str(next_unit / "counter.ac")
    assert str(types / "types.interface.ac") in prerequisites
    assert str(types / "unit.json") in prerequisites
    assert str(types / "types.ac") not in prerequisites

    # link consumes the full body closure, so the same provider is refused...
    destination = workspace.out / "program.ac"
    failed = _link(cli_environment, (types, counter), "demo.counter.Counter", destination)
    diagnostic = _diagnostic(failed, "link")
    assert _OWNER_MISMATCH not in diagnostic, diagnostic
    _nothing_published(destination)
    # ...and the refusal leaves the provider exactly as it was, unrepaired.
    assert _tree(types) == header_only


# --------------------------------------------------------------------------
# corrupt control state and journal (item 19)
# --------------------------------------------------------------------------


def test_corrupt_journal_is_refused_and_leaves_the_recovery_evidence(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """A corrupt transaction is refused, never mistaken for an absent input.

    The same corruption is applied first to an unfinished transaction whose
    destination still exists, to capture the product's own diagnostic for it,
    and then to the unfinished transaction whose destination was moved aside.
    Both must be refused for the same stated reason, and neither may lose the
    evidence on disk.
    """

    types, counter = _published(cli_environment, workspace)
    corrupt = "{}"

    # Calibration: the same corrupt journal on a transaction whose destination
    # still exists. This state is refused by the file-level validator today.
    calibration = workspace.units / "types-stable"
    _ok(_compile(cli_environment, workspace, "types.py", output=calibration))
    _interrupt_provider_replace(workspace, point="after_journal_prepared", unit="types-stable")
    _assert_interrupted_state(workspace, point="after_journal_prepared", unit="types-stable")
    calibration_journal = _control(calibration) / "journal.json"
    calibration_journal.write_text(corrupt, encoding="utf-8")

    baseline = _compile(
        cli_environment,
        workspace,
        "counter.py",
        interface=(calibration,),
        output=workspace.units / "counter-baseline",
    )
    baseline_diagnostic = _diagnostic(baseline, "compile")
    assert "journal" in baseline_diagnostic.lower(), baseline_diagnostic
    assert calibration_journal.read_text(encoding="utf-8") == corrupt
    _nothing_published(workspace.units / "counter-baseline")

    # The identical corruption in the unfinished transaction: the unit's
    # destination does not exist yet, but the evidence does.
    before = _interrupt_provider_replace(workspace, point="after_previous_saved")
    unit = workspace.units / "types"
    control = _control(unit)
    journal = control / "journal.json"
    original_journal = journal.read_text(encoding="utf-8")
    journal.write_text(corrupt, encoding="utf-8")
    evidence = _tree(control)

    result = _compile(
        cli_environment,
        workspace,
        "counter.py",
        interface=(unit,),
        output=workspace.units / "counter-after-corruption",
    )

    diagnostic = _diagnostic(result, "compile")
    assert diagnostic == baseline_diagnostic, (diagnostic, baseline_diagnostic)
    for missing in _MISSING_INPUT:
        assert missing not in diagnostic, diagnostic
    assert _MISSING_CONTROL not in diagnostic, diagnostic
    assert not unit.exists()
    assert _tree(control) == evidence
    assert journal.read_text(encoding="utf-8") == corrupt
    assert (control / "previous").is_dir()
    assert (control / "stage").is_dir()
    assert _tree(control / "previous") == before

    # The state is still recoverable once the corruption is gone.
    journal.write_text(original_journal, encoding="utf-8")
    _ok(
        _compile(
            cli_environment,
            workspace,
            "counter.py",
            interface=(unit,),
            output=workspace.units / "counter-repaired",
        )
    )
    _assert_recovered(workspace, before)


def test_unknown_control_entry_is_refused_and_leaves_the_recovery_evidence(
    workspace: _Workspace, cli_environment: dict[str, str]
) -> None:
    """A corrupt control state is named, not treated as an unmanaged path."""

    _published(cli_environment, workspace)
    before = _interrupt_provider_replace(workspace, point="after_previous_saved")
    unit = workspace.units / "types"
    control = _control(unit)
    (control / "user-note.txt").write_text("mine\n", encoding="utf-8")
    evidence = _tree(control)

    result = _compile(
        cli_environment,
        workspace,
        "counter.py",
        interface=(unit,),
        output=workspace.units / "counter-after-corruption",
    )

    diagnostic = _diagnostic(result, "compile")
    assert "control" in diagnostic.lower(), diagnostic
    for missing in _MISSING_INPUT:
        assert missing not in diagnostic, diagnostic
    assert _MISSING_CONTROL not in diagnostic, diagnostic
    assert not unit.exists()
    assert _tree(control) == evidence
    assert (control / "user-note.txt").read_text(encoding="utf-8") == "mine\n"
    assert _tree(control / "previous") == before
    _nothing_published(workspace.units / "counter-after-corruption")
