"""The implementation owns provenance when its declaration is imported first."""

from pathlib import Path

import pytest
from agentic_circuit._capture_worker import _flatten_source_closure
from agentic_circuit._source_closure import SourceClosure, SourceClosureEntry


def test_entry_body_wins_same_name_declaration_provenance(tmp_path: Path) -> None:
    declaration = tmp_path / "pkg" / "aa_interface.py"
    implementation = tmp_path / "pkg" / "zz_module.py"
    declaration_source = """\
import agentic_circuit as ac

# ndf:requires DAV-SPE-TOP-ASM-2001
@ac.module_decl(source="pkg/zz_module.py")
def unit() -> None:
    ...
"""
    implementation_source = """\
import agentic_circuit as ac
from pkg.aa_interface import unit

# ndf: DAV-SPE-TOP-ASM-2001
@ac.module(declaration=unit)
def unit() -> None:
    pass
"""
    entries = (
        SourceClosureEntry(
            "pkg/aa_interface.py", str(declaration), declaration_source
        ),
        SourceClosureEntry(
            "pkg/zz_module.py", str(implementation), implementation_source
        ),
    )
    for order in (entries, tuple(reversed(entries))):
        _, ndf, locations, nodes = _flatten_source_closure(
            SourceClosure(entries=order), implementation
        )
        assert ndf["unit"].ids == ("DAV-SPE-TOP-ASM-2001",)
        assert ndf["unit"].requires == ()
        assert locations["unit"][0] == "pkg/zz_module.py"
        assert nodes["unit"][0][1].file == "pkg/zz_module.py"


def test_conflicting_dependency_declarations_still_fail(tmp_path: Path) -> None:
    entry = tmp_path / "pkg" / "entry.py"
    closure = SourceClosure(
        entries=(
            SourceClosureEntry(
                "pkg/aa.py",
                str(tmp_path / "pkg" / "aa.py"),
                "# ndf: DAV-ONE-TWO-0001\n@ac.module_decl(source='pkg/unit.py')\n"
                "def unit(): ...\n",
            ),
            SourceClosureEntry(
                "pkg/bb.py",
                str(tmp_path / "pkg" / "bb.py"),
                "# ndf: DAV-ONE-TWO-0002\n@ac.module_decl(source='pkg/unit.py')\n"
                "def unit(): ...\n",
            ),
            SourceClosureEntry("pkg/entry.py", str(entry), "pass\n"),
        )
    )
    with pytest.raises(ValueError, match="ambiguous NDF metadata"):
        _flatten_source_closure(closure, entry)


def test_source_unit_keeps_only_reachable_imported_helper_closure(
    tmp_path: Path,
) -> None:
    helper = tmp_path / "pkg" / "helpers.py"
    interface = tmp_path / "pkg" / "interface.py"
    implementation = tmp_path / "pkg" / "unit.py"
    helper_source = """\
import agentic_circuit as ac

def helper_leaf(value: ac.u8) -> ac.u8:
    return value + 1

def helper_reachable(value: ac.u8) -> ac.u8:
    return helper_leaf(value)

def helper_unreachable(value):
    return forbidden_host_call(value)
"""
    interface_source = """\
import agentic_circuit as ac

@ac.module_decl(source="pkg/unit.py")
def unit(value: ac.u8) -> ac.u8:
    ...
"""
    implementation_source = """\
import agentic_circuit as ac
from pkg.helpers import helper_reachable, helper_unreachable
from pkg.interface import unit

@ac.module
def unit(value: ac.u8) -> ac.u8:
    return helper_reachable(value)

@ac.rule
def unused(value: ac.u8) -> ac.u8:
    return helper_unreachable(value)
"""
    closure = SourceClosure(
        entries=(
            SourceClosureEntry("pkg/helpers.py", str(helper), helper_source),
            SourceClosureEntry(
                "pkg/interface.py", str(interface), interface_source
            ),
            SourceClosureEntry(
                "pkg/unit.py", str(implementation), implementation_source
            ),
        )
    )
    source, _, locations, nodes = _flatten_source_closure(
        closure, implementation, "unit"
    )

    assert "def helper_reachable(" in source
    assert "def helper_leaf(" in source
    assert "def helper_unreachable(" not in source
    assert locations["helper_reachable"][0] == "pkg/helpers.py"
    assert locations["helper_leaf"][0] == "pkg/helpers.py"
    assert nodes["helper_reachable"][0][1].file == "pkg/helpers.py"
    assert nodes["helper_leaf"][0][1].file == "pkg/helpers.py"
