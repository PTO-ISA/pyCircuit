from __future__ import annotations

from collections.abc import Mapping
from dataclasses import dataclass
from typing import Any

from ..connectors import Connector, ConnectorBundle, ConnectorError, ConnectorStruct
from ..dsl import Signal
from ..hw import Circuit
from ..literals import LiteralValue
from ..spec.types import BundleSpec, StagePipeSpec, StructSpec


@dataclass(frozen=True)
class _I1Field:
    width: int = 1
    signed: bool = False


@dataclass(frozen=True)
class SpecBinding:
    spec: BundleSpec | StagePipeSpec | StructSpec
    value: ConnectorBundle | ConnectorStruct | Mapping[str, Any]


def bind(
    spec: BundleSpec | StagePipeSpec | StructSpec,
    value: ConnectorBundle | ConnectorStruct | Mapping[str, Any],
) -> SpecBinding:
    return SpecBinding(spec=spec, value=value)


def _normalize_prefix(prefix: str | None) -> str:
    p = "" if prefix is None else str(prefix)
    return p


def _path_port_name(path: str) -> str:
    return str(path).replace(".", "_")


def _iter_fields(
    spec: BundleSpec | StagePipeSpec | StructSpec,
) -> list[tuple[str, Any, str]]:
    out: list[tuple[str, Any, str]] = []
    if isinstance(spec, BundleSpec):
        for f in spec.fields:
            out.append((f.name, f, f.name))
        return out

    if isinstance(spec, StagePipeSpec):
        for f in spec.payload.fields:
            out.append((f.name, f, f.name))
        if spec.has_valid:
            out.append((spec.valid_name, _I1Field(), spec.valid_name))
        if spec.has_ready:
            out.append((spec.ready_name, _I1Field(), spec.ready_name))
        return out

    if isinstance(spec, StructSpec):
        seen_ports: set[str] = set()
        for path, fld in spec.flatten_fields():
            pname = _path_port_name(path)
            if pname in seen_ports:
                raise ConnectorError(
                    f"struct {spec.name!r} has colliding port name {pname!r}; use unique field paths"
                )
            seen_ports.add(pname)
            out.append((path, fld, pname))
        return out

    raise TypeError(
        f"expected BundleSpec/StagePipeSpec/StructSpec, got {type(spec).__name__}"
    )


def _port_name(prefix: str | None, local_name: str) -> str:
    p = _normalize_prefix(prefix)
    return f"{p}{local_name}"


def _spec_kind(spec: BundleSpec | StagePipeSpec | StructSpec) -> str:
    if isinstance(spec, BundleSpec):
        return "bundle"
    if isinstance(spec, StagePipeSpec):
        return "stage_pipe"
    if isinstance(spec, StructSpec):
        return "struct"
    raise TypeError(f"unsupported spec type: {type(spec).__name__}")


def _spec_layout_meta(spec: BundleSpec | StagePipeSpec | StructSpec) -> dict[str, Any]:
    fields = list(spec.layout_fields())
    slices = dict(spec.field_slices())
    fields_out: list[dict[str, Any]] = []
    field_map: dict[str, list[int]] = {}
    for path, width, signed, logic_kind in fields:
        lsb, w2 = slices[str(path)]
        if int(w2) != int(width):
            raise ConnectorError(
                f"internal: field slice width mismatch for {path!r}: slice={w2} field={width}"
            )
        field_map[str(path)] = [int(lsb), int(width)]
        fields_out.append(
            {
                "path": str(path),
                "lsb": int(lsb),
                "width": int(width),
                "signed": bool(signed),
                "logic_kind": str(logic_kind),
            }
        )
    return {
        "kind": _spec_kind(spec),
        "name": str(getattr(spec, "name", "")),
        "layout_id": str(spec.layout_id()),
        "total_width": int(spec.total_width()),
        "field_map": field_map,
        "fields": fields_out,
    }


def _maybe_record_layout_group(
    m: Circuit,
    *,
    usage: str,
    spec: BundleSpec | StagePipeSpec | StructSpec,
    prefix: str | None,
    ports: Mapping[str, str],
) -> None:
    hook = getattr(m, "_record_hardened_layout_group", None)
    if not callable(hook):
        return
    group = {
        "usage": str(usage),
        "prefix": _normalize_prefix(prefix),
        "spec": _spec_layout_meta(spec),
        "ports": {str(k): str(v) for k, v in dict(ports).items()},
    }
    hook(group)


def _connector_width(c: Connector) -> int:
    ty = str(c.ty)
    if not ty.startswith("i"):
        raise ConnectorError(f"expected integer connector type, got {ty!r}")
    try:
        w = int(ty[1:])
    except ValueError as e:
        raise ConnectorError(f"invalid integer connector type: {ty!r}") from e
    if w <= 0:
        raise ConnectorError(f"invalid integer connector width: {ty!r}")
    return w


def _connector_signed(c: Connector) -> bool:
    if hasattr(c, "signed"):
        return bool(c.signed)
    rv = c.read()
    return bool(getattr(rv, "signed", False))


def _values_mapping(
    values: ConnectorBundle | ConnectorStruct | Mapping[str, Any]
) -> Mapping[str, Any]:
    if isinstance(values, ConnectorBundle):
        return {k: v for k, v in values.items()}
    if isinstance(values, ConnectorStruct):
        return values.flatten()
    return dict(values)


def _inputs_bundle(
    m: Circuit,
    spec: BundleSpec | StagePipeSpec | StructSpec,
    *,
    prefix: str | None = None,
) -> ConnectorBundle:
    out: dict[str, Connector] = {}
    ports: dict[str, str] = {}
    for key, f, pname in _iter_fields(spec):
        port = _port_name(prefix, pname)
        ports[str(key)] = port
        out[key] = m.input_connector(
            port, width=int(f.width), signed=bool(getattr(f, "signed", False))
        )
    _maybe_record_layout_group(m, usage="inputs", spec=spec, prefix=prefix, ports=ports)
    bundle_spec = spec if isinstance(spec, (BundleSpec, StagePipeSpec)) else None
    return ConnectorBundle(out, spec=bundle_spec)


def _outputs_bundle(
    m: Circuit,
    spec: BundleSpec | StagePipeSpec | StructSpec,
    values: ConnectorBundle | ConnectorStruct | Mapping[str, Any],
    *,
    prefix: str | None = None,
) -> ConnectorBundle:
    vals = dict(_values_mapping(values))

    exp = [k for k, _, _ in _iter_fields(spec)]
    got = sorted(str(k) for k in vals.keys())
    missing = sorted(set(exp) - set(got))
    extra = sorted(set(got) - set(exp))
    if missing or extra:
        parts: list[str] = []
        if missing:
            parts.append("missing: " + ", ".join(missing))
        if extra:
            parts.append("extra: " + ", ".join(extra))
        raise ConnectorError(f"outputs key mismatch ({'; '.join(parts)})")

    out: dict[str, Connector] = {}
    ports: dict[str, str] = {}
    for key, f, pname in _iter_fields(spec):
        c = m.as_connector(vals[key], name=key)
        got_w = _connector_width(c)
        exp_w = int(f.width)
        if got_w != exp_w:
            raise ConnectorError(
                f"outputs[{key!r}] width mismatch: expected i{exp_w}, got i{got_w}"
            )
        exp_signed = bool(getattr(f, "signed", False))
        got_signed = _connector_signed(c)
        if got_signed != exp_signed:
            raise ConnectorError(
                f"outputs[{key!r}] signedness mismatch: expected signed={exp_signed}, got signed={got_signed}"
            )
        port = _port_name(prefix, pname)
        ports[str(key)] = port
        out[key] = m.output_connector(port, c)
    _maybe_record_layout_group(
        m, usage="outputs", spec=spec, prefix=prefix, ports=ports
    )
    bundle_spec = spec if isinstance(spec, (BundleSpec, StagePipeSpec)) else None
    return ConnectorBundle(out, spec=bundle_spec)


def _as_signal(v: Connector | Signal, *, ctx: str) -> Signal:
    if isinstance(v, Connector):
        rv = v.read()
        if not isinstance(rv, Signal):
            raise ConnectorError(
                f"{ctx}: expected clock/reset connector payload Signal, got {type(rv).__name__}"
            )
        return rv
    if isinstance(v, Signal):
        return v
    raise ConnectorError(f"{ctx}: expected Connector or Signal, got {type(v).__name__}")


def _resolve_init(init: Mapping[str, Any] | Any, key: str) -> Any:
    if isinstance(init, Mapping):
        if key in init:
            return init[key]
        if "*" in init:
            return init["*"]
        return 0
    return init


def _state_bundle(
    m: Circuit,
    spec: BundleSpec | StagePipeSpec | StructSpec,
    *,
    clk: Connector | Signal,
    rst: Connector | Signal,
    prefix: str | None = None,
    init: Mapping[str, Any] | Any = 0,
    en: Connector | Signal | int | LiteralValue = 1,
) -> ConnectorBundle:
    clk_sig = _as_signal(clk, ctx="_state_bundle(clk)")
    rst_sig = _as_signal(rst, ctx="_state_bundle(rst)")
    out: dict[str, Connector] = {}
    for key, f, pname in _iter_fields(spec):
        name = _port_name(prefix, pname)
        out[key] = m.reg_connector(
            name,
            clk=clk_sig,
            rst=rst_sig,
            width=int(f.width),
            init=_resolve_init(init, key),
            en=en,
        )
    bundle_spec = spec if isinstance(spec, (BundleSpec, StagePipeSpec)) else None
    return ConnectorBundle(out, spec=bundle_spec)


def _inputs_struct(
    m: Circuit, spec: StructSpec, *, prefix: str | None = None
) -> ConnectorStruct:
    if not isinstance(spec, StructSpec):
        raise TypeError(f"inputs expects StructSpec, got {type(spec).__name__}")
    bundle = _inputs_bundle(m, spec, prefix=prefix)
    return ConnectorStruct.from_flat({k: v for k, v in bundle.items()}, spec=spec)


def _outputs_struct(
    m: Circuit,
    spec: StructSpec,
    values: ConnectorStruct | Mapping[str, Any],
    *,
    prefix: str | None = None,
) -> ConnectorStruct:
    if not isinstance(spec, StructSpec):
        raise TypeError(f"outputs expects StructSpec, got {type(spec).__name__}")
    bundle = _outputs_bundle(m, spec, values, prefix=prefix)
    return ConnectorStruct.from_flat({k: v for k, v in bundle.items()}, spec=spec)


def _state_struct(
    m: Circuit,
    spec: StructSpec,
    *,
    clk: Connector | Signal,
    rst: Connector | Signal,
    prefix: str | None = None,
    init: Mapping[str, Any] | Any = 0,
    en: Connector | Signal | int | LiteralValue = 1,
) -> ConnectorStruct:
    if not isinstance(spec, StructSpec):
        raise TypeError(f"state expects StructSpec, got {type(spec).__name__}")
    bundle = _state_bundle(m, spec, clk=clk, rst=rst, prefix=prefix, init=init, en=en)
    return ConnectorStruct.from_flat({k: v for k, v in bundle.items()}, spec=spec)


def inputs(
    m: Circuit,
    spec: BundleSpec | StagePipeSpec | StructSpec,
    *,
    prefix: str | None = None,
) -> ConnectorBundle | ConnectorStruct:
    if isinstance(spec, StructSpec):
        return _inputs_struct(m, spec, prefix=prefix)
    return _inputs_bundle(m, spec, prefix=prefix)


def outputs(
    m: Circuit,
    spec: BundleSpec | StagePipeSpec | StructSpec,
    values: ConnectorBundle | ConnectorStruct | Mapping[str, Any],
    *,
    prefix: str | None = None,
) -> ConnectorBundle | ConnectorStruct:
    if isinstance(spec, StructSpec):
        return _outputs_struct(m, spec, values, prefix=prefix)
    return _outputs_bundle(m, spec, values, prefix=prefix)


def state(
    m: Circuit,
    spec: BundleSpec | StagePipeSpec | StructSpec,
    *,
    clk: Connector | Signal,
    rst: Connector | Signal,
    prefix: str | None = None,
    init: Mapping[str, Any] | Any = 0,
    en: Connector | Signal | int | LiteralValue = 1,
) -> ConnectorBundle | ConnectorStruct:
    if isinstance(spec, StructSpec):
        return _state_struct(m, spec, clk=clk, rst=rst, prefix=prefix, init=init, en=en)
    return _state_bundle(m, spec, clk=clk, rst=rst, prefix=prefix, init=init, en=en)


def ports(
    m: Circuit,
    spec_bindings: Mapping[
        str,
        Connector
        | ConnectorBundle
        | ConnectorStruct
        | Mapping[str, Any]
        | SpecBinding
        | tuple[Any, Any]
        | Any,
    ],
) -> dict[str, Connector]:
    out: dict[str, Connector] = {}
    for pname, bound in spec_bindings.items():
        p = str(pname)
        if isinstance(bound, tuple) and len(bound) == 2:
            sp0, sp1 = bound
            if isinstance(sp0, (BundleSpec, StagePipeSpec, StructSpec)):
                bound = SpecBinding(spec=sp0, value=sp1)

        if isinstance(bound, SpecBinding):
            spec = bound.spec
            vals = dict(_values_mapping(bound.value))

            exp_keys = [k for k, _, _ in _iter_fields(spec)]
            got_keys = sorted(str(k) for k in vals.keys())
            missing = sorted(set(exp_keys) - set(got_keys))
            extra = sorted(set(got_keys) - set(exp_keys))
            if missing or extra:
                parts: list[str] = []
                if missing:
                    parts.append("missing: " + ", ".join(missing))
                if extra:
                    parts.append("extra: " + ", ".join(extra))
                raise ConnectorError(f"ports[{p!r}] key mismatch ({'; '.join(parts)})")

            for key, field, pname_local in _iter_fields(spec):
                port = f"{p}_{pname_local}"
                c = m.as_connector(vals[key], name=port)
                exp_w = int(getattr(field, "width", 0))
                got_w = _connector_width(c)
                if got_w != exp_w:
                    raise ConnectorError(
                        f"ports[{p!r}][{key!r}] width mismatch: expected i{exp_w}, got i{got_w}"
                    )
                exp_signed = bool(getattr(field, "signed", False))
                got_signed = _connector_signed(c)
                if exp_signed != got_signed:
                    raise ConnectorError(
                        f"ports[{p!r}][{key!r}] signedness mismatch: "
                        + f"expected signed={exp_signed}, got signed={got_signed}"
                    )
                out[port] = c
            continue

        if isinstance(bound, Connector):
            out[p] = m.as_connector(bound, name=p)
            continue
        if isinstance(bound, ConnectorBundle):
            for k, v in bound.items():
                port = f"{p}_{k}"
                out[port] = m.as_connector(v, name=port)
            continue
        if isinstance(bound, ConnectorStruct):
            for k, v in bound.items():
                port = f"{p}_{_path_port_name(k)}"
                out[port] = m.as_connector(v, name=port)
            continue
        if isinstance(bound, Mapping):
            for k, v in bound.items():
                port = f"{p}_{str(k)}"
                out[port] = m.as_connector(v, name=port)
            continue
        out[p] = m.as_connector(bound, name=p)
    return out


def unbind(
    spec: BundleSpec | StagePipeSpec | StructSpec,
    flat_values: Mapping[str, Any],
    *,
    prefix: str | None = None,
) -> dict[str, Any]:
    """Reverse helper for `bind(...)`: recover spec keys from flat/prefixed names."""

    vals = dict(flat_values)
    out: dict[str, Any] = {}
    for key, _f, pname in _iter_fields(spec):
        k0 = str(key)
        k1 = _port_name(prefix, pname)
        if k0 in vals:
            out[k0] = vals[k0]
            continue
        if k1 in vals:
            out[k0] = vals[k1]
            continue
        raise ConnectorError(
            f"unbind missing key for field {k0!r} (accepted names: {k0!r}, {k1!r})"
        )
    return out


def unflatten(
    spec: BundleSpec | StagePipeSpec | StructSpec,
    flat_values: Mapping[str, Any],
    *,
    prefix: str | None = None,
) -> Mapping[str, Any]:
    """Build a nested mapping from flattened spec fields.

    - For `StructSpec`, returns nested dicts by dotted leaf path.
    - For non-struct specs, returns flat dict keyed by spec field name.
    """

    flat = unbind(spec, flat_values, prefix=prefix)
    if not isinstance(spec, StructSpec):
        return flat

    nested: dict[str, Any] = {}
    for path, value in flat.items():
        parts = str(path).split(".")
        cur = nested
        for p in parts[:-1]:
            nxt = cur.get(p)
            if not isinstance(nxt, dict):
                nxt = {}
                cur[p] = nxt
            cur = nxt
        cur[parts[-1]] = value
    return nested
