# ruff: noqa: T201 -- CLI emits metrics and evidence paths.
"""Deterministic public input prefixes, independent of DUT tree algorithms."""


def prefixes():
    yield "empty", 1, []
    for n in (128, 512, 2048):
        yield f"ascending-{n}", n, list(range(n))
        yield f"descending-{n}", n, list(reversed(range(n)))
        # An odd multiplier is a permutation modulo each power-of-two length.
        yield f"permuted-{n}", n, [(i * 73 + 19) % n for i in range(n)]
    yield "highest-bit", 0xFFFFFFFF, [0, 0x80000000, 1, 0x80000001]
    yield "u32-range", 0xFFFFFFFF, [0xFFFFFFFE, 0, 0x7FFFFFFF, 0x80000000]
    yield "low-bits-equal", 0xFFFFFFFF, [i << 24 for i in range(128)]
    yield "long-prefix", 0xFFFFFFFF, [0xFFFFFF00 + i for i in range(128)]
    # Zero plus every one-hot u32 key forces all 32 discriminating bit positions.
    yield "depth-32", 0xFFFFFFFF, [0] + [1 << bit for bit in range(32)]
    yield (
        "depth-32-reverse",
        0xFFFFFFFF,
        [1 << bit for bit in reversed(range(32))] + [0],
    )


def validate(keys, tokens):
    """First-error oracle in external order: range takes priority over duplicate."""
    if len(tokens) > 2048 or len(tokens) > keys:
        return 2
    seen = set()
    for token in tokens:
        if not 0 <= token < keys:
            return 3
        if token in seen:
            return 4
        seen.add(token)
    return 0


if __name__ == "__main__":
    import json

    print(
        json.dumps(
            [
                {
                    "name": name,
                    "keys": keys,
                    "tokens": tokens,
                    "expected_code": validate(keys, tokens),
                }
                for name, keys, tokens in prefixes()
            ],
            indent=2,
        )
    )
