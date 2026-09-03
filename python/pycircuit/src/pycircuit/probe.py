from __future__ import annotations

import fnmatch
import inspect
import json
from collections.abc import Iterable, Mapping
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .path_shortening import InstancePathShorteningPolicy, xxhash64


class ProbeError(RuntimeError):
    pass


def _normalize_at(at: str | None) -> str:
    raw = "xfer" if at is None else str(at).strip().lower()
    if raw in {"pre"}:
        return "tick"
    if raw in {"post"}:
        return "xfer"
    if raw not in {"tick", "xfer"}:
        raise ProbeError("probe `at` must be 'tick' or 'xfer'")
    return raw


def _normalize_tags(tags: Mapping[str, Any] | None) -> dict[str, Any]:
    if not tags:
        return {}
    out: dict[str, Any] = {}
    for k in sorted(tags.keys(), key=lambda x: str(x)):
        kk = str(k).strip()
        if not kk:
            raise ProbeError("probe tag keys must be non-empty")
        v = tags[k]
        if v is None:
            continue
        if isinstance(v, (bool, int, str)):
            out[kk] = v
        else:
            out[kk] = str(v)
    return out


def _join_path(prefix: str, leaf: str) -> str:
    if not prefix:
        return leaf
    if not leaf:
        return prefix
    return f"{prefix}.{leaf}"


def _flatten_probe_value(value: Any, *, prefix: str = "") -> list[tuple[str, ProbeRef]]:
    out: list[tuple[str, ProbeRef]] = []

    def rec(v: Any, path: str) -> None:
        if isinstance(v, ProbeRef):
            out.append((path or "value", v))
            return
        if isinstance(v, Mapping):
            for raw_key in sorted(v.keys(), key=lambda x: str(x)):
                key = str(raw_key).strip()
                if not key:
                    raise ProbeError("probe emit mapping keys must be non-empty")
                rec(v[raw_key], _join_path(path, key))
            return
        if isinstance(v, (tuple, list)):
            for idx, item in enumerate(v):
                rec(item, _join_path(path, str(idx)))
            return
        flatten = getattr(v, "flatten", None)
        if callable(flatten):
            flat = flatten()
            if isinstance(flat, Mapping):
                rec(flat, path)
                return
        raise ProbeError(f"unsupported probe emit value: {type(v).__name__}")

    rec(value, prefix)
    return out


@dataclass(frozen=True)
class ProbeCatalogInstance:
    module: str
    instance_path: str


@dataclass(frozen=True)
class ProbeCatalogEntry:
    canonical_path: str
    instance_path: str
    field_path: str
    module: str
    kind: str
    subkind: str
    dir: str
    width_bits: int
    ty: str

    def as_dict(self) -> dict[str, Any]:
        return {
            "canonical_path": self.canonical_path,
            "instance_path": self.instance_path,
            "field_path": self.field_path,
            "module": self.module,
            "kind": self.kind,
            "subkind": self.subkind,
            "dir": self.dir,
            "width_bits": int(self.width_bits),
            "ty": self.ty,
        }


@dataclass(frozen=True)
class ProbeCatalog:
    version: int
    top: str
    root_instance: str
    instances: tuple[ProbeCatalogInstance, ...]
    entries: tuple[ProbeCatalogEntry, ...]

    @staticmethod
    def from_dict(obj: Mapping[str, Any]) -> ProbeCatalog:
        version = int(obj.get("version", 1))
        top = str(obj.get("top", "")).strip()
        root_instance = str(obj.get("root_instance", "dut")).strip() or "dut"
        raw_instances = obj.get("instances", [])
        raw_entries = obj.get("entries", [])
        if not isinstance(raw_instances, list) or not isinstance(raw_entries, list):
            raise ProbeError(
                "invalid probe catalog: `instances` and `entries` must be lists"
            )
        instances: list[ProbeCatalogInstance] = []
        for raw in raw_instances:
            if not isinstance(raw, Mapping):
                continue
            module = str(raw.get("module", "")).strip()
            instance_path = str(raw.get("instance_path", "")).strip()
            if module and instance_path:
                instances.append(
                    ProbeCatalogInstance(module=module, instance_path=instance_path)
                )
        entries: list[ProbeCatalogEntry] = []
        for raw in raw_entries:
            if not isinstance(raw, Mapping):
                continue
            canonical_path = str(raw.get("canonical_path", "")).strip()
            instance_path = str(raw.get("instance_path", "")).strip()
            field_path = str(raw.get("field_path", "")).strip()
            module = str(raw.get("module", "")).strip()
            kind = str(raw.get("kind", "")).strip()
            subkind = str(raw.get("subkind", "")).strip()
            dir_ = str(raw.get("dir", "")).strip()
            ty = str(raw.get("ty", "")).strip()
            if not canonical_path or not instance_path or not field_path or not module:
                continue
            entries.append(
                ProbeCatalogEntry(
                    canonical_path=canonical_path,
                    instance_path=instance_path,
                    field_path=field_path,
                    module=module,
                    kind=kind,
                    subkind=subkind,
                    dir=dir_,
                    width_bits=int(raw.get("width_bits", 0)),
                    ty=ty,
                )
            )
        return ProbeCatalog(
            version=version,
            top=top,
            root_instance=root_instance,
            instances=tuple(instances),
            entries=tuple(entries),
        )

    def as_dict(self) -> dict[str, Any]:
        return {
            "version": int(self.version),
            "top": self.top,
            "root_instance": self.root_instance,
            "instances": [
                {"module": inst.module, "instance_path": inst.instance_path}
                for inst in self.instances
            ],
            "entries": [entry.as_dict() for entry in self.entries],
        }


def load_probe_catalog(path: Path) -> ProbeCatalog:
    p = Path(path).resolve()
    if not p.is_file():
        raise ProbeError(f"probe catalog not found: {p}")
    try:
        obj = json.loads(p.read_text(encoding="utf-8"))
    except Exception as e:  # noqa: BLE001
        raise ProbeError(f"failed to parse probe catalog JSON: {p}") from e
    if not isinstance(obj, Mapping):
        raise ProbeError(f"invalid probe catalog JSON: {p}")
    return ProbeCatalog.from_dict(obj)


@dataclass(frozen=True)
class ProbeRef:
    relative_path: str
    source: ProbeCatalogEntry


class ProbeView:
    def __init__(
        self, *, root_instance: str, entries: Iterable[ProbeCatalogEntry]
    ) -> None:
        self.root_instance = str(root_instance)
        prefix = f"{self.root_instance}."
        index: dict[str, ProbeRef] = {}
        for entry in entries:
            if entry.instance_path == self.root_instance:
                rel = entry.field_path
            elif entry.instance_path.startswith(prefix):
                child = entry.instance_path[len(prefix) :]
                rel = f"{child}.{entry.field_path}"
            else:
                continue
            ref = ProbeRef(relative_path=rel, source=entry)
            index[rel] = ref
        self._index = index

    def read(self, path: str) -> ProbeRef:
        key = str(path).strip()
        if not key:
            raise ProbeError("probe read path must be non-empty")
        ref = self._index.get(key)
        if ref is None:
            raise ProbeError(
                f"probe path not found under {self.root_instance}: {key!r}"
            )
        return ref

    def glob(self, pattern: str) -> tuple[ProbeRef, ...]:
        pat = str(pattern).strip()
        if not pat:
            raise ProbeError("probe glob pattern must be non-empty")
        out = [
            ref
            for key, ref in sorted(self._index.items())
            if fnmatch.fnmatchcase(key, pat)
        ]
        return tuple(out)

    def paths(self) -> tuple[str, ...]:
        return tuple(sorted(self._index.keys()))


@dataclass(frozen=True)
class ResolvedProbeLeaf:
    probe_name: str
    target_module: str
    target_instance: str
    field_path: str
    canonical_path: str
    source_path: str
    source_relative_path: str
    source_module: str
    kind: str
    subkind: str
    width_bits: int
    ty: str
    at: str
    tags: Mapping[str, Any]

    def as_dict(self) -> dict[str, Any]:
        return {
            "probe_name": self.probe_name,
            "target_module": self.target_module,
            "target_instance": self.target_instance,
            "field_path": self.field_path,
            "canonical_path": self.canonical_path,
            "source_path": self.source_path,
            "source_relative_path": self.source_relative_path,
            "source_module": self.source_module,
            "kind": self.kind,
            "subkind": self.subkind,
            "width_bits": int(self.width_bits),
            "ty": self.ty,
            "at": self.at,
            "tags": dict(self.tags),
        }


class ProbeBuilder:
    def __init__(
        self, *, probe_name: str, target_module: str, target_instance: str
    ) -> None:
        self.probe_name = str(probe_name)
        self.target_module = str(target_module)
        self.target_instance = str(target_instance)
        self._leaves: dict[str, ResolvedProbeLeaf] = {}

    def emit(
        self,
        prefix: str,
        value: ProbeRef | Mapping[str, Any] | Any,
        *,
        at: str = "xfer",
        tags: dict[str, object] | None = None,
    ) -> None:
        raw_prefix = str(prefix).strip()
        if not raw_prefix:
            raise ProbeError("probe emit prefix must be non-empty")
        at_norm = _normalize_at(at)
        tags_norm = _normalize_tags(tags)
        for suffix, ref in _flatten_probe_value(value, prefix=raw_prefix):
            field_path = str(suffix).strip()
            canonical_path = (
                f"{self.target_instance}:probe.{self.probe_name}.{field_path}"
            )
            leaf = ResolvedProbeLeaf(
                probe_name=self.probe_name,
                target_module=self.target_module,
                target_instance=self.target_instance,
                field_path=field_path,
                canonical_path=canonical_path,
                source_path=ref.source.canonical_path,
                source_relative_path=ref.relative_path,
                source_module=ref.source.module,
                kind=ref.source.kind,
                subkind=ref.source.subkind,
                width_bits=int(ref.source.width_bits),
                ty=ref.source.ty,
                at=at_norm,
                tags=tags_norm,
            )
            prev = self._leaves.get(field_path)
            if prev is not None and prev != leaf:
                raise ProbeError(
                    f"duplicate probe leaf {canonical_path!r} resolves to multiple sources "
                    f"({prev.source_path!r} vs {leaf.source_path!r})"
                )
            self._leaves[field_path] = leaf

    def leaves(self) -> tuple[ResolvedProbeLeaf, ...]:
        return tuple(self._leaves[key] for key in sorted(self._leaves.keys()))


@dataclass(frozen=True)
class ResolvedProbePlan:
    name: str
    target_base: str
    target_symbols: tuple[str, ...]
    leaves: tuple[ResolvedProbeLeaf, ...]

    def as_dict(self) -> dict[str, Any]:
        return {
            "version": 1,
            "name": self.name,
            "target_base": self.target_base,
            "target_symbols": list(self.target_symbols),
            "leaves": [leaf.as_dict() for leaf in self.leaves],
        }


@dataclass(frozen=True)
class TbProbeHandle:
    path: str
    source_path: str
    at: str
    tags: Mapping[str, Any]
    width_bits: int
    ty: str


class TbProbes:
    def __init__(self, handles: Iterable[TbProbeHandle]) -> None:
        self._by_path = {handle.path: handle for handle in handles}

    def __getitem__(self, path: str) -> TbProbeHandle:
        key = str(path).strip()
        if key not in self._by_path:
            raise ProbeError(f"unknown testbench probe path: {key!r}")
        return self._by_path[key]

    def glob(self, pattern: str) -> tuple[TbProbeHandle, ...]:
        pat = str(pattern).strip()
        if not pat:
            raise ProbeError("testbench probe glob must be non-empty")
        return tuple(
            handle
            for key, handle in sorted(self._by_path.items())
            if fnmatch.fnmatchcase(key, pat)
        )

    def paths(self) -> tuple[str, ...]:
        return tuple(sorted(self._by_path.keys()))

    @staticmethod
    def from_probe_manifest(obj: Mapping[str, Any]) -> TbProbes:
        raw_probes = obj.get("probes", [])
        if not isinstance(raw_probes, list):
            raise ProbeError("invalid probe manifest: missing `probes` list")
        handles: list[TbProbeHandle] = []
        for raw in raw_probes:
            if not isinstance(raw, Mapping):
                continue
            path = str(raw.get("canonical_path", "")).strip()
            if not path or ":probe." not in path:
                continue
            handles.append(
                TbProbeHandle(
                    path=path,
                    source_path=str(raw.get("source_path", "")).strip(),
                    at=_normalize_at(str(raw.get("obs", "xfer"))),
                    tags=_normalize_tags(
                        raw.get("tags") if isinstance(raw.get("tags"), Mapping) else {}
                    ),
                    width_bits=int(raw.get("width_bits", 0)),
                    ty=str(raw.get("ty", "")).strip(),
                )
            )
        return TbProbes(handles)


def collect_probe_functions(mod: object) -> list[Any]:
    out: list[Any] = []
    for _name, value in sorted(vars(mod).items(), key=lambda kv: kv[0]):
        if callable(value) and getattr(value, "__pycircuit_kind__", None) == "probe":
            out.append(value)
    return out


def resolve_probe_function(
    probe_fn: Any,
    *,
    catalog: ProbeCatalog,
    target_base: str,
    target_symbols: Iterable[str],
    params_by_symbol: Mapping[str, Mapping[str, Any]],
) -> ResolvedProbePlan:
    target_symbols_tuple = tuple(
        sorted(set(str(sym) for sym in target_symbols if str(sym).strip()))
    )
    if not target_symbols_tuple:
        raise ProbeError(
            f"probe {getattr(probe_fn, '__name__', probe_fn)!r} target {target_base!r} did not match any compiled symbols"
        )
    probe_name = str(
        getattr(
            probe_fn, "__pycircuit_probe_name__", getattr(probe_fn, "__name__", "probe")
        )
    ).strip()
    if not probe_name:
        raise ProbeError("probe name must be non-empty")

    sig = inspect.signature(probe_fn)
    params = list(sig.parameters.values())
    if len(params) < 2:
        raise ProbeError(f"@probe {probe_name!r} must take at least `(p, dut)`")

    leaves: list[ResolvedProbeLeaf] = []
    for inst in catalog.instances:
        if inst.module not in target_symbols_tuple:
            continue
        visible_entries = [
            entry
            for entry in catalog.entries
            if entry.instance_path == inst.instance_path
            or entry.instance_path.startswith(f"{inst.instance_path}.")
        ]
        view = ProbeView(root_instance=inst.instance_path, entries=visible_entries)
        builder = ProbeBuilder(
            probe_name=probe_name,
            target_module=inst.module,
            target_instance=inst.instance_path,
        )
        call_kwargs: dict[str, Any] = {}
        bound_params = dict(params_by_symbol.get(inst.module, {}))
        for p in params[2:]:
            if p.kind in (
                inspect.Parameter.VAR_POSITIONAL,
                inspect.Parameter.VAR_KEYWORD,
            ):
                raise ProbeError(f"@probe {probe_name!r} must not use *args/**kwargs")
            if p.name in bound_params:
                call_kwargs[p.name] = bound_params[p.name]
                continue
            if p.default is inspect._empty:
                raise ProbeError(
                    f"@probe {probe_name!r} requires target param {p.name!r}, but it was not available for symbol {inst.module!r}"
                )
        try:
            probe_fn(builder, view, **call_kwargs)
        except ProbeError:
            raise
        except Exception as e:  # noqa: BLE001
            raise ProbeError(
                f"@probe {probe_name!r} failed for target instance {inst.instance_path!r}: {e}"
            ) from e
        leaves.extend(builder.leaves())

    return ResolvedProbePlan(
        name=probe_name,
        target_base=str(target_base),
        target_symbols=target_symbols_tuple,
        leaves=tuple(sorted(leaves, key=lambda leaf: leaf.canonical_path)),
    )


def build_resolved_probe_manifest(
    *,
    top: str,
    root_instance: str,
    explicit_plans: Iterable[ResolvedProbePlan],
    catalog: ProbeCatalog | None = None,
) -> dict[str, Any]:
    entries: list[dict[str, Any]] = []
    seen: set[str] = set()
    if catalog is not None:
        for raw in catalog.entries:
            if raw.canonical_path in seen:
                continue
            seen.add(raw.canonical_path)
            entries.append(
                {
                    "canonical_path": raw.canonical_path,
                    "instance_path": raw.instance_path,
                    "field_path": raw.field_path,
                    "module": raw.module,
                    "kind": raw.kind,
                    "subkind": raw.subkind,
                    "dir": raw.dir,
                    "width_bits": int(raw.width_bits),
                    "ty": raw.ty,
                }
            )
    for plan in explicit_plans:
        for leaf in plan.leaves:
            if leaf.canonical_path in seen:
                raise ProbeError(
                    f"duplicate resolved probe path in manifest: {leaf.canonical_path!r}"
                )
            seen.add(leaf.canonical_path)
            entries.append(
                {
                    "canonical_path": leaf.canonical_path,
                    "instance_path": leaf.target_instance,
                    "field_path": f"probe.{leaf.probe_name}.{leaf.field_path}",
                    "module": leaf.target_module,
                    "kind": leaf.kind,
                    "subkind": leaf.subkind,
                    "dir": "probe",
                    "width_bits": int(leaf.width_bits),
                    "ty": leaf.ty,
                    "obs": leaf.at,
                    "tags": dict(leaf.tags),
                    "source_path": leaf.source_path,
                    "source_relative_path": leaf.source_relative_path,
                    "probe_name": leaf.probe_name,
                }
            )
    entries.sort(key=lambda entry: str(entry.get("canonical_path", "")))

    used_probe_ids: set[int] = set()
    for entry in entries:
        canonical_path = str(entry.get("canonical_path", "")).strip()
        if not canonical_path:
            continue
        suffix = 0
        while True:
            raw = canonical_path if suffix == 0 else f"{canonical_path}#{suffix}"
            probe_id = int(xxhash64(raw.encode("utf-8"), seed=0))
            if probe_id not in used_probe_ids:
                used_probe_ids.add(probe_id)
                entry["probe_id"] = f"0x{probe_id:016x}"
                break
            suffix += 1

    policy = InstancePathShorteningPolicy()
    return {
        "version": 3,
        "top": str(top),
        "root_instance": str(root_instance),
        "probe_count": len(entries),
        "instance_path_policy": {
            "max_segments": int(policy.max_segments),
            "max_chars": int(policy.max_chars),
            "keep_head": int(policy.keep_head),
            "keep_tail": int(policy.keep_tail),
        },
        "probes": entries,
    }
