"""Unit tests for the public ``pycircuit compile`` and ``pycircuit link`` commands.

Both native helpers are replaced by deterministic fakes this file writes itself,
so argument validation, the ``--parameters`` rules, the entry-owner report
mapping, publication shape, and every fail-closed diagnostic are exercised
without MLIR or a built toolchain. End-to-end behaviour against the real helpers
lives in ``tests/system/test_driver_compile_link.py``; nothing here duplicates
it.

The commands are driven in-process through ``pycircuit.cli.main`` because these
tests are about the driver's own decisions (what it passes to the helper, what
it accepts, what it prints). The facts an in-process call cannot show honestly -
the real process exit status, the absence of an interpreter traceback, and the
one-line stderr shape for real native failures - are asserted at the subprocess
boundary in the system test.

The oracles are the fakes' own deterministic output (which the driver must copy
byte-for-byte into the documented locations) and the exact publication control
metadata, never "the output exists".
"""

from __future__ import annotations

import contextlib
import io
import json
import sys
from dataclasses import dataclass
from pathlib import Path

import pytest
from pycircuit import _driver, _native_verify
from pycircuit.cli import main

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

# --------------------------------------------------------------------------
# fake native helpers
# --------------------------------------------------------------------------

_COMPILER_HEADER = """\
import json
import os
import sys

arguments = sys.argv[1:]
LOG_PATH = @LOG@
MARKER = @MARKER@


def value(flag):
    return arguments[arguments.index(flag) + 1] if flag in arguments else None


def values(flag):
    return [
        arguments[index + 1]
        for index, item in enumerate(arguments)
        if item == flag
    ]


def write_artifacts():
    package = value("--package")
    path = value("--path")
    open(value("--body-out"), "w", encoding="utf-8").write(
        "// %s body package=%s path=%s\\n" % (MARKER, package, path))
    open(value("--interface-out"), "w", encoding="utf-8").write(
        "// %s interface package=%s path=%s\\n" % (MARKER, package, path))


def write_deps(entries):
    json.dump(entries, open(value("--deps-out"), "w", encoding="utf-8"))


def log(record):
    with open(LOG_PATH, "a", encoding="utf-8") as handle:
        handle.write(json.dumps(record) + "\\n")
"""

_COMPILE_OK = """\
write_artifacts()
write_deps([])
"""

_COMPILE_CONSUMES_TYPES = """\
write_artifacts()
write_deps([{"package": "demo", "path": "types.py"}])
"""

_COMPILE_CONSUMES_UNSUPPLIED = """\
write_artifacts()
write_deps([{"package": "demo", "path": "elsewhere.py"}])
"""

_COMPILE_REJECTS = """\
sys.stderr.write("synthetic native rejection\\n")
sys.exit(3)
"""

_COMPILE_WRITES_NOTHING = """\
sys.exit(0)
"""

_COMPILE_LOGS_INVOCATION = """\
write_artifacts()
write_deps([{"package": "demo", "path": "types.py"}])
log({
    "package": value("--package"),
    "path": value("--path"),
    "capture": value("--capture"),
    "capture_existed": os.path.isfile(value("--capture") or ""),
    "headers": [
        {"path": item, "text": open(item, encoding="utf-8").read()}
        for item in values("--header")
    ],
})
"""

_DESIGN_HEADER = """\
import hashlib
import json
import os
import re
import sys

arguments = sys.argv[1:]
RECORD_PATH = @RECORD@
PRODUCED_PATH = @PRODUCED@


def value(flag):
    return arguments[arguments.index(flag) + 1] if flag in arguments else None


def values(flag):
    return [
        arguments[index + 1]
        for index, item in enumerate(arguments)
        if item == flag
    ]


def read(path):
    return open(path, encoding="utf-8").read()


def write_program(text):
    open(value("--output"), "w", encoding="utf-8").write(text)


def write_report(payload):
    open(value("--entry-owner-out"), "w", encoding="utf-8").write(payload)
    output = value("--output")
    if output is not None and os.path.isfile(output):
        remember(read(output), payload)


def write_record():
    open(RECORD_PATH, "w", encoding="utf-8").write(json.dumps({
        "top": value("--top"),
        "target": value("--target"),
        "output": value("--output"),
        "report": value("--entry-owner-out"),
        "bodies": [
            {"path": item, "text": read(item)} for item in values("--body")
        ],
        "headers": [
            {"path": item, "text": read(item)} for item in values("--header")
        ],
    }))


# ---------------------------------------------------------------------------
# Read-only verification shapes.
#
# The driver verifies an artifact it is about to replace by asking this harness:
#
#   --body B --header H --verify-only --unit-owner-out R
#   --design P --verify-only --entry-owner-out R
#
# Verification here is honest for synthetic artifacts. A unit reports the owner
# its own body/header declares, or the owner its receipt declares when the
# synthetic text carries no attribute, and a unit that declares neither is
# refused exactly like a native verification failure. A program reports the
# owner recorded when this harness produced those exact bytes, so a program
# this harness did not author is refused. Verifying real artifacts against the
# shared native verifier is covered end to end by
# tests/system/test_artifact_verify_recovery.py.
# ---------------------------------------------------------------------------

OWNER_ATTRIBUTE = re.compile(
    r'ac\\.source_owner = \\{package = "([^"]*)", path = "([^"]*)"\\}'
)


def declared_unit_owner(path):
    match = OWNER_ATTRIBUTE.search(read(path))
    if match is not None:
        return {"package": match.group(1), "path": match.group(2)}
    receipt = os.path.join(os.path.dirname(os.path.abspath(path)), "unit.json")
    if not os.path.isfile(receipt):
        return None
    value = json.load(open(receipt, encoding="utf-8"))
    source = value.get("source") if isinstance(value, dict) else None
    if not isinstance(source, dict) or set(source) != {"package", "path"}:
        return None
    return {"package": source["package"], "path": source["path"]}


def produced():
    if not os.path.isfile(PRODUCED_PATH):
        return []
    return json.load(open(PRODUCED_PATH, encoding="utf-8"))


def remember(text, report):
    entries = produced()
    entries.append({
        "sha256": hashlib.sha256(text.encode("utf-8")).hexdigest(),
        "report": report,
    })
    json.dump(entries, open(PRODUCED_PATH, "w", encoding="utf-8"))


def verify_only():
    body = value("--body")
    header = value("--header")
    if body is not None and header is not None:
        owners = [declared_unit_owner(body), declared_unit_owner(header)]
        if owners[0] is None or owners[1] is None:
            sys.stderr.write("error: source unit failed verification\\n")
            sys.exit(1)
        open(value("--unit-owner-out"), "w", encoding="utf-8").write(
            json.dumps({"body": owners[0], "header": owners[1]}))
        sys.exit(0)
    digest = hashlib.sha256(open(value("--design"), "rb").read()).hexdigest()
    for entry in produced():
        if entry["sha256"] == digest:
            open(value("--entry-owner-out"), "w", encoding="utf-8").write(
                entry["report"])
            sys.exit(0)
    sys.stderr.write("error: program failed verification\\n")
    sys.exit(1)


if "--verify-only" in arguments:
    verify_only()
"""

_OWNER_REPORT = (
    '{"source": {"package": "demo", "path": "counter.py"}, '
    '"definition": "@\\"demo.counter.Counter\\""}'
)

_OTHER_OWNER_REPORT = (
    '{"source": {"package": "demo", "path": "other.py"}, '
    '"definition": "@\\"demo.other.Other\\""}'
)


def _executable(path: Path, body: str) -> Path:
    path.write_text(f"#!{sys.executable}\n" + body, encoding="utf-8")
    path.chmod(0o755)
    return path


def _fake_compiler(
    tmp_path: Path,
    behaviour: str,
    *,
    name: str = "fake-source-unit",
    log: Path | None = None,
    marker: str = "synthetic",
) -> Path:
    header = _COMPILER_HEADER.replace(
        "@LOG@", repr(str(log)) if log is not None else "None"
    ).replace("@MARKER@", repr(marker))
    return _executable(tmp_path / name, header + "\n" + behaviour)


def _fake_design(
    tmp_path: Path,
    record: Path,
    behaviour: str,
    *,
    name: str = "fake-design-harness",
) -> Path:
    header = _DESIGN_HEADER.replace("@RECORD@", repr(str(record))).replace(
        "@PRODUCED@", repr(str(tmp_path / f"{name}-produced.json"))
    )
    return _executable(tmp_path / name, header + "\n" + behaviour)


def _design_ok(program: str, report: str, *, record: bool = True) -> str:
    lines = [f"write_program({program!r})", f"write_report({report!r})"]
    if record:
        lines.insert(0, "write_record()")
    return "\n".join(lines) + "\n"


# --------------------------------------------------------------------------
# CLI driver and workspace helpers
# --------------------------------------------------------------------------


@dataclass(frozen=True, slots=True)
class _Cli:
    code: int
    stdout: str
    stderr: str
    argparse_exit: bool

    @property
    def diagnostic(self) -> str:
        lines = self.stderr.splitlines()
        assert len(lines) == 1, f"expected one diagnostic line, got {self.stderr!r}"
        assert "Traceback (most recent call last)" not in self.stderr
        return lines[0]


def _cli(*argv: str) -> _Cli:
    stdout, stderr = io.StringIO(), io.StringIO()
    with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
        try:
            code = main(list(argv))
        except SystemExit as exit_error:  # argparse usage errors
            code = int(exit_error.code or 0)
            raised = True
        else:
            raised = False
    return _Cli(
        code=code,
        stdout=stdout.getvalue(),
        stderr=stderr.getvalue(),
        argparse_exit=raised,
    )


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


def _reject(cli: _Cli, command: str) -> str:
    assert not cli.argparse_exit, cli.stderr
    assert cli.code == 1, (cli.code, cli.stderr)
    assert cli.stdout == ""
    diagnostic = cli.diagnostic
    assert diagnostic.startswith(f"pycircuit {command}: "), diagnostic
    return diagnostic


def _workspace(tmp_path: Path) -> tuple[Path, Path]:
    root = tmp_path / "src"
    root.mkdir()
    units = tmp_path / "units"
    units.mkdir()
    return root, units


def _publish(
    root: Path,
    units: Path,
    name: str,
    *,
    prefix: str = "demo",
    interface: tuple[Path, ...] = (),
    replace: bool = False,
) -> _Cli:
    argv = [
        "compile",
        "-c",
        str(root / name),
        "--source-root",
        str(root),
        "--package-prefix",
        prefix,
        "-o",
        str(units / Path(name).stem),
    ]
    for unit in interface:
        argv += ["-I", str(unit)]
    if replace:
        argv.append("--replace")
    return _cli(*argv)


@pytest.fixture(autouse=True)
def _verification_harness(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    """Every publication now verifies the artifact it installs.

    ``compile`` verifies the unit it publishes and ``link`` verifies the program,
    both through the private design harness, so each test needs one answering
    the read-only verification shapes. Tests that need a specific linked program
    or owner report install their own harness over this one.
    """

    record = tmp_path / "autouse-record.json"
    harness = _fake_design(tmp_path, record, _design_ok("// autouse\n", _OWNER_REPORT))
    monkeypatch.setenv("PYCIRCUIT_LINKER", str(harness))


# --------------------------------------------------------------------------
# compile: published shape
# --------------------------------------------------------------------------


def test_compile_publishes_exactly_the_four_documented_files(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    root, units = _workspace(tmp_path)
    (root / "types.py").write_text(DECLARATION, encoding="utf-8")
    compiler = _fake_compiler(tmp_path, _COMPILE_OK)
    monkeypatch.setenv("PYCIRCUIT_SOURCE_COMPILER", str(compiler))

    cli = _publish(root, units, "types.py")

    assert cli.code == 0, cli.stderr
    assert cli.stderr == "" and cli.stdout == ""
    unit = units / "types"
    assert sorted(entry.name for entry in unit.iterdir()) == [
        "types.ac",
        "types.d",
        "types.interface.ac",
        "unit.json",
    ]
    # The receipt is exactly the documented closed object.
    assert json.loads((unit / "unit.json").read_text(encoding="utf-8")) == {
        "kind": "pycircuit-source-unit",
        "source": {"package": "demo", "path": "types.py"},
        "files": {
            "body": "types.ac",
            "interface": "types.interface.ac",
            "depfile": "types.d",
        },
    }
    # Each artifact carries the fake's own deterministic text, so a body and an
    # interface swapped by the driver would fail here.
    assert (unit / "types.ac").read_text(encoding="utf-8") == (
        "// synthetic body package=demo path=types.py\n"
    )
    assert (unit / "types.interface.ac").read_text(encoding="utf-8") == (
        "// synthetic interface package=demo path=types.py\n"
    )
    # The depfile names the published target, the captured source, and the
    # helper that produced it, and nothing else.
    depfile = (unit / "types.d").read_text(encoding="utf-8")
    target, separator, prerequisites = depfile.rstrip("\n").partition(": ")
    assert separator == ": "
    assert target == str(unit / "types.ac")
    assert prerequisites.split(" ") == [
        str(root / "types.py"),
        str(compiler),
    ]
    # Only the unit directory and its control directory appear beside it.
    assert sorted(entry.name for entry in units.iterdir()) == [
        ".types.pycircuit-publication",
        "types",
    ]


def test_compile_receipt_follows_the_source_file_stem(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """The published stem is the source file's, not the output directory's."""

    root, units = _workspace(tmp_path)
    (root / "counter.py").write_text(COUNTER, encoding="utf-8")
    monkeypatch.setenv(
        "PYCIRCUIT_SOURCE_COMPILER", str(_fake_compiler(tmp_path, _COMPILE_OK))
    )

    cli = _publish(root, units, "counter.py")
    assert cli.code == 0, cli.stderr

    # -o was units/counter, so this case does not discriminate; publish the same
    # source under a differently named destination to prove the stem source.
    renamed = _cli(
        "compile",
        "-c",
        str(root / "counter.py"),
        "--source-root",
        str(root),
        "--package-prefix",
        "demo",
        "-o",
        str(units / "victim"),
    )
    assert renamed.code == 0, renamed.stderr
    assert sorted(entry.name for entry in (units / "victim").iterdir()) == [
        "counter.ac",
        "counter.d",
        "counter.interface.ac",
        "unit.json",
    ]
    receipt = json.loads((units / "victim" / "unit.json").read_text(encoding="utf-8"))
    assert receipt["files"] == {
        "body": "counter.ac",
        "interface": "counter.interface.ac",
        "depfile": "counter.d",
    }


def test_compile_default_package_prefix_is_empty(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    root, units = _workspace(tmp_path)
    (root / "types.py").write_text(DECLARATION, encoding="utf-8")
    monkeypatch.setenv(
        "PYCIRCUIT_SOURCE_COMPILER", str(_fake_compiler(tmp_path, _COMPILE_OK))
    )

    cli = _cli(
        "compile",
        "-c",
        str(root / "types.py"),
        "--source-root",
        str(root),
        "-o",
        str(units / "types"),
    )

    assert cli.code == 0, cli.stderr
    receipt = json.loads((units / "types" / "unit.json").read_text(encoding="utf-8"))
    assert receipt["source"] == {"package": "", "path": "types.py"}


# --------------------------------------------------------------------------
# compile: replace semantics
# --------------------------------------------------------------------------


def test_compile_republishes_only_under_the_same_owner(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    root, units = _workspace(tmp_path)
    (root / "types.py").write_text(DECLARATION, encoding="utf-8")
    first = _fake_compiler(tmp_path, _COMPILE_OK, name="first")
    monkeypatch.setenv("PYCIRCUIT_SOURCE_COMPILER", str(first))
    assert _publish(root, units, "types.py").code == 0
    unit = units / "types"
    before = {entry.name: entry.read_bytes() for entry in unit.iterdir()}

    # An existing destination without --replace fails and changes nothing.
    cli = _publish(root, units, "types.py")
    assert "already exists" in _reject(cli, "compile")
    assert {entry.name: entry.read_bytes() for entry in unit.iterdir()} == before

    # The same owner with --replace republishes.
    changed = _fake_compiler(tmp_path, _COMPILE_OK, name="second", marker="replaced")
    monkeypatch.setenv("PYCIRCUIT_SOURCE_COMPILER", str(changed))
    replaced = _publish(root, units, "types.py", replace=True)
    assert replaced.code == 0, replaced.stderr
    assert (unit / "types.ac").read_text(encoding="utf-8") == (
        "// replaced body package=demo path=types.py\n"
    )

    # A different owner (different package) may not replace this unit.
    other_owner = _publish(root, units, "types.py", prefix="other", replace=True)
    diagnostic = _reject(other_owner, "compile")
    assert "owner" in diagnostic, diagnostic
    assert (unit / "types.ac").read_text(encoding="utf-8") == (
        "// replaced body package=demo path=types.py\n"
    )
    assert json.loads((unit / "unit.json").read_text(encoding="utf-8"))["source"] == {
        "package": "demo",
        "path": "types.py",
    }


# --------------------------------------------------------------------------
# compile: -I interface units
# --------------------------------------------------------------------------


def test_compile_supplies_listed_interfaces_as_snapshot_headers(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    root, units = _workspace(tmp_path)
    (root / "types.py").write_text(DECLARATION, encoding="utf-8")
    (root / "counter.py").write_text(COUNTER, encoding="utf-8")
    provider = _fake_compiler(tmp_path, _COMPILE_OK, name="provider")
    monkeypatch.setenv("PYCIRCUIT_SOURCE_COMPILER", str(provider))
    assert _publish(root, units, "types.py").code == 0

    log = tmp_path / "counter-invocations.jsonl"
    consumer = _fake_compiler(
        tmp_path, _COMPILE_LOGS_INVOCATION, name="consumer", log=log
    )
    monkeypatch.setenv("PYCIRCUIT_SOURCE_COMPILER", str(consumer))

    cli = _publish(root, units, "counter.py", interface=(units / "types",))

    assert cli.code == 0, cli.stderr
    (record,) = [
        json.loads(line) for line in log.read_text(encoding="utf-8").splitlines()
    ]
    assert record["package"] == "demo"
    assert record["path"] == "counter.py"
    assert record["capture_existed"] is True
    (header,) = record["headers"]
    # The provider is read as an in-lock snapshot, not from its published path.
    assert Path(header["path"]).parent != units / "types"
    assert header["text"] == "// synthetic interface package=demo path=types.py\n"
    # The consumed interface and its receipt, never the provider body, are
    # prerequisites of the published artifact.
    depfile = (units / "counter" / "counter.d").read_text(encoding="utf-8")
    assert str(units / "types" / "types.interface.ac") in depfile
    assert str(units / "types" / "unit.json") in depfile
    assert str(units / "types" / "types.ac") not in depfile


def test_compile_rejects_a_consumed_but_unsupplied_interface(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    root, units = _workspace(tmp_path)
    (root / "types.py").write_text(DECLARATION, encoding="utf-8")
    (root / "counter.py").write_text(COUNTER, encoding="utf-8")
    monkeypatch.setenv(
        "PYCIRCUIT_SOURCE_COMPILER", str(_fake_compiler(tmp_path, _COMPILE_OK))
    )
    assert _publish(root, units, "types.py").code == 0

    liar = _fake_compiler(tmp_path, _COMPILE_CONSUMES_UNSUPPLIED, name="liar")
    monkeypatch.setenv("PYCIRCUIT_SOURCE_COMPILER", str(liar))

    cli = _publish(root, units, "counter.py", interface=(units / "types",))

    assert "was not supplied" in _reject(cli, "compile")
    _assert_nothing_published(units / "counter")


def test_compile_rejects_two_interface_units_with_the_same_source(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    root, units = _workspace(tmp_path)
    (root / "types.py").write_text(DECLARATION, encoding="utf-8")
    (root / "counter.py").write_text(COUNTER, encoding="utf-8")
    compiler = _fake_compiler(tmp_path, _COMPILE_OK)
    monkeypatch.setenv("PYCIRCUIT_SOURCE_COMPILER", str(compiler))
    assert _publish(root, units, "types.py").code == 0
    assert _publish(root, units, "types.py").code == 1  # already published
    assert (
        _cli(
            "compile",
            "-c",
            str(root / "types.py"),
            "--source-root",
            str(root),
            "--package-prefix",
            "demo",
            "-o",
            str(units / "types-copy"),
        ).code
        == 0
    )

    cli = _publish(
        root, units, "counter.py", interface=(units / "types", units / "types-copy")
    )

    assert "same source" in _reject(cli, "compile")
    _assert_nothing_published(units / "counter")


@pytest.mark.parametrize("kind", ["missing", "unmanaged"])
def test_compile_rejects_an_interface_unit_that_is_not_published(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, kind: str
) -> None:
    root, units = _workspace(tmp_path)
    (root / "counter.py").write_text(COUNTER, encoding="utf-8")
    monkeypatch.setenv(
        "PYCIRCUIT_SOURCE_COMPILER", str(_fake_compiler(tmp_path, _COMPILE_OK))
    )
    interface = units / kind
    if kind == "unmanaged":
        interface.mkdir()

    cli = _publish(root, units, "counter.py", interface=(interface,))

    _reject(cli, "compile")
    _assert_nothing_published(units / "counter")


# --------------------------------------------------------------------------
# compile: diagnostics
# --------------------------------------------------------------------------


def test_compile_native_rejection_is_one_line_on_stderr(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    root, units = _workspace(tmp_path)
    (root / "counter.py").write_text(COUNTER, encoding="utf-8")
    monkeypatch.setenv(
        "PYCIRCUIT_SOURCE_COMPILER", str(_fake_compiler(tmp_path, _COMPILE_REJECTS))
    )

    cli = _publish(root, units, "counter.py")

    diagnostic = _reject(cli, "compile")
    assert "rejected the source" in diagnostic
    assert "synthetic native rejection" in diagnostic
    _assert_nothing_published(units / "counter")


def test_compile_fails_closed_when_the_helper_produces_no_artifact(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    root, units = _workspace(tmp_path)
    (root / "counter.py").write_text(COUNTER, encoding="utf-8")
    monkeypatch.setenv(
        "PYCIRCUIT_SOURCE_COMPILER",
        str(_fake_compiler(tmp_path, _COMPILE_WRITES_NOTHING)),
    )

    cli = _publish(root, units, "counter.py")

    assert "did not produce both artifacts" in _reject(cli, "compile")
    _assert_nothing_published(units / "counter")


def test_compile_names_the_env_override_when_the_helper_cannot_be_resolved(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    root, units = _workspace(tmp_path)
    (root / "types.py").write_text(DECLARATION, encoding="utf-8")

    # An override that does not name a file is reported with its own name, and
    # it is reported before the source is even captured.
    monkeypatch.setenv("PYCIRCUIT_SOURCE_COMPILER", str(tmp_path / "absent-helper"))
    diagnostic = _reject(_publish(root, units, "types.py"), "compile")
    assert "PYCIRCUIT_SOURCE_COMPILER" in diagnostic, diagnostic
    assert str(tmp_path / "absent-helper") in diagnostic, diagnostic

    # With no override and no bundled toolchain the same name is still the fix.
    # Helper resolution lives in the shared private verification module the
    # driver delegates to, so that is where the bundle lookup is stubbed out.
    monkeypatch.delenv("PYCIRCUIT_SOURCE_COMPILER")
    monkeypatch.setattr(
        _native_verify.packaged_toolchain, "tool_executable", lambda name: None
    )
    diagnostic = _reject(_publish(root, units, "types.py"), "compile")
    assert "PYCIRCUIT_SOURCE_COMPILER" in diagnostic, diagnostic
    assert "pycircuit-source-unit" in diagnostic, diagnostic
    _assert_nothing_published(units / "types")


def test_compile_rejects_a_source_that_escapes_the_source_root(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    root, units = _workspace(tmp_path)
    outside = tmp_path / "outside.py"
    outside.write_text(DECLARATION, encoding="utf-8")
    monkeypatch.setenv(
        "PYCIRCUIT_SOURCE_COMPILER", str(_fake_compiler(tmp_path, _COMPILE_OK))
    )

    cli = _cli(
        "compile",
        "-c",
        str(outside),
        "--source-root",
        str(root),
        "--package-prefix",
        "demo",
        "-o",
        str(units / "outside"),
    )

    assert "escapes source root" in _reject(cli, "compile")
    _assert_nothing_published(units / "outside")


def test_compile_requires_its_documented_arguments() -> None:
    missing_source_root = _cli("compile", "-c", "counter.py", "-o", "out")
    assert missing_source_root.argparse_exit
    assert missing_source_root.code == 2
    assert "--source-root" in missing_source_root.stderr

    missing_source = _cli("compile", "--source-root", ".", "-o", "out")
    assert missing_source.argparse_exit
    assert missing_source.code == 2
    # The approved surface spells this flag `-c`; the parser must require it.
    assert "the following arguments are required: -c" in missing_source.stderr

    missing_output = _cli("compile", "-c", "counter.py", "--source-root", ".")
    assert missing_output.argparse_exit
    assert missing_output.code == 2
    assert "the following arguments are required: -o" in missing_output.stderr


def test_link_requires_its_documented_arguments() -> None:
    missing_top = _cli("link", "unit", "-o", "out.ac")
    assert missing_top.argparse_exit
    assert missing_top.code == 2
    assert "--top" in missing_top.stderr

    missing_output = _cli("link", "unit", "--top", "demo.counter.Counter")
    assert missing_output.argparse_exit
    assert missing_output.code == 2
    # The approved surface spells this flag `-o`; the parser must require it.
    assert "the following arguments are required: -o" in missing_output.stderr


@pytest.mark.parametrize("command", ["compile", "link"])
def test_publication_paths_that_traverse_a_symlink_are_a_diagnostic_not_a_traceback(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, command: str
) -> None:
    root, units = _workspace(tmp_path)
    (root / "types.py").write_text(DECLARATION, encoding="utf-8")
    monkeypatch.setenv(
        "PYCIRCUIT_SOURCE_COMPILER", str(_fake_compiler(tmp_path, _COMPILE_OK))
    )
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(
            _fake_design(
                tmp_path, tmp_path / "record.json", _design_ok("// p\n", _OWNER_REPORT)
            )
        ),
    )
    real = tmp_path / "real"
    real.mkdir()
    alias = tmp_path / "alias"
    alias.symlink_to(real, target_is_directory=True)

    if command == "compile":
        cli = _cli(
            "compile",
            "-c",
            str(root / "types.py"),
            "--source-root",
            str(root),
            "--package-prefix",
            "demo",
            "-o",
            str(alias / "types"),
        )
    else:
        assert _publish(root, units, "types.py").code == 0
        cli = _cli(
            "link",
            str(units / "types"),
            "--top",
            "demo.types.Word",
            "-o",
            str(alias / "program.ac"),
        )

    diagnostic = _reject(cli, command)
    assert "symlink" in diagnostic, diagnostic
    assert list(real.iterdir()) == []


# --------------------------------------------------------------------------
# link: published shape and native hand-off
# --------------------------------------------------------------------------


def _two_units(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> tuple[Path, Path]:
    root, units = _workspace(tmp_path)
    (root / "types.py").write_text(DECLARATION, encoding="utf-8")
    (root / "counter.py").write_text(COUNTER, encoding="utf-8")
    monkeypatch.setenv(
        "PYCIRCUIT_SOURCE_COMPILER", str(_fake_compiler(tmp_path, _COMPILE_OK))
    )
    assert _publish(root, units, "types.py").code == 0
    assert _publish(root, units, "counter.py", interface=(units / "types",)).code == 0
    return root, units


def test_link_publishes_one_program_file_and_the_control_directory(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _, units = _two_units(tmp_path, monkeypatch)
    record = tmp_path / "design-record.json"
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(
            _fake_design(
                tmp_path, record, _design_ok("// linked program\n", _OWNER_REPORT)
            )
        ),
    )
    out = tmp_path / "out"
    out.mkdir()
    destination = out / "counter.program.ac"

    cli = _cli(
        "link",
        str(units / "types"),
        str(units / "counter"),
        "--top",
        "demo.counter.Counter",
        "-o",
        str(destination),
    )

    assert cli.code == 0, cli.stderr
    assert cli.stderr == "" and cli.stdout == ""
    # Exactly the artifact and its control directory: no stage, journal, or
    # scratch file survives a committed publication.
    assert sorted(entry.name for entry in out.iterdir()) == [
        ".counter.program.ac.pycircuit-publication",
        "counter.program.ac",
    ]
    assert destination.read_text(encoding="utf-8") == "// linked program\n"
    control = out / ".counter.program.ac.pycircuit-publication"
    assert sorted(entry.name for entry in control.iterdir()) == ["lock", "owner.json"]
    assert json.loads((control / "owner.json").read_text(encoding="utf-8")) == {
        "kind": "pycircuit-publication-control",
        "destination": "counter.program.ac",
    }


def test_link_publishes_the_helper_bytes_verbatim(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """The driver adjudicates no IR itself: it copies the helper's bytes.

    The harness installed here is the one that produced these bytes, so it
    verifies them as its own artifact; the driver only validates the harness's
    closed owner report and never reads the program as MLIR. That the driver
    refuses a program the shared native verifier rejects is covered end to end
    by ``tests/system/test_artifact_verify_recovery.py``.
    """

    _, units = _two_units(tmp_path, monkeypatch)
    not_mlir = "this is not MLIR at all\n{{{\n"
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(
            _fake_design(
                tmp_path, tmp_path / "record.json", _design_ok(not_mlir, _OWNER_REPORT)
            )
        ),
    )
    destination = tmp_path / "program.ac"

    cli = _cli(
        "link",
        str(units / "types"),
        str(units / "counter"),
        "--top",
        "demo.counter.Counter",
        "-o",
        str(destination),
    )

    assert cli.code == 0, cli.stderr
    assert destination.read_text(encoding="utf-8") == not_mlir


def test_link_replace_refuses_a_program_verified_for_another_root(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """The stand-in verifies content, so a foreign-owner program is refused.

    The harness reports the owner it recorded for those exact bytes. Replacing
    the published program with one that belongs to another root therefore fails
    the owner check and leaves the old bytes alone, exactly as the real helper
    does in ``tests/system/test_artifact_verify_recovery.py``.
    """

    _, units = _two_units(tmp_path, monkeypatch)
    destination = tmp_path / "program.ac"
    first = _cli(
        "link",
        str(units / "types"),
        str(units / "counter"),
        "--top",
        "demo.counter.Counter",
        "-o",
        str(destination),
    )
    assert first.code == 0, first.stderr
    before = destination.read_bytes()

    # The same harness name reuses the same recorded-bytes ledger, so the first
    # program's owner is still known while this harness authors a new program
    # for a different root.
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(
            _fake_design(
                tmp_path,
                tmp_path / "other-record.json",
                _design_ok("// other program\n", _OTHER_OWNER_REPORT),
                name="fake-design-harness",
            )
        ),
    )
    replaced = _cli(
        "link",
        str(units / "types"),
        str(units / "counter"),
        "--top",
        "demo.counter.Counter",
        "-o",
        str(destination),
        "--replace",
    )

    diagnostic = _reject(replaced, "link")
    assert "owner does not match" in diagnostic, diagnostic
    assert destination.read_bytes() == before


def test_link_hands_the_helper_in_lock_snapshot_copies(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _, units = _two_units(tmp_path, monkeypatch)
    record_path = tmp_path / "design-record.json"
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(
            _fake_design(
                tmp_path, record_path, _design_ok("// linked\n", _OWNER_REPORT)
            )
        ),
    )
    destination = tmp_path / "program.ac"

    cli = _cli(
        "link",
        str(units / "types"),
        str(units / "counter"),
        "--top",
        "demo.counter.Counter",
        "-o",
        str(destination),
    )
    assert cli.code == 0, cli.stderr

    record = json.loads(record_path.read_text(encoding="utf-8"))
    assert record["top"] == "demo.counter.Counter"
    assert record["target"] == "final"
    assert record["output"] != str(destination)
    assert record["report"] != str(destination)
    # One body and one header per unit, both scratch copies outside the
    # published directories, paired correctly and copied verbatim.
    published = {
        unit: (
            (units / unit / f"{unit}.interface.ac").read_text(encoding="utf-8"),
            (units / unit / f"{unit}.ac").read_text(encoding="utf-8"),
        )
        for unit in ("types", "counter")
    }
    assert len(record["bodies"]) == len(record["headers"]) == 2
    seen = set()
    for body, header in zip(record["bodies"], record["headers"], strict=True):
        assert Path(body["path"]).parent != units / "types"
        assert Path(header["path"]).parent != units / "counter"
        assert Path(body["path"]).parent == Path(header["path"]).parent
        assert (header["text"], body["text"]) in published.values()
        seen.add((header["text"], body["text"]))
    assert seen == set(published.values())


def test_link_requires_the_helper_to_produce_both_outputs(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _, units = _two_units(tmp_path, monkeypatch)
    destination = tmp_path / "program.ac"

    for label, behaviour in (
        ("program-only", "write_program('// p\\n')"),
        ("report-only", f"write_report({_OWNER_REPORT!r})"),
    ):
        path = _fake_design(
            tmp_path, tmp_path / "record.json", behaviour + "\n", name=label
        )
        monkeypatch.setenv("PYCIRCUIT_LINKER", str(path))
        cli = _cli(
            "link",
            str(units / "types"),
            str(units / "counter"),
            "--top",
            "demo.counter.Counter",
            "-o",
            str(destination),
        )
        assert "no program or owner report" in _reject(cli, "link")
        _assert_nothing_published(destination)


# --------------------------------------------------------------------------
# link: entry owner report
# --------------------------------------------------------------------------


def _published_owners(monkeypatch: pytest.MonkeyPatch) -> list[dict[str, object]]:
    """Record the owner the driver hands to the publication protocol."""

    captured: list[dict[str, object]] = []
    real = _driver._publish_file

    def spy(destination, *, owner, **kwargs):
        captured.append({"destination": str(destination), "owner": owner})
        return real(destination, owner=owner, **kwargs)

    monkeypatch.setattr(_driver, "_publish_file", spy)
    return captured


def test_link_owner_is_the_reported_root_source_not_a_provider(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _, units = _two_units(tmp_path, monkeypatch)
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(
            _fake_design(
                tmp_path,
                tmp_path / "record.json",
                _design_ok("// linked\n", _OWNER_REPORT),
            )
        ),
    )
    captured = _published_owners(monkeypatch)
    destination = tmp_path / "program.ac"

    cli = _cli(
        "link",
        str(units / "types"),
        str(units / "counter"),
        "--top",
        "demo.counter.Counter",
        "-o",
        str(destination),
    )

    assert cli.code == 0, cli.stderr
    assert captured == [
        {
            "destination": str(destination),
            "owner": {
                "kind": "program",
                "source": {"package": "demo", "path": "counter.py"},
                "definition": '@"demo.counter.Counter"',
            },
        }
    ]


@pytest.mark.parametrize(
    ("payload", "expected"),
    [
        (
            '{"source": {"package": "demo", "path": "counter.py"}}',
            "entry owner report is not closed",
        ),
        (
            '{"source": {"package": "demo"}, "definition": "@\\"demo.counter.Counter\\""}',
            "has no source owner",
        ),
        (
            '{"source": {"package": 7, "path": "counter.py"}, '
            '"definition": "@\\"demo.counter.Counter\\""}',
            "entry owner report is not textual",
        ),
        (
            '{"source": {"package": "demo", "path": "../counter.py"}, '
            '"definition": "@\\"demo.counter.Counter\\""}',
            "source path is invalid",
        ),
        (
            '{"source": {"package": "demo", "path": "counter.py"}, '
            '"definition": "demo.counter.Counter"}',
            "definition is not canonical",
        ),
        ("not json at all", "Expecting value"),
    ],
)
def test_link_rejects_a_malformed_entry_owner_report(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    payload: str,
    expected: str,
) -> None:
    _, units = _two_units(tmp_path, monkeypatch)
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(
            _fake_design(
                tmp_path, tmp_path / "record.json", _design_ok("// linked\n", payload)
            )
        ),
    )
    destination = tmp_path / "program.ac"

    cli = _cli(
        "link",
        str(units / "types"),
        str(units / "counter"),
        "--top",
        "demo.counter.Counter",
        "-o",
        str(destination),
    )

    diagnostic = _reject(cli, "link")
    assert expected in diagnostic, diagnostic
    _assert_nothing_published(destination)


# --------------------------------------------------------------------------
# link: unit arguments and destination
# --------------------------------------------------------------------------


@pytest.mark.parametrize("unit_kind", ["file", "absent"])
def test_link_rejects_a_unit_argument_that_is_not_a_directory(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, unit_kind: str
) -> None:
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(
            _fake_design(
                tmp_path, tmp_path / "record.json", _design_ok("// p\n", _OWNER_REPORT)
            )
        ),
    )
    unit = tmp_path / "unit"
    if unit_kind == "file":
        unit.write_text("not a directory\n", encoding="utf-8")
    destination = tmp_path / "program.ac"

    cli = _cli(
        "link", str(unit), "--top", "demo.counter.Counter", "-o", str(destination)
    )

    diagnostic = _reject(cli, "link")
    assert "is not a directory" in diagnostic, diagnostic
    # The private receipt reader reports any read failure as an encoding
    # problem; the public command must name the real cause first.
    assert "UTF-8" not in diagnostic and "encoding" not in diagnostic
    _assert_nothing_published(destination)


def test_link_rejects_a_directory_that_is_not_a_published_unit(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(
            _fake_design(
                tmp_path, tmp_path / "record.json", _design_ok("// p\n", _OWNER_REPORT)
            )
        ),
    )
    unit = tmp_path / "unmanaged"
    unit.mkdir()
    destination = tmp_path / "program.ac"

    cli = _cli(
        "link", str(unit), "--top", "demo.counter.Counter", "-o", str(destination)
    )

    diagnostic = _reject(cli, "link")
    assert "not a published source unit" in diagnostic, diagnostic
    _assert_nothing_published(destination)


def test_link_rejects_two_units_declaring_the_same_source_before_linking(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    root, units = _workspace(tmp_path)
    (root / "types.py").write_text(DECLARATION, encoding="utf-8")
    monkeypatch.setenv(
        "PYCIRCUIT_SOURCE_COMPILER", str(_fake_compiler(tmp_path, _COMPILE_OK))
    )
    assert _publish(root, units, "types.py").code == 0
    assert (
        _cli(
            "compile",
            "-c",
            str(root / "types.py"),
            "--source-root",
            str(root),
            "--package-prefix",
            "demo",
            "-o",
            str(units / "types-again"),
        ).code
        == 0
    )
    record = tmp_path / "design-record.json"
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(_fake_design(tmp_path, record, _design_ok("// p\n", _OWNER_REPORT))),
    )
    destination = tmp_path / "program.ac"

    cli = _cli(
        "link",
        str(units / "types"),
        str(units / "types-again"),
        "--top",
        "demo.types.Word",
        "-o",
        str(destination),
    )

    assert "same source" in _reject(cli, "link")
    # Both units were inspected before the linker was ever invoked.
    assert not record.exists()
    _assert_nothing_published(destination)


def test_link_requires_an_existing_destination_parent(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _, units = _two_units(tmp_path, monkeypatch)
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(
            _fake_design(
                tmp_path, tmp_path / "record.json", _design_ok("// p\n", _OWNER_REPORT)
            )
        ),
    )
    destination = tmp_path / "absent" / "program.ac"

    cli = _cli(
        "link",
        str(units / "types"),
        str(units / "counter"),
        "--top",
        "demo.counter.Counter",
        "-o",
        str(destination),
    )

    assert "parent does not exist" in _reject(cli, "link")
    assert not (tmp_path / "absent").exists()


def test_link_rejects_a_destination_that_is_a_directory(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _, units = _two_units(tmp_path, monkeypatch)
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(
            _fake_design(
                tmp_path, tmp_path / "record.json", _design_ok("// p\n", _OWNER_REPORT)
            )
        ),
    )
    destination = tmp_path / "out"
    destination.mkdir()

    cli = _cli(
        "link",
        str(units / "types"),
        str(units / "counter"),
        "--top",
        "demo.counter.Counter",
        "-o",
        str(destination),
    )

    diagnostic = _reject(cli, "link")
    assert "expected file" in diagnostic, diagnostic
    assert list(destination.iterdir()) == []


def test_link_requires_at_least_one_unit(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(
            _fake_design(
                tmp_path, tmp_path / "record.json", _design_ok("// p\n", _OWNER_REPORT)
            )
        ),
    )

    with pytest.raises(_driver._DriverError, match="at least one"):
        _driver.link_command(units=[], top="demo.types.Word", output=tmp_path / "p.ac")


# --------------------------------------------------------------------------
# link: --parameters
# --------------------------------------------------------------------------


def test_link_rejects_non_empty_parameter_bindings_before_any_other_work(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """Fail-closed must win over every other input problem."""

    bindings = tmp_path / "bindings.json"
    bindings.write_text('[{"name": "Width", "value": 8}]', encoding="utf-8")
    monkeypatch.setenv("PYCIRCUIT_LINKER", str(tmp_path / "also-absent"))

    cli = _cli(
        "link",
        str(tmp_path / "no-such-unit"),
        "--top",
        "demo.types.Word",
        "--parameters",
        str(bindings),
        "-o",
        str(tmp_path / "program.ac"),
    )

    diagnostic = _reject(cli, "link")
    assert "static parameter bindings are not implemented yet" in diagnostic, diagnostic
    assert not (tmp_path / "program.ac").exists()


@pytest.mark.parametrize(
    ("text", "expected"),
    [
        ("{}", "must be an ordered JSON array"),
        ('"bindings"', "must be an ordered JSON array"),
        ("3", "must be an ordered JSON array"),
        ("null", "must be an ordered JSON array"),
        ("[1,", "not valid JSON"),
        ("", "not valid JSON"),
    ],
)
def test_link_rejects_parameters_that_are_not_an_empty_array(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    text: str,
    expected: str,
) -> None:
    _, units = _two_units(tmp_path, monkeypatch)
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(
            _fake_design(
                tmp_path, tmp_path / "record.json", _design_ok("// p\n", _OWNER_REPORT)
            )
        ),
    )
    bindings = tmp_path / "bindings.json"
    bindings.write_text(text, encoding="utf-8")
    destination = tmp_path / "program.ac"

    cli = _cli(
        "link",
        str(units / "types"),
        str(units / "counter"),
        "--top",
        "demo.counter.Counter",
        "--parameters",
        str(bindings),
        "-o",
        str(destination),
    )

    diagnostic = _reject(cli, "link")
    assert expected in diagnostic, diagnostic
    _assert_nothing_published(destination)


def test_link_accepts_omitted_and_empty_parameter_bindings(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _, units = _two_units(tmp_path, monkeypatch)
    monkeypatch.setenv(
        "PYCIRCUIT_LINKER",
        str(
            _fake_design(
                tmp_path,
                tmp_path / "record.json",
                _design_ok("// linked\n", _OWNER_REPORT),
            )
        ),
    )
    bindings = tmp_path / "bindings.json"
    bindings.write_text("[]", encoding="utf-8")
    omitted = tmp_path / "omitted.ac"
    explicit = tmp_path / "explicit.ac"

    first = _cli(
        "link",
        str(units / "types"),
        str(units / "counter"),
        "--top",
        "demo.counter.Counter",
        "-o",
        str(omitted),
    )
    second = _cli(
        "link",
        str(units / "types"),
        str(units / "counter"),
        "--top",
        "demo.counter.Counter",
        "--parameters",
        str(bindings),
        "-o",
        str(explicit),
    )

    assert first.code == 0, first.stderr
    assert second.code == 0, second.stderr
    assert omitted.read_bytes() == explicit.read_bytes() == b"// linked\n"
