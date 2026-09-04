from __future__ import annotations
from pathlib import Path
from typing import Dict, List


def _is_excluded(path: Path, repo_root: Path, excluded_names: set) -> bool:
    rel = path.relative_to(repo_root)
    return any(part in excluded_names for part in rel.parts)


def discover_files(repo_root: Path, source: Dict) -> List[Path]:
    exts = set(source.get("extensions", [".v", ".sv", ".vh", ".svh"]))
    excluded = set(source.get("exclude_dirs", [".git", "build"]))
    # ``path_hints`` controls the primary RTL tree.  Include/vendor hints are
    # deliberately separate so a source can keep discovery bounded while still
    # materialising the headers and technology wrappers required to elaborate a
    # selected top.  Older source manifests do not have these fields.
    hints = []
    for field in ("path_hints", "include_hints", "vendor_hints"):
        hints.extend(source.get(field) or [])
    roots = [repo_root / h for h in hints if (repo_root / h).exists()] or [repo_root]
    found = set()
    for root in roots:
        for path in root.rglob("*"):
            if path.is_file() and path.suffix.lower() in exts and not _is_excluded(path, repo_root, excluded):
                found.add(path)
    return sorted(found)
