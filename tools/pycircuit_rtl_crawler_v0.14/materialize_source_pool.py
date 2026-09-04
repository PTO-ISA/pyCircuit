#!/usr/bin/env python3
"""Materialize configured RTL repositories into a reproducible local pool.

This is intentionally separate from discovery: clone/copy provenance is
recorded first, and the crawler can then scan the resulting files.  Sparse
checkout follows each source's path_hints to keep large repositories bounded.
"""
from __future__ import annotations

import argparse
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path


RTL_SUFFIXES = {".v", ".sv", ".vh", ".svh"}
LICENSE_NAMES = {"license", "license.txt", "license.md", "copying", "copyright"}


def run(cmd, cwd=None):
    return subprocess.run(cmd, cwd=str(cwd) if cwd else None, text=True, capture_output=True)


def metadata(repo: Path, source: dict, status: str, error: str = "") -> dict:
    sha = remote = branch = ""
    if (repo / ".git").exists():
        x = run(["git", "rev-parse", "HEAD"], repo)
        sha = x.stdout.strip() if x.returncode == 0 else ""
        x = run(["git", "remote", "get-url", "origin"], repo)
        remote = x.stdout.strip() if x.returncode == 0 else ""
        x = run(["git", "branch", "--show-current"], repo)
        branch = x.stdout.strip() if x.returncode == 0 else ""
    files = [p for p in repo.rglob("*") if p.is_file() and ".git" not in p.parts]
    rtl = [p for p in files if p.suffix.lower() in RTL_SUFFIXES]
    licenses = [
        p.relative_to(repo).as_posix()
        for p in files
        if p.name.lower() in LICENSE_NAMES
        or any(part.lower().startswith("license") for part in p.relative_to(repo).parts[:-1])
    ]
    if status == "failed":
        content_status = "failed"
    elif not files:
        content_status = "empty"
    elif not rtl:
        content_status = "no_direct_rtl"
    else:
        content_status = "rtl_available"
    return {
        "project": source["project"],
        "repo": source["repo"],
        "path": str(repo),
        "branch": branch,
        "commit_sha": sha,
        "clone_status": status,
        "content_status": content_status,
        "path_hints": source.get("path_hints", []),
        "include_hints": source.get("include_hints", []),
        "vendor_hints": source.get("vendor_hints", []),
        "dependency_projects": source.get("dependency_projects", []),
        "checkout_mode": "sparse" if source.get("path_hints") else "full",
        "error": error,
        "materialized_at": datetime.now(timezone.utc).isoformat(),
        "file_count": len(files),
        "rtl_file_count": len(rtl),
        "license_files": sorted(licenses),
        "license_gate": False,
        "license_recording": True,
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--sources", default="sources.expanded.completed.v0.3.json")
    ap.add_argument("--root", default="build/source-cache-v0.4/repos")
    ap.add_argument("--manifest", default="build/source-cache-v0.4/source-manifest.json")
    ap.add_argument("--source", action="append", default=[])
    ap.add_argument("--no-update", action="store_true")
    args = ap.parse_args()

    cfg = json.loads(Path(args.sources).read_text(encoding="utf-8"))
    selected = [s for s in cfg.get("sources", []) if s.get("enabled", True)]
    # Dependency-only repositories are materialised with the wave but are not
    # scanned as independent discovery sources (see ``scan`` in v0.4 config).
    by_project = {s.get("project"): s for s in cfg.get("sources", [])}
    required = set()
    for s in selected:
        required.update(s.get("dependency_projects", []) or [])
    for project in sorted(required):
        dep = by_project.get(project)
        if dep and dep not in selected:
            selected.append(dep)
    if args.source:
        wanted = set(args.source)
        selected = [s for s in selected if s["project"] in wanted]
    root = Path(args.root)
    records = []
    for source in selected:
        repo = root / source["project"]
        status, error = "reused", ""
        try:
            hints = []
            for field in ("path_hints", "include_hints", "vendor_hints"):
                hints.extend(str(x).strip("/") for x in source.get(field, []) if str(x).strip("/") not in {"", "."})
            if not (repo / ".git").exists():
                root.mkdir(parents=True, exist_ok=True)
                cmd = ["git", "clone", "--depth", "1", "--no-tags", "--filter=blob:none", "--sparse"]
                if source.get("branch"):
                    cmd += ["--branch", source["branch"]]
                cmd += [source["repo"], str(repo)]
                result = run(cmd)
                if result.returncode != 0:
                    raise RuntimeError((result.stderr or result.stdout).strip())
                if hints:
                    result = run(["git", "sparse-checkout", "set", *hints], repo)
                    if result.returncode != 0:
                        raise RuntimeError((result.stderr or result.stdout).strip())
                status = "cloned"
            else:
                # Existing sparse checkouts from v0.3/v0.4 may not contain the
                # newly declared include/vendor roots.  Expand the sparse set
                # even with --no-update; this fetches only the requested blobs
                # and never resets user files.
                if hints and (run(["git", "config", "--get", "core.sparseCheckout"], repo).stdout.strip().lower() == "true"):
                    result = run(["git", "sparse-checkout", "set", *hints], repo)
                    if result.returncode != 0:
                        message = (result.stderr or result.stdout).strip()
                        # Never discard a user's local RTL edits merely to
                        # expand the sparse set.  The candidate builder can
                        # still use the already materialized checkout; record
                        # the skipped expansion in the manifest instead of
                        # misclassifying the repository as a clone failure.
                        if "unstaged changes" in message.lower():
                            error = f"sparse expansion skipped: {message}"
                        else:
                            raise RuntimeError(message)
                if not args.no_update:
                    result = run(["git", "fetch", "--depth", "1", "--no-tags", "origin"], repo)
                    if result.returncode != 0:
                        raise RuntimeError((result.stderr or result.stdout).strip())
                    result = run(["git", "reset", "--hard", "FETCH_HEAD"], repo)
                    if result.returncode != 0:
                        raise RuntimeError((result.stderr or result.stdout).strip())
                    status = "updated"
        except Exception as exc:  # keep the manifest useful for partial waves
            status, error = "failed", str(exc)
        records.append(metadata(repo, source, status, error))
        print(f"[{status}] {source['project']}: {records[-1]['rtl_file_count']} RTL files")

    # A single-source wave must not discard the manifest entries produced by
    # earlier waves.  Merge by project when the output file already exists.
    out_path = Path(args.manifest)
    merged_records = records
    if args.source and out_path.exists():
        try:
            previous = json.loads(out_path.read_text(encoding="utf-8"))
            previous_by_project = {
                x.get("project"): x for x in previous.get("sources", [])
            }
            previous_by_project.update({x["project"]: x for x in records})
            order = {s["project"]: i for i, s in enumerate(cfg.get("sources", []))}
            merged_records = sorted(
                previous_by_project.values(), key=lambda x: order.get(x.get("project"), 10**6)
            )
        except (OSError, ValueError, TypeError):
            merged_records = records
    out = {
        "schema": "pycircuit-rtl-source-manifest-v0.4",
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "license_policy": "informational_only",
        "source_config": Path(args.sources).name,
        "sources": merged_records,
    }
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(out, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {out_path} ({len(records)} sources)")


if __name__ == "__main__":
    main()
