from __future__ import annotations

from collections import defaultdict, deque
from pathlib import Path
from typing import Dict, List, Tuple, Set

from rtl_scanner import discover_files
from sv_parser import parse_sv_file


def resolve_include(name: str, by_basename: Dict[str, List[str]], all_repo_files: List[str]) -> Tuple[str, str]:
    inc = name.replace('\\', '/').lstrip('./')

    exact = [p for p in all_repo_files if p == inc]
    if len(exact) == 1:
        return 'resolved', exact[0]

    suffix = [p for p in all_repo_files if p.endswith('/' + inc)]
    if len(suffix) == 1:
        return 'resolved', suffix[0]
    if len(suffix) > 1:
        return 'ambiguous', '|'.join(sorted(suffix))

    hits = by_basename.get(Path(inc).name, [])
    if len(hits) == 1:
        return 'resolved', hits[0]
    if len(hits) > 1:
        return 'ambiguous', '|'.join(sorted(hits))
    return 'unresolved', ''


def derive_include_root(include_name: str, resolved_file: str) -> str:
    """Return repo-relative include root for -I.

    Example:
      include_name  = common_cells/registers.svh
      resolved_file = include/common_cells/registers.svh
      => include
    """
    inc = include_name.replace('\\', '/').lstrip('./')
    res = resolved_file.replace('\\', '/').lstrip('./')
    if res == inc:
        return '.'
    suffix = '/' + inc
    if res.endswith(suffix):
        root = res[:-len(suffix)]
        return root or '.'
    return str(Path(res).parent.as_posix())


class RepositoryModel:
    def __init__(self, repo_dir: Path, source: Dict):
        self.repo_dir = repo_dir.resolve()
        self.source = source
        self.rtl_files = discover_files(self.repo_dir, source)
        self.parsed_by_file: Dict[str, Dict] = {}
        self.module_index: Dict[str, List[Dict]] = defaultdict(list)
        self.package_index: Dict[str, List[str]] = defaultdict(list)
        self.interface_index: Dict[str, List[str]] = defaultdict(list)
        self.by_basename: Dict[str, List[str]] = defaultdict(list)
        self.all_repo_files: List[str] = []
        self.parse_warnings: List[Dict] = []
        self._build()

    def _build(self):
        for path in self.rtl_files:
            rel = path.relative_to(self.repo_dir).as_posix()
            self.all_repo_files.append(rel)
            self.by_basename[path.name].append(rel)
            try:
                pf = parse_sv_file(path, self.repo_dir)
            except Exception as exc:
                self.parse_warnings.append({'file': rel, 'error': str(exc)})
                continue
            self.parsed_by_file[rel] = pf
            for pkg in pf.get('packages_declared', []):
                self.package_index[pkg].append(rel)
            for iface in pf.get('interfaces_declared', []):
                self.interface_index[iface].append(rel)
            for mod in pf.get('modules', []):
                self.module_index[mod['module']].append(mod)

    def find_top(self, module_name: str, file_hint: str = '') -> Dict:
        hits = self.module_index.get(module_name, [])
        if file_hint:
            hits = [m for m in hits if m['file'] == file_hint]
        if not hits:
            raise KeyError(f'module not found: {module_name}')
        if len(hits) > 1:
            files = ', '.join(sorted(m['file'] for m in hits))
            raise ValueError(f'ambiguous top module {module_name}; files: {files}')
        return hits[0]

    def resolve_module(self, module_name: str) -> Tuple[str, str, Dict | None]:
        hits = self.module_index.get(module_name, [])
        if len(hits) == 1:
            return 'resolved', hits[0]['file'], hits[0]
        if len(hits) > 1:
            return 'ambiguous', '|'.join(sorted(m['file'] for m in hits)), None
        return 'external_or_unresolved', '', None

    def resolve_package(self, package_name: str) -> Tuple[str, str]:
        hits = self.package_index.get(package_name, [])
        if len(hits) == 1:
            return 'resolved', hits[0]
        if len(hits) > 1:
            return 'ambiguous', '|'.join(sorted(hits))
        return 'external_or_unresolved', ''

    def resolve_interface(self, interface_name: str) -> Tuple[str, str]:
        hits = self.interface_index.get(interface_name, [])
        if len(hits) == 1:
            return 'resolved', hits[0]
        if len(hits) > 1:
            return 'ambiguous', '|'.join(sorted(hits))
        return 'external_or_unresolved', ''

    def resolve_include(self, include_name: str) -> Tuple[str, str]:
        return resolve_include(include_name, self.by_basename, self.all_repo_files)


def build_dependency_closure(model: RepositoryModel, top_module: str, top_file: str = '', prune_modules=None, prune_packages=None, prune_includes=None) -> Dict:
    top = model.find_top(top_module, top_file)
    prune_modules = set(prune_modules or [])
    prune_packages = set(prune_packages or [])
    prune_includes = set(prune_includes or [])

    module_queue = deque([top])
    file_queue = deque()
    seen_modules: Set[Tuple[str, str]] = set()
    seen_files: Set[str] = set()

    module_files: Set[str] = set()
    package_files: Set[str] = set()
    header_files: Set[str] = set()
    include_roots: Set[str] = set()
    edges: List[Dict] = []
    unresolved: List[Dict] = []

    def add_edge(parent_kind, parent_name, parent_file, dep_kind, dep_name, status, resolved_file, instance_name=''):
        row = {
            'parent_kind': parent_kind,
            'parent_name': parent_name,
            'parent_file': parent_file,
            'dependency_kind': dep_kind,
            'dependency_name': dep_name,
            'instance_name': instance_name,
            'resolution_status': status,
            'resolved_file': resolved_file,
        }
        edges.append(row)
        if status not in {'resolved', 'pruned_config_dead'}:
            unresolved.append(row)

    def enqueue_file(rel: str, role: str):
        if not rel:
            return
        if role == 'package':
            package_files.add(rel)
        elif role == 'header':
            header_files.add(rel)
        else:
            module_files.add(rel)
        if rel not in seen_files:
            file_queue.append(rel)

    while module_queue or file_queue:
        while module_queue:
            mod = module_queue.popleft()
            key = (mod['module'], mod['file'])
            if key in seen_modules:
                continue
            seen_modules.add(key)
            enqueue_file(mod['file'], 'module')

            for inst in mod.get('instances', []):
                dep_name = inst['module_type']
                if dep_name in prune_modules:
                    add_edge(
                        'module', mod['module'], mod['file'],
                        'submodule', dep_name,
                        'pruned_config_dead', '',
                        inst.get('instance_name', '')
                    )
                    continue

                status, resolved_file, dep_mod = model.resolve_module(dep_name)
                # SystemVerilog interface instances are returned by the
                # lightweight parser in the same instance stream as modules.
                # Reclassify a name that resolves uniquely in the interface
                # index so the closure includes its declaration file.
                if dep_mod is None:
                    iface_status, iface_file = model.resolve_interface(dep_name)
                    if iface_status == 'resolved':
                        add_edge('module', mod['module'], mod['file'], 'interface', dep_name, iface_status, iface_file, inst.get('instance_name', ''))
                        enqueue_file(iface_file, 'module')
                        continue
                add_edge('module', mod['module'], mod['file'], 'submodule', dep_name, status, resolved_file, inst.get('instance_name', ''))
                if dep_mod is not None and (dep_mod['module'], dep_mod['file']) not in seen_modules:
                    module_queue.append(dep_mod)

            for iface in mod.get('interfaces', []):
                status, resolved = model.resolve_interface(iface)
                add_edge('module', mod['module'], mod['file'], 'interface', iface, status, resolved)
                if status == 'resolved':
                    enqueue_file(resolved, 'module')

            for inc in mod.get('includes', []):
                if inc in prune_includes:
                    add_edge('module', mod['module'], mod['file'], 'include', inc, 'pruned_config_dead', '')
                    continue
                status, resolved = model.resolve_include(inc)
                add_edge('module', mod['module'], mod['file'], 'include', inc, status, resolved)
                if status == 'resolved':
                    enqueue_file(resolved, 'header')
                    include_roots.add(derive_include_root(inc, resolved))
                    # Verilator resolves relative include expressions such as
                    # ``../../src/foo.v`` from an include/search root.  Keep
                    # the parent directory of the referencing file as well;
                    # the logical root derived from the resolved path alone
                    # would otherwise point one or two levels too high.
                    if '/' in inc.replace('\\', '/') and inc.replace('\\', '/').startswith('.'):
                        include_roots.add(str(Path(mod['file']).parent.as_posix()))

            for pkg in mod.get('imports', []):
                if pkg in prune_packages:
                    add_edge(
                        'module', mod['module'], mod['file'],
                        'package', pkg, 'pruned_config_dead', ''
                    )
                    continue
                status, resolved = model.resolve_package(pkg)
                add_edge('module', mod['module'], mod['file'], 'package', pkg, status, resolved)
                if status == 'resolved':
                    enqueue_file(resolved, 'package')

        while file_queue:
            rel = file_queue.popleft()
            if rel in seen_files:
                continue
            seen_files.add(rel)
            pf = model.parsed_by_file.get(rel)
            if not pf:
                continue

            # File-level includes/imports matter for package/header dependency closure too.
            for inc in pf.get('includes', []):
                if inc in prune_includes:
                    add_edge('file', rel, rel, 'include', inc, 'pruned_config_dead', '')
                    continue
                status, resolved = model.resolve_include(inc)
                add_edge('file', rel, rel, 'include', inc, status, resolved)
                if status == 'resolved':
                    enqueue_file(resolved, 'header')
                    include_roots.add(derive_include_root(inc, resolved))
                    if '/' in inc.replace('\\', '/') and inc.replace('\\', '/').startswith('.'):
                        include_roots.add(str(Path(rel).parent.as_posix()))

            for pkg in pf.get('imports', []):
                if pkg in prune_packages:
                    add_edge(
                        'file', rel, rel,
                        'package', pkg, 'pruned_config_dead', ''
                    )
                    continue
                status, resolved = model.resolve_package(pkg)
                add_edge('file', rel, rel, 'package', pkg, status, resolved)
                if status == 'resolved':
                    enqueue_file(resolved, 'package')

    # De-duplicate edges while retaining deterministic order.
    uniq = {}
    for e in edges:
        key = (
            e['parent_kind'], e['parent_name'], e['parent_file'], e['dependency_kind'],
            e['dependency_name'], e['instance_name'], e['resolution_status'], e['resolved_file']
        )
        uniq[key] = e
    edges = sorted(uniq.values(), key=lambda e: (
        e['parent_file'], e['dependency_kind'], e['dependency_name'], e['instance_name']
    ))
    unresolved = [e for e in edges if e['resolution_status'] not in {'resolved', 'pruned_config_dead'}]

    # Some headers are deliberately in the same source file tree; don't compile .svh as units.
    package_files = {p for p in package_files if Path(p).suffix.lower() in {'.sv', '.v'}}
    module_files = {p for p in module_files if Path(p).suffix.lower() in {'.sv', '.v'}}

    return {
        'top_module': top_module,
        'top_file': top['file'],
        'module_files': sorted(module_files),
        'package_files': sorted(package_files),
        'header_files': sorted(header_files),
        'include_roots': sorted(include_roots),
        'edges': edges,
        'unresolved': unresolved,
        'closure_status': 'COMPLETE' if not unresolved else 'PARTIAL',
        'parse_warnings': model.parse_warnings,
        'pruned_modules': sorted(prune_modules),
        'pruned_packages': sorted(prune_packages),
        'pruned_includes': sorted(prune_includes),
    }
