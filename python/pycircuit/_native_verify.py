"""Read-only native verification of already published artifacts.

The driver never decides by itself whether an artifact is valid IR. It asks the
private helper, which runs the same native verifier the link and emit paths use,
and it compares the owners the helper reports with the owners it was given.
Python still parses no MLIR: it only validates the helper's closed JSON report.
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

from . import packaged_toolchain
from ._publication import _PublicationError

_HELPERS: dict[str, tuple[str, str]] = {
    "source-unit": ("pycircuit-source-unit", "PYCIRCUIT_SOURCE_COMPILER"),
    "design": ("pycircuit-link", "PYCIRCUIT_LINKER"),
    "emit": ("pycircuit-emit", "PYCIRCUIT_EMITTER"),
}
# Private pycircuit-link status; not exposed as a pycircuit CLI exit code.
_SOURCE_UNIT_OWNER_MISMATCH_EXIT = 3


def native_helper(kind: str) -> Path:
    """Resolve one private native helper, or fail with the exact override."""

    name, variable = _HELPERS[kind]
    override = os.environ.get(variable)
    if override:
        path = Path(override)
        if not path.is_file():
            raise _PublicationError(f"{variable} does not name a file: {path}")
        return path
    bundled = packaged_toolchain.tool_executable(name)
    if bundled is not None:
        return bundled
    raise _PublicationError(
        f"cannot locate `{name}`: the bundled toolchain is not installed in "
        f"this tree and {variable} is not set"
    )


def _strict_object(value: object, fields: set[str], what: str) -> dict[str, object]:
    if type(value) is not dict or set(value) != fields:
        raise _PublicationError(f"{what} report fields are not closed")
    return value


def _owner_object(value: object, what: str) -> dict[str, str]:
    owner = _strict_object(value, {"package", "path"}, what)
    package = owner["package"]
    path = owner["path"]
    if type(package) is not str or type(path) is not str:
        raise _PublicationError(f"{what} report is not textual")
    return {"package": package, "path": path}


def _run(
    arguments: list[str],
    what: str,
    *,
    source_unit_owner_mismatch_exit: bool = False,
) -> None:
    completed = subprocess.run(arguments, text=True, capture_output=True, check=False)
    if completed.returncode != 0:
        if (
            source_unit_owner_mismatch_exit
            and completed.returncode == _SOURCE_UNIT_OWNER_MISMATCH_EXIT
        ):
            raise _PublicationError(
                "source-unit internal owner does not match its receipt"
            )
        # The native tools report over several lines; the driver's diagnostic
        # contract is one line, so the detail is folded into one.
        detail = " ".join(
            (completed.stderr.strip() or completed.stdout.strip()).split()
        )
        raise _PublicationError(f"{what} failed native verification: {detail}")


def verify_program_owner(path: Path) -> dict[str, object]:
    """Return the publication owner of a natively verified program artifact."""

    helper = native_helper("design")
    scratch = Path(tempfile.mkdtemp(prefix="pycircuit-verify-")).resolve()
    try:
        report = scratch / "entry-owner.json"
        _run(
            [
                str(helper),
                "--design",
                str(path),
                "--verify-only",
                "--entry-owner-out",
                str(report),
            ],
            "published program",
        )
        if not report.is_file():
            raise _PublicationError(
                "published program verification produced no owner report"
            )
        value = _strict_object(
            json.loads(report.read_text(encoding="utf-8")),
            {"source", "definition"},
            "published program owner",
        )
    finally:
        shutil.rmtree(scratch, ignore_errors=True)
    source = _owner_object(value["source"], "published program owner")
    definition = value["definition"]
    if type(definition) is not str:
        raise _PublicationError("published program owner report is not textual")
    return {
        "kind": "program",
        "source": {"package": source["package"], "path": source["path"]},
        "definition": definition,
    }


def verify_source_unit_owners(
    body: Path, header: Path
) -> tuple[dict[str, str], dict[str, str]]:
    """Return the body and header owners of a natively verified source unit."""

    helper = native_helper("design")
    scratch = Path(tempfile.mkdtemp(prefix="pycircuit-verify-")).resolve()
    try:
        report = scratch / "unit-owner.json"
        _run(
            [
                str(helper),
                "--body",
                str(body),
                "--header",
                str(header),
                "--verify-only",
                "--unit-owner-out",
                str(report),
            ],
            "published source unit",
            source_unit_owner_mismatch_exit=True,
        )
        if not report.is_file():
            raise _PublicationError("source unit verification produced no owner report")
        value = _strict_object(
            json.loads(report.read_text(encoding="utf-8")),
            {"body", "header"},
            "source unit owner",
        )
    finally:
        shutil.rmtree(scratch, ignore_errors=True)
    return (
        _owner_object(value["body"], "source unit body owner"),
        _owner_object(value["header"], "source unit header owner"),
    )
