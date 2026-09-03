from __future__ import annotations

import fnmatch
import json
import re
from collections.abc import Mapping
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from .design import Design
from .path_shortening import shorten_instance_path
from .tb import _sanitize_id


class TraceConfigError(RuntimeError):
    pass


def _as_str_list(v: Any, *, field: str) -> list[str]:
    if v is None:
        return []
    if isinstance(v, str):
        return [v]
    if isinstance(v, list):
        out: list[str] = []
        for x in v:
            if not isinstance(x, str):
                raise TraceConfigError(f"{field} must be a string list")
            s = x.strip()
            if not s:
                raise TraceConfigError(f"{field} entries must be non-empty strings")
            out.append(s)
        return out
    raise TraceConfigError(f"{field} must be a string list")


def _as_int(v: Any, *, field: str) -> int:
    try:
        iv = int(v)
    except Exception as e:  # noqa: BLE001
        raise TraceConfigError(f"{field} must be an integer") from e
    return int(iv)


def _as_int_list(v: Any, *, field: str) -> list[int]:
    if v is None:
        return []
    if isinstance(v, list):
        out: list[int] = []
        for x in v:
            out.append(_as_int(x, field=field))
        return out
    return [_as_int(v, field=field)]


def _unique_canonical_names(raw: list[str], *, ctx: str) -> list[str]:
    used: set[str] = set()
    out: list[str] = []
    for r in raw:
        s = str(r).strip()
        if not s:
            raise TraceConfigError(f"{ctx} entries must be non-empty strings")
        if s in used:
            raise TraceConfigError(f"duplicate {ctx} entry: {s!r}")
        used.add(s)
        out.append(s)
    return out


def _normalize_instance_glob(pat: str) -> str:
    p = str(pat).strip()
    if not p:
        raise TraceConfigError("instance glob patterns must be non-empty")
    # Convenience: patterns are relative to root if they don't name a root.
    if p == "**" or p.startswith("dut") or p.startswith("**."):
        return p
    return f"dut.{p}"


def _match_hier_glob(pat: str, path: str) -> bool:
    # Segment-wise glob where:
    # - "*" matches within a segment
    # - "**" matches 0+ segments
    p_segs = [s for s in str(pat).split(".") if s != ""]
    x_segs = [s for s in str(path).split(".") if s != ""]

    def rec(pi: int, xi: int) -> bool:
        if pi == len(p_segs):
            return xi == len(x_segs)
        if p_segs[pi] == "**":
            # Match zero segments.
            if rec(pi + 1, xi):
                return True
            # Match one segment and stay on "**".
            return xi < len(x_segs) and rec(pi, xi + 1)
        if xi >= len(x_segs):
            return False
        if not fnmatch.fnmatchcase(x_segs[xi], p_segs[pi]):
            return False
        return rec(pi + 1, xi + 1)

    return rec(0, 0)


@dataclass(frozen=True)
class TraceWindow:
    begin_cycle: int | None = None
    end_cycle: int | None = None

    def as_dict(self) -> dict[str, Any]:
        return {"begin_cycle": self.begin_cycle, "end_cycle": self.end_cycle}


@dataclass(frozen=True)
class ProbeSelector:
    families: tuple[str, ...] = ()
    stages: tuple[str, ...] = ()
    lanes: tuple[int, ...] = ()
    ats: tuple[str, ...] = ()
    tags: tuple[tuple[str, Any], ...] = ()

    def matches(self, meta: Mapping[str, Any]) -> bool:
        if self.ats:
            if str(meta.get("at", "")).strip().lower() not in set(self.ats):
                return False
        tags = meta.get("tags", {})
        if not isinstance(tags, Mapping):
            tags = {}

        def tag_str(key: str) -> str:
            v = tags.get(key)
            return "" if v is None else str(v).strip().lower()

        if self.families:
            if tag_str("family") not in set(self.families):
                return False
        if self.stages:
            if tag_str("stage") not in set(self.stages):
                return False
        if self.lanes:
            try:
                lane_v = int(tags.get("lane"))
            except Exception:  # noqa: BLE001
                return False
            if lane_v not in set(self.lanes):
                return False

        for k, v in self.tags:
            if tags.get(k) != v:
                return False
        return True


@dataclass(frozen=True)
class TraceRule:
    instance_globs: tuple[str, ...]
    port_globs: tuple[str, ...] = ()
    probes: ProbeSelector | None = None


@dataclass(frozen=True)
class TraceConfig:
    version: int
    rules: tuple[TraceRule, ...]
    window: TraceWindow | None = None
    source_json: str | None = None

    def as_dict(self) -> dict[str, Any]:
        out: dict[str, Any] = {"version": int(self.version)}
        out["rules"] = [
            {
                "instances": list(r.instance_globs),
                "ports": list(r.port_globs),
                "probes": (
                    None
                    if r.probes is None
                    else {
                        "families": list(r.probes.families),
                        "stages": list(r.probes.stages),
                        "lanes": list(r.probes.lanes),
                        "at": list(r.probes.ats),
                        "tags": [{k: v} for k, v in r.probes.tags],
                    }
                ),
            }
            for r in self.rules
        ]
        if self.window is not None:
            out["window"] = self.window.as_dict()
        return out


@dataclass(frozen=True)
class TracePlan:
    version: int
    enabled_signals: tuple[str, ...]
    enabled_instances: tuple[str, ...]
    signal_obs: dict[str, str] = field(default_factory=dict)
    window: TraceWindow | None = None
    config: TraceConfig | None = None

    def as_dict(self) -> dict[str, Any]:
        out: dict[str, Any] = {
            "version": int(self.version),
            "enabled_instances": list(self.enabled_instances),
            "enabled_signals": list(self.enabled_signals),
        }
        if self.signal_obs:
            out["signal_obs"] = dict(self.signal_obs)
        if self.window is not None:
            out["window"] = self.window.as_dict()
        if self.config is not None:
            out["config"] = self.config.as_dict()
        return out


def load_trace_config(path: Path) -> TraceConfig:
    p = Path(path).resolve()
    if not p.is_file():
        raise TraceConfigError(f"trace config not found: {p}")
    try:
        text = p.read_text(encoding="utf-8")
        obj = json.loads(text)
    except Exception as e:  # noqa: BLE001
        raise TraceConfigError(f"failed to parse trace config JSON: {p}") from e
    return parse_trace_config(obj, source_json=text)


def parse_trace_config(obj: Any, *, source_json: str | None = None) -> TraceConfig:
    if not isinstance(obj, Mapping):
        raise TraceConfigError("trace config must be a JSON object")
    version = int(obj.get("version", 1))
    if version != 1:
        raise TraceConfigError(f"unsupported trace config version: {version}")

    rules_raw = obj.get("rules", None)
    if not isinstance(rules_raw, list) or not rules_raw:
        raise TraceConfigError("trace config requires non-empty `rules` list")

    rules: list[TraceRule] = []
    for i, r in enumerate(rules_raw):
        if not isinstance(r, Mapping):
            raise TraceConfigError(f"rules[{i}] must be an object")
        inst_globs = [
            _normalize_instance_glob(x)
            for x in _as_str_list(r.get("instances"), field=f"rules[{i}].instances")
        ]
        if not inst_globs:
            raise TraceConfigError(f"rules[{i}].instances must be non-empty")

        port_globs = tuple(_as_str_list(r.get("ports"), field=f"rules[{i}].ports"))
        probes_obj = r.get("probes", None)
        probes: ProbeSelector | None = None
        if probes_obj is not None:
            if not isinstance(probes_obj, Mapping):
                raise TraceConfigError(f"rules[{i}].probes must be an object")
            families = tuple(
                s.strip().lower()
                for s in _as_str_list(
                    probes_obj.get("families"), field=f"rules[{i}].probes.families"
                )
            )
            stages = tuple(
                s.strip().lower()
                for s in _as_str_list(
                    probes_obj.get("stages"), field=f"rules[{i}].probes.stages"
                )
            )
            lanes = tuple(
                _as_int_list(probes_obj.get("lanes"), field=f"rules[{i}].probes.lanes")
            )
            ats = tuple(
                s.strip().lower()
                for s in _as_str_list(
                    probes_obj.get("at"), field=f"rules[{i}].probes.at"
                )
            )
            for at in ats:
                if at not in {"tick", "xfer"}:
                    raise TraceConfigError(
                        f"rules[{i}].probes.at entries must be 'tick' or 'xfer'"
                    )
            tags: list[tuple[str, Any]] = []
            tags_obj = probes_obj.get("tags", None)
            if tags_obj is not None:
                if not isinstance(tags_obj, Mapping):
                    raise TraceConfigError(f"rules[{i}].probes.tags must be an object")
                for k in sorted(tags_obj.keys(), key=lambda x: str(x)):
                    kk = str(k).strip()
                    if not kk:
                        raise TraceConfigError(
                            f"rules[{i}].probes.tags keys must be non-empty"
                        )
                    tags.append((kk, tags_obj[k]))
            probes = ProbeSelector(
                families=families, stages=stages, lanes=lanes, ats=ats, tags=tuple(tags)
            )

        if not port_globs and probes is None:
            raise TraceConfigError(
                f"rules[{i}] must include `ports` and/or `probes` selectors"
            )

        rules.append(
            TraceRule(
                instance_globs=tuple(inst_globs),
                port_globs=tuple(port_globs),
                probes=probes,
            )
        )

    # Optional window config.
    window: TraceWindow | None = None
    w = obj.get("window", None)
    if w is not None:
        if not isinstance(w, Mapping):
            raise TraceConfigError("window must be an object")
        if "begin_cycle" in w or "end_cycle" in w:
            b = (
                None
                if "begin_cycle" not in w
                else _as_int(w.get("begin_cycle"), field="window.begin_cycle")
            )
            e = (
                None
                if "end_cycle" not in w
                else _as_int(w.get("end_cycle"), field="window.end_cycle")
            )
            if b is not None and b < 0:
                raise TraceConfigError("window.begin_cycle must be >= 0")
            if e is not None and e < 0:
                raise TraceConfigError("window.end_cycle must be >= 0")
            if b is not None and e is not None and b > e:
                raise TraceConfigError("window.begin_cycle must be <= window.end_cycle")
            window = TraceWindow(begin_cycle=b, end_cycle=e)
        else:
            trig = w.get("trigger", None)
            if not isinstance(trig, Mapping) or "cycle" not in trig:
                raise TraceConfigError("window.trigger must be an object with `cycle`")
            trig_cycle = _as_int(trig.get("cycle"), field="window.trigger.cycle")
            pre = _as_int(w.get("pre", 0), field="window.pre")
            post = _as_int(w.get("post", 0), field="window.post")
            if trig_cycle < 0 or pre < 0 or post < 0:
                raise TraceConfigError("window cycle/pre/post must be >= 0")
            begin = max(0, int(trig_cycle) - int(pre))
            end = int(trig_cycle) + int(post)
            window = TraceWindow(begin_cycle=int(begin), end_cycle=int(end))

    return TraceConfig(
        version=version, rules=tuple(rules), window=window, source_json=source_json
    )


_INSTANCE_CALLEE_RE = re.compile(r"\bcallee\s*=\s*@([A-Za-z_][A-Za-z0-9_\$]*)\b")
_INSTANCE_NAME_RE = re.compile(r'\bname\s*=\s*"((?:\\.|[^"\\])*)"')
_INSTANCE_SHORT_NAME_RE = re.compile(r'\bshort_name\s*=\s*"((?:\\.|[^"\\])*)"')


def _instance_ops_in_func_mlir(func_mlir: str) -> list[tuple[str, str]]:
    out: list[tuple[str, str]] = []
    for line in func_mlir.splitlines():
        if "pyc.instance" not in line:
            continue
        m_callee = _INSTANCE_CALLEE_RE.search(line)
        if not m_callee:
            continue
        callee = str(m_callee.group(1))
        raw_name = callee
        m_name = _INSTANCE_NAME_RE.search(line)
        if m_name:
            raw_name = json.loads('"' + m_name.group(1) + '"')
        m_short = _INSTANCE_SHORT_NAME_RE.search(line)
        if m_short:
            raw_name = json.loads('"' + m_short.group(1) + '"')
        out.append((str(raw_name), callee))
    return out


def _hardened_payload_from_mod(mod: Any) -> dict[str, Any] | None:
    # `mod` is a `dsl.Module`/`hw.Circuit` instance with private `_func_attrs`
    # containing MLIR attribute literals.
    func_attrs = getattr(mod, "_func_attrs", None)  # noqa: SLF001
    if not isinstance(func_attrs, Mapping):
        return None
    lit = func_attrs.get("pyc.hardened")
    if not isinstance(lit, str) or not lit:
        return None
    try:
        hardened_json = json.loads(lit)
        payload = json.loads(hardened_json)
    except Exception:  # noqa: BLE001
        return None
    if isinstance(payload, Mapping):
        return dict(payload)
    return None


_HARDENED_ATTR_RE = re.compile(r'\bpyc\.hardened\s*=\s*"((?:\\.|[^"\\])*)"')


def _probe_table_from_pyc_text(sym: str, pyc_text: str) -> dict[str, dict[str, Any]]:
    # Extract the `pyc.hardened` JSON payload for a specific module symbol.
    #
    # The attribute lives on the `func.func @<sym>` header line and contains a
    # JSON string, which itself encodes the hardened payload object.
    sym = str(sym)
    for line in pyc_text.splitlines():
        if f"func.func @{sym}" not in line:
            continue
        m = _HARDENED_ATTR_RE.search(line)
        if not m:
            return {}
        try:
            lit = json.loads('"' + m.group(1) + '"')
            payload = json.loads(str(lit))
        except Exception:  # noqa: BLE001
            return {}
        if not isinstance(payload, Mapping):
            return {}
        pt = payload.get("probe_table", {})
        if not isinstance(pt, Mapping):
            return {}
        out: dict[str, dict[str, Any]] = {}
        for k, v in pt.items():
            if isinstance(k, str) and isinstance(v, Mapping):
                out[str(k)] = dict(v)
        return out
    return {}


def compute_trace_plan_from_artifacts(
    *,
    manifest: Mapping[str, Any],
    module_paths: Mapping[str, Path],
    config: TraceConfig,
    probe_manifest: Mapping[str, Any] | None = None,
) -> TracePlan:
    """Compute a TracePlan using already-emitted `.pyc` artifacts + project manifest.

    This is the cache-hit path for incremental builds: avoid re-running JIT
    compile when the frontend artifacts are unchanged.
    """

    top = str(manifest.get("top", "")).strip()
    modules = manifest.get("modules", None)
    if not top or not isinstance(modules, list):
        raise TraceConfigError(
            "invalid project manifest: missing `top` or `modules` list"
        )

    module_ports: dict[str, list[str]] = {}
    module_out_port_name: dict[str, dict[str, str]] = {}
    module_probes: dict[str, dict[str, dict[str, Any]]] = {}

    for m in modules:
        if not isinstance(m, Mapping):
            continue
        sym = str(m.get("name", "")).strip()
        if not sym:
            continue
        in_raw = [str(x).strip() for x in (m.get("arg_names") or [])]
        out_raw = [str(x).strip() for x in (m.get("result_names") or [])]
        all_names = _unique_canonical_names(
            [*in_raw, *out_raw], ctx=f"module {sym} port"
        )
        module_ports[sym] = list(all_names)
        module_out_port_name[sym] = {r: r for r in out_raw}

        pyc_path = module_paths.get(sym)
        probe_table: dict[str, dict[str, Any]] = {}
        if isinstance(pyc_path, Path) and pyc_path.is_file():
            try:
                text = pyc_path.read_text(encoding="utf-8")
            except OSError:
                text = ""
            probe_table = _probe_table_from_pyc_text(sym, text)
        module_probes[sym] = probe_table

    instances: list[tuple[str, str, str]] = []

    def visit(sym: str, full_path: str, stack: tuple[str, ...]) -> None:
        ipath = shorten_instance_path(full_path)
        instances.append((ipath, sym, full_path))
        if sym in stack:
            return
        pyc_path = module_paths.get(sym)
        if not isinstance(pyc_path, Path) or not pyc_path.is_file():
            return
        try:
            func_mlir = pyc_path.read_text(encoding="utf-8")
        except OSError:
            return
        children = _instance_ops_in_func_mlir(func_mlir)
        for raw_name, callee in sorted(
            children, key=lambda x: (_sanitize_id(x[0]), x[1])
        ):
            seg = _sanitize_id(raw_name)
            child_full = f"{full_path}.{seg}"
            visit(str(callee), child_full, stack=(*stack, sym))

    visit(str(top), "dut", stack=())

    enabled: set[str] = set()
    enabled_full_paths: set[str] = set()
    enabled_probe_instances: set[str] = set()
    signal_obs: dict[str, str] = {}
    for ipath, sym, full_path in instances:
        ports = module_ports.get(sym, [])
        probes = module_probes.get(sym, {})

        for rule in config.rules:
            if not any(_match_hier_glob(p, ipath) for p in rule.instance_globs):
                continue
            for port in ports:
                if rule.port_globs and not any(
                    fnmatch.fnmatchcase(port, pg) for pg in rule.port_globs
                ):
                    continue
                if rule.port_globs:
                    enabled.add(f"{ipath}:{port}")
                    enabled_full_paths.add(full_path)
            if rule.probes is not None:
                for port, meta in probes.items():
                    if rule.probes.matches(meta):
                        unique_out = module_out_port_name.get(sym, {}).get(
                            port, str(port)
                        )
                        sig = f"{ipath}:{unique_out}"
                        enabled.add(sig)
                        enabled_full_paths.add(full_path)
                        at = meta.get("at", None)
                        if isinstance(at, str) and at.strip():
                            signal_obs[sig] = at.strip().lower()

    if probe_manifest is not None:
        raw_probes = probe_manifest.get("probes", [])
        if isinstance(raw_probes, list):
            for raw in raw_probes:
                if not isinstance(raw, Mapping):
                    continue
                canonical_path = str(raw.get("canonical_path", "")).strip()
                instance_path = str(raw.get("instance_path", "")).strip()
                if not canonical_path or not instance_path:
                    continue
                meta: dict[str, Any] = {
                    "at": raw.get("obs", None),
                    "tags": raw.get("tags", {}),
                }
                for rule in config.rules:
                    if rule.probes is None:
                        continue
                    if not any(
                        _match_hier_glob(p, instance_path) for p in rule.instance_globs
                    ):
                        continue
                    if rule.probes.matches(meta):
                        enabled.add(canonical_path)
                        enabled_probe_instances.add(instance_path)
                        at = raw.get("obs", None)
                        if isinstance(at, str) and at.strip():
                            signal_obs[canonical_path] = at.strip().lower()

    enabled_signals = tuple(sorted(enabled))
    signal_obs = {s: v for s, v in signal_obs.items() if s in enabled_signals}

    enabled_instances_set: set[str] = set()
    for full_path in enabled_full_paths:
        parts = [p for p in str(full_path).split(".") if p]
        for i in range(1, len(parts) + 1):
            enabled_instances_set.add(shorten_instance_path(".".join(parts[:i])))
    for instance_path in enabled_probe_instances:
        parts = [p for p in str(instance_path).split(".") if p]
        for i in range(1, len(parts) + 1):
            enabled_instances_set.add(shorten_instance_path(".".join(parts[:i])))
    enabled_instances = tuple(sorted(enabled_instances_set))

    return TracePlan(
        version=1,
        enabled_signals=enabled_signals,
        enabled_instances=enabled_instances,
        signal_obs=signal_obs,
        window=config.window,
        config=config,
    )


def compute_trace_plan(*, design: Design, config: TraceConfig) -> TracePlan:
    module_ports: dict[str, list[str]] = {}
    module_out_port_name: dict[str, dict[str, str]] = {}
    module_probes: dict[str, dict[str, dict[str, Any]]] = {}

    for cm in design.modules():
        in_raw = [str(x).strip() for x in cm.arg_names]
        out_raw = [str(x).strip() for x in cm.result_names]
        all_names = _unique_canonical_names(
            [*in_raw, *out_raw], ctx=f"module {cm.sym_name} port"
        )
        module_ports[cm.sym_name] = list(all_names)
        module_out_port_name[cm.sym_name] = {r: r for r in out_raw}

        hardened = _hardened_payload_from_mod(cm.mod)
        probe_table: dict[str, dict[str, Any]] = {}
        if hardened is not None:
            pt = hardened.get("probe_table", {})
            if isinstance(pt, Mapping):
                for k, v in pt.items():
                    if isinstance(k, str) and isinstance(v, Mapping):
                        probe_table[str(k)] = dict(v)
        module_probes[cm.sym_name] = probe_table

    # Build a flat instance list from the top module by parsing `pyc.instance`.
    instances: list[tuple[str, str, str]] = []

    def visit(sym: str, full_path: str, stack: tuple[str, ...]) -> None:
        ipath = shorten_instance_path(full_path)
        instances.append((ipath, sym, full_path))
        cm = design.lookup(sym)
        if cm is None:
            return
        if sym in stack:
            return
        children = _instance_ops_in_func_mlir(cm.mod.emit_func_mlir())
        # Deterministic order independent of frontend call order.
        for raw_name, callee in sorted(
            children, key=lambda x: (_sanitize_id(x[0]), x[1])
        ):
            seg = _sanitize_id(raw_name)
            child_full = f"{full_path}.{seg}"
            visit(str(callee), child_full, stack=(*stack, sym))

    visit(str(design.top), "dut", stack=())

    enabled: set[str] = set()
    enabled_full_paths: set[str] = set()
    signal_obs: dict[str, str] = {}
    for ipath, sym, full_path in instances:
        ports = module_ports.get(sym, [])
        probes = module_probes.get(sym, {})

        for rule in config.rules:
            if not any(_match_hier_glob(p, ipath) for p in rule.instance_globs):
                continue
            # Port globs.
            for port in ports:
                if rule.port_globs and not any(
                    fnmatch.fnmatchcase(port, pg) for pg in rule.port_globs
                ):
                    continue
                if rule.port_globs:
                    enabled.add(f"{ipath}:{port}")
                    enabled_full_paths.add(full_path)
            # Probe tag selectors.
            if rule.probes is not None:
                for port, meta in probes.items():
                    if rule.probes.matches(meta):
                        unique_out = module_out_port_name.get(sym, {}).get(
                            port, str(port)
                        )
                        sig = f"{ipath}:{unique_out}"
                        enabled.add(sig)
                        enabled_full_paths.add(full_path)
                        at = meta.get("at", None)
                        if isinstance(at, str) and at.strip():
                            signal_obs[sig] = at.strip().lower()

    enabled_signals = tuple(sorted(enabled))
    signal_obs = {s: v for s, v in signal_obs.items() if s in enabled_signals}

    enabled_instances_set: set[str] = set()
    for full_path in enabled_full_paths:
        parts = [p for p in str(full_path).split(".") if p]
        for i in range(1, len(parts) + 1):
            enabled_instances_set.add(shorten_instance_path(".".join(parts[:i])))
    enabled_instances = tuple(sorted(enabled_instances_set))

    return TracePlan(
        version=1,
        enabled_signals=enabled_signals,
        enabled_instances=enabled_instances,
        signal_obs=signal_obs,
        window=config.window,
        config=config,
    )
