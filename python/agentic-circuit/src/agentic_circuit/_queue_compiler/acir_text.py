"""Deterministic ACIR text rendering for the Queue frontend."""

from __future__ import annotations

from _pycircuit_semantics import (
    ArrayType,
    BitsType,
    BoolType,
    EnumType,
    StructType,
    TupleType,
    ValueType,
)

from .._canonical_json import canonical_json_bytes, canonical_mlir_string, sha256_bytes
from .._static_eval import FrozenMap, StaticValue, static_json_value
from .errors import QueueFrontendError
from .model import (
    BitfieldBinding,
    CandidateSetBinding,
    EnumBinding,
    Payload,
    StaticConfigBinding,
    StaticTypeCheck,
)
from .type_rendering import _render_type


def _render_queue_type(payload: ValueType, *, lanes: int = 1, rate: int = 1) -> str:
    parameters = [_render_type(payload)]
    if lanes != 1:
        parameters.append(f"lanes = {lanes}")
    if rate != 1:
        parameters.append(f"rate = {rate}")
    return "!ac.queue<" + ", ".join(parameters) + ">"


def _render_dense_i64(values: tuple[int, ...]) -> str:
    return "array<i64: " + ", ".join(str(value) for value in values) + ">"


def _table_schema_id(entry_type: ValueType, shape: tuple[int, ...]) -> str:
    return sha256_bytes(
        canonical_json_bytes(
            {
                "entry": _render_type(entry_type),
                "layout": "row_major",
                "layout_version": 1,
                "shape": list(shape),
            }
        )
    )


def _render_table_init_value(value: object, descriptor: ValueType) -> str:
    if isinstance(descriptor, BoolType) and type(value) is bool:
        return f"{1 if value else 0} : i1"
    if isinstance(descriptor, BitsType) and type(value) is int:
        return f"{value} : i{descriptor.width}"
    if isinstance(descriptor, EnumType) and type(value) is str:
        return canonical_mlir_string(value)
    if isinstance(descriptor, StructType) and isinstance(value, dict):
        return (
            "{"
            + ", ".join(
                f"{field.name} = "
                + _render_table_init_value(value[field.name], field.type)
                for field in descriptor.fields
            )
            + "}"
        )
    if isinstance(descriptor, TupleType) and isinstance(value, list):
        return (
            "["
            + ", ".join(
                _render_table_init_value(item, item_type)
                for item, item_type in zip(value, descriptor.elements, strict=True)
            )
            + "]"
        )
    if isinstance(descriptor, ArrayType) and isinstance(value, list):
        return (
            "["
            + ", ".join(
                _render_table_init_value(item, descriptor.element) for item in value
            )
            + "]"
        )
    raise AssertionError("validated Table initializer cannot be rendered")


def _render_static_mlir_value(value: StaticValue) -> str:
    if type(value) is bool:
        return "true" if value else "false"
    if type(value) is int:
        return f"{value} : i64"
    if type(value) is str:
        return canonical_mlir_string(value)
    if isinstance(value, FrozenMap):
        return canonical_mlir_string(
            canonical_json_bytes(static_json_value(value)).decode("utf-8")
        )
    raise QueueFrontendError(
        "ACPY-MODULE-007: module ac.const arguments must lower to bool, int, "
        "str, or canonical config attributes"
    )


def _render_static_mlir_dictionary(
    values: tuple[tuple[str, StaticValue], ...],
) -> str:
    return (
        "{"
        + ", ".join(
            f"{name} = {_render_static_mlir_value(value)}"
            for name, value in sorted(values)
        )
        + "}"
    )


def _render_interface_display_attributes(
    inputs: tuple[str, ...], outputs: tuple[str, ...]
) -> str:
    fields = []
    if inputs:
        fields.append(
            "ac.input_display_names = ["
            + ", ".join(canonical_mlir_string(name) for name in inputs)
            + "]"
        )
    if outputs:
        fields.append(
            "ac.output_display_names = ["
            + ", ".join(canonical_mlir_string(name) for name in outputs)
            + "]"
        )
    return "" if not fields else " attributes {" + ", ".join(fields) + "}"


def _render_table_domain_attributes(candidate: CandidateSetBinding) -> str:
    return (
        "{domain_axes = "
        + _render_dense_i64(candidate.domain_axes)
        + ", domain_shape = "
        + _render_dense_i64(candidate.domain_shape)
        + ", domain_strides = "
        + _render_dense_i64(candidate.domain_strides)
        + f", domain_offset = {candidate.domain_offset} : i64}}"
    )


def _render_bitfield(binding: BitfieldBinding, indent: str) -> str:
    fields = ", ".join(
        f"{{lsb = {lsb} : i64, msb = {msb} : i64, name = {canonical_mlir_string(name)}}}"
        for name, (msb, lsb) in binding.layout.fields.items()
    )
    return (
        f"{indent}ac.bitfield @{binding.name} width {binding.layout.width} "
        f"fingerprint {canonical_mlir_string(binding.layout.fingerprint)} fields [{fields}]"
    )


def _align(value: int, alignment: int) -> int:
    return ((value + alignment - 1) // alignment) * alignment


def _abi_layout(descriptor: ValueType) -> tuple[int, int]:
    from _pycircuit_semantics import ArrayType, StructType, TupleType

    if isinstance(descriptor, StructType):
        members = tuple(field.type for field in descriptor.fields)
    elif isinstance(descriptor, TupleType):
        members = descriptor.elements
    elif isinstance(descriptor, ArrayType):
        element_size, element_alignment = _abi_layout(descriptor.element)
        stride = _align(element_size, element_alignment)
        return stride * descriptor.length, element_alignment
    else:
        size = max(1, (descriptor.bit_width() + 7) // 8)
        return size, size

    offset = 0
    alignment = 1
    for member in members:
        member_size, member_alignment = _abi_layout(member)
        offset = _align(offset, member_alignment) + member_size
        alignment = max(alignment, member_alignment)
    return _align(offset, alignment), alignment


def _payload_layout_entry(payload: Payload) -> str:
    size, alignment = _abi_layout(payload.descriptor)
    return (
        f"{payload.acir_type} = "
        f'{{abi_alignment = {alignment} : i64, endianness = "little", '
        f"preferred_alignment = {alignment} : i64, size = {size} : i64}}"
    )


def _render_static_type_attributes(
    bindings: tuple[tuple[str, int], ...],
    payloads: tuple[Payload, ...],
    extra_checks: tuple[StaticTypeCheck, ...] = (),
    config_bindings: tuple[StaticConfigBinding, ...] = (),
) -> str:
    checks = (
        tuple(check for payload in payloads for check in payload.static_type_checks)
        + extra_checks
    )
    identities = tuple(
        payload
        for payload in payloads
        if payload.descriptor.symbol != payload.descriptor.name
    )
    if not bindings and not checks and not identities and not config_bindings:
        return ""
    attributes: list[str] = []
    if config_bindings:
        attributes.append(
            "ac.static_config_bindings = ["
            + ", ".join(
                "{root = "
                + canonical_mlir_string(binding.root)
                + ", schema = "
                + canonical_mlir_string(binding.schema)
                + ", schema_sha256 = "
                + canonical_mlir_string(binding.schema_sha256)
                + ", type = "
                + canonical_mlir_string(binding.type_name)
                + ", value = "
                + canonical_mlir_string(binding.value)
                + "}"
                for binding in config_bindings
            )
            + "]"
        )
    if bindings:
        attributes.append(
            "ac.static_type_bindings = {"
            + ", ".join(f"{name} = {value} : i64" for name, value in bindings)
            + "}"
        )
    if checks:
        rendered_checks = []
        for check in checks:
            program = (
                "["
                + ", ".join(canonical_mlir_string(token) for token in check.program)
                + "]"
            )
            rendered_checks.append(
                "{program = "
                + program
                + f", result = {check.result} : i64, target = "
                + canonical_mlir_string(check.target)
                + (
                    ""
                    if check.concrete_type is None
                    else ", type = " + _render_type(check.concrete_type)
                )
                + "}"
            )
        attributes.append(
            "ac.static_type_checks = [" + ", ".join(rendered_checks) + "]"
        )
    if identities:
        rendered_identities: list[str] = []
        checks_by_target = {check.target: check for check in checks}
        for payload in identities:
            descriptor = payload.descriptor
            targets = sorted(
                target
                for target in checks_by_target
                if target[: len(descriptor.symbol) + 1] == descriptor.symbol + "."
            )
            parameters_by_name: dict[str, str] = {}
            for target in targets:
                for token in checks_by_target[target].program:
                    if token[:6] != "param:":
                        continue
                    parameter = token[6:]
                    for name, _ in descriptor.static_bindings:
                        if parameter == name or parameter.endswith("__" + name):
                            parameters_by_name[name] = parameter
            rendered_bindings = []
            for name, value in descriptor.static_bindings:
                parameter = parameters_by_name.get(name)
                if parameter is None:
                    raise QueueFrontendError(
                        "ACPY-TYPE-008: specialized struct binding lacks a verifier program"
                    )
                rendered_bindings.append(
                    "{name = "
                    + canonical_mlir_string(name)
                    + ", parameter = "
                    + canonical_mlir_string(parameter)
                    + f", value = {value} : i64}}"
                )
            rendered_identities.append(
                "{bindings = ["
                + ", ".join(rendered_bindings)
                + "], fingerprint = "
                + canonical_mlir_string(descriptor.specialization_fingerprint)
                + ", source = "
                + canonical_mlir_string(descriptor.name)
                + ", symbol = "
                + canonical_mlir_string(descriptor.symbol)
                + ", targets = ["
                + ", ".join(canonical_mlir_string(target) for target in targets)
                + "]}"
            )
        attributes.append(
            "ac.static_type_identities = [" + ", ".join(rendered_identities) + "]"
        )
    return ", " + ", ".join(attributes)


def _enum_layout_entry(binding: EnumBinding) -> str:
    size, alignment = _abi_layout(binding.descriptor)
    return (
        f"{_render_type(binding.descriptor)} = "
        f'{{abi_alignment = {alignment} : i64, endianness = "little", '
        f"preferred_alignment = {alignment} : i64, size = {size} : i64}}"
    )


def _render_enum(binding: EnumBinding, indent: str) -> str:
    enumerants = (
        "["
        + ", ".join(
            canonical_mlir_string(item) for item in binding.descriptor.enumerants
        )
        + "]"
    )
    if binding.descriptor.values is None:
        return f"{indent}ac.enum @{binding.name} enumerants {enumerants}"
    values = (
        "[" + ", ".join(f"{value} : i64" for value in binding.descriptor.values) + "]"
    )
    return (
        f"{indent}ac.enum @{binding.name} enumerants {enumerants} "
        f"values {values} width {binding.descriptor.encoding_width}"
    )
