from __future__ import annotations

from dataclasses import dataclass


# Must match `library/cpp/pyc_probe_registry.hpp` (Decision 0017).
@dataclass(frozen=True)
class InstancePathShorteningPolicy:
    max_segments: int = 16
    max_chars: int = 240
    keep_head: int = 2
    keep_tail: int = 2


_MASK64 = 0xFFFFFFFFFFFFFFFF


def _rotl64(x: int, r: int) -> int:
    x &= _MASK64
    return ((x << r) & _MASK64) | (x >> (64 - r))


def xxhash64(data: bytes, seed: int = 0) -> int:
    # xxHash64 reference algorithm; must match the C++ runtime implementation
    # used for probe_id and instance-path shortening.
    prime1 = 11400714785074694791
    prime2 = 14029467366897019727
    prime3 = 1609587929392839161
    prime4 = 9650029242287828579
    prime5 = 2870177450012600261

    def read64(i: int) -> int:
        return int.from_bytes(data[i : i + 8], "little")

    def read32(i: int) -> int:
        return int.from_bytes(data[i : i + 4], "little")

    def round_(acc: int, inp: int) -> int:
        acc = (acc + (inp * prime2)) & _MASK64
        acc = _rotl64(acc, 31)
        acc = (acc * prime1) & _MASK64
        return acc

    def merge_round(acc: int, val: int) -> int:
        val = round_(0, val)
        acc ^= val
        acc = (acc * prime1 + prime4) & _MASK64
        return acc

    n = len(data)
    p = 0

    if n >= 32:
        v1 = (seed + prime1 + prime2) & _MASK64
        v2 = (seed + prime2) & _MASK64
        v3 = seed & _MASK64
        v4 = (seed - prime1) & _MASK64

        limit = n - 32
        while p <= limit:
            v1 = round_(v1, read64(p))
            p += 8
            v2 = round_(v2, read64(p))
            p += 8
            v3 = round_(v3, read64(p))
            p += 8
            v4 = round_(v4, read64(p))
            p += 8

        h64 = (
            _rotl64(v1, 1) + _rotl64(v2, 7) + _rotl64(v3, 12) + _rotl64(v4, 18)
        ) & _MASK64
        h64 = merge_round(h64, v1)
        h64 = merge_round(h64, v2)
        h64 = merge_round(h64, v3)
        h64 = merge_round(h64, v4)
    else:
        h64 = (seed + prime5) & _MASK64

    h64 = (h64 + n) & _MASK64

    while p + 8 <= n:
        k1 = read64(p)
        k1 = (k1 * prime2) & _MASK64
        k1 = _rotl64(k1, 31)
        k1 = (k1 * prime1) & _MASK64
        h64 ^= k1
        h64 = (_rotl64(h64, 27) * prime1 + prime4) & _MASK64
        p += 8

    if p + 4 <= n:
        h64 ^= (read32(p) * prime1) & _MASK64
        h64 = (_rotl64(h64, 23) * prime2 + prime3) & _MASK64
        p += 4

    while p < n:
        h64 ^= (data[p] * prime5) & _MASK64
        h64 = (_rotl64(h64, 11) * prime1) & _MASK64
        p += 1

    # Avalanche.
    h64 ^= h64 >> 33
    h64 = (h64 * prime2) & _MASK64
    h64 ^= h64 >> 29
    h64 = (h64 * prime3) & _MASK64
    h64 ^= h64 >> 32
    return h64 & _MASK64


def shorten_instance_path(
    full_path: str,
    policy: InstancePathShorteningPolicy = InstancePathShorteningPolicy(),
) -> str:
    # Defensive: if a caller passes a canonical_path, only shorten the instance
    # prefix.
    full_path = str(full_path)
    if ":" in full_path:
        inst, sep, rest = full_path.partition(":")
        return shorten_instance_path(inst, policy) + sep + rest

    segs = [s for s in full_path.split(".") if s]
    if len(segs) <= int(policy.max_segments) and len(full_path) <= int(
        policy.max_chars
    ):
        return full_path

    hash_seg = f"_h{xxhash64(full_path.encode('utf-8'), seed=0):016x}"

    def join(head: int, tail: int) -> str:
        n = len(segs)
        if n == 0:
            return full_path
        head = min(int(head), n)
        tail = min(int(tail), n - head)  # drop overlaps
        out_segs: list[str] = []
        out_segs.extend(segs[:head])
        out_segs.append(hash_seg)
        if tail:
            out_segs.extend(segs[n - tail :])
        return ".".join(out_segs)

    n = len(segs)
    if n <= 1:
        return full_path

    max_out_segs = max(1, int(policy.max_segments))
    want_head = max(1, int(policy.keep_head))
    want_tail = max(1, int(policy.keep_tail))

    cands: list[tuple[int, int]] = [
        (min(want_head, n), min(want_tail, n)),
        (2, 2),
        (2, 1),
        (1, 2),
        (1, 1),
        (1, 0),
        (0, 1),
        (0, 0),
    ]

    for head, tail in cands:
        head = min(int(head), n)
        tail = min(int(tail), n)
        if head == 0 and n >= 1:
            head = 1  # keep root segment (usually "dut")

        while head + 1 + tail > max_out_segs:
            if tail > 0:
                tail -= 1
            elif head > 1:
                head -= 1
            else:
                break

        out = join(head, tail)
        if len(out) <= int(policy.max_chars) and len(
            [s for s in out.split(".") if s]
        ) <= int(policy.max_segments):
            return out

    # Worst-case fallback: root + hash.
    if segs:
        return f"{segs[0]}.{hash_seg}"
    return hash_seg
