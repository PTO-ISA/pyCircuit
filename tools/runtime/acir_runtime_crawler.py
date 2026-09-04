#!/usr/bin/env python3
"""Small, deterministic RTL crawler for the Agentic Circuit runtime.

The crawler is deliberately independent of LLVM/MLIR.  It discovers a bounded
set of Verilog/SystemVerilog candidates, closes their local dependencies, and
then applies the two inexpensive structural gates used by the runtime:
Verilator lint and Yosys ``check/stat``.  A source can be local or hosted by
GitHub, GitLab, Codeberg, SourceHut, or any generic Git server.  Remote sources
are opt-in in the checked-in configuration so a normal build never downloads
code or consumes an unbounded amount of memory.

The module is also usable as a library.  In particular, projects embedding
Agentic Circuit can call :func:`crawl_runtime` and consume the returned JSON
compatible catalog without importing PyYAML or any compiler package.
"""

from __future__ import annotations

import argparse
import dataclasses
import hashlib
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import time
from collections import Counter
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence
from urllib.parse import urlparse


RTL_SUFFIXES = {".v", ".sv", ".vh", ".svh"}
PROVIDERS = {"github", "gitlab", "codeberg", "sourcehut", "git"}
_TOOL_VERSION_CACHE: dict[str, str | None] = {}
_WSL_CAD_BIN = "/opt/oss-cad/oss-cad-suite/bin"


class CrawlerError(RuntimeError):
    """Raised for malformed crawler configuration or an unusable source."""


@dataclasses.dataclass(frozen=True)
class SourceSpec:
    name: str
    url: str
    provider: str
    ref: str | None = None
    commit: str | None = None
    subdir: str = "."
    enabled: bool = True
    local_path: str | None = None
    include_dirs: tuple[str, ...] = ()
    license: str | None = None
    priority: str | None = None
    families: tuple[str, ...] = ()


@dataclasses.dataclass(frozen=True)
class PortInfo:
    name: str
    direction: str
    width: str = "1"


@dataclasses.dataclass(frozen=True)
class ModuleInfo:
    name: str
    path: str
    ports: tuple[PortInfo, ...]
    parameters: tuple[str, ...] = ()
    dependencies: tuple[str, ...] = ()
    includes: tuple[str, ...] = ()
    imports: tuple[str, ...] = ()


def detect_provider(url: str) -> str:
    """Return a stable provider name for a repository URL or local path."""

    if re.match(r"^[A-Za-z]:[\\/]", url) or url.startswith("\\\\"):
        return "local"
    parsed = urlparse(url)
    if not parsed.scheme or parsed.scheme == "file":
        return "local"
    host = parsed.hostname.lower() if parsed.hostname else ""
    if host == "github.com" or host.endswith(".github.com"):
        return "github"
    if host == "gitlab.com" or host.endswith(".gitlab.com"):
        return "gitlab"
    if host == "codeberg.org" or host.endswith(".codeberg.org"):
        return "codeberg"
    if host in {"sr.ht", "git.sr.ht"} or host.endswith(".sr.ht"):
        return "sourcehut"
    return "git"


def _expand(value: str, *, config_dir: Path) -> str:
    value = os.path.expandvars(value)
    value = value.replace("${CONFIG_DIR}", str(config_dir))
    return value


def _load_yaml_minimal(text: str) -> Any:
    """Load the tiny YAML subset used by crawler configs without dependencies.

    Full YAML remains supported when PyYAML is installed.  This fallback is
    intentionally conservative: mappings, lists, strings, booleans, numbers,
    and comments are enough for source/target manifests and avoid silently
    interpreting complex YAML in a surprising way.
    """

    try:
        import yaml  # type: ignore
    except ImportError:
        pass
    else:
        return yaml.safe_load(text)
    # A useful error is safer than a partially parsed manifest.
    raise CrawlerError("YAML config requires PyYAML; use JSON or install pyyaml")


def load_config(path: str | Path) -> dict[str, Any]:
    path = Path(path)
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        raise CrawlerError(f"cannot read config {path}: {exc}") from exc
    try:
        value = json.loads(text) if path.suffix.lower() == ".json" else _load_yaml_minimal(text)
    except (json.JSONDecodeError, TypeError) as exc:
        raise CrawlerError(f"invalid config {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise CrawlerError("crawler config must be a mapping")
    return value


def parse_sources(config: Mapping[str, Any], *, config_dir: Path) -> list[SourceSpec]:
    raw = config.get("sources", [])
    if not isinstance(raw, list):
        raise CrawlerError("sources must be a list")
    result: list[SourceSpec] = []
    for item in raw:
        if not isinstance(item, Mapping):
            raise CrawlerError("each source must be a mapping")
        name = str(item.get("name", "")).strip()
        # ``repo`` is accepted for compatibility with the pyCircuit v0.14
        # source manifest; ``url`` remains the canonical spelling here.
        url = str(item.get("url", item.get("repo", item.get("path", "")))).strip()
        if not name or not url:
            raise CrawlerError("source requires name and url/path")
        local = item.get("local_path")
        local_path = _expand(str(local), config_dir=config_dir) if local else None
        provider = str(item.get("provider", detect_provider(url))).lower()
        if provider == "generic":
            provider = "git"
        if provider != "local" and provider not in PROVIDERS:
            raise CrawlerError(f"unsupported source provider {provider!r}")
        include_dirs = tuple(_expand(str(value), config_dir=config_dir) for value in item.get("include_dirs", []) if value)
        license_name = str(item.get("license", "")).strip() or None
        families = tuple(str(value) for value in item.get("families", []) if value)
        priority = str(item.get("priority", "")).strip() or None
        result.append(SourceSpec(
            name=name,
            url=url,
            provider=provider,
            ref=str(item.get("ref")) if item.get("ref") else None,
            commit=str(item.get("commit")) if item.get("commit") else None,
            subdir=str(item.get("subdir", ".")),
            enabled=bool(item.get("enabled", True)),
            local_path=local_path,
            include_dirs=include_dirs,
            license=license_name,
            priority=priority,
            families=families,
        ))
    return result


def discover_rtl_files(root: str | Path, *, max_files: int = 256, overflow: str = "error") -> list[Path]:
    """Find RTL files in deterministic order, bounded by ``max_files``.

    ``error`` is the release-safe default.  ``truncate`` is useful for broad
    exploratory scans of large repositories; the catalog records that the
    source was bounded so a truncated result is never mistaken for complete
    coverage.
    """

    root = Path(root)
    if not root.is_dir():
        raise CrawlerError(f"RTL source directory does not exist: {root}")
    files = sorted((p for p in root.rglob("*") if p.is_file() and p.suffix.lower() in RTL_SUFFIXES), key=lambda p: p.as_posix().lower())
    if overflow not in {"error", "truncate"}:
        raise CrawlerError(f"unknown file overflow policy {overflow!r}")
    if len(files) > max_files and overflow == "error":
        raise CrawlerError(f"source {root} contains {len(files)} RTL files (limit {max_files})")
    return files[:max_files] if overflow == "truncate" else files


_MODULE_RE = re.compile(r"\bmodule\s+(?P<name>[A-Za-z_][A-Za-z0-9_$]*)\b(?P<header>.*?);", re.S)
_PORT_RE = re.compile(r"\b(input|output|inout)\b\s*(?:(?:wire|reg|logic|signed|unsigned)\s+)*(?:\[[^]]+\]\s*)?(?P<name>[A-Za-z_][A-Za-z0-9_$]*)", re.I)
_PARAM_RE = re.compile(r"\bparameter\b[^,)]*?(?P<name>[A-Za-z_][A-Za-z0-9_$]*)\s*(?==|[,)]|$)", re.I)
_INCLUDE_RE = re.compile(r"`include\s+\"([^\"]+)\"")
_PACKAGE_RE = re.compile(r"\bpackage\s+([A-Za-z_][A-Za-z0-9_$]*)\s*;")
_IMPORT_RE = re.compile(r"\bimport\s+([A-Za-z_][A-Za-z0-9_$]*)\s*::")
_COMMENT_RE = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)
_INSTANT_RE = re.compile(r"\b(?P<module>[A-Za-z_][A-Za-z0-9_$]*)\s*(?:#\s*\([^;]*?\))?\s+[A-Za-z_][A-Za-z0-9_$]*\s*\(", re.S)
_KEYWORDS = {"module", "endmodule", "assign", "always", "begin", "end", "if", "else", "for", "case", "function", "task", "wire", "reg", "logic", "input", "output", "inout", "generate", "genvar"}


def parse_rtl_modules(path: str | Path) -> list[ModuleInfo]:
    path = Path(path)
    text = path.read_text(encoding="utf-8", errors="replace")
    # Comments frequently contain prose such as "the module calculates ...";
    # mask them before looking for declarations so documentation is never
    # reported as an RTL module.  Newlines are retained for useful diagnostics.
    scan_text = _COMMENT_RE.sub(lambda match: "".join("\n" if char == "\n" else " " for char in match.group(0)), text)
    result: list[ModuleInfo] = []
    for match in _MODULE_RE.finditer(scan_text):
        header = match.group("header")
        ports: list[PortInfo] = []
        # Keep direction/width together for ANSI and simple non-ANSI headers.
        for port_match in _PORT_RE.finditer(header):
            direction = port_match.group(1).lower()
            before = header[max(0, port_match.start() - 100) : port_match.end()]
            width_match = re.search(r"(\[[^]]+\])\s*" + re.escape(port_match.group("name")) + r"\s*$", before)
            ports.append(PortInfo(port_match.group("name"), direction, width_match.group(1) if width_match else "1"))
        params = tuple(dict.fromkeys(_PARAM_RE.findall(header)))
        instantiations = []
        end_match = re.search(r"\bendmodule\b", scan_text[match.end() :], re.S)
        body_end = match.end() + end_match.start() if end_match else len(text)
        body = scan_text[match.end() : body_end]
        for inst in _INSTANT_RE.finditer(body):
            name = inst.group("module")
            if name not in _KEYWORDS and name != match.group("name"):
                instantiations.append(name)
        includes = tuple(dict.fromkeys(_INCLUDE_RE.findall(body)))
        imports = tuple(dict.fromkeys(_IMPORT_RE.findall(body)))
        result.append(ModuleInfo(match.group("name"), str(path), tuple(ports), params, tuple(dict.fromkeys(instantiations)), includes, imports))
    return result


def index_rtl(root: str | Path, *, max_files: int = 256, overflow: str = "error") -> dict[str, ModuleInfo]:
    modules: dict[str, ModuleInfo] = {}
    for path in discover_rtl_files(root, max_files=max_files, overflow=overflow):
        for module in parse_rtl_modules(path):
            modules.setdefault(module.name, module)
    return modules


def dependency_closure(modules: Mapping[str, ModuleInfo], roots: Iterable[str]) -> list[ModuleInfo]:
    """Return root modules and in-tree instantiated modules in stable order."""

    seen: set[str] = set()
    result: list[ModuleInfo] = []

    def visit(name: str) -> None:
        if name in seen or name not in modules:
            return
        seen.add(name)
        module = modules[name]
        result.append(module)
        for dep in module.dependencies:
            visit(dep)

    for root in roots:
        visit(root)
    return result


def include_closure(files: Sequence[Path], *, search_roots: Sequence[Path] = ()) -> list[Path]:
    """Add local ``include``/package files without traversing outside roots."""

    result = list(dict.fromkeys(Path(path) for path in files))
    roots = [Path(root).resolve() for root in search_roots]
    package_files: dict[str, Path] = {}
    for root in roots:
        try:
            package_candidates = (path for path in root.rglob("*") if path.is_file() and path.suffix.lower() in RTL_SUFFIXES)
            for candidate in package_candidates:
                try:
                    package_text = candidate.read_text(encoding="utf-8", errors="replace")
                except OSError:
                    continue
                for package in _PACKAGE_RE.findall(package_text):
                    package_files.setdefault(package, candidate.resolve())
        except OSError:
            continue
    queue = list(result)
    while queue:
        path = queue.pop(0)
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
            names = _INCLUDE_RE.findall(text)
            imported_packages = _IMPORT_RE.findall(text)
        except OSError:
            continue
        for name in names:
            candidates = [path.parent / name] + [root / name for root in roots]
            include = next((candidate.resolve() for candidate in candidates if candidate.is_file()), None)
            if include is None or (roots and not any(_under_root(include, root) for root in roots)):
                continue
            if include not in result:
                result.append(include)
                queue.append(include)
        for package in imported_packages:
            package_path = package_files.get(package)
            if package_path is not None and package_path not in result:
                result.append(package_path)
                queue.append(package_path)
    return result


def _under_root(path: Path, root: Path) -> bool:
    try:
        path.relative_to(root)
    except ValueError:
        return False
    return True


def _source_root(spec: SourceSpec, *, cache_dir: Path, config_dir: Path, clone_timeout: int) -> tuple[Path, dict[str, Any]]:
    if spec.local_path:
        path = Path(spec.local_path)
    elif spec.provider == "local" or not urlparse(spec.url).scheme:
        parsed = urlparse(spec.url)
        raw_path = parsed.path if parsed.scheme == "file" else _expand(spec.url, config_dir=config_dir)
        if os.name == "nt" and re.match(r"^/[A-Za-z]:[\\/]", raw_path):
            raw_path = raw_path[1:]
        path = Path(raw_path)
        if not path.is_absolute():
            path = config_dir / path
    else:
        cache_dir.mkdir(parents=True, exist_ok=True)
        path = cache_dir / re.sub(r"[^A-Za-z0-9_.-]+", "_", spec.name)
        if not path.exists():
            command = ["git", "clone", "--depth", "1"]
            if spec.ref:
                command += ["--branch", spec.ref]
            command += [spec.url, str(path)]
            completed = subprocess.run(command, capture_output=True, text=True, timeout=clone_timeout, check=False)
            if completed.returncode:
                raise CrawlerError(f"git clone failed for {spec.name}: {completed.stderr.strip()}")
    root = (path / spec.subdir).resolve()
    if not root.is_dir():
        raise CrawlerError(f"source {spec.name} subdir does not exist: {root}")
    commit = None
    git = shutil.which("git")
    if git:
        completed = subprocess.run([git, "-C", str(path), "rev-parse", "HEAD"], capture_output=True, text=True, timeout=clone_timeout, check=False)
        if completed.returncode == 0:
            commit = completed.stdout.strip()
        # A shallow branch clone may not start at the pinned revision.  Fetch
        # exactly that object for remote sources, then detach at it.  Local
        # checkouts are never mutated: a mismatch is reported below so the
        # caller can fix the cache explicitly.
        if spec.commit and commit != spec.commit and spec.provider != "local" and not spec.local_path:
            fetched = subprocess.run([git, "-C", str(path), "fetch", "--depth", "1", "origin", spec.commit], capture_output=True, text=True, timeout=clone_timeout, check=False)
            checked_out = subprocess.run([git, "-C", str(path), "checkout", "--detach", spec.commit], capture_output=True, text=True, timeout=clone_timeout, check=False) if fetched.returncode == 0 else fetched
            if checked_out.returncode == 0:
                commit = spec.commit
    if spec.commit and commit != spec.commit:
        raise CrawlerError(f"source {spec.name} is at {commit or 'unknown commit'}, expected pinned commit {spec.commit}")
    # Preserve declared licensing while also recording a conventional license
    # file when the source makes one available.  This is metadata-only; the
    # crawler never attempts to interpret or execute license text.
    license_file = None
    for candidate in (path / "LICENSE", path / "LICENSE.md", path / "COPYING", root / "LICENSE", root / "COPYING"):
        if candidate.is_file():
            license_file = str(candidate.resolve())
            break
    provenance: dict[str, Any] = {
        "commit": commit,
        "declared_commit": spec.commit,
        "ref": spec.ref,
        "resolved_path": str(root),
        "source_fingerprint": hashlib.sha256(f"{spec.url}\n{spec.ref or ''}\n{commit or ''}".encode("utf-8")).hexdigest()[:16],
    }
    if spec.license:
        provenance["license"] = spec.license
    if license_file:
        provenance["license_file"] = license_file
    return root, provenance


def _tool_command(tool: str) -> list[str] | None:
    """Resolve a native executable or ``wsl:<command>`` without a shell."""

    if tool.startswith("wsl:"):
        command = tool[4:].strip()
        if not command:
            return None
        parts = shlex.split(command)
        # The same runner is used from both Windows and inside WSL.  When the
        # caller is already on Linux, invoking ``wsl.exe`` would start a
        # nested distribution and may return its diagnostics in the Windows
        # console encoding (which is not necessarily UTF-8).  Resolve the
        # requested command directly in that case; the ``wsl:`` spelling still
        # documents that the tool belongs to the CAD environment.
        if os.name != "nt":
            if parts and parts[0] in {"verilator", "yosys", "sta"}:
                return [f"{_WSL_CAD_BIN}/{parts[0]}", *parts[1:]]
            return parts
        if parts and parts[0] in {"verilator", "yosys", "sta"}:
            return ["wsl.exe", "--", f"{_WSL_CAD_BIN}/{parts[0]}", *parts[1:]]
        return ["wsl.exe", *parts]
    parts = shlex.split(tool) if any(char.isspace() for char in tool) else [tool]
    if not parts:
        return None
    executable = shutil.which(parts[0]) or (parts[0] if Path(parts[0]).exists() else None)
    if executable is None and os.name == "nt" and parts[0] in {"verilator", "yosys", "sta"} and shutil.which("wsl.exe"):
        # Windows developers commonly keep the HDL tools in WSL.  Prefer that
        # installation automatically, while retaining an explicit ``wsl:``
        # spelling for reproducible CI logs.
        return ["wsl.exe", "--", f"{_WSL_CAD_BIN}/{parts[0]}", *parts[1:]]
    return [executable, *parts[1:]] if executable else None


def _tool_version(tool: str | None) -> str | None:
    if not tool:
        return None
    if tool in _TOOL_VERSION_CACHE:
        return _TOOL_VERSION_CACHE[tool]
    command = _tool_command(tool)
    if not command:
        _TOOL_VERSION_CACHE[tool] = None
        return None
    try:
        result = subprocess.run([*command, "--version"], capture_output=True, text=True, timeout=10, check=False)
    except (OSError, subprocess.TimeoutExpired):
        _TOOL_VERSION_CACHE[tool] = None
        return None
    line = (result.stdout or result.stderr).splitlines()
    version = line[0].strip() if line and result.returncode == 0 else None
    _TOOL_VERSION_CACHE[tool] = version
    return version


def _run_gate(tool: str | None, command: Sequence[str], timeout: int, *, cwd: Path | None = None) -> dict[str, Any]:
    if not tool:
        return {"status": "skipped", "reason": "tool disabled"}
    tool_command = _tool_command(tool)
    if not tool_command:
        return {"status": "skipped", "reason": f"{tool} not found"}
    if tool_command[0].lower().endswith("wsl.exe") and _tool_version(tool) is None:
        return {"status": "skipped", "reason": f"{tool} is not installed in WSL"}
    if tool.startswith("wsl:") or tool_command[0].lower().endswith("wsl.exe"):
        command = [_wsl_arg(arg) for arg in command]
    try:
        result = subprocess.run([*tool_command, *command], capture_output=True, text=True, timeout=timeout, check=False, cwd=str(cwd) if cwd else None)
    except subprocess.TimeoutExpired as exc:
        return {"status": "failed", "reason": f"timeout after {timeout}s", "stderr": str(exc)}
    except OSError as exc:
        return {"status": "failed", "reason": str(exc)}
    return {"status": "passed" if result.returncode == 0 else "failed", "returncode": result.returncode, "stdout": result.stdout[-4000:], "stderr": result.stderr[-4000:]}


def _looks_like_windows_path(value: str) -> bool:
    return bool(re.match(r"^[A-Za-z]:[\\/]", value))


def _wsl_path(value: str) -> str:
    match = re.match(r"^([A-Za-z]):[\\/](.*)$", value)
    if not match:
        return value
    return "/mnt/" + match.group(1).lower() + "/" + match.group(2).replace("\\", "/")


def _wsl_arg(value: str) -> str:
    if _looks_like_windows_path(value):
        return _wsl_path(value)
    # Yosys receives a single script argument containing quoted source paths.
    if '"' in value:
        return re.sub(r"([A-Za-z]):[\\/]([^\";]+)", lambda match: "/mnt/" + match.group(1).lower() + "/" + match.group(2).replace("\\", "/"), value)
    # read_slang paths are intentionally unquoted (its command parser treats
    # quotes literally), so convert each space-delimited drive path separately.
    return re.sub(r"([A-Za-z]):[\\/][^\s;]+", lambda match: "/mnt/" + match.group(1).lower() + "/" + match.group(0)[3:].replace("\\", "/"), value)


def _quote_yosys(path: Path) -> str:
    return '"' + str(path).replace('"', '\\"') + '"'


def _parse_yosys_qor(stdout: str) -> dict[str, int]:
    """Extract small, stable QoR counters from Yosys ``stat`` output."""

    section = stdout.rsplit("=== design hierarchy ===", 1)[-1]
    result: dict[str, int] = {}
    for label, key in (("wires", "wires"), ("wire bits", "wire_bits"), ("ports", "ports"), ("port bits", "port_bits"), ("cells", "cells")):
        match = re.search(rf"(?m)^\s*(\d+)\s+{re.escape(label)}\s*$", section)
        if match:
            result[key] = int(match.group(1))
    return result


def validate_candidate(
    files: Sequence[Path],
    top: str,
    *,
    verilator: str | None = "verilator",
    yosys: str | None = "yosys",
    timeout: int = 30,
    verilator_timeout: int | None = None,
    yosys_timeout: int | None = None,
    include_dirs: Sequence[Path] = (),
) -> dict[str, Any]:
    provided_include_dirs = list(dict.fromkeys(Path(path).resolve() for path in include_dirs))
    include_dirs = list(dict.fromkeys([*provided_include_dirs, *(Path(file).parent.resolve() for file in files)]))
    # Headers are dependencies, not compilation units.  Feeding .svh/.vh
    # directly to Verilator/Yosys can hide the actual include-path problem and
    # causes Yosys to parse macro fragments as standalone Verilog modules.
    source_files = [Path(file) for file in files if Path(file).suffix.lower() not in {".svh", ".vh"}]
    if not source_files:
        source_files = [Path(file) for file in files]
    verilator_includes = [f"-I{directory}" for directory in include_dirs]
    verilator_cmd = ["--lint-only", "-Wno-fatal", "--top-module", top, *verilator_includes, *map(str, source_files)]
    # Yosys parses ``-Idir`` as one option token; quoting the directory after
    # ``-I`` makes the quote part of the option in some OSS CAD Suite builds.
    yosys_search_dirs = provided_include_dirs or include_dirs
    try:
        common_path = os.path.commonpath([str(path) for path in yosys_search_dirs]) if yosys_search_dirs else None
    except ValueError:
        common_path = None
    yosys_cwd = Path(common_path) if common_path else None
    # Keep Yosys include options relative to its working directory.  This
    # avoids Windows/WSL interop rewriting an option such as ``-I/mnt/e`` to
    # ``-IE:`` before Yosys sees it.
    relative_dirs = []
    for directory in yosys_search_dirs:
        relative = os.path.relpath(directory, yosys_cwd) if yosys_cwd else str(directory)
        relative_dirs.append(relative.replace("\\", "/"))
    yosys_includes = " ".join(f"-I{directory}" for directory in dict.fromkeys(relative_dirs))
    # The bundled OSS CAD Suite provides Yosys' Slang frontend.  It accepts
    # modern SystemVerilog constructs (packages, streaming-compatible code,
    # packed arrays) that the legacy Verilog frontend rejects or expands
    # pathologically.  Set ACIR_RUNTIME_YOSYS_FRONTEND=verilog to force the
    # legacy parser on installations that do not ship read_slang.
    frontend = os.environ.get("ACIR_RUNTIME_YOSYS_FRONTEND", "slang").strip().lower()
    read_command = "read_verilog -sv" if frontend == "verilog" else "read_slang"
    if read_command == "read_slang":
        # Slang builds one compilation unit so packages and macro imports are
        # visible to all dependent modules.  Separate read_slang invocations
        # would reject a later chparam or lose package symbols.
        # read_slang's command parser treats quote characters as part of the
        # filename; all packaged paths are space-free and can be passed raw.
        read_files = f"{read_command} {yosys_includes} {' '.join(str(path).replace(chr(92), '/') for path in source_files)};"
    else:
        read_files = " ".join(f"{read_command} {yosys_includes} {_quote_yosys(path)};" for path in source_files)
    # ``synth`` is intentionally small and deterministic; it both elaborates
    # the dependency closure and gives the catalog a synthesis/QoR gate.
    yosys_cmd = ["-p", f"{read_files} hierarchy -check -top {top}; synth -top {top}; check; stat"]
    yosys_result = _run_gate(yosys, yosys_cmd, max(1, yosys_timeout or timeout), cwd=yosys_cwd)
    if yosys_result.get("stdout"):
        qor = _parse_yosys_qor(str(yosys_result["stdout"]))
        if qor:
            yosys_result["qor"] = qor
    result = {"verilator": _run_gate(verilator, verilator_cmd, max(1, verilator_timeout or timeout)), "yosys": yosys_result, "synthesis": yosys_result}
    statuses = [item["status"] for item in result.values()]
    result["status"] = "failed" if "failed" in statuses else ("passed" if "passed" in statuses else "skipped")
    return result


def _target_matches(target: Mapping[str, Any], module: ModuleInfo, path: Path) -> bool:
    requested = target.get("module")
    if requested and requested != module.name:
        return False
    keywords = target.get("keywords", [])
    if keywords and not any(str(keyword).lower() in (module.name + " " + str(path)).lower() for keyword in keywords):
        return False
    return True


def generate_wrapper(module: ModuleInfo, target: Mapping[str, Any], output: str | Path) -> Path:
    """Emit a thin canonical-port wrapper around a discovered module."""

    interface = target.get("interface")
    if not isinstance(interface, Mapping):
        raise CrawlerError(f"target {target.get('name', module.name)} has no interface adapter")
    wrapper_name = str(interface.get("wrapper_module", "pyc_runtime_" + module.name))
    ports = interface.get("ports")
    if not isinstance(ports, list) or not ports:
        raise CrawlerError(f"target {target.get('name', module.name)} interface.ports must be a non-empty list")
    params = interface.get("parameters", [])
    lines = [f"// Generated by acir_runtime_crawler.py; source module: {module.name}", f"module {wrapper_name} #("]
    param_lines = []
    for param in params:
        if not isinstance(param, Mapping):
            continue
        param_lines.append(f"  parameter integer {param['name']} = {param.get('default', 1)}")
    lines.append(",\n".join(param_lines) if param_lines else "  parameter integer UNUSED = 1")
    lines.append(") (")
    decls = []
    connections = []
    for port in ports:
        if not isinstance(port, Mapping):
            continue
        name = str(port["name"])
        direction = str(port.get("direction", "input"))
        width = str(port.get("width", "1"))
        decls.append(f"  {direction} wire [{width}-1:0] {name}")
        source = str(port.get("source", name))
        connections.append(f"    .{source}({name})")
    lines.append(",\n".join(decls))
    lines.append(");")
    param_map = []
    for param in params:
        if isinstance(param, Mapping) and param.get("source"):
            param_map.append(f"    .{param['source']}({param['name']})")
    instance = f"  {module.name}"
    if param_map:
        instance += " #(\n" + ",\n".join(param_map) + "\n  )"
    instance += " u_impl (\n" + ",\n".join(connections) + "\n  );"
    lines += [instance, "endmodule", ""]
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines), encoding="utf-8")
    return output


def _target_list(config: Mapping[str, Any]) -> list[Mapping[str, Any]]:
    targets = config.get("targets", [])
    if not isinstance(targets, list):
        raise CrawlerError("targets must be a list")
    return [item for item in targets if isinstance(item, Mapping)]


def _read_checkpoint(path: Path, config_digest: str) -> dict[str, Any]:
    """Load only checkpoints produced for the exact same source config."""
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}
    if not isinstance(value, dict) or value.get("config_sha256") != config_digest:
        return {}
    entries = value.get("entries")
    if not isinstance(entries, dict):
        return {}
    safe_entries = {str(key): item for key, item in entries.items() if isinstance(item, Mapping)}
    return {"entries": safe_entries}


def _write_checkpoint(path: Path, config_digest: str, entries: Mapping[str, Mapping[str, Any]]) -> None:
    path.write_text(
        json.dumps({"schema": "acir-runtime-crawler-checkpoint-v0.2", "config_sha256": config_digest, "entries": entries}, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def crawl_runtime(
    config_path: str | Path,
    output_dir: str | Path,
    *,
    cache_dir: str | Path | None = None,
    verilator: str | None = "verilator",
    yosys: str | None = "yosys",
    timeout: int = 30,
    verilator_timeout: int | None = None,
    yosys_timeout: int | None = None,
    max_files: int = 256,
    max_candidates: int = 32,
    max_candidates_per_source: int = 0,
    clone_timeout: int = 60,
    no_tools: bool = False,
    source_filter: Sequence[str] = (),
    file_overflow: str = "error",
    resume: bool = False,
    package: bool = False,
) -> dict[str, Any]:
    """Discover, validate, and package a bounded runtime catalog.

    The v0.2 additions are deliberately opt-in or backward compatible:
    separate Verilator/Yosys timeouts, fair per-source candidate caps,
    deterministic file truncation for exploratory scans, source filtering, and
    checkpoint/resume.  Existing callers can continue to pass only ``timeout``
    and ``max_files``.
    """

    config_path = Path(config_path).resolve()
    config = load_config(config_path)
    config_dir = config_path.parent
    output_dir = Path(output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    cache_dir = Path(cache_dir or (output_dir / ".source-cache")).resolve()
    if no_tools:
        verilator = yosys = None
    sources = parse_sources(config, config_dir=config_dir)
    if source_filter:
        wanted = set(source_filter)
        sources = [source for source in sources if source.name in wanted]
        if not sources:
            raise CrawlerError("none of the requested --source names exist or are enabled")
    targets = _target_list(config)
    policy = config.get("policy", {})
    allow_tool_skip = bool(policy.get("allow_skipped_tools", True)) if isinstance(policy, Mapping) else True
    config_digest = hashlib.sha256(config_path.read_bytes()).hexdigest()
    checkpoint_path = output_dir / "checkpoint.json"
    prior = _read_checkpoint(checkpoint_path, config_digest) if resume else {}
    completed: dict[str, dict[str, Any]] = prior.get("entries", {}) if isinstance(prior, dict) else {}
    catalog: dict[str, Any] = {
        "schema": "acir-runtime-catalog-v0.1",
        "crawler_version": "0.2",
        "generated_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "tools": {"verilator": _tool_version(verilator), "yosys": _tool_version(yosys)},
        "entries": list(completed.values()),
        "limits": {
            "max_files": max_files,
            "file_overflow": file_overflow,
            "max_candidates": max_candidates,
            "max_candidates_per_source": max_candidates_per_source,
            "timeout_seconds": timeout,
            "verilator_timeout_seconds": verilator_timeout or timeout,
            "yosys_timeout_seconds": yosys_timeout or timeout,
            "clone_timeout_seconds": clone_timeout,
        },
        "source_filter": list(source_filter),
        "resumed": bool(prior),
    }
    candidate_count = sum(1 for entry in completed.values() if entry.get("candidate_key"))
    source_candidate_counts: dict[str, int] = Counter(str(entry.get("source", "")) for entry in completed.values() if entry.get("candidate_key"))
    wrappers_dir = output_dir / "verilog"
    package_dir = output_dir / "package"
    for source in sources:
        if not source.enabled:
            continue
        try:
            root, provenance = _source_root(source, cache_dir=cache_dir, config_dir=config_dir, clone_timeout=clone_timeout)
            # Probe one file beyond the configured limit so the catalog can
            # distinguish complete discovery from deterministic truncation.
            probe = discover_rtl_files(root, max_files=max_files + 1, overflow="truncate")
            source_truncated = len(probe) > max_files
            modules = index_rtl(root, max_files=max_files, overflow=file_overflow)
            provenance = {
                **provenance,
                "rtl_file_count": len(probe),
                "file_limit_status": "truncated" if source_truncated else "complete",
                "source_priority": source.priority,
                "families": list(source.families),
            }
        except (CrawlerError, OSError) as exc:
            catalog["entries"].append({"source": source.name, "provider": source.provider, "status": "source-error", "error": str(exc)})
            _write_checkpoint(checkpoint_path, config_digest, {entry.get("candidate_key", f"source-error:{entry.get('source', '')}"): entry for entry in catalog["entries"] if isinstance(entry, Mapping)})
            continue
        source_count = source_candidate_counts.get(source.name, 0)
        for target in targets:
            wanted_source = target.get("source")
            if wanted_source and wanted_source != source.name:
                continue
            for module in modules.values():
                if candidate_count >= max_candidates:
                    break
                if max_candidates_per_source and source_count >= max_candidates_per_source:
                    break
                if not _target_matches(target, module, Path(module.path)):
                    continue
                candidate_key = f"{source.name}|{target.get('name', module.name)}|{module.name}|{provenance.get('commit') or ''}"
                if candidate_key in completed:
                    continue
                candidate_count += 1
                source_count += 1
                closure = dependency_closure(modules, [module.name])
                extra_include_dirs = [Path(item) if Path(item).is_absolute() else root / item for item in source.include_dirs]
                files = include_closure([Path(item.path) for item in closure], search_roots=[root, *extra_include_dirs])
                entry: dict[str, Any] = {
                    "name": str(target.get("name", module.name)),
                    "candidate_key": candidate_key,
                    "candidate_id": hashlib.sha256(candidate_key.encode("utf-8")).hexdigest()[:16],
                    "target_priority": target.get("priority"),
                    "module": module.name,
                    "source": source.name,
                    "provider": source.provider,
                    "source_url": source.url,
                    "provenance": provenance,
                    "files": [str(item) for item in files],
                    "ports": [dataclasses.asdict(port) for port in module.ports],
                    "parameters": list(module.parameters),
                    "dependencies": [item.name for item in closure[1:]],
                }
                try:
                    wrapper = generate_wrapper(module, target, wrappers_dir / f"{entry['name']}.v")
                    entry["wrapper"] = str(wrapper)
                    if package:
                        # Copy the complete dependency closure into a
                        # self-contained package.  Relative paths are kept
                        # stable so the package can be moved or installed.
                        packaged_files: list[str] = []
                        source_package = package_dir / "sources" / source.name
                        for source_file in files:
                            source_path = Path(source_file).resolve()
                            try:
                                relative = source_path.relative_to(root.resolve())
                            except ValueError:
                                relative = Path(source_path.name)
                            destination = source_package / relative
                            destination.parent.mkdir(parents=True, exist_ok=True)
                            shutil.copy2(source_path, destination)
                            packaged_files.append(str(destination.relative_to(output_dir)).replace("\\", "/"))
                        entry["package"] = {
                            "root": str(package_dir.relative_to(output_dir)).replace("\\", "/"),
                            "files": packaged_files,
                            "wrapper": str(wrapper.relative_to(output_dir)).replace("\\", "/"),
                        }
                    wrapper_interface = target.get("interface")
                    validation_top = str(wrapper_interface.get("wrapper_module", module.name)) if isinstance(wrapper_interface, Mapping) else module.name
                    validation = validate_candidate(
                        [*files, wrapper], validation_top,
                        verilator=verilator,
                        yosys=yosys,
                        timeout=timeout,
                        verilator_timeout=verilator_timeout,
                        yosys_timeout=yosys_timeout,
                        include_dirs=[root, *extra_include_dirs],
                    )
                    entry["validation"] = validation
                    entry["validation_top"] = validation_top
                    gates_pass = validation["status"] != "failed" and (allow_tool_skip or all(validation[name]["status"] == "passed" for name in ("verilator", "yosys")))
                    entry["status"] = "accepted" if gates_pass else "rejected"
                    if not gates_pass and validation["status"] != "failed":
                        entry["error"] = "one or more validation tools were skipped (policy requires every gate)"
                except CrawlerError as exc:
                    entry["status"] = "unpackaged"
                    entry["error"] = str(exc)
                catalog["entries"].append(entry)
                completed[candidate_key] = entry
                source_candidate_counts[source.name] = source_count
                _write_checkpoint(checkpoint_path, config_digest, completed)
            if candidate_count >= max_candidates:
                break
        if candidate_count >= max_candidates:
            break
    catalog["accepted"] = sum(entry.get("status") == "accepted" for entry in catalog["entries"])
    catalog["rejected"] = sum(entry.get("status") == "rejected" for entry in catalog["entries"])
    catalog["errors"] = sum(entry.get("status") in {"source-error", "unpackaged"} for entry in catalog["entries"])
    catalog_path = output_dir / "catalog.json"
    catalog_path.write_text(json.dumps(catalog, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    _write_checkpoint(checkpoint_path, config_digest, completed)
    return catalog


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--cache-dir", type=Path)
    parser.add_argument("--verilator", default="verilator", help="executable or empty to skip")
    parser.add_argument("--yosys", default="yosys", help="executable or empty to skip")
    parser.add_argument("--timeout", type=int, default=30)
    parser.add_argument("--verilator-timeout", type=int, help="override Verilator timeout (seconds)")
    parser.add_argument("--yosys-timeout", type=int, help="override Yosys timeout (seconds)")
    parser.add_argument("--clone-timeout", type=int, default=60)
    parser.add_argument("--max-files", type=int, default=256)
    parser.add_argument("--max-candidates", type=int, default=32)
    parser.add_argument("--max-candidates-per-source", type=int, default=0, help="fairness cap per source; 0 means only the global cap applies")
    parser.add_argument("--file-overflow", choices=("error", "truncate"), default="error", help="fail on oversized sources or deterministically scan the first max-files")
    parser.add_argument("--source", action="append", default=[], help="restrict the crawl to a named source; repeatable")
    parser.add_argument("--resume", action="store_true", help="reuse completed entries from output/checkpoint.json when the config is unchanged")
    parser.add_argument("--package", action="store_true", help="copy accepted dependency closures into output/package for relocation")
    parser.add_argument("--no-tools", action="store_true")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        catalog = crawl_runtime(args.config, args.output, cache_dir=args.cache_dir, verilator=args.verilator or None, yosys=args.yosys or None, timeout=max(1, args.timeout), verilator_timeout=max(1, args.verilator_timeout) if args.verilator_timeout else None, yosys_timeout=max(1, args.yosys_timeout) if args.yosys_timeout else None, clone_timeout=max(1, args.clone_timeout), max_files=max(1, args.max_files), max_candidates=max(1, args.max_candidates), max_candidates_per_source=max(0, args.max_candidates_per_source), source_filter=args.source, file_overflow=args.file_overflow, resume=args.resume, package=args.package, no_tools=args.no_tools)
    except CrawlerError as exc:
        print(f"acir-runtime-crawl: error: {exc}", file=sys.stderr)
        return 2
    print(json.dumps({"catalog": str(Path(args.output).resolve() / 'catalog.json'), "accepted": catalog["accepted"], "rejected": catalog["rejected"], "errors": catalog["errors"]}, sort_keys=True))
    return 0 if catalog["rejected"] == 0 and catalog["errors"] == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
