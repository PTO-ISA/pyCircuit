from __future__ import annotations

from collections.abc import Iterable, Iterator, Mapping, MutableMapping
from dataclasses import dataclass
from typing import Any

from .spec.types import BundleSpec, StagePipeSpec, StructSpec


class ConnectorError(TypeError):
    pass


def _merge_owner(owner: Any | None, candidate: Any | None, *, ctx: str) -> Any | None:
    if candidate is None:
        return owner
    if owner is None:
        return candidate
    if owner is not candidate:
        raise ConnectorError(f"{ctx}: fields must belong to the same Circuit")
    return owner


def _owner_from_leaf(v: Any) -> Any | None:
    if isinstance(v, Connector):
        return v.owner
    q = getattr(v, "q", None)
    q_owner = getattr(q, "m", None)
    if q_owner is not None:
        return q_owner
    m_owner = getattr(v, "m", None)
    if m_owner is not None:
        return m_owner
    return None


def _scan_owner_tree(v: Any, *, ctx: str) -> Any | None:
    owner: Any | None = None
    if isinstance(v, ConnectorBundle):
        for c in v.values():
            owner = _merge_owner(owner, c.owner, ctx=ctx)
        return owner
    if isinstance(v, ConnectorStruct):
        for c in v.values():
            owner = _merge_owner(owner, c.owner, ctx=ctx)
        return owner
    if isinstance(v, Mapping):
        for vv in v.values():
            owner = _merge_owner(owner, _scan_owner_tree(vv, ctx=ctx), ctx=ctx)
        return owner
    return _owner_from_leaf(v)


class Connector:
    """Base class for inter-module connection objects."""

    owner: Any
    name: str

    @property
    def ty(self) -> str:
        raise NotImplementedError

    def read(self) -> Any:
        raise NotImplementedError

    def out(self) -> Any:
        """Read connector payload as a value suitable for expressions."""
        value = self.read()
        if hasattr(value, "out"):
            return value.out()
        return value

    def __bool__(self) -> bool:
        raise TypeError(
            "Connector cannot be used as a Python boolean. "
            "Use hardware comparisons/selects and keep conditions as i1 values."
        )

    @property
    def width(self) -> int:
        return int(self.out().width)

    def __getitem__(self, idx: Any) -> Any:
        return self.out()[idx]

    def __getattr__(self, name: str) -> Any:
        return getattr(self.out(), name)

    def __add__(self, other: Any) -> Any:
        return self.out() + other

    def __radd__(self, other: Any) -> Any:
        return other + self.out()

    def __sub__(self, other: Any) -> Any:
        return self.out() - other

    def __rsub__(self, other: Any) -> Any:
        return other - self.out()

    def __mul__(self, other: Any) -> Any:
        return self.out() * other

    def __rmul__(self, other: Any) -> Any:
        return other * self.out()

    def __floordiv__(self, other: Any) -> Any:
        return self.out() // other

    def __rfloordiv__(self, other: Any) -> Any:
        return other // self.out()

    def __truediv__(self, other: Any) -> Any:
        return self.out() / other

    def __rtruediv__(self, other: Any) -> Any:
        return other / self.out()

    def __mod__(self, other: Any) -> Any:
        return self.out() % other

    def __rmod__(self, other: Any) -> Any:
        return other % self.out()

    def __and__(self, other: Any) -> Any:
        return self.out() & other

    def __rand__(self, other: Any) -> Any:
        return other & self.out()

    def __or__(self, other: Any) -> Any:
        return self.out() | other

    def __ror__(self, other: Any) -> Any:
        return other | self.out()

    def __xor__(self, other: Any) -> Any:
        return self.out() ^ other

    def __rxor__(self, other: Any) -> Any:
        return other ^ self.out()

    def __invert__(self) -> Any:
        return ~self.out()

    def __lshift__(self, other: Any) -> Any:
        return self.out() << other

    def __rshift__(self, other: Any) -> Any:
        return self.out() >> other

    def __eq__(self, other: object) -> Any:  # type: ignore[override]
        return self.out() == other

    def __ne__(self, other: object) -> Any:  # type: ignore[override]
        return self.out() != other

    def __lt__(self, other: Any) -> Any:
        return self.out() < other

    def __gt__(self, other: Any) -> Any:
        return self.out() > other

    def __le__(self, other: Any) -> Any:
        return self.out() <= other

    def __ge__(self, other: Any) -> Any:
        return self.out() >= other


@dataclass(frozen=True, eq=False)
class WireConnector(Connector):
    owner: Any
    name: str
    wire: Any

    def __post_init__(self) -> None:
        maybe_owner = getattr(self.wire, "m", None)
        if maybe_owner is not None and maybe_owner is not self.owner:
            raise ConnectorError("wire connector must belong to the declaring Circuit")
        if not hasattr(self.wire, "ty"):
            raise ConnectorError("wire connector payload must expose a `ty` attribute")

    @property
    def ty(self) -> str:
        return str(self.wire.ty)

    @property
    def signed(self) -> bool:
        return bool(getattr(self.wire, "signed", False))

    def read(self) -> Any:
        return self.wire


@dataclass(frozen=True, eq=False)
class RegConnector(Connector):
    owner: Any
    name: str
    reg: Any

    def __post_init__(self) -> None:
        q = getattr(self.reg, "q", None)
        if q is None or getattr(q, "m", None) is not self.owner:
            raise ConnectorError("reg connector must belong to the declaring Circuit")

    @property
    def ty(self) -> str:
        return str(self.reg.ty)

    def read(self) -> Any:
        return self.reg.q

    def set(self, value: Any, *, when: Any = 1) -> None:
        self.reg.set(value, when=when)


class ConnectorBundle:
    def __init__(
        self,
        fields: Mapping[str, Any],
        *,
        spec: BundleSpec | StagePipeSpec | None = None,
    ) -> None:
        self.spec = spec
        out: MutableMapping[str, Connector] = {}
        owner_hint = _scan_owner_tree(fields, ctx="ConnectorBundle")
        for k, v in fields.items():
            key = str(k)
            if not key:
                raise ConnectorError("bundle field name must be non-empty")
            if isinstance(v, Connector):
                out[key] = v
                owner_hint = _merge_owner(
                    owner_hint, v.owner, ctx=f"ConnectorBundle[{key!r}]"
                )
                continue
            if owner_hint is None:
                raise ConnectorError(
                    f"bundle field {key!r}: cannot infer owning Circuit for implicit coercion"
                )
            c = owner_hint.as_connector(v, name=key)
            if not isinstance(c, Connector):
                raise ConnectorError(
                    f"bundle field {key!r}: implicit coercion did not produce a Connector"
                )
            out[key] = c
            owner_hint = _merge_owner(
                owner_hint, c.owner, ctx=f"ConnectorBundle[{key!r}]"
            )
        self.fields: dict[str, Connector] = dict(out)

        if spec is not None:
            exp: set[str]
            if isinstance(spec, BundleSpec):
                exp = {f.name for f in spec.fields}
            elif isinstance(spec, StagePipeSpec):
                exp = {f.name for f in spec.payload.fields}
                if spec.has_valid:
                    exp.add(str(spec.valid_name))
                if spec.has_ready:
                    exp.add(str(spec.ready_name))
            else:
                raise ConnectorError(
                    f"ConnectorBundle spec must be BundleSpec or StagePipeSpec, got {type(spec).__name__}"
                )

            got = set(self.fields.keys())
            missing = sorted(exp - got)
            extra = sorted(got - exp)
            if missing or extra:
                parts: list[str] = []
                if missing:
                    parts.append("missing: " + ", ".join(missing))
                if extra:
                    parts.append("extra: " + ", ".join(extra))
                raise ConnectorError(
                    f"ConnectorBundle spec mismatch ({'; '.join(parts)})"
                )

    def __getitem__(self, key: str) -> Connector:
        return self.fields[str(key)]

    def __iter__(self) -> Iterator[str]:
        return iter(self.fields)

    def __len__(self) -> int:
        return len(self.fields)

    def items(self) -> Iterable[tuple[str, Connector]]:
        return self.fields.items()

    def values(self) -> Iterable[Connector]:
        return self.fields.values()

    def keys(self) -> Iterable[str]:
        return self.fields.keys()


class ConnectorStruct:
    """Nested/flattened connector group for structured inter-module wiring."""

    def __init__(
        self, fields: Mapping[str, Any], *, spec: StructSpec | None = None
    ) -> None:
        self.spec = spec
        flat: dict[str, Connector] = {}
        owner_hint = _scan_owner_tree(fields, ctx="ConnectorStruct")
        self._flatten_input(fields, out=flat, owner_hint=owner_hint)
        if not flat:
            raise ConnectorError("ConnectorStruct requires at least one field")

        owners = {id(v.owner): v.owner for v in flat.values()}
        if len(owners) != 1:
            raise ConnectorError(
                "ConnectorStruct fields must belong to the same Circuit"
            )

        if spec is not None:
            exp = set(spec.leaf_paths())
            got = set(flat.keys())
            missing = sorted(exp - got)
            extra = sorted(got - exp)
            if missing or extra:
                parts: list[str] = []
                if missing:
                    parts.append("missing: " + ", ".join(missing))
                if extra:
                    parts.append("extra: " + ", ".join(extra))
                raise ConnectorError(
                    f"ConnectorStruct spec mismatch ({'; '.join(parts)})"
                )

        self.fields: dict[str, Connector] = dict(
            sorted(flat.items(), key=lambda kv: kv[0])
        )

    @staticmethod
    def _flatten_input(
        v: Mapping[str, Any],
        *,
        out: dict[str, Connector],
        owner_hint: Any | None,
        prefix: str = "",
    ) -> None:
        for raw_k, raw_v in v.items():
            k = str(raw_k)
            if not k:
                raise ConnectorError("ConnectorStruct field name must be non-empty")
            path = f"{prefix}.{k}" if prefix else k

            if isinstance(raw_v, Connector):
                out[path] = raw_v
                continue
            if isinstance(raw_v, ConnectorBundle):
                for kk, vv in raw_v.items():
                    out[f"{path}.{kk}"] = vv
                continue
            if isinstance(raw_v, ConnectorStruct):
                for kk, vv in raw_v.items():
                    out[f"{path}.{kk}"] = vv
                continue
            if isinstance(raw_v, Mapping):
                ConnectorStruct._flatten_input(
                    raw_v, out=out, owner_hint=owner_hint, prefix=path
                )
                continue
            if owner_hint is None:
                raise ConnectorError(
                    f"ConnectorStruct field {path!r}: cannot infer owning Circuit for implicit coercion"
                )
            c = owner_hint.as_connector(raw_v, name=path)
            if not isinstance(c, Connector):
                raise ConnectorError(
                    f"ConnectorStruct field {path!r}: implicit coercion did not produce a Connector"
                )
            out[path] = c

    @classmethod
    def from_flat(
        cls,
        fields: Mapping[str, Connector],
        *,
        spec: StructSpec | None = None,
    ) -> ConnectorStruct:
        return cls(dict(fields), spec=spec)

    def flatten(self) -> dict[str, Connector]:
        return dict(self.fields)

    def __getitem__(self, key: str) -> Connector | ConnectorStruct:
        k = str(key)
        if k in self.fields:
            return self.fields[k]
        prefix = f"{k}."
        sub = {
            kk[len(prefix) :]: vv
            for kk, vv in self.fields.items()
            if kk.startswith(prefix)
        }
        if not sub:
            raise KeyError(k)
        return ConnectorStruct.from_flat(sub)

    def __iter__(self) -> Iterator[str]:
        return iter(self.fields)

    def __len__(self) -> int:
        return len(self.fields)

    def items(self) -> Iterable[tuple[str, Connector]]:
        return self.fields.items()

    def values(self) -> Iterable[Connector]:
        return self.fields.values()

    def keys(self) -> Iterable[str]:
        return self.fields.keys()


@dataclass(frozen=True)
class ModuleInstanceHandle:
    name: str
    symbol: str
    inputs: Mapping[str, Connector]
    outputs: Connector | ConnectorBundle


@dataclass(frozen=True)
class ModuleCollectionHandle:
    name: str
    instances: Mapping[str, ModuleInstanceHandle]
    outputs: Mapping[str, Connector | ConnectorBundle | ConnectorStruct]

    def __getitem__(self, key: str) -> ModuleInstanceHandle:
        return self.instances[str(key)]

    def keys(self) -> Iterable[str]:
        return self.instances.keys()

    def items(self) -> Iterable[tuple[str, ModuleInstanceHandle]]:
        return self.instances.items()

    def output(self, key: str) -> Connector | ConnectorBundle | ConnectorStruct:
        return self.outputs[str(key)]


ConnectorLike = Connector | ConnectorBundle | ConnectorStruct


def is_connector(v: Any) -> bool:
    return isinstance(v, Connector)


def is_connector_bundle(v: Any) -> bool:
    return isinstance(v, ConnectorBundle)


def is_connector_struct(v: Any) -> bool:
    return isinstance(v, ConnectorStruct)


def connector_owner(v: ConnectorLike) -> Any | None:
    if isinstance(v, Connector):
        return v.owner
    if isinstance(v, ConnectorBundle):
        owner: Any | None = None
        for c in v.values():
            if owner is None:
                owner = c.owner
                continue
            if c.owner is not owner:
                raise ConnectorError(
                    "connector bundle fields must belong to the same Circuit"
                )
        return owner
    if isinstance(v, ConnectorStruct):
        owner: Any | None = None
        for c in v.values():
            if owner is None:
                owner = c.owner
                continue
            if c.owner is not owner:
                raise ConnectorError(
                    "connector struct fields must belong to the same Circuit"
                )
        return owner
    return None


def connector_to_wire(v: Connector, *, ctx: str) -> Any:
    if not isinstance(v, Connector):
        raise ConnectorError(f"{ctx}: expected Connector, got {type(v).__name__}")
    return v.read()
