"""Safety invariants for the shared Ninja-output directory preparer."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
PREPARER = ROOT / "cmake/prepare_output.py"


def _run(destination: Path) -> subprocess.CompletedProcess[str]:
    # The installed helper is independent of any design's local Python files.
    return subprocess.run(
        [sys.executable, str(PREPARER), str(destination)],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


def _tree(path: Path) -> dict[str, tuple[str, bytes | str]]:
    tree = {}
    for item in sorted(path.rglob("*")):
        relative = item.relative_to(path).as_posix()
        if item.is_symlink():
            tree[relative] = ("symlink", os.readlink(item))
        elif item.is_file():
            tree[relative] = ("file", item.read_bytes())
        elif item.is_dir():
            tree[relative] = ("directory", b"")
    return tree


def test_empty_nested_output_tree_is_pruned(tmp_path: Path) -> None:
    destination = tmp_path / "units" / "types"
    (destination / "empty-child").mkdir(parents=True)

    result = _run(destination)

    assert result.returncode == 0, result.stderr
    assert not destination.exists()


def test_nonempty_output_tree_is_preserved_byte_for_byte(tmp_path: Path) -> None:
    destination = tmp_path / "units" / "counter"
    (destination / "nested").mkdir(parents=True)
    (destination / "nested" / "receipt.json").write_bytes(b"owned bytes\n")
    (destination / "empty-sibling").mkdir()
    before = _tree(destination)

    result = _run(destination)

    assert result.returncode == 0, result.stderr
    assert _tree(destination) == before


def test_symlinked_ancestor_is_rejected_without_changes(tmp_path: Path) -> None:
    target = tmp_path / "target"
    target.mkdir()
    sentinel = target / "keep.txt"
    sentinel.write_bytes(b"external bytes\n")
    linked_parent = tmp_path / "linked-parent"
    linked_parent.symlink_to(target, target_is_directory=True)
    before = _tree(target)
    destination = linked_parent / "output"

    result = _run(destination)

    assert result.returncode != 0
    assert linked_parent.is_symlink()
    assert _tree(target) == before
    assert not (target / "output").exists()


def test_symlink_parent_then_dotdot_is_rejected_without_changes(tmp_path: Path) -> None:
    lexical_parent = tmp_path / "lexical"
    lexical_parent.mkdir()
    physical_parent = tmp_path / "physical" / "nested"
    physical_parent.mkdir(parents=True)
    linked_ancestor = lexical_parent / "jump"
    linked_ancestor.symlink_to(physical_parent, target_is_directory=True)

    # The OS resolves jump/.. against the symlink target's parent. A textual
    # abspath/normpath would instead erase the link and choose lexical/output.
    physical_output = physical_parent.parent / "output"
    physical_output.mkdir()
    (physical_output / "keep.txt").write_bytes(b"physical bytes\n")
    normalized_output = lexical_parent / "output"
    normalized_output.mkdir()
    (normalized_output / "keep.txt").write_bytes(b"lexical bytes\n")
    before_physical = _tree(physical_output)
    before_normalized = _tree(normalized_output)
    destination = linked_ancestor / ".." / "output"

    result = _run(destination)

    assert result.returncode != 0
    assert linked_ancestor.is_symlink()
    assert _tree(physical_output) == before_physical
    assert _tree(normalized_output) == before_normalized


def test_child_symlink_is_rejected_before_empty_siblings_are_removed(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "cpp"
    empty_sibling = destination / "empty-sibling"
    empty_sibling.mkdir(parents=True)
    target = tmp_path / "target"
    target.mkdir()
    (target / "keep.txt").write_bytes(b"target bytes\n")
    linked_child = destination / "linked-child"
    linked_child.symlink_to(target, target_is_directory=True)
    before = _tree(destination)
    target_before = _tree(target)

    result = _run(destination)

    assert result.returncode != 0
    assert _tree(destination) == before
    assert _tree(target) == target_before
