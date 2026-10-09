"""Unit tests for the private per-source compile orchestration.

These tests use a deterministic fake native compiler so orchestration behaviour
(locking, snapshotting, publication, receipt, depfile, failure handling) is
exercised without depending on MLIR. End-to-end behaviour with the real native
compiler is covered by ``tests/system/test_source_compile_publication.py``.
"""

from __future__ import annotations

import json
import stat
import sys
from pathlib import Path

import pytest
from pycircuit._publication import _PublicationError
from pycircuit._source_compile import _compile_source_unit
from pycircuit._source_unit_files import _load_full_source_unit

PREFIX = """\
import sys

arguments = sys.argv[1:]


def value(flag):
    return arguments[arguments.index(flag) + 1] if flag in arguments else None


"""

WRITES_ARTIFACTS = """\
body = value("--body-out")
interface = value("--interface-out")
package = value("--package")
source = value("--path")
owner = 'ac.source_owner = {package = "%s", path = "%s"}' % (package, source)
open(body, "w", encoding="utf-8").write("// body " + owner + "\\n")
open(interface, "w", encoding="utf-8").write("// interface " + owner + "\\n")
"""

OK = (
    WRITES_ARTIFACTS
    + """\
open(value("--deps-out"), "w", encoding="utf-8").write("[]")
"""
)

CONSUMES_TYPES = (
    WRITES_ARTIFACTS
    + """\
open(value("--deps-out"), "w", encoding="utf-8").write(
    '[{"package": "demo", "path": "types.py"}]')
"""
)

CONSUMES_UNSUPPLIED = (
    WRITES_ARTIFACTS
    + """\
open(value("--deps-out"), "w", encoding="utf-8").write(
    '[{"package": "demo", "path": "elsewhere.py"}]')
"""
)

EXITS_FAILING = """\
sys.stderr.write("native compiler rejected the source\\n")
sys.exit(1)
"""

WRITES_NOTHING = """\
sys.exit(0)
"""

# The body and interface declare an owner the receipt does not: publication must
# refuse it, which also proves the fake design harness reports what the artifact
# under test declares rather than confirming everything it is shown.
WRITES_FOREIGN_OWNER = """\
owner = 'ac.source_owner = {package = "elsewhere", path = "%s"}' % value("--path")
open(value("--body-out"), "w", encoding="utf-8").write("// body " + owner + "\\n")
open(value("--interface-out"), "w", encoding="utf-8").write(
    "// interface " + owner + "\\n")
open(value("--deps-out"), "w", encoding="utf-8").write("[]")
"""

DECLARATION = """\
from typing import Annotated

Word = Annotated[int, range(256)]
"""

LEAF = """\
from pycircuit import module, rule
from .types import Word

@module
def Counter():
    state: Word = 254

    @rule
    def tick():
        nonlocal state
        state = (state + 1) & 255

    tick()
"""


def _fake_compiler(tmp_path: Path, behaviour: str, name: str = "fake-native") -> Path:
    compiler = tmp_path / name
    compiler.write_text(f"#!{sys.executable}\n" + PREFIX + behaviour, encoding="utf-8")
    compiler.chmod(compiler.stat().st_mode | stat.S_IEXEC)
    return compiler


# The private design harness gained two read-only verification shapes; the unit
# shape is the one this file reaches, because publishing a source unit verifies
# the body and header it is about to install. This stand-in answers that shape
# with the owner each synthetic artifact declares, in the same closed report the
# real helper writes, and it refuses an artifact that declares no owner exactly
# like a native verification failure. Native verification of real artifacts is
# covered end to end by tests/system/test_artifact_verify_recovery.py.
_FAKE_DESIGN = r'''\
import json
import re
import sys

arguments = sys.argv[1:]


def value(flag):
    return arguments[arguments.index(flag) + 1] if flag in arguments else None


OWNER = re.compile(
    r'ac\.source_owner = \{package = "([^"]*)", path = "([^"]*)"\}'
)


def declared(path):
    """The owner the synthetic artifact at ``path`` declares, or None."""

    match = OWNER.search(open(path, encoding="utf-8").read())
    if match is None:
        return None
    return {"package": match.group(1), "path": match.group(2)}


body = declared(value("--body"))
header = declared(value("--header"))
if "--verify-only" not in arguments or body is None or header is None:
    sys.stderr.write("error: source unit failed verification\n")
    sys.exit(1)
open(value("--unit-owner-out"), "w", encoding="utf-8").write(
    json.dumps({"body": body, "header": header}))
'''


def _fake_design_harness(tmp_path: Path) -> Path:
    harness = tmp_path / "fake-design-harness"
    harness.write_text(f"#!{sys.executable}\n" + _FAKE_DESIGN, encoding="utf-8")
    harness.chmod(harness.stat().st_mode | stat.S_IEXEC)
    return harness


@pytest.fixture(autouse=True)
def _design_harness(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    """Every publication in this file verifies its unit through the helper."""

    monkeypatch.setenv("PYCIRCUIT_LINKER", str(_fake_design_harness(tmp_path)))


def _workspace(tmp_path: Path) -> tuple[Path, Path]:
    root = tmp_path / "src"
    root.mkdir()
    (root / "types.py").write_text(DECLARATION, encoding="utf-8")
    (root / "counter.py").write_text(LEAF, encoding="utf-8")
    units = tmp_path / "units"
    units.mkdir()
    return root, units


def _publish_types(
    root: Path,
    units: Path,
    compiler: Path,
    name: str = "types",
    *,
    replace: bool = False,
):
    return _compile_source_unit(
        root / "types.py",
        source_root=root,
        package="demo",
        native_compiler=compiler,
        output=units / name,
        replace=replace,
    )


def test_declaration_unit_publishes_the_closed_four_file_set(tmp_path: Path) -> None:
    root, units = _workspace(tmp_path)
    result = _publish_types(root, units, _fake_compiler(tmp_path, OK))

    assert sorted(entry.name for entry in (units / "types").iterdir()) == [
        "types.ac",
        "types.d",
        "types.interface.ac",
        "unit.json",
    ]
    assert (result.body, result.interface, result.depfile) == (
        "types.ac",
        "types.interface.ac",
        "types.d",
    )
    assert json.loads((units / "types" / "unit.json").read_text(encoding="utf-8")) == {
        "kind": "pycircuit-source-unit",
        "source": {"package": "demo", "path": "types.py"},
        "files": {
            "body": "types.ac",
            "interface": "types.interface.ac",
            "depfile": "types.d",
        },
    }
    assert (
        "ac.source_owner"
        in _load_full_source_unit(units / "types", owner=result.owner).body
    )


def test_depfile_lists_only_consumed_interfaces(tmp_path: Path) -> None:
    root, units = _workspace(tmp_path)
    _publish_types(root, units, _fake_compiler(tmp_path, OK, "c1"))
    (root / "extra.py").write_text(DECLARATION, encoding="utf-8")
    _compile_source_unit(
        root / "extra.py",
        source_root=root,
        package="demo",
        native_compiler=_fake_compiler(tmp_path, OK, "c2"),
        output=units / "extra",
    )

    _compile_source_unit(
        root / "counter.py",
        source_root=root,
        package="demo",
        native_compiler=_fake_compiler(tmp_path, CONSUMES_TYPES, "c3"),
        output=units / "counter",
        interface_units=[units / "types", units / "extra"],
    )

    depfile = (units / "counter" / "counter.d").read_text(encoding="utf-8")
    assert depfile.startswith(f"{(units / 'counter' / 'counter.ac')}: ")
    assert str(root / "counter.py") in depfile
    assert str(units / "types" / "types.interface.ac") in depfile
    assert str(units / "types" / "unit.json") in depfile
    # the supplied but unconsumed unit and every scratch input stay out
    assert "extra" not in depfile
    assert "transport" not in depfile and "header_" not in depfile
    assert depfile.endswith("\n")


def test_depfile_escapes_make_special_characters(tmp_path: Path) -> None:
    root = tmp_path / "src dir#1$"
    root.mkdir()
    (root / "counter.py").write_text(LEAF, encoding="utf-8")
    units = tmp_path / "units"
    units.mkdir()

    _compile_source_unit(
        root / "counter.py",
        source_root=root,
        package="demo",
        native_compiler=_fake_compiler(tmp_path, OK),
        output=units / "counter",
    )

    depfile = (units / "counter" / "counter.d").read_text(encoding="utf-8")
    assert "src\\ dir\\#1$$" in depfile, depfile


def test_native_failure_publishes_nothing_and_preserves_existing_bytes(
    tmp_path: Path,
) -> None:
    root, units = _workspace(tmp_path)
    _publish_types(root, units, _fake_compiler(tmp_path, OK, "good"))
    before = {entry.name: entry.read_bytes() for entry in (units / "types").iterdir()}

    with pytest.raises(_PublicationError, match="rejected the source"):
        _compile_source_unit(
            root / "types.py",
            source_root=root,
            package="demo",
            native_compiler=_fake_compiler(tmp_path, EXITS_FAILING, "bad"),
            output=units / "types",
            replace=True,
        )

    assert {
        entry.name: entry.read_bytes() for entry in (units / "types").iterdir()
    } == before


def test_native_compiler_must_produce_both_artifacts(tmp_path: Path) -> None:
    root, units = _workspace(tmp_path)
    with pytest.raises(_PublicationError, match="did not produce both artifacts"):
        _compile_source_unit(
            root / "types.py",
            source_root=root,
            package="demo",
            native_compiler=_fake_compiler(tmp_path, WRITES_NOTHING),
            output=units / "types",
        )
    assert not (units / "types").exists()


def test_existing_output_requires_replace(tmp_path: Path) -> None:
    root, units = _workspace(tmp_path)
    _publish_types(root, units, _fake_compiler(tmp_path, OK, "first"))

    with pytest.raises(_PublicationError, match="already exists"):
        _publish_types(root, units, _fake_compiler(tmp_path, OK, "second"))

    replaced = _publish_types(
        root,
        units,
        _fake_compiler(
            tmp_path, OK.replace('"// body "', '"// replaced body "'), "third"
        ),
        replace=True,
    )
    assert "replaced body" in (units / "types" / replaced.body).read_text(
        encoding="utf-8"
    )


def test_duplicate_interface_units_are_rejected(tmp_path: Path) -> None:
    """Two managed units declaring the same source owner are ambiguous."""
    root, units = _workspace(tmp_path)
    compiler = _fake_compiler(tmp_path, OK)
    _publish_types(root, units, compiler, "types-a")
    _publish_types(root, units, compiler, "types-b")

    with pytest.raises(_PublicationError, match="same source"):
        _compile_source_unit(
            root / "counter.py",
            source_root=root,
            package="demo",
            native_compiler=compiler,
            output=units / "counter",
            interface_units=[units / "types-a", units / "types-b"],
        )


def test_publication_refuses_a_unit_whose_internal_owner_disagrees(
    tmp_path: Path,
) -> None:
    """Publishing verifies the unit, and the verifier is asked what it declares.

    The fake compiler writes a body and interface that declare ``elsewhere``
    while the driver's receipt says ``demo``: the stand-in harness reports the
    declared owner, so the disagreement is refused and nothing is installed.
    """

    root, units = _workspace(tmp_path)

    with pytest.raises(_PublicationError, match="internal owner does not match"):
        _compile_source_unit(
            root / "types.py",
            source_root=root,
            package="demo",
            native_compiler=_fake_compiler(tmp_path, WRITES_FOREIGN_OWNER),
            output=units / "types",
        )

    assert not (units / "types").exists()


def test_unmanaged_interface_unit_is_rejected(tmp_path: Path) -> None:
    root, units = _workspace(tmp_path)
    unmanaged = units / "loose"
    unmanaged.mkdir()
    (unmanaged / "unit.json").write_text(
        json.dumps(
            {
                "kind": "pycircuit-source-unit",
                "source": {"package": "demo", "path": "types.py"},
                "files": {
                    "body": "types.ac",
                    "interface": "types.interface.ac",
                    "depfile": "types.d",
                },
            }
        ),
        encoding="utf-8",
    )
    (unmanaged / "types.interface.ac").write_text("// loose\n", encoding="utf-8")

    with pytest.raises(_PublicationError, match="unmanaged publication input"):
        _compile_source_unit(
            root / "counter.py",
            source_root=root,
            package="demo",
            native_compiler=_fake_compiler(tmp_path, OK),
            output=units / "counter",
            interface_units=[unmanaged],
        )


def test_consumed_but_unsupplied_interface_fails_closed(tmp_path: Path) -> None:
    root, units = _workspace(tmp_path)
    _publish_types(root, units, _fake_compiler(tmp_path, OK, "publisher"))

    with pytest.raises(_PublicationError, match="was not supplied"):
        _compile_source_unit(
            root / "counter.py",
            source_root=root,
            package="demo",
            native_compiler=_fake_compiler(tmp_path, CONSUMES_UNSUPPLIED, "liar"),
            output=units / "counter",
            interface_units=[units / "types"],
        )
    assert not (units / "counter").exists()


def test_header_only_provider_is_accepted_while_full_read_rejects_it(
    tmp_path: Path,
) -> None:
    """Normal compilation consumes a header-only provider; the full validator is
    reserved for recovery and still rejects an incomplete artifact."""
    root, units = _workspace(tmp_path)
    compiler = _fake_compiler(tmp_path, OK)
    published = _publish_types(root, units, compiler)

    (units / "types" / published.body).unlink()
    (units / "types" / published.depfile).unlink()
    with pytest.raises(_PublicationError, match="not closed"):
        _load_full_source_unit(units / "types", owner=published.owner)

    dependent = _compile_source_unit(
        root / "counter.py",
        source_root=root,
        package="demo",
        native_compiler=compiler,
        output=units / "counter",
        interface_units=[units / "types"],
    )
    assert (units / "counter" / dependent.body).is_file()


def test_each_call_invokes_the_compiler_once_for_exactly_one_source(
    tmp_path: Path,
) -> None:
    """One call compiles one source: the native compiler is invoked once per
    call, with the relative path of that call's source and nothing else."""
    root, units = _workspace(tmp_path)
    log = tmp_path / "invocations.log"
    behaviour = (
        WRITES_ARTIFACTS
        + f'\nopen({str(log)!r}, "a", encoding="utf-8").write(value("--path") + "\\n")'
        + '\nopen(value("--deps-out"), "w", encoding="utf-8").write("[]")'
    )
    compiler = _fake_compiler(tmp_path, behaviour)

    _publish_types(root, units, compiler)
    _compile_source_unit(
        root / "counter.py",
        source_root=root,
        package="demo",
        native_compiler=compiler,
        output=units / "counter",
        interface_units=[units / "types"],
    )

    assert log.read_text(encoding="utf-8").splitlines() == ["types.py", "counter.py"]
