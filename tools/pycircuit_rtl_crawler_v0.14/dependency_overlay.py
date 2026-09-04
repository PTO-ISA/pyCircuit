"""Resolve a candidate's cross-repository RTL dependencies.

The original closure builder intentionally operated on one checkout.  Real
IP repositories commonly import technology cells, common-cells helpers or a
vendor assertion library from a sibling checkout.  This module keeps those
repositories separate (and preserves provenance) while adding only the files
needed by the selected top to the candidate filelist.
"""
from __future__ import annotations

from collections import defaultdict, deque
from pathlib import Path
import re
from typing import Dict, Iterable, List, Tuple

from dependency_closure import RepositoryModel, derive_include_root


RTL_UNITS = {".v", ".sv"}


def _spec_parts(spec: str) -> Tuple[str, str] | None:
    if not isinstance(spec, str) or ":" not in spec:
        return None
    project, rel = spec.split(":", 1)
    project, rel = project.strip(), rel.strip().replace("\\", "/").lstrip("/")
    if not project or not rel:
        return None
    return project, rel


def apply_dependency_overlay(
    primary_model: RepositoryModel,
    closure: Dict,
    dependency_models: Dict[str, RepositoryModel],
    module_overrides: Dict[str, str] | None = None,
    package_overrides: Dict[str, str] | None = None,
    include_overrides: Dict[str, str] | None = None,
    prune_modules: Iterable[str] | None = None,
    prune_packages: Iterable[str] | None = None,
    prune_includes: Iterable[str] | None = None,
) -> Dict:
    """Extend ``closure`` with uniquely-resolvable sibling-repo dependencies.

    Override values use ``project:repo/relative/file``.  A resolved external
    file is emitted as an absolute path in ``candidate.f`` and is recorded in
    the manifest under ``external_*`` fields; primary-repository files remain
    relative to the source checkout.
    """
    module_overrides = module_overrides or {}
    package_overrides = package_overrides or {}
    include_overrides = include_overrides or {}
    prune_modules = set(prune_modules or [])
    prune_packages = set(prune_packages or [])
    prune_includes = set(prune_includes or [])
    primary_project = primary_model.source.get("project", "__primary__")
    models: Dict[str, RepositoryModel] = {primary_project: primary_model, **dependency_models}

    module_index: Dict[str, List[Tuple[str, Dict]]] = defaultdict(list)
    package_index: Dict[str, List[Tuple[str, str]]] = defaultdict(list)
    interface_index: Dict[str, List[Tuple[str, str]]] = defaultdict(list)
    include_index: Dict[str, List[Tuple[str, str]]] = defaultdict(list)
    for project, model in models.items():
        for name, mods in model.module_index.items():
            module_index[name].extend((project, m) for m in mods)
        for name, files in model.package_index.items():
            package_index[name].extend((project, rel) for rel in files)
        for name, files in model.interface_index.items():
            interface_index[name].extend((project, rel) for rel in files)
        for rel in model.all_repo_files:
            include_index[Path(rel).name].append((project, rel))

    # Include roots/files are kept as explicit absolute paths for sibling repos.
    external_files: List[str] = []
    external_package_files: List[str] = []
    external_include_roots: List[str] = []
    selected_external = set()
    selected_primary = {*(closure.get("module_files", [])), *(closure.get("package_files", []))}
    # Some legacy/vendor include files use ``.vi`` (or a header extension)
    # while declaring real Verilog modules.  Keep an index of those
    # declarations so an unresolved instance can be reconciled after the
    # include override is selected.  The source remains the vendor file; this
    # is only bookkeeping for dependency closure provenance.
    overlay_include_modules: Dict[str, str] = {}

    def add_external_file(project: str, rel: str, role: str) -> None:
        model = models[project]
        path = (model.repo_dir / rel).resolve()
        if project == primary_project:
            # A primary-repository file selected through an override may not
            # have been visited by the native closure walk.  Preserve its
            # source directory as an include root for relative ``../../``
            # includes (common in generated/platform RTL).
            closure.setdefault("include_roots", []).append(str(Path(rel).parent.as_posix()))
            if role == "package":
                closure.setdefault("package_files", []).append(rel)
            elif role == "module":
                closure.setdefault("module_files", []).append(rel)
            else:
                closure.setdefault("header_files", []).append(rel)
            return
        key = (project, rel)
        if key in selected_external:
            return
        selected_external.add(key)
        if path.suffix.lower() in RTL_UNITS:
            if role == "package":
                external_package_files.append(str(path))
            else:
                external_files.append(str(path))
        elif path.suffix.lower() in {".vi", ".vh", ".svh"} and path.is_file():
            try:
                text = path.read_text(encoding="utf-8", errors="replace")
            except OSError:
                text = ""
            for name in re.findall(r"(?m)^\s*module\s+([A-Za-z_][A-Za-z0-9_$]*)", text):
                overlay_include_modules[name] = f"@{project}/{rel}"
        # Verilator resolves quoted includes relative to the source file, but
        # vendor trees often use a logical prefix (e.g. common_cells/foo.svh)
        # whose root is several levels above the file.  Add the ancestor roots
        # up to the checkout boundary so both layouts are covered.
        root = model.repo_dir.resolve()
        cur = path.parent.resolve()
        while True:
            external_include_roots.append(str(cur))
            if cur == root or root not in cur.parents:
                break
            cur = cur.parent

    def choose_file(spec: str, index: Dict[str, List[Tuple[str, str]]]):
        parts = _spec_parts(spec)
        if not parts:
            return None
        project, rel = parts
        if project not in models or rel not in models[project].all_repo_files:
            return None
        return project, rel

    def choose_module(name: str):
        override = choose_file(module_overrides.get(name, ""), {})
        if override:
            project, rel = override
            hits = [m for p, m in module_index.get(name, []) if p == project and m["file"] == rel]
            if hits:
                return project, hits[0]
        hits = module_index.get(name, [])
        # Prefer an explicitly indexed primary definition when it is unique;
        # otherwise accept a unique sibling definition.
        if len(hits) == 1:
            return hits[0]
        primary_hits = [x for x in hits if x[0] == primary_project]
        if len(primary_hits) == 1:
            return primary_hits[0]
        return None

    def choose_package(name: str):
        override = choose_file(package_overrides.get(name, ""), {})
        if override:
            project, rel = override
            if (project, rel) in package_index.get(name, []):
                return override
        hits = package_index.get(name, [])
        if len(hits) == 1:
            return hits[0]
        primary_hits = [x for x in hits if x[0] == primary_project]
        return primary_hits[0] if len(primary_hits) == 1 else None

    def choose_interface(name: str):
        hits = interface_index.get(name, [])
        if len(hits) == 1:
            return hits[0]
        primary_hits = [x for x in hits if x[0] == primary_project]
        return primary_hits[0] if len(primary_hits) == 1 else None

    def choose_include(name: str):
        override = choose_file(include_overrides.get(name, ""), {})
        if override:
            return override
        inc = name.replace("\\", "/").lstrip("./")
        candidates = []
        for project, model in models.items():
            status, resolved = model.resolve_include(inc)
            if status == "resolved":
                candidates.append((project, resolved))
        # A suffix match in one project wins over a duplicate in another only
        # when the candidate has an explicit override; otherwise stay safe.
        unique = sorted(set(candidates))
        return unique[0] if len(unique) == 1 else None

    # Work over unresolved edges and recursively inspect any newly selected
    # external file.  Duplicate edge records are harmless and are collapsed at
    # the end just like the native closure builder.
    overlay_edges: List[Dict] = []
    queue = deque(closure.get("unresolved", []))
    seen_nodes = set()
    seen_edges = set()
    while queue:
        edge = queue.popleft()
        edge_key = (
            edge.get("parent_kind", ""), edge.get("parent_name", ""),
            edge.get("parent_file", ""), edge.get("dependency_kind", ""),
            edge.get("dependency_name", ""), edge.get("instance_name", ""),
        )
        if edge_key in seen_edges:
            continue
        seen_edges.add(edge_key)
        if edge not in closure.get("edges", []):
            overlay_edges.append(edge)
        kind, name = edge.get("dependency_kind"), edge.get("dependency_name", "")
        if (kind == "submodule" and name in prune_modules) or (kind == "package" and name in prune_packages) or (kind == "include" and name in prune_includes):
            edge["resolution_status"] = "pruned_config_dead"
            edge["resolved_file"] = ""
            continue
        selected = None
        role = "header"
        if kind == "submodule":
            selected = choose_module(name)
            role = "module"
        elif kind == "package":
            selected = choose_package(name)
            role = "package"
        elif kind == "interface":
            selected = choose_interface(name)
            role = "module"
        elif kind == "include":
            selected = choose_include(name)
            role = "header"
        if not selected:
            continue

        project, payload = selected
        rel = payload["file"] if isinstance(payload, dict) else payload
        edge["resolution_status"] = "resolved"
        edge["resolved_file"] = rel if project == primary_project else f"@{project}/{rel}"
        edge["resolved_source_project"] = primary_project if project == primary_project else project
        add_external_file(project, rel, role)

        node = (project, rel, role, payload.get("module", "") if isinstance(payload, dict) else "")
        if node in seen_nodes:
            continue
        seen_nodes.add(node)
        pf = models[project].parsed_by_file.get(rel, {})
        parent_name = payload.get("module", rel) if isinstance(payload, dict) else rel
        parent_kind = "module" if isinstance(payload, dict) else "file"
        parent_file = rel
        selected_module = None
        if isinstance(payload, dict):
            selected_module = next(
                (m for m in pf.get("modules", []) if m.get("module") == payload.get("module")),
                payload,
            )
        for inst in (selected_module or {}).get("instances", []):
            queue.append({
                "parent_kind": parent_kind,
                "parent_name": parent_name,
                "parent_file": parent_file,
                "dependency_kind": "submodule",
                "dependency_name": inst.get("module_type", ""),
                "instance_name": inst.get("instance_name", ""),
                "resolution_status": "external_or_unresolved",
                "resolved_file": "",
            })
        for inc in pf.get("includes", []):
            queue.append({
                "parent_kind": "file",
                "parent_name": rel,
                "parent_file": rel,
                "dependency_kind": "include",
                "dependency_name": inc,
                "instance_name": "",
                "resolution_status": "unresolved",
                "resolved_file": "",
            })
        for pkg in pf.get("imports", []):
            queue.append({
                "parent_kind": "file",
                "parent_name": rel,
                "parent_file": rel,
                "dependency_kind": "package",
                "dependency_name": pkg,
                "instance_name": "",
                "resolution_status": "external_or_unresolved",
                "resolved_file": "",
            })

    # The parser can return duplicate module records when a file is selected by
    # both a direct and a transitive edge.  Keep filelists deterministic.
    closure["module_files"] = sorted(set(closure.get("module_files", [])))
    closure["package_files"] = sorted(set(closure.get("package_files", [])))
    closure["header_files"] = sorted(set(closure.get("header_files", [])))
    closure["external_files"] = sorted(set(external_files))
    closure["external_package_files"] = sorted(set(external_package_files))
    closure["external_include_roots"] = sorted(set(external_include_roots))
    closure["edges"] = sorted(
        closure.get("edges", []) + overlay_edges,
        key=lambda e: (e.get("parent_file", ""), e.get("dependency_kind", ""), e.get("dependency_name", ""), e.get("instance_name", "")),
    )
    closure["unresolved"] = [
        e for e in closure.get("edges", [])
        if e.get("resolution_status") not in {"resolved", "pruned_config_dead"}
    ]
    # Include overrides are processed as headers because they are normally
    # pulled in by a quoted `include.  If such a file also declares a module
    # (the Berkeley HardFloat ``HardFloat_specialize.vi`` pattern), resolve
    # the corresponding instance edge against that exact vendor file.
    if overlay_include_modules:
        for edge in closure.get("edges", []):
            if edge.get("dependency_kind") != "submodule":
                continue
            resolved = overlay_include_modules.get(str(edge.get("dependency_name", "")))
            if not resolved:
                continue
            if edge.get("resolution_status") not in {"resolved", "pruned_config_dead"}:
                edge["resolution_status"] = "resolved_include_module"
                edge["resolved_file"] = resolved
        closure["unresolved"] = [
            e for e in closure.get("edges", [])
            if e.get("resolution_status") not in {
                "resolved", "resolved_include_module", "pruned_config_dead",
            }
        ]
    # Keep a compact provenance record; paths are absolute only for the actual
    # filelist and can be relativised by callers when exporting reports.
    closure["overlay_projects"] = sorted({p for p, _ in selected_external})
    closure["closure_status"] = "COMPLETE" if not closure["unresolved"] else "PARTIAL"
    return closure
