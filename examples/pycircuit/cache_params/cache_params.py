from __future__ import annotations

from pycircuit import (
    Circuit,
    CycleAwareCircuit,
    CycleAwareDomain,
    compile_cycle_aware,
    const,
    ct,
    u,
)


@const
def _cache_cfg(
    m: Circuit,
    *,
    ways: int,
    sets: int,
    line_bytes: int,
    addr_width: int,
    data_width: int,
) -> tuple[int, int, int, int, int, int, int]:
    _ = m
    ways_i = max(1, int(ways))
    sets_i = max(1, int(sets))
    line_b = ct.pow2_ceil(max(1, int(line_bytes)))
    addr_w = max(1, int(addr_width))
    data_w = max(1, int(data_width))

    off_bits = ct.clog2(line_b)
    idx_bits = ct.clog2(sets_i)
    tag_bits = max(1, addr_w - off_bits - idx_bits)
    word_bytes = ct.div_ceil(data_w, 8)
    line_words = ct.div_ceil(line_b, max(1, word_bytes))
    return (ways_i, sets_i, line_b, off_bits, idx_bits, tag_bits, line_words)


def build(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    ways: int = 4,
    sets: int = 64,
    line_bytes: int = 64,
    addr_width: int = 40,
    data_width: int = 64,
) -> None:
    _ = domain
    ways_cfg, sets_cfg, line_bytes_cfg, off_bits, idx_bits, tag_bits, line_words = (
        _cache_cfg(
            m,
            ways=ways,
            sets=sets,
            line_bytes=line_bytes,
            addr_width=addr_width,
            data_width=data_width,
        )
    )
    _ = (ways_cfg, sets_cfg, line_bytes_cfg)

    addr = m.input("addr", width=addr_width)
    tag = addr[off_bits + idx_bits : addr_width]

    m.output("tag", tag)
    m.output("line_words", u(max(1, ct.clog2(512)), line_words))
    m.output("tag_bits", u(max(1, ct.clog2(512)), tag_bits))


build.__pycircuit_name__ = "cache_params"


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build,
            name="cache_params",
            eager=True,
            ways=4,
            sets=64,
            line_bytes=64,
            addr_width=40,
            data_width=64,
        ).emit_mlir()
    )
