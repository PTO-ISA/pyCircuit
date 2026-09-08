"""Read and validate the Agentic execution journal in a PYC6TRC3 container."""

from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path
from typing import BinaryIO, Iterator



MAGIC = b"PYC6TRC3"
REPLAY_CHUNK = 0xAC01


class ReplayError(ValueError):
    """The journal cannot establish a complete, ordered execution history."""


def integer(value: object) -> int:
    if not isinstance(value, dict) or set(value) != {"bits", "width", "signed"}:
        raise ReplayError("expected a typed integer")
    return int(value["bits"])


class _Decoder:
    def __init__(self, data: bytes):
        self.data = memoryview(data)
        self.offset = 0

    def take(self, size: int) -> memoryview:
        if size < 0 or self.offset + size > len(self.data):
            raise ReplayError("truncated value")
        result = self.data[self.offset : self.offset + size]
        self.offset += size
        return result

    def uint(self, size: int) -> int:
        return int.from_bytes(self.take(size), "little")

    def string(self) -> str:
        return bytes(self.take(self.uint(4))).decode("utf-8", errors="strict")

    def value(self, depth: int = 0) -> object:
        if depth > 256:
            raise ReplayError("invalid value nesting")
        tag = self.uint(1)
        if tag == 0:
            return None
        if tag == 1:
            value = self.uint(1)
            if value > 1:
                raise ReplayError("invalid boolean")
            return bool(value)
        if tag == 2:
            width, signed, bits = self.uint(4), self.uint(1), self.uint(8)
            if not 1 <= width <= 64 or signed > 1:
                raise ReplayError("invalid integer type")
            bits &= (1 << width) - 1
            return {"bits": str(bits), "width": width, "signed": bool(signed)}
        if tag == 3:
            return self.string()
        if tag == 4:
            return [self.value(depth + 1) for _ in range(self.uint(4))]
        if tag == 5:
            result = {}
            for _ in range(self.uint(4)):
                key = self.string()
                if key in result:
                    raise ReplayError("duplicate object field")
                result[key] = self.value(depth + 1)
            return result
        if tag == 6:
            return {"float64_bits": bytes(self.take(8)).hex()}
        raise ReplayError(f"unknown value tag {tag}")


def records(stream: BinaryIO) -> Iterator[dict]:
    if stream.read(8) != MAGIC:
        raise ReplayError("expected PYC6TRC3 header")
    header = stream.read(8)
    if len(header) != 8 or struct.unpack("<II", header) != (3, 1):
        raise ReplayError("unsupported trace version or flags")
    sequence = 0
    while header := stream.read(8):
        if len(header) != 8:
            raise ReplayError("truncated chunk header")
        size, kind = struct.unpack("<II", header)
        payload = stream.read(size)
        if len(payload) != size:
            raise ReplayError("truncated chunk payload")
        if kind != REPLAY_CHUNK:
            continue
        decoder = _Decoder(payload)
        value = decoder.value()
        if decoder.offset != len(payload) or not isinstance(value, dict):
            raise ReplayError("invalid event payload")
        if integer(value.get("sequence")) != sequence:
            raise ReplayError("event sequence is not contiguous")
        sequence += 1
        yield value


@dataclass
class Replay:
    manifest: dict
    initial: dict
    commits: list[dict]
    events: list[dict]
    sources: list[dict]
    final: dict
    complete: bool
    diagnostic: str | None


def read_replay(path: Path) -> Replay:
    manifest, initial, state = {}, {}, {}
    commits, events, sources = [], [], []
    active = None
    complete = False
    diagnostic = None
    try:
        with path.open("rb") as stream:
            for record in records(stream):
                kind = record.get("kind")
                if complete:
                    raise ReplayError("records after end marker")
                if kind == "source":
                    sources.append(record)
                elif kind == "manifest":
                    if manifest or initial:
                        raise ReplayError("duplicate manifest")
                    if record.get("format") != "agentic-circuit-replay" or integer(record.get("version")) != 1:
                        raise ReplayError("unsupported replay format")
                    objects = record.get("objects")
                    if not isinstance(objects, list):
                        raise ReplayError("missing object inventory")
                    ids = [integer(obj["id"]) for obj in objects]
                    if len(set(ids)) != len(ids):
                        raise ReplayError("duplicate object identity")
                    manifest = record
                elif kind == "initial":
                    if not manifest or initial:
                        raise ReplayError("initial snapshot ordering")
                    initial = record["state"]
                    if set(initial) != {str(integer(obj["id"])) for obj in manifest["objects"]}:
                        raise ReplayError("initial state does not cover inventory")
                    state = dict(initial)
                elif kind == "barrier_begin":
                    if not manifest or active is not None or integer(record["batch"]) != len(commits):
                        raise ReplayError("invalid barrier begin")
                    active = (integer(record["time"]), integer(record["delta"]))
                elif kind == "event":
                    events.append(record)
                elif kind == "commit":
                    if active != (integer(record["time"]), integer(record["delta"])) or integer(record["batch"]) != len(commits):
                        raise ReplayError("commit does not close active barrier")
                    changes = record["changes"]
                    for object_id, change in changes.items():
                        if object_id not in state or state[object_id] != change["before"]:
                            raise ReplayError("commit before-value differs from reconstructed state")
                    state.update({key: value["after"] for key, value in changes.items()})
                    commits.append(record)
                    active = None
                elif kind == "end":
                    if active is not None or record["state"] != state:
                        raise ReplayError("final snapshot differs from reconstructed state")
                    complete = True
                else:
                    raise ReplayError(f"unknown replay event {kind!r}")
    except (ReplayError, UnicodeError, KeyError, TypeError) as error:
        diagnostic = str(error)
        complete = False
    if not manifest or not initial:
        raise ReplayError(diagnostic or "missing replay inventory/initial snapshot")
    if not complete and not diagnostic:
        diagnostic = "missing end marker; showing only complete commit barriers"
    return Replay(manifest, initial, commits, events, sources, state, complete, diagnostic)
