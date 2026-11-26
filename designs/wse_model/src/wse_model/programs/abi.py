"""Version-one simulator program ABI, independent of any collective algorithm.

This is a functional-model loading interface, not a proposed hardware ISA.
Raw instructions remain runtime inputs to the compiled pyCircuit machine.
"""

from __future__ import annotations

import hashlib
import struct
from collections.abc import Iterable
from dataclasses import asdict, dataclass
from enum import IntEnum

ABI_VERSION = 1
COMMAND_BYTES = 64
_COMMAND = struct.Struct("<16I")
_REGION = struct.Struct("<3I")


class Kind(IntEnum):
    INSTALL = 1
    NEXT_EPOCH = 2
    POST = 3
    COPY = 4
    WAIT = 5
    RETIRE = 6
    END = 7
    SEND = 8
    BARRIER = 9


def _uint(name: str, value: int, width: int = 32) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or not 0 <= value < (1 << width):
        raise ValueError(f"{name} must be an unsigned {width}-bit integer")
    return value


@dataclass(frozen=True)
class DeviceProfile:
    """Explicit resources of the existing functional verification profile."""

    nodes: int = 40
    rows: int = 5
    cols: int = 8
    bank_bytes: int = 65536
    beat_bytes: int = 8
    epoch_bits: int = 8
    early_slots: int = 2
    seen_slots: int = 56
    max_commands: int = 1024

    def validate(self) -> None:
        actual = (40, 5, 8, 65536, 8, 8, 2, 56, 1024)
        declared = tuple(asdict(self).values())
        if declared != actual:
            raise ValueError(
                "profile must match the compiled 40-node, 2/56-slot functional machine"
            )


DEFAULT_PROFILE = DeviceProfile()


@dataclass(frozen=True)
class Command:
    kind: Kind
    node: int = 0
    opcode: int = 0
    epoch: int = 0
    seq: int = 0
    source: int = 0
    src: int = 0
    dst: int = 0
    nbytes: int = 0
    capacity: int = 0
    expected: int = 0
    compiled_expected: int = 0
    route: int = 0
    self_copy: bool = False

    def words(self) -> tuple[int, ...]:
        kind = Kind(self.kind)
        if not isinstance(self.self_copy, bool):
            raise ValueError("self_copy must be Boolean")
        values = (
            int(kind),
            self.node,
            self.opcode,
            self.epoch,
            self.seq,
            self.source,
            self.src,
            self.dst,
            self.nbytes,
            self.capacity,
            self.expected,
            self.compiled_expected,
        )
        for name, value in zip(
            (
                "kind",
                "node",
                "opcode",
                "epoch",
                "seq",
                "source",
                "src",
                "dst",
                "nbytes",
                "capacity",
                "expected",
                "compiled_expected",
            ),
            values,
            strict=True,
        ):
            _uint(name, value)
        _uint("route", self.route, 80)
        if self.node >= 40 or self.source >= 40:
            raise ValueError("node and source must fit the 40-node route space")
        if self.opcode > 7 or self.epoch > 255 or self.seq > 65535:
            raise ValueError("opcode, epoch or sequence exceeds its carrier")
        return (
            *values,
            self.route & 0xFFFFFFFF,
            (self.route >> 32) & 0xFFFFFFFF,
            self.route >> 64,
            int(self.self_copy),
        )

    def pack(self) -> bytes:
        """Encode complete values; protocol errors are still checked by hardware."""
        return _COMMAND.pack(*self.words())

    @classmethod
    def unpack(cls, data: bytes) -> Command:
        if len(data) != COMMAND_BYTES:
            raise ValueError("an instruction must contain exactly 64 bytes")
        words = _COMMAND.unpack(data)
        if words[14] > 0xFFFF or words[15] & ~1:
            raise ValueError("nonzero reserved route or flag bits")
        result = cls(
            kind=Kind(words[0]),
            node=words[1],
            opcode=words[2],
            epoch=words[3],
            seq=words[4],
            source=words[5],
            src=words[6],
            dst=words[7],
            nbytes=words[8],
            capacity=words[9],
            expected=words[10],
            compiled_expected=words[11],
            route=words[12] | words[13] << 32 | words[14] << 64,
            self_copy=bool(words[15]),
        )
        result.words()
        return result


@dataclass(frozen=True)
class Program:
    commands: tuple[Command, ...]
    profile: DeviceProfile = DEFAULT_PROFILE

    def pack(self) -> bytes:
        self.profile.validate()
        if not self.commands or len(self.commands) > self.profile.max_commands:
            raise ValueError("program length must fit the declared instruction capacity")
        if self.commands[-1].kind != Kind.END:
            raise ValueError("a complete program must end explicitly")
        return b"".join(command.pack() for command in self.commands)

    def manifest(self) -> dict:
        binary = self.pack()
        return {
            "schema": "wse-functional-program/1",
            "abi_version": ABI_VERSION,
            "profile": asdict(self.profile),
            "command_bytes": COMMAND_BYTES,
            "command_count": len(self.commands),
            "program_sha256": hashlib.sha256(binary).hexdigest(),
            "commands": [
                {**asdict(command), "kind": int(command.kind), "operation": Kind(command.kind).name}
                for command in self.commands
            ],
        }


@dataclass(frozen=True)
class MemoryRegion:
    node: int
    address: int
    nbytes: int

    def validate(self, profile: DeviceProfile = DEFAULT_PROFILE) -> None:
        profile.validate()
        for name, value in (
            ("node", self.node),
            ("address", self.address),
            ("nbytes", self.nbytes),
        ):
            _uint(name, value)
        if self.node >= profile.nodes or self.address % profile.beat_bytes:
            raise ValueError("memory region has an invalid node or unaligned address")
        if self.nbytes <= 0 or self.nbytes % profile.beat_bytes:
            raise ValueError("memory region must contain positive whole beats")
        if self.address + self.nbytes > profile.bank_bytes:
            raise ValueError("memory region exceeds a node bank")

    def pack(self, profile: DeviceProfile = DEFAULT_PROFILE) -> bytes:
        self.validate(profile)
        return _REGION.pack(self.node, self.address, self.nbytes)


@dataclass(frozen=True)
class MemoryPatch:
    node: int
    address: int
    data: bytes

    @property
    def region(self) -> MemoryRegion:
        return MemoryRegion(self.node, self.address, len(self.data))


def encode_image(patches: Iterable[MemoryPatch], profile: DeviceProfile = DEFAULT_PROFILE) -> bytes:
    return b"".join(patch.region.pack(profile) + patch.data for patch in patches)


def decode_image(data: bytes, profile: DeviceProfile = DEFAULT_PROFILE) -> tuple[MemoryPatch, ...]:
    patches, offset = [], 0
    while offset < len(data):
        if offset + _REGION.size > len(data):
            raise ValueError("truncated memory image header")
        region = MemoryRegion(*_REGION.unpack_from(data, offset))
        region.validate(profile)
        offset += _REGION.size
        end = offset + region.nbytes
        if end > len(data):
            raise ValueError("truncated memory image payload")
        patches.append(MemoryPatch(region.node, region.address, data[offset:end]))
        offset = end
    return tuple(patches)


def encode_readback(
    regions: Iterable[MemoryRegion], profile: DeviceProfile = DEFAULT_PROFILE
) -> bytes:
    return b"".join(region.pack(profile) for region in regions)


def decode_readback(
    data: bytes, profile: DeviceProfile = DEFAULT_PROFILE
) -> tuple[MemoryRegion, ...]:
    if len(data) % _REGION.size:
        raise ValueError("truncated readback range")
    regions = tuple(MemoryRegion(*words) for words in _REGION.iter_unpack(data))
    for region in regions:
        region.validate(profile)
    return regions
