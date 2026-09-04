#!/usr/bin/env python3
"""Vendor structurally validated candidates into a reviewable runtime bundle.

The full validation campaign proves dependency closure, Verilator elaboration/
lint, bounded simulation and (where applicable) Yosys synthesis.  It does not
invent a semantic oracle.  This command therefore creates a self-contained
runtime *candidate* release, preserving that distinction in the manifest.
Candidates can be promoted to ``catalog.json`` only after an oracle and a
stable parameterized API have been reviewed.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
from pathlib import Path
from typing import Any, Mapping


SCHEMA = "acir-runtime-candidate-bundle-v0.26"
PROJECT_LICENSE_DIR = {
    "basejump_stl": "basejump-stl",
    "pulp_common_cells": "pulp-common-cells",
    "opentitan": "opentitan",
    "vortex": "vortex-v0.4",
}


def _read(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object: {path}")
    return value


def _sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _copy(src: Path, dst: Path) -> None:
    if not src.is_file():
        raise FileNotFoundError(src)
    dst.parent.mkdir(parents=True, exist_ok=True)
    try:
        shutil.copy2(src, dst)
    except PermissionError:
        # Windows can briefly hold a source file open while the preceding
        # Verilator/Yosys process exits.  Reading the file and writing a new
        # destination is safe here and avoids copying source metadata.
        dst.write_bytes(src.read_bytes())


def _case_rows(report: Mapping[str, Any]) -> dict[str, Mapping[str, Any]]:
    return {
        str(row.get("case_key")): row
        for row in report.get("case_results", [])
        if isinstance(row, Mapping) and row.get("case_key")
    }


def _license_path(runtime_root: Path, project: str) -> Path:
    directory = runtime_root / "licenses" / PROJECT_LICENSE_DIR.get(project, project)
    preferred = directory / "LICENSE"
    if preferred.is_file():
        return preferred
    choices = sorted(path for path in directory.glob("*") if path.is_file())
    if choices:
        return choices[0]
    raise FileNotFoundError(f"no vendored license for {project}: {directory}")


def _host_path(raw: str) -> Path:
    """Resolve a manifest path written as a Windows or WSL absolute path."""
    if raw.startswith("/mnt/") and len(raw) > 6:
        drive = raw[5].upper()
        return Path(f"{drive}:/" + raw[7:])
    return Path(raw)


def _resolve_closure_path(raw: str, source_cache: Path, project: str, validation_root: Path) -> Path:
    candidate = _host_path(raw)
    if candidate.is_absolute() and candidate.is_file():
        return candidate
    # Relative module/header/package paths are rooted at the cloned repo.
    relative = Path(raw)
    source_candidate = source_cache / project / relative
    if source_candidate.is_file():
        return source_candidate
    # Generated files are emitted below the validation candidate directory.
    generated = validation_root / "candidate-builds" / project
    matches = list(generated.glob(f"*/_generated/{relative.name}"))
    if matches:
        return matches[0]
    return candidate


def _destination_for_external(path: Path, source_cache: Path, project: str, module: str, output: Path) -> Path:
    """Choose a stable in-bundle path for external/generated dependencies."""
    try:
        relative = path.relative_to(source_cache)
    except ValueError:
        relative = None
    if relative and len(relative.parts) >= 2:
        return output / "vendor" / relative
    return output / "generated" / project / module / path.name


def stage(
    validation_root: Path,
    adapters_root: Path,
    promotion_path: Path,
    output: Path,
    runtime_root: Path,
) -> dict[str, Any]:
    report = _read(validation_root / "report.json")
    promotion = _read(promotion_path)
    campaign = _read(validation_root / "campaign.json")
    source_cache = Path(str(campaign.get("crawler_root", ""))) / "build" / "source-cache-v0.4" / "repos"
    if not source_cache.is_dir():
        raise FileNotFoundError(f"source cache is unavailable: {source_cache}")
    cases = _case_rows(report)
    promotion_entries = {
        str(row.get("candidate_id")): row
        for row in promotion.get("entries", [])
        if isinstance(row, Mapping) and row.get("candidate_id")
    }
    adapter_catalog = _read(adapters_root / "catalog.json")
    adapter_entries = {
        str(row.get("name")): row
        for row in adapter_catalog.get("entries", [])
        if isinstance(row, Mapping) and row.get("name")
    }
    strict_verification = (adapters_root / "verification.json").is_file()

    wrappers_dir = output / "wrappers"
    sources_dir = output / "vendor"
    licenses_dir = output / "licenses"
    manifests_dir = output / "manifests"
    for directory in (wrappers_dir, sources_dir, licenses_dir, manifests_dir):
        directory.mkdir(parents=True, exist_ok=True)

    entries: list[dict[str, Any]] = []
    blocked_verification: list[dict[str, Any]] = []
    errors: list[str] = []
    for adapter_manifest_path in sorted((adapters_root / "manifests").glob("*.json")):
        adapter = _read(adapter_manifest_path)
        name = str(adapter.get("name", ""))
        parts = name.split("/")
        if len(parts) != 3 or parts[0] != "candidate":
            errors.append(f"invalid adapter name: {name}")
            continue
        project, module = parts[1], parts[2]
        candidate_name = f"{project}/{module}"
        promotion_row = next(
            (row for row in promotion_entries.values() if str(row.get("project")) == project and str(row.get("module")) == module),
            {},
        )
        candidate_id = str(promotion_row.get("candidate_id", "unknown"))
        wrapper_rel = Path(str(adapter.get("wrapper", "")))
        wrapper_src = adapters_root / wrapper_rel
        if not wrapper_src.is_file():
            errors.append(f"{candidate_name}: wrapper missing: {wrapper_src}")
            continue
        wrapper_dst_rel = Path("wrappers") / project / f"{module}__{candidate_id[:8]}.sv"
        wrapper_dst = output / wrapper_dst_rel
        _copy(wrapper_src, wrapper_dst)

        source_manifest_path = validation_root / "candidate-builds" / project / module / "manifest.json"
        source_manifest = _read(source_manifest_path)
        copied_sources: list[str] = []
        missing_sources: list[str] = []
        closure_groups = [
            ("source", [
                *source_manifest.get("module_files", []),
                *source_manifest.get("header_files", []),
                *source_manifest.get("package_files", []),
            ]),
            ("external", [
                *source_manifest.get("external_files", []),
                *source_manifest.get("external_package_files", []),
            ]),
            ("generated", [
                *source_manifest.get("generated_files", []),
                *source_manifest.get("generated_header_files", []),
            ]),
        ]
        for group, paths in closure_groups:
            for rel in paths:
                raw = str(rel)
                rel_path = Path(raw)
                src = _resolve_closure_path(raw, source_cache, project, validation_root)
                if not src.is_file():
                    missing_sources.append(raw)
                    continue
                if group == "source":
                    dst_rel = Path("vendor") / project / rel_path
                else:
                    dst_rel = _destination_for_external(src, source_cache, project, module, output).relative_to(output)
                _copy(src, output / dst_rel)
                copied_sources.append(dst_rel.as_posix())

        license_rel = Path("licenses") / project / "LICENSE"
        try:
            license_src = _license_path(runtime_root, project)
        except FileNotFoundError:
            # Some frozen public mirrors do not carry a license file in the
            # checked-out tree.  Keep the candidate reproducible, but make
            # the unresolved licensing gate explicit; this is not an upstream
            # license and cannot satisfy formal catalog promotion.
            license_rel = Path("licenses") / project / "LICENSE-UNSPECIFIED.txt"
            notice = (
                f"No license file was found in the local source-cache checkout for {project}.\n"
                "This marker is generated by the candidate staging tool and is not an upstream license.\n"
            )
            (output / license_rel).parent.mkdir(parents=True, exist_ok=True)
            (output / license_rel).write_text(notice, encoding="utf-8")
            license_status = "not_found_in_checkout"
        else:
            _copy(license_src, output / license_rel)
            license_status = "vendored"

        sweep = adapter.get("sweep_configs", [])
        structural = str(promotion_row.get("structural_status", "")) == "passed"
        non_synth = bool(sweep) and all(
            isinstance(item, Mapping) and item.get("synthesis") == "NOT_APPLICABLE_NON_SYNTH"
            for item in sweep
        )
        adapter_verification = adapter_entries.get(name, {}).get("verification", {})
        if strict_verification and (
            adapter_verification.get("verilator_lint") != "PASS"
            or adapter_verification.get("synthesis") not in {"PASS", "NOT_APPLICABLE_NON_SYNTH"}
        ):
            blocked_verification.append({
                "name": name,
                "project": project,
                "module": module,
                "verilator_lint": adapter_verification.get("verilator_lint", "NOT_RECORDED"),
                "synthesis": adapter_verification.get("synthesis", "NOT_RECORDED"),
            })
            continue
        status = "staged_non_synth" if non_synth else "staged_structural"
        entry = {
            "name": f"candidate/{project}/{module}",
            "candidate_id": candidate_id,
            "module": module,
            "project": project,
            "status": status,
            "promotion": promotion_row.get("promotion", "review_required"),
            "semantic_status": "missing",
            "wrapper": wrapper_dst_rel.as_posix(),
            "source_files": copied_sources,
            "license_file": license_rel.as_posix(),
            "license_status": license_status,
            "include_roots": [
                (Path("vendor") / project / Path(str(item))).as_posix()
                for item in source_manifest.get("include_roots", [])
            ],
            "provenance": {
                "repository": promotion_row.get("provenance", {}).get("repository"),
                "commit": promotion_row.get("provenance", {}).get("commit"),
                "source_file": promotion_row.get("source_file"),
                "license": promotion_row.get("license"),
            },
            "interface": adapter.get("interface", {}),
            "validation": {
                "closure": promotion_row.get("structural_status"),
                "verilator_lint": adapter_verification.get("verilator_lint", "NOT_RECORDED"),
                "simulation": [item.get("simulation") for item in sweep if isinstance(item, Mapping)],
                "synthesis": [item.get("synthesis") for item in sweep if isinstance(item, Mapping)],
                "sweep_configs": sweep,
                "mapped_area": [item.get("mapped_area") for item in sweep if isinstance(item, Mapping)],
                "note": "structural validation only; class-specific semantic oracle is required before catalog promotion",
            },
        }
        if missing_sources:
            entry["status"] = "staged_incomplete"
            entry["missing_sources"] = missing_sources
            errors.append(f"{candidate_name}: missing {len(missing_sources)} closure files")
        if not structural:
            entry["status"] = "blocked"
        entries.append(entry)

    hashes: dict[str, str] = {}
    for path in sorted(output.rglob("*")):
        if path.is_file() and path.name != "manifest.json":
            hashes[path.relative_to(output).as_posix()] = _sha(path)
    summary = {
        "candidates_seen": len(promotion.get("entries", [])),
        "packaged": len(entries),
        "structural_staged": sum(row["status"] == "staged_structural" for row in entries),
        "non_synth_staged": sum(row["status"] == "staged_non_synth" for row in entries),
        "blocked": sum(row["status"] == "blocked" for row in entries),
        "incomplete": sum(row["status"] == "staged_incomplete" for row in entries),
        "verification_blocked": len(blocked_verification),
        "errors": len(errors),
    }
    result = {
        "schema": SCHEMA,
        "release": "runtime-rtl-v0.26-candidates",
        "generated_from": {
            "validation_report": str((validation_root / "report.json").resolve()),
            "promotion_manifest": str(promotion_path.resolve()),
            "adapter_catalog": str((adapters_root / "catalog.json").resolve()),
        },
        "summary": summary,
        "policy": {
            "runtime_catalog_promotion": "requires semantic oracle, reviewed canonical API, provenance and license closure",
            "structural_evidence": "dependency closure + Verilator + bounded simulation + Yosys where synthesizable",
        },
        "entries": sorted(entries, key=lambda row: str(row.get("name", ""))),
        "blocked_verification": sorted(blocked_verification, key=lambda row: str(row.get("name", ""))),
        "sha256": hashes,
        "errors": errors,
    }
    release_tag = output.name or "v0.26"
    (manifests_dir / f"candidate-bundle-{release_tag}.json").write_text(
        json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    return result


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--validation-root", type=Path, default=Path(".pycircuit_out/runtime-full-validation"))
    parser.add_argument("--adapters", type=Path, default=Path(".pycircuit_out/runtime-full-validation/adapters-v3"))
    parser.add_argument("--promotion", type=Path, default=Path(".pycircuit_out/runtime-full-validation/runtime-candidates-v2.json"))
    parser.add_argument("--output", type=Path, default=Path("library/verilog/candidates/v0.26"))
    parser.add_argument("--runtime-root", type=Path, default=Path("library/verilog"))
    args = parser.parse_args(argv)
    result = stage(
        args.validation_root.resolve(), args.adapters.resolve(), args.promotion.resolve(), args.output.resolve(), args.runtime_root.resolve()
    )
    print(json.dumps({"output": str(args.output.resolve()), **result["summary"]}, sort_keys=True))
    return 0 if not result["errors"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
