from __future__ import annotations

from collections.abc import Mapping
from dataclasses import asdict, dataclass, fields, is_dataclass
from typing import Any

from .types import (
    BundleSpec,
    DecodeRule,
    FieldSpec,
    ModuleFamilySpec,
    ParamSet,
    ParamSpace,
    ParamSpec,
    SigLeafSpec,
    SignatureSpec,
    StagePipeSpec,
    StructSpec,
    filter_param_space,
    map_param_specs,
)


@dataclass
class _BundleBuilder:
    _name: str
    _fields: list[FieldSpec]

    def field(self, name: str, *, width: int, signed: bool = False) -> _BundleBuilder:
        self._fields.append(FieldSpec(name=name, width=width, signed=signed))
        return self

    def build(self) -> BundleSpec:
        return BundleSpec(name=self._name, fields=tuple(self._fields))


@dataclass
class _StructBuilder:
    _name: str
    _entries: list[tuple[str, StructSpec | tuple[int, bool]]]

    def field(self, path: str, *, width: int, signed: bool = False) -> _StructBuilder:
        self._entries.append((str(path), (int(width), bool(signed))))
        return self

    def nested(self, path: str, spec: StructSpec) -> _StructBuilder:
        if not isinstance(spec, StructSpec):
            raise TypeError(f"nested() expects StructSpec, got {type(spec).__name__}")
        self._entries.append((str(path), spec))
        return self

    def build(self) -> StructSpec:
        if not self._entries:
            raise ValueError("struct() requires at least one field")
        leaves: dict[str, tuple[int, bool]] = {}
        for path, spec in self._entries:
            if isinstance(spec, StructSpec):
                for sub_path, sub_field in spec.flatten_fields():
                    leaves[f"{path}.{sub_path}"] = (
                        int(sub_field.width or 0),
                        bool(sub_field.signed),
                    )
            else:
                leaves[path] = (int(spec[0]), bool(spec[1]))
        return StructSpec.from_leaf_map(name=self._name, fields=leaves)


@dataclass
class _SignatureBuilder:
    _name: str
    _leaves: list[SigLeafSpec]

    def in_(self, path: str, *, width: int, signed: bool = False) -> _SignatureBuilder:
        self._leaves.append(
            SigLeafSpec(
                path=str(path), direction="in", width=int(width), signed=bool(signed)
            )
        )
        return self

    def out_(self, path: str, *, width: int, signed: bool = False) -> _SignatureBuilder:
        self._leaves.append(
            SigLeafSpec(
                path=str(path), direction="out", width=int(width), signed=bool(signed)
            )
        )
        return self

    def build(self) -> SignatureSpec:
        if not self._leaves:
            raise ValueError("signature() requires at least one leaf")
        return SignatureSpec(name=self._name, leaves=tuple(self._leaves))


@dataclass
class _ParamsBuilder:
    _specs: list[ParamSpec]

    def add(
        self,
        name: str,
        *,
        default: bool | int | str,
        min_value: int | None = None,
        max_value: int | None = None,
        choices: tuple[bool | int | str, ...] = (),
    ) -> _ParamsBuilder:
        self._specs.append(
            ParamSpec(
                name=name,
                default=default,
                min_value=min_value,
                max_value=max_value,
                choices=choices,
            )
        )
        return self

    def build(self, values: Mapping[str, bool | int | str] | None = None) -> ParamSet:
        return map_param_specs(tuple(self._specs), values=values)


@dataclass
class _RulesetBuilder:
    _rules: list[DecodeRule]

    def rule(
        self,
        *,
        name: str,
        mask: int,
        match: int,
        updates: Mapping[str, bool | int | str],
        priority: int = 0,
    ) -> _RulesetBuilder:
        self._rules.append(
            DecodeRule.from_mapping(
                name=name,
                mask=mask,
                match=match,
                updates=updates,
                priority=priority,
            )
        )
        return self

    def build(self) -> tuple[DecodeRule, ...]:
        return tuple(
            sorted(self._rules, key=lambda r: (int(r.priority), r.name), reverse=True)
        )


def _canonical_template_obj(v: Any) -> Any:
    if v is None or isinstance(v, (bool, int, str)):
        return v
    if isinstance(v, (list, tuple)):
        return [_canonical_template_obj(x) for x in v]
    if isinstance(v, dict):
        return {
            str(k): _canonical_template_obj(vv)
            for k, vv in sorted(v.items(), key=lambda kv: str(kv[0]))
        }
    fn = getattr(v, "__pyc_template_value__", None)
    if callable(fn):
        return _canonical_template_obj(fn())
    if is_dataclass(v):
        out: dict[str, Any] = {}
        for f in fields(v):
            out[str(f.name)] = _canonical_template_obj(getattr(v, f.name))
        return out
    raise TypeError(f"valueclass canonicalization does not support {type(v).__name__}")


def valueclass(_cls: type[Any] | None = None, *, frozen: bool = True):
    """Decorator that marks a class as a canonical template value object.

    The class is converted into a dataclass (frozen by default) and receives
    `__pyc_template_value__()` for stable template caching/identity.
    """

    def wrap(cls: type[Any]) -> type[Any]:
        dc = cls if is_dataclass(cls) else dataclass(frozen=frozen)(cls)

        if not hasattr(dc, "__pyc_template_value__"):

            def __pyc_template_value__(self) -> dict[str, Any]:
                payload = _canonical_template_obj(asdict(self))
                if not isinstance(payload, dict):
                    raise TypeError("valueclass canonical payload must be a mapping")
                return {
                    "kind": "valueclass",
                    "type": f"{dc.__module__}.{dc.__qualname__}",
                    "fields": payload,
                }

            dc.__pyc_template_value__ = __pyc_template_value__

        return dc

    if _cls is not None:
        return wrap(_cls)
    return wrap


def bundle(name: str) -> _BundleBuilder:
    return _BundleBuilder(_name=str(name), _fields=[])


def struct(name: str) -> _StructBuilder:
    return _StructBuilder(_name=str(name), _entries=[])


def signature(name: str) -> _SignatureBuilder:
    return _SignatureBuilder(_name=str(name), _leaves=[])


def stage_pipe(
    name: str,
    *,
    payload: BundleSpec,
    has_valid: bool = True,
    has_ready: bool = False,
    valid_name: str = "valid",
    ready_name: str = "ready",
) -> StagePipeSpec:
    return StagePipeSpec(
        name=str(name),
        payload=payload,
        has_valid=bool(has_valid),
        has_ready=bool(has_ready),
        valid_name=str(valid_name),
        ready_name=str(ready_name),
    )


def params() -> _ParamsBuilder:
    return _ParamsBuilder(_specs=[])


def ruleset() -> _RulesetBuilder:
    return _RulesetBuilder(_rules=[])


def module_family(
    name: str,
    *,
    module: Any,
    params: ParamSet | Mapping[str, bool | int | str] | None = None,
) -> ModuleFamilySpec:
    return ModuleFamilySpec(name=str(name), module=module, params=params)


def filtered(space: ParamSpace, pred) -> ParamSpace:
    return filter_param_space(space, pred)
