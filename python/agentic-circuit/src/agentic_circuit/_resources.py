"""Static protocol, queue, resource, and address records."""

from __future__ import annotations

from dataclasses import dataclass
from itertools import combinations
from typing import TypeAlias

from ._types import ResourceRef


class FrontendRuleError(ValueError):
    def __init__(self, code: str, message: str) -> None:
        self.code = code
        self.message = message
        super().__init__(f"{code}: {message}")


def _nonempty(value: object, field: str, code: str) -> str:
    if type(value) is not str or not value:
        raise FrontendRuleError(code, f"{field} must be a non-empty string")
    return value


@dataclass(frozen=True, slots=True)
class ProtocolContract:
    identity: str
    producer_role: str
    consumer_role: str
    payload_type: str
    time_domain: str

    def __post_init__(self) -> None:
        for field in (
            "identity",
            "producer_role",
            "consumer_role",
            "payload_type",
            "time_domain",
        ):
            _nonempty(getattr(self, field), field, "ACPY-PROTOCOL-004")
        if self.producer_role == self.consumer_role:
            raise FrontendRuleError(
                "ACPY-PROTOCOL-004", "producer and consumer roles must differ"
            )


def verify_protocol_roles(
    contract: ProtocolContract, producer_role: str, consumer_role: str
) -> None:
    if (
        producer_role != contract.producer_role
        or consumer_role != contract.consumer_role
        or producer_role == consumer_role
    ):
        raise FrontendRuleError(
            "ACPY-PROTOCOL-004",
            f"roles must be {contract.producer_role!r} and {contract.consumer_role!r}",
        )


@dataclass(frozen=True, slots=True)
class QueueSpec:
    name: str
    payload_type: str
    protocol: str
    depth: int
    time_domain: str


def queue(
    name: str,
    *,
    payload_type: str,
    protocol: str,
    depth: int,
    time_domain: str = "default",
) -> QueueSpec:
    _nonempty(name, "queue name", "ACPY-RESOURCE-002")
    _nonempty(payload_type, "payload type", "ACPY-RESOURCE-002")
    _nonempty(protocol, "protocol", "ACPY-RESOURCE-002")
    _nonempty(time_domain, "time domain", "ACPY-RESOURCE-002")
    if type(depth) is not int or depth <= 0:
        raise FrontendRuleError(
            "ACPY-RESOURCE-002", "queue depth must be a positive static integer"
        )
    return QueueSpec(name, payload_type, protocol, depth, time_domain)


@dataclass(frozen=True, slots=True)
class AddressSpaceSpec:
    name: str
    width: int
    unit: str = "byte"


def address_space(name: str, *, width: int, unit: str = "byte") -> AddressSpaceSpec:
    _nonempty(name, "address-space name", "ACPY-ADDRESS-001")
    _nonempty(unit, "address-space unit", "ACPY-ADDRESS-001")
    if type(width) is not int or width <= 0:
        raise FrontendRuleError(
            "ACPY-ADDRESS-001", "address width must be a positive static integer"
        )
    return AddressSpaceSpec(name, width, unit)


@dataclass(frozen=True, slots=True)
class AddressMapEntry:
    start: int
    end: int
    target: ResourceRef[object, object]
    priority: int

    @property
    def range(self) -> tuple[int, int]:
        return (self.start, self.end)


AddressEntryLike: TypeAlias = AddressMapEntry | tuple[object, object, object, object]


@dataclass(frozen=True, slots=True)
class AddressMapSpec:
    space: AddressSpaceSpec
    entries: tuple[AddressMapEntry, ...]


def _entry(value: AddressEntryLike, space: AddressSpaceSpec) -> AddressMapEntry:
    if isinstance(value, AddressMapEntry):
        entry = value
    elif type(value) is tuple and len(value) == 4:
        entry = AddressMapEntry(value[0], value[1], value[2], value[3])
    else:
        raise FrontendRuleError(
            "ACPY-ADDRESS-001", "address-map entry must have four fields"
        )
    if type(entry.start) is not int or type(entry.end) is not int:
        raise FrontendRuleError(
            "ACPY-STATIC-002", "address bounds must be static integers"
        )
    if entry.start < 0 or entry.end <= entry.start:
        raise FrontendRuleError(
            "ACPY-ADDRESS-001", "address range must be finite and non-empty"
        )
    if entry.end.bit_length() > space.width:
        raise FrontendRuleError(
            "ACPY-ADDRESS-001", "address range exceeds its declared space"
        )
    if not isinstance(entry.target, ResourceRef):
        raise FrontendRuleError(
            "ACPY-ADDRESS-001", "address target must be a ResourceRef"
        )
    if type(entry.priority) is not int or entry.priority < 0:
        raise FrontendRuleError(
            "ACPY-ADDRESS-001", "address priority must be a non-negative integer"
        )
    return entry


def verify_address_map(
    entries: tuple[AddressMapEntry, ...],
) -> tuple[AddressMapEntry, ...]:
    ordered = tuple(
        sorted(
            entries,
            key=lambda item: (
                item.start,
                item.end,
                item.priority,
                item.target.stable_name,
            ),
        )
    )
    for left, right in combinations(ordered, 2):
        overlap = max(left.start, right.start) < min(left.end, right.end)
        if overlap and left.priority == right.priority:
            raise FrontendRuleError(
                "ACPY-ADDRESS-003", "ambiguous equal-priority address overlap"
            )
    return ordered


def address_map(space: AddressSpaceSpec, *entries: AddressEntryLike) -> AddressMapSpec:
    if not isinstance(space, AddressSpaceSpec):
        raise FrontendRuleError(
            "ACPY-ADDRESS-001", "address map requires a declared address space"
        )
    normalized = tuple(_entry(entry, space) for entry in entries)
    return AddressMapSpec(space, verify_address_map(normalized))
