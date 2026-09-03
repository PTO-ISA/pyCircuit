from __future__ import annotations

import hashlib
import json
from collections.abc import Callable, Iterable, Mapping, Sequence
from dataclasses import dataclass, field
from typing import Any

_ALLOWED_PARAM_TYPES = (bool, int, str)
_ALLOWED_PORT_DIRS = ("in", "out")
_DEFAULT_LOGIC_KIND = "logic"


def _check_name(name: str, *, ctx: str) -> str:
    out = str(name).strip()
    if not out:
        raise ValueError(f"{ctx} name must be non-empty")
    return out


def _check_dir(direction: str, *, ctx: str) -> str:
    d = str(direction).strip().lower()
    if d not in _ALLOWED_PORT_DIRS:
        raise ValueError(f"{ctx} direction must be 'in' or 'out', got {direction!r}")
    return d


def _check_logic_kind(kind: str, *, ctx: str) -> str:
    k = str(kind).strip().lower()
    if not k:
        raise ValueError(f"{ctx} logic_kind must be non-empty")
    return k


def _as_leaf_field(v: StructFieldSpec) -> tuple[int, bool]:
    if not isinstance(v, StructFieldSpec):
        raise TypeError(f"expected StructFieldSpec, got {type(v).__name__}")
    if not v.is_leaf:
        raise ValueError(f"field {v.name!r} is a nested struct, expected leaf")
    return int(v.width), bool(v.signed)


def _split_path(path: str | Sequence[str], *, ctx: str = "path") -> tuple[str, ...]:
    if isinstance(path, str):
        raw = [p for p in path.split(".") if p]
    else:
        raw = [str(p).strip() for p in path if str(p).strip()]
    if not raw:
        raise ValueError(f"{ctx} must be non-empty")
    return tuple(_check_name(p, ctx=ctx) for p in raw)


def _join_path(parts: Sequence[str]) -> str:
    return ".".join(str(p) for p in parts)


def _canonical_key(v: bool | int | str) -> str:
    if isinstance(v, bool):
        return "1" if v else "0"
    return str(v)


def _canonical_layout_payload(
    kind: str, fields: Sequence[tuple[str, int, bool, str]]
) -> str:
    payload = {
        "kind": str(kind),
        "fields": [
            {
                "path": str(path),
                "width": int(width),
                "signed": bool(signed),
                "logic_kind": str(logic_kind),
            }
            for path, width, signed, logic_kind in fields
        ],
    }
    return json.dumps(
        payload, sort_keys=True, separators=(",", ":"), ensure_ascii=False
    )


def _layout_id(kind: str, fields: Sequence[tuple[str, int, bool, str]]) -> str:
    text = _canonical_layout_payload(kind, fields)
    return hashlib.sha256(text.encode("utf-8")).hexdigest()[:16]


def _field_slices(
    fields: Sequence[tuple[str, int, bool, str]]
) -> dict[str, tuple[int, int]]:
    out: dict[str, tuple[int, int]] = {}
    lsb = 0
    for path, width, _signed, _logic_kind in reversed(fields):
        out[str(path)] = (int(lsb), int(width))
        lsb += int(width)
    return out


@dataclass(frozen=True)
class FieldSpec:
    name: str
    width: int
    signed: bool = False
    logic_kind: str = _DEFAULT_LOGIC_KIND

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _check_name(self.name, ctx="field"))
        w = int(self.width)
        if w <= 0:
            raise ValueError("field width must be > 0")
        object.__setattr__(self, "width", w)
        object.__setattr__(self, "signed", bool(self.signed))
        object.__setattr__(
            self,
            "logic_kind",
            _check_logic_kind(self.logic_kind, ctx=f"field {self.name!r}"),
        )

    def __pyc_template_value__(self) -> dict[str, Any]:
        return {
            "kind": "field",
            "name": self.name,
            "width": self.width,
            "signed": self.signed,
            "logic_kind": self.logic_kind,
        }


@dataclass(frozen=True)
class BundleSpec:
    name: str
    fields: tuple[FieldSpec, ...] = field(default_factory=tuple)

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _check_name(self.name, ctx="bundle"))
        fs = tuple(self.fields)
        if not fs:
            raise ValueError("bundle must contain at least one field")
        seen: set[str] = set()
        for f in fs:
            if not isinstance(f, FieldSpec):
                raise TypeError(
                    f"bundle field must be FieldSpec, got {type(f).__name__}"
                )
            if f.name in seen:
                raise ValueError(
                    f"duplicate field name in bundle {self.name!r}: {f.name!r}"
                )
            seen.add(f.name)
        object.__setattr__(self, "fields", fs)

    def __pyc_template_value__(self) -> dict[str, Any]:
        return {
            "kind": "bundle",
            "name": self.name,
            "fields": [f.__pyc_template_value__() for f in self.fields],
        }

    def layout_fields(self) -> tuple[tuple[str, int, bool, str], ...]:
        return tuple(
            (f.name, int(f.width), bool(f.signed), str(f.logic_kind))
            for f in self.fields
        )

    def total_width(self) -> int:
        return sum(int(f.width) for f in self.fields)

    def layout_id(self) -> str:
        return _layout_id("bundle", self.layout_fields())

    def field_slices(self) -> dict[str, tuple[int, int]]:
        return _field_slices(self.layout_fields())


@dataclass(frozen=True)
class StagePipeSpec:
    name: str
    payload: BundleSpec
    has_valid: bool = True
    has_ready: bool = False
    valid_name: str = "valid"
    ready_name: str = "ready"

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _check_name(self.name, ctx="stage pipe"))
        if not isinstance(self.payload, BundleSpec):
            raise TypeError("stage pipe payload must be a BundleSpec")
        object.__setattr__(self, "has_valid", bool(self.has_valid))
        object.__setattr__(self, "has_ready", bool(self.has_ready))
        object.__setattr__(
            self, "valid_name", _check_name(self.valid_name, ctx="valid field")
        )
        object.__setattr__(
            self, "ready_name", _check_name(self.ready_name, ctx="ready field")
        )

    def __pyc_template_value__(self) -> dict[str, Any]:
        return {
            "kind": "stage_pipe",
            "name": self.name,
            "payload": self.payload.__pyc_template_value__(),
            "has_valid": self.has_valid,
            "has_ready": self.has_ready,
            "valid_name": self.valid_name,
            "ready_name": self.ready_name,
        }

    def layout_fields(self) -> tuple[tuple[str, int, bool, str], ...]:
        out: list[tuple[str, int, bool, str]] = []
        out.extend(list(self.payload.layout_fields()))
        if self.has_valid:
            out.append((str(self.valid_name), 1, False, "control"))
        if self.has_ready:
            out.append((str(self.ready_name), 1, False, "control"))
        return tuple(out)

    def total_width(self) -> int:
        return sum(int(w) for _p, w, _s, _k in self.layout_fields())

    def layout_id(self) -> str:
        return _layout_id("stage_pipe", self.layout_fields())

    def field_slices(self) -> dict[str, tuple[int, int]]:
        return _field_slices(self.layout_fields())


@dataclass(frozen=True)
class StructFieldSpec:
    name: str
    width: int | None = None
    signed: bool = False
    struct: StructSpec | None = None
    logic_kind: str = _DEFAULT_LOGIC_KIND

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _check_name(self.name, ctx="struct field"))

        if self.struct is not None:
            if not isinstance(self.struct, StructSpec):
                raise TypeError(
                    f"struct field {self.name!r}: `struct` must be StructSpec"
                )
            if self.width is not None:
                raise ValueError(
                    f"struct field {self.name!r}: nested struct cannot also set width"
                )
            object.__setattr__(self, "width", None)
            object.__setattr__(self, "signed", False)
            object.__setattr__(self, "logic_kind", "aggregate")
            return

        if self.width is None:
            raise ValueError(f"struct field {self.name!r}: leaf width must be set")
        w = int(self.width)
        if w <= 0:
            raise ValueError(f"struct field {self.name!r}: width must be > 0")
        object.__setattr__(self, "width", w)
        object.__setattr__(self, "signed", bool(self.signed))
        object.__setattr__(
            self,
            "logic_kind",
            _check_logic_kind(self.logic_kind, ctx=f"struct field {self.name!r}"),
        )

    @property
    def is_leaf(self) -> bool:
        return self.struct is None

    def __pyc_template_value__(self) -> dict[str, Any]:
        if self.is_leaf:
            return {
                "kind": "struct_field",
                "name": self.name,
                "width": int(self.width or 0),
                "signed": bool(self.signed),
                "logic_kind": str(self.logic_kind),
            }
        return {
            "kind": "struct_field",
            "name": self.name,
            "struct": self.struct.__pyc_template_value__(),
        }


@dataclass(frozen=True)
class StructSpec:
    name: str
    fields: tuple[StructFieldSpec, ...] = field(default_factory=tuple)

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _check_name(self.name, ctx="struct"))
        fs = tuple(self.fields)
        if not fs:
            raise ValueError("struct must contain at least one field")

        seen: set[str] = set()
        for f in fs:
            if not isinstance(f, StructFieldSpec):
                raise TypeError(
                    f"struct field must be StructFieldSpec, got {type(f).__name__}"
                )
            if f.name in seen:
                raise ValueError(
                    f"duplicate field name in struct {self.name!r}: {f.name!r}"
                )
            seen.add(f.name)

        object.__setattr__(self, "fields", tuple(sorted(fs, key=lambda x: x.name)))

    def field_map(self) -> dict[str, StructFieldSpec]:
        return {f.name: f for f in self.fields}

    def _leaf_map(
        self, *, prefix: tuple[str, ...] = ()
    ) -> dict[tuple[str, ...], StructFieldSpec]:
        out: dict[tuple[str, ...], StructFieldSpec] = {}
        for f in self.fields:
            p = (*prefix, f.name)
            if f.is_leaf:
                out[p] = f
            else:
                out.update(f.struct._leaf_map(prefix=p))
        return out

    def leaf_paths(self) -> tuple[str, ...]:
        paths = [_join_path(p) for p in self._leaf_map().keys()]
        return tuple(sorted(paths))

    def flatten_fields(self) -> tuple[tuple[str, StructFieldSpec], ...]:
        leaves = self._leaf_map()
        out = [
            (_join_path(p), v) for p, v in sorted(leaves.items(), key=lambda kv: kv[0])
        ]
        return tuple(out)

    def get_field(self, path: str | Sequence[str]) -> StructFieldSpec:
        parts = _split_path(path, ctx="field path")
        cur: StructSpec = self
        for i, part in enumerate(parts):
            fmap = cur.field_map()
            if part not in fmap:
                raise KeyError(_join_path(parts[: i + 1]))
            fld = fmap[part]
            if i == len(parts) - 1:
                return fld
            if fld.is_leaf:
                raise KeyError(_join_path(parts[: i + 1]))
            cur = fld.struct
        raise KeyError(_join_path(parts))

    @classmethod
    def from_leaf_map(
        cls,
        *,
        name: str,
        fields: Mapping[str, tuple[int, bool] | StructFieldSpec],
    ) -> StructSpec:
        root: dict[str, Any] = {}

        for raw_path, spec in fields.items():
            parts = _split_path(str(raw_path), ctx="leaf path")
            node = root
            for part in parts[:-1]:
                child = node.setdefault(part, {})
                if not isinstance(child, dict):
                    raise ValueError(f"path conflict at {part!r} for {raw_path!r}")
                node = child

            leaf_name = parts[-1]
            if isinstance(spec, StructFieldSpec):
                w, s = _as_leaf_field(spec)
            else:
                w, s = int(spec[0]), bool(spec[1])
            node[leaf_name] = (w, s)

        def _build_struct(struct_name: str, node: dict[str, Any]) -> StructSpec:
            out_fields: list[StructFieldSpec] = []
            for fname in sorted(node.keys()):
                v = node[fname]
                if isinstance(v, dict):
                    out_fields.append(
                        StructFieldSpec(name=fname, struct=_build_struct(fname, v))
                    )
                else:
                    w, s = v
                    out_fields.append(
                        StructFieldSpec(name=fname, width=int(w), signed=bool(s))
                    )
            return StructSpec(name=struct_name, fields=tuple(out_fields))

        return _build_struct(_check_name(name, ctx="struct"), root)

    def add_field(
        self,
        path: str | Sequence[str],
        *,
        width: int | None = None,
        signed: bool = False,
        struct: StructSpec | None = None,
    ) -> StructSpec:
        leaf_map = self._leaf_map()
        parts = _split_path(path, ctx="field path")

        if struct is not None:
            if width is not None:
                raise ValueError("nested add_field cannot set width")
            nested_map = struct._leaf_map(prefix=parts)
            for k in nested_map:
                if k in leaf_map:
                    raise ValueError(f"field already exists: {_join_path(k)}")
            leaf_map.update(nested_map)
        else:
            if width is None:
                raise ValueError("add_field requires width for leaf fields")
            key = tuple(parts)
            if key in leaf_map:
                raise ValueError(f"field already exists: {_join_path(key)}")
            leaf_map[key] = StructFieldSpec(
                name=parts[-1], width=int(width), signed=bool(signed)
            )

        return StructSpec.from_leaf_map(
            name=self.name,
            fields={_join_path(k): _as_leaf_field(v) for k, v in leaf_map.items()},
        )

    def remove_field(self, path: str | Sequence[str]) -> StructSpec:
        parts = _split_path(path, ctx="field path")
        target = tuple(parts)
        leaf_map = self._leaf_map()

        removed = [
            k for k in leaf_map.keys() if k == target or k[: len(target)] == target
        ]
        if not removed:
            raise KeyError(_join_path(parts))

        for k in removed:
            leaf_map.pop(k)
        if not leaf_map:
            raise ValueError("remove_field would produce an empty struct")

        return StructSpec.from_leaf_map(
            name=self.name,
            fields={_join_path(k): _as_leaf_field(v) for k, v in leaf_map.items()},
        )

    def rename_field(self, path: str | Sequence[str], new_name: str) -> StructSpec:
        parts = _split_path(path, ctx="field path")
        dst_name = _check_name(new_name, ctx="field")
        if dst_name == parts[-1]:
            return self

        src = tuple(parts)
        prefix = src[:-1]
        dst_prefix = (*prefix, dst_name)
        leaf_map = self._leaf_map()

        impacted = [k for k in leaf_map.keys() if k == src or k[: len(src)] == src]
        if not impacted:
            raise KeyError(_join_path(parts))

        for k in impacted:
            replaced = (*dst_prefix, *k[len(src) :])
            if replaced in leaf_map and replaced not in impacted:
                raise ValueError(
                    f"rename target already exists: {_join_path(replaced)}"
                )

        moved: dict[tuple[str, ...], StructFieldSpec] = {}
        for k in impacted:
            moved[(*dst_prefix, *k[len(src) :])] = leaf_map.pop(k)
        leaf_map.update(moved)

        return StructSpec.from_leaf_map(
            name=self.name,
            fields={_join_path(k): _as_leaf_field(v) for k, v in leaf_map.items()},
        )

    def select_fields(self, paths: Sequence[str | Sequence[str]]) -> StructSpec:
        leaf_map = self._leaf_map()
        if not paths:
            raise ValueError("select_fields requires at least one path")

        selected: dict[tuple[str, ...], StructFieldSpec] = {}
        for p in paths:
            parts = _split_path(p, ctx="field path")
            key = tuple(parts)
            matched = [k for k in leaf_map.keys() if k == key or k[: len(key)] == key]
            if not matched:
                raise KeyError(_join_path(parts))
            for k in matched:
                selected[k] = leaf_map[k]

        return StructSpec.from_leaf_map(
            name=self.name,
            fields={_join_path(k): _as_leaf_field(v) for k, v in selected.items()},
        )

    def drop_fields(self, paths: Sequence[str | Sequence[str]]) -> StructSpec:
        if not paths:
            return self
        leaf_map = self._leaf_map()
        for p in paths:
            parts = _split_path(p, ctx="field path")
            key = tuple(parts)
            matched = [
                k for k in list(leaf_map.keys()) if k == key or k[: len(key)] == key
            ]
            if not matched:
                raise KeyError(_join_path(parts))
            for k in matched:
                leaf_map.pop(k, None)
        if not leaf_map:
            raise ValueError("drop_fields would produce an empty struct")
        return StructSpec.from_leaf_map(
            name=self.name,
            fields={_join_path(k): _as_leaf_field(v) for k, v in leaf_map.items()},
        )

    def merge(self, other: StructSpec, *, name: str | None = None) -> StructSpec:
        if not isinstance(other, StructSpec):
            raise TypeError(f"merge expects StructSpec, got {type(other).__name__}")

        out = self._leaf_map()
        for k, v in other._leaf_map().items():
            if k in out:
                w0, s0 = _as_leaf_field(out[k])
                w1, s1 = _as_leaf_field(v)
                if w0 != w1 or s0 != s1:
                    raise ValueError(
                        f"merge conflict at {_join_path(k)!r}: left=(width={w0}, signed={s0}) right=(width={w1}, signed={s1})"
                    )
            out[k] = v

        return StructSpec.from_leaf_map(
            name=str(name) if name is not None else self.name,
            fields={_join_path(k): _as_leaf_field(v) for k, v in out.items()},
        )

    def with_prefix(self, prefix: str) -> StructSpec:
        p = str(prefix)
        if not p:
            return self
        leaves = {
            _join_path((f"{p}{k[0]}", *k[1:])): _as_leaf_field(v)
            for k, v in self._leaf_map().items()
        }
        return StructSpec.from_leaf_map(name=self.name, fields=leaves)

    def with_suffix(self, suffix: str) -> StructSpec:
        sfx = str(suffix)
        if not sfx:
            return self
        leaves = {
            _join_path((f"{k[0]}{sfx}", *k[1:])): _as_leaf_field(v)
            for k, v in self._leaf_map().items()
        }
        return StructSpec.from_leaf_map(name=self.name, fields=leaves)

    def __pyc_template_value__(self) -> dict[str, Any]:
        return {
            "kind": "struct",
            "name": self.name,
            "fields": [f.__pyc_template_value__() for f in self.fields],
        }

    def layout_fields(self) -> tuple[tuple[str, int, bool, str], ...]:
        return tuple(
            (
                path,
                int(f.width or 0),
                bool(f.signed),
                str(getattr(f, "logic_kind", _DEFAULT_LOGIC_KIND)),
            )
            for path, f in self.flatten_fields()
        )

    def total_width(self) -> int:
        return sum(int(w) for _p, w, _s, _k in self.layout_fields())

    def layout_id(self) -> str:
        return _layout_id("struct", self.layout_fields())

    def field_slices(self) -> dict[str, tuple[int, int]]:
        return _field_slices(self.layout_fields())


@dataclass(frozen=True)
class SigLeafSpec:
    path: str
    direction: str
    width: int
    signed: bool = False

    def __post_init__(self) -> None:
        parts = _split_path(self.path, ctx="signature leaf path")
        object.__setattr__(self, "path", _join_path(parts))
        object.__setattr__(
            self,
            "direction",
            _check_dir(self.direction, ctx=f"signature leaf {self.path!r}"),
        )
        w = int(self.width)
        if w <= 0:
            raise ValueError(f"signature leaf {self.path!r}: width must be > 0")
        object.__setattr__(self, "width", w)
        object.__setattr__(self, "signed", bool(self.signed))

    def __pyc_template_value__(self) -> dict[str, Any]:
        return {
            "kind": "sig_leaf",
            "path": self.path,
            "dir": self.direction,
            "width": self.width,
            "signed": self.signed,
        }


@dataclass(frozen=True)
class SignatureSpec:
    name: str
    leaves: tuple[SigLeafSpec, ...] = field(default_factory=tuple)

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _check_name(self.name, ctx="signature"))
        ls = tuple(self.leaves)
        if not ls:
            raise ValueError("signature must contain at least one leaf")
        seen: set[str] = set()
        out: list[SigLeafSpec] = []
        for l in ls:
            if not isinstance(l, SigLeafSpec):
                raise TypeError(
                    f"signature leaf must be SigLeafSpec, got {type(l).__name__}"
                )
            if l.path in seen:
                raise ValueError(
                    f"duplicate signature leaf path in {self.name!r}: {l.path!r}"
                )
            seen.add(l.path)
            out.append(l)
        object.__setattr__(self, "leaves", tuple(sorted(out, key=lambda x: x.path)))

    def leaf_paths(self) -> tuple[str, ...]:
        return tuple(l.path for l in self.leaves)

    def get_leaf(self, path: str | Sequence[str]) -> SigLeafSpec:
        p = _join_path(_split_path(path, ctx="signature leaf path"))
        for l in self.leaves:
            if l.path == p:
                return l
        raise KeyError(p)

    def flip(self) -> SignatureSpec:
        flipped: list[SigLeafSpec] = []
        for l in self.leaves:
            d = "out" if l.direction == "in" else "in"
            flipped.append(
                SigLeafSpec(path=l.path, direction=d, width=l.width, signed=l.signed)
            )
        return SignatureSpec(name=self.name, leaves=tuple(flipped))

    def as_struct(self) -> StructSpec:
        leaves: dict[str, tuple[int, bool]] = {
            l.path: (int(l.width), bool(l.signed)) for l in self.leaves
        }
        return StructSpec.from_leaf_map(name=f"{self.name}_shape", fields=leaves)

    def __pyc_template_value__(self) -> dict[str, Any]:
        return {
            "kind": "signature",
            "name": self.name,
            "leaves": [l.__pyc_template_value__() for l in self.leaves],
        }

    @classmethod
    def from_leaf_map(
        cls,
        *,
        name: str,
        fields: Mapping[str, tuple[str, int, bool] | SigLeafSpec],
    ) -> SignatureSpec:
        leaves: list[SigLeafSpec] = []
        for raw_path, spec in fields.items():
            if isinstance(spec, SigLeafSpec):
                leaves.append(spec)
                continue
            d, w, s = spec
            leaves.append(
                SigLeafSpec(
                    path=str(raw_path), direction=str(d), width=int(w), signed=bool(s)
                )
            )
        return SignatureSpec(name=str(name), leaves=tuple(leaves))


@dataclass(frozen=True)
class ParamSpec:
    name: str
    default: bool | int | str
    min_value: int | None = None
    max_value: int | None = None
    choices: tuple[bool | int | str, ...] = field(default_factory=tuple)

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _check_name(self.name, ctx="param"))
        if not isinstance(self.default, _ALLOWED_PARAM_TYPES):
            raise TypeError(
                f"param default must be bool/int/str, got {type(self.default).__name__}"
            )
        if self.min_value is not None:
            object.__setattr__(self, "min_value", int(self.min_value))
        if self.max_value is not None:
            object.__setattr__(self, "max_value", int(self.max_value))
        if (
            self.min_value is not None
            and self.max_value is not None
            and int(self.max_value) < int(self.min_value)
        ):
            raise ValueError("param max_value must be >= min_value")
        cs = tuple(self.choices)
        for c in cs:
            if not isinstance(c, _ALLOWED_PARAM_TYPES):
                raise TypeError(
                    f"param choice must be bool/int/str, got {type(c).__name__}"
                )
        object.__setattr__(self, "choices", cs)

    def validate(self, value: bool | int | str) -> bool | int | str:
        if not isinstance(value, _ALLOWED_PARAM_TYPES):
            raise TypeError(
                f"param {self.name!r} expects bool/int/str, got {type(value).__name__}"
            )
        if self.choices and value not in self.choices:
            raise ValueError(f"param {self.name!r} value {value!r} not in choices")
        if isinstance(value, int):
            if self.min_value is not None and value < int(self.min_value):
                raise ValueError(
                    f"param {self.name!r} value {value} < min {int(self.min_value)}"
                )
            if self.max_value is not None and value > int(self.max_value):
                raise ValueError(
                    f"param {self.name!r} value {value} > max {int(self.max_value)}"
                )
        return value

    def __pyc_template_value__(self) -> dict[str, Any]:
        return {
            "kind": "param_spec",
            "name": self.name,
            "default": self.default,
            "min_value": self.min_value,
            "max_value": self.max_value,
            "choices": list(self.choices),
        }


@dataclass(frozen=True)
class ParamSet:
    values: tuple[tuple[str, bool | int | str], ...]
    name: str | None = None

    def __post_init__(self) -> None:
        vals = tuple((str(k), v) for k, v in self.values)
        if not vals:
            raise ValueError("ParamSet must contain at least one value")
        for k, v in vals:
            _check_name(k, ctx="param key")
            if not isinstance(v, _ALLOWED_PARAM_TYPES):
                raise TypeError(
                    f"param value for {k!r} must be bool/int/str, got {type(v).__name__}"
                )
        object.__setattr__(self, "values", tuple(sorted(vals, key=lambda kv: kv[0])))
        if self.name is not None:
            object.__setattr__(self, "name", _check_name(self.name, ctx="variant"))

    @classmethod
    def from_mapping(
        cls, values: Mapping[str, bool | int | str], *, name: str | None = None
    ) -> ParamSet:
        return cls(values=tuple((str(k), v) for k, v in values.items()), name=name)

    def as_dict(self) -> dict[str, bool | int | str]:
        return {k: v for k, v in self.values}

    def __getitem__(self, key: str) -> bool | int | str:
        k = str(key)
        for kk, vv in self.values:
            if kk == k:
                return vv
        raise KeyError(k)

    def __pyc_template_value__(self) -> dict[str, Any]:
        out: dict[str, Any] = {
            "kind": "param_set",
            "values": [[k, v] for k, v in self.values],
        }
        if self.name is not None:
            out["name"] = self.name
        return out


@dataclass(frozen=True)
class ParamSpace:
    variants: tuple[ParamSet, ...]

    def __post_init__(self) -> None:
        vs = tuple(self.variants)
        if not vs:
            raise ValueError("ParamSpace must contain at least one variant")
        for v in vs:
            if not isinstance(v, ParamSet):
                raise TypeError(
                    f"param space variants must be ParamSet, got {type(v).__name__}"
                )
        object.__setattr__(self, "variants", vs)

    def __iter__(self):
        return iter(self.variants)

    def __len__(self) -> int:
        return len(self.variants)

    def __pyc_template_value__(self) -> dict[str, Any]:
        return {
            "kind": "param_space",
            "variants": [v.__pyc_template_value__() for v in self.variants],
        }


@dataclass(frozen=True)
class DecodeRule:
    name: str
    mask: int
    match: int
    updates: tuple[tuple[str, bool | int | str], ...]
    priority: int = 0

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _check_name(self.name, ctx="decode rule"))
        object.__setattr__(self, "mask", int(self.mask))
        object.__setattr__(self, "match", int(self.match))
        object.__setattr__(self, "priority", int(self.priority))
        ups = tuple((str(k), v) for k, v in self.updates)
        if not ups:
            raise ValueError("decode rule must contain at least one update")
        for k, v in ups:
            _check_name(k, ctx="decode update key")
            if not isinstance(v, _ALLOWED_PARAM_TYPES):
                raise TypeError(
                    f"decode update value for {k!r} must be bool/int/str, got {type(v).__name__}"
                )
        object.__setattr__(self, "updates", tuple(sorted(ups, key=lambda kv: kv[0])))

    @classmethod
    def from_mapping(
        cls,
        *,
        name: str,
        mask: int,
        match: int,
        updates: Mapping[str, bool | int | str],
        priority: int = 0,
    ) -> DecodeRule:
        return cls(
            name=name,
            mask=int(mask),
            match=int(match),
            updates=tuple(updates.items()),
            priority=priority,
        )

    def __pyc_template_value__(self) -> dict[str, Any]:
        return {
            "kind": "decode_rule",
            "name": self.name,
            "mask": self.mask,
            "match": self.match,
            "updates": [[k, v] for k, v in self.updates],
            "priority": self.priority,
        }


def _normalize_family_params(
    v: ParamSet | Mapping[str, bool | int | str] | None
) -> ParamSet | None:
    if v is None:
        return None
    if isinstance(v, ParamSet):
        return v
    if isinstance(v, Mapping):
        if not v:
            return None
        return ParamSet.from_mapping(dict(v))
    raise TypeError(
        f"module family params must be ParamSet/mapping/None, got {type(v).__name__}"
    )


@dataclass(frozen=True)
class ModuleFamilySpec:
    name: str
    module: Any
    params: ParamSet | Mapping[str, bool | int | str] | None = None

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _check_name(self.name, ctx="module family"))
        if not callable(self.module):
            raise TypeError(f"module family {self.name!r}: module must be callable")
        object.__setattr__(self, "params", _normalize_family_params(self.params))

    def list(self, count: int, *, name: str | None = None) -> ModuleListSpec:
        return ModuleListSpec(
            name=str(name or self.name), family=self, count=int(count)
        )

    def vector(self, count: int, *, name: str | None = None) -> ModuleVectorSpec:
        return ModuleVectorSpec(
            name=str(name or self.name), family=self, count=int(count)
        )

    def map(
        self, keys: Iterable[bool | int | str], *, name: str | None = None
    ) -> ModuleMapSpec:
        return ModuleMapSpec(name=str(name or self.name), family=self, keys=tuple(keys))

    def dict(
        self,
        entries: Mapping[
            bool | int | str, ParamSet | Mapping[str, bool | int | str] | None
        ],
        *,
        name: str | None = None,
    ) -> ModuleDictSpec:
        return ModuleDictSpec(
            name=str(name or self.name), family=self, entries=tuple(entries.items())
        )

    def __pyc_template_value__(self) -> dict[str, Any]:
        out: dict[str, Any] = {
            "kind": "module_family",
            "name": self.name,
            "module": {
                "module": str(getattr(self.module, "__module__", "")),
                "qualname": str(
                    getattr(
                        self.module,
                        "__qualname__",
                        getattr(self.module, "__name__", self.module),
                    )
                ),
            },
        }
        if self.params is not None:
            out["params"] = self.params.__pyc_template_value__()
        return out


@dataclass(frozen=True)
class ModuleListSpec:
    name: str
    family: ModuleFamilySpec
    count: int

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _check_name(self.name, ctx="module list"))
        if not isinstance(self.family, ModuleFamilySpec):
            raise TypeError(
                f"module list {self.name!r}: family must be ModuleFamilySpec"
            )
        c = int(self.count)
        if c <= 0:
            raise ValueError("module list count must be > 0")
        object.__setattr__(self, "count", c)

    def keys(self) -> tuple[str, ...]:
        return tuple(str(i) for i in range(self.count))

    def __pyc_template_value__(self) -> dict[str, Any]:
        return {
            "kind": "module_list",
            "name": self.name,
            "family": self.family.__pyc_template_value__(),
            "count": self.count,
        }


@dataclass(frozen=True)
class ModuleVectorSpec:
    name: str
    family: ModuleFamilySpec
    count: int

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _check_name(self.name, ctx="module vector"))
        if not isinstance(self.family, ModuleFamilySpec):
            raise TypeError(
                f"module vector {self.name!r}: family must be ModuleFamilySpec"
            )
        c = int(self.count)
        if c <= 0:
            raise ValueError("module vector count must be > 0")
        object.__setattr__(self, "count", c)

    def keys(self) -> tuple[str, ...]:
        return tuple(str(i) for i in range(self.count))

    def __pyc_template_value__(self) -> dict[str, Any]:
        return {
            "kind": "module_vector",
            "name": self.name,
            "family": self.family.__pyc_template_value__(),
            "count": self.count,
        }


@dataclass(frozen=True)
class ModuleMapSpec:
    name: str
    family: ModuleFamilySpec
    keys_raw: tuple[bool | int | str, ...]

    def __init__(
        self, *, name: str, family: ModuleFamilySpec, keys: Iterable[bool | int | str]
    ):
        object.__setattr__(self, "name", str(name))
        object.__setattr__(self, "family", family)
        object.__setattr__(self, "keys_raw", tuple(keys))
        self.__post_init__()

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _check_name(self.name, ctx="module map"))
        if not isinstance(self.family, ModuleFamilySpec):
            raise TypeError(
                f"module map {self.name!r}: family must be ModuleFamilySpec"
            )

        keys = tuple(self.keys_raw)
        if not keys:
            raise ValueError("module map must contain at least one key")
        canon = [_canonical_key(k) for k in keys]
        if len(set(canon)) != len(canon):
            raise ValueError("module map keys must be unique")
        object.__setattr__(self, "keys_raw", tuple(sorted(keys, key=_canonical_key)))

    def keys(self) -> tuple[str, ...]:
        return tuple(_canonical_key(k) for k in self.keys_raw)

    def __pyc_template_value__(self) -> dict[str, Any]:
        return {
            "kind": "module_map",
            "name": self.name,
            "family": self.family.__pyc_template_value__(),
            "keys": list(self.keys()),
        }


@dataclass(frozen=True)
class ModuleDictSpec:
    name: str
    family: ModuleFamilySpec
    entries_raw: tuple[
        tuple[bool | int | str, ParamSet | Mapping[str, bool | int | str] | None], ...
    ]

    def __init__(
        self,
        *,
        name: str,
        family: ModuleFamilySpec,
        entries: Iterable[
            tuple[bool | int | str, ParamSet | Mapping[str, bool | int | str] | None]
        ],
    ):
        object.__setattr__(self, "name", str(name))
        object.__setattr__(self, "family", family)
        object.__setattr__(self, "entries_raw", tuple(entries))
        self.__post_init__()

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _check_name(self.name, ctx="module dict"))
        if not isinstance(self.family, ModuleFamilySpec):
            raise TypeError(
                f"module dict {self.name!r}: family must be ModuleFamilySpec"
            )

        if not self.entries_raw:
            raise ValueError("module dict must contain at least one entry")

        out: list[tuple[str, ParamSet | None]] = []
        seen: set[str] = set()
        for raw_key, raw_param in self.entries_raw:
            k = _canonical_key(raw_key)
            if k in seen:
                raise ValueError(f"module dict duplicate key: {k!r}")
            seen.add(k)
            out.append((k, _normalize_family_params(raw_param)))

        out = sorted(out, key=lambda kv: kv[0])
        object.__setattr__(self, "entries_raw", tuple(out))

    def keys(self) -> tuple[str, ...]:
        return tuple(k for k, _ in self.entries_raw)

    def params_for(self, key: str) -> ParamSet | None:
        k = str(key)
        for kk, vv in self.entries_raw:
            if kk == k:
                return vv
        raise KeyError(k)

    def __pyc_template_value__(self) -> dict[str, Any]:
        return {
            "kind": "module_dict",
            "name": self.name,
            "family": self.family.__pyc_template_value__(),
            "entries": [
                [k, (v.__pyc_template_value__() if isinstance(v, ParamSet) else None)]
                for k, v in self.entries_raw
            ],
        }


ModuleCollectionSpec = (
    ModuleListSpec | ModuleVectorSpec | ModuleMapSpec | ModuleDictSpec
)


def iter_module_collection(
    spec: ModuleCollectionSpec,
) -> tuple[tuple[str, ParamSet | None], ...]:
    if isinstance(spec, (ModuleListSpec, ModuleVectorSpec)):
        return tuple((k, None) for k in spec.keys())
    if isinstance(spec, ModuleMapSpec):
        return tuple((k, None) for k in spec.keys())
    if isinstance(spec, ModuleDictSpec):
        return tuple((k, p) for k, p in spec.entries_raw)
    raise TypeError(f"expected module collection spec, got {type(spec).__name__}")


def ensure_bundle_spec(spec: BundleSpec | StagePipeSpec) -> BundleSpec:
    if isinstance(spec, BundleSpec):
        return spec
    if isinstance(spec, StagePipeSpec):
        return spec.payload
    raise TypeError(f"expected BundleSpec/StagePipeSpec, got {type(spec).__name__}")


def map_param_specs(
    specs: Sequence[ParamSpec],
    values: Mapping[str, bool | int | str] | None = None,
) -> ParamSet:
    vals: dict[str, bool | int | str] = {}
    values_dict = dict(values or {})
    for s in specs:
        if not isinstance(s, ParamSpec):
            raise TypeError(
                f"param spec list must contain ParamSpec, got {type(s).__name__}"
            )
        if s.name in values_dict:
            vals[s.name] = s.validate(values_dict[s.name])
        else:
            vals[s.name] = s.default
    extra = sorted(set(values_dict.keys()) - set(vals.keys()))
    if extra:
        raise ValueError(f"unknown param value keys: {', '.join(extra)}")
    return ParamSet.from_mapping(vals)


def filter_param_space(
    space: ParamSpace, pred: Callable[[ParamSet], bool]
) -> ParamSpace:
    out = [v for v in space if bool(pred(v))]
    if not out:
        raise ValueError("filtered ParamSpace is empty")
    return ParamSpace(tuple(out))
