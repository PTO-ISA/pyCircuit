"""Deterministic ACIR text rendering for the Queue frontend."""

from __future__ import annotations

import re

from _pycircuit_semantics import (
    ArrayType,
    BitsType,
    BoolType,
    EnumType,
    StructType,
    TupleType,
    ValueType,
)

from .._canonical_json import canonical_mlir_string
from .model import (
    BitfieldBinding,
    CandidateSetBinding,
    EnumBinding,
    Payload,
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
    entry = re.sub("[^A-Za-z0-9]+", "_", _render_type(entry_type)).strip("_")
    dimensions = "x".join(map(str, shape))
    return f"row_major_{entry}_{dimensions}"


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


def _render_interface_display_attributes(
    inputs: tuple[str, ...],
    outputs: tuple[str, ...],
    extra_fields: tuple[str, ...] = (),
) -> str:
    fields = list(extra_fields)
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
        f"fields [{fields}]"
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
