from __future__ import annotations

from pycircuit.hw import Circuit, ClockDomain, Wire
from pycircuit.literals import u


def Cache(
    m: Circuit,
    cd: ClockDomain,
    req_valid: Wire,
    req_addr: Wire,
    req_write: Wire,
    req_wdata: Wire,
    req_wmask: Wire,
    *,
    ways: int = 4,
    sets: int = 64,
    line_bytes: int = 64,
    addr_width: int = 64,
    data_width: int = 64,
    write_back: bool = True,
    write_allocate: bool = True,
    replacement: str = "plru",
):
    """Structural cache baseline.

    Default policy contract:
    - write_back=True
    - write_allocate=True
    - replacement="plru"

    This pyc6 implementation is intentionally compact and hierarchy-preserving; it keeps
    state visible to the compiler flow without flattening into primitive wires.
    """

    _ = (line_bytes, write_back, write_allocate, replacement)
    req_valid_w = req_valid
    req_addr_w = req_addr
    req_write_w = req_write
    req_wdata_w = req_wdata
    _req_wmask_w = req_wmask
    _ = _req_wmask_w
    ways_i = max(1, int(ways))
    sets_i = max(1, int(sets))
    set_bits = max(1, (sets_i - 1).bit_length())
    tag_bits = max(1, int(addr_width) - set_bits)
    plru_bits = max(1, ways_i - 1)
    way_idx_bits = max(1, (ways_i - 1).bit_length())

    tags = [
        m.out(f"cache_tag_{i}", domain=cd, width=tag_bits, init=0)
        for i in range(ways_i)
    ]
    valids = [
        m.out(f"cache_valid_{i}", domain=cd, width=1, init=0) for i in range(ways_i)
    ]
    dirty = [
        m.out(f"cache_dirty_{i}", domain=cd, width=1, init=0) for i in range(ways_i)
    ]
    data = [
        m.out(f"cache_data_{i}", domain=cd, width=int(data_width), init=0)
        for i in range(ways_i)
    ]
    plru = m.out("cache_plru", domain=cd, width=plru_bits, init=0)

    req_tag = req_addr_w[set_bits : set_bits + tag_bits]

    hit = u(1, 0)
    hit_data = u(int(data_width), 0)
    hit_way = u(way_idx_bits, 0)

    for i in range(ways_i):
        way_hit = valids[i].out() & (tags[i].out() == req_tag)
        hit_data = way_hit._select_internal(data[i].out(), hit_data)
        hit_way = way_hit._select_internal(u(way_idx_bits, i), hit_way)
        hit = hit | way_hit

    victim_way = plru.out()[0:way_idx_bits]

    do_alloc = req_valid_w & (~hit)
    do_write_alloc = req_valid_w & req_write_w & do_alloc

    for i in range(ways_i):
        sel_hit = hit & (hit_way == i)
        sel_victim = do_alloc & (victim_way == i)

        tags[i].set(req_tag, when=sel_victim)
        valids[i].set(1, when=sel_victim)

        data[i].set(req_wdata_w, when=sel_hit & req_write_w)
        data[i].set(req_wdata_w, when=sel_victim & req_write_w)

        dirty[i].set(1, when=sel_hit & req_write_w)
        dirty[i].set(do_write_alloc, when=sel_victim)

    plru.set(plru.out() + 1, when=req_valid_w)

    resp_valid = req_valid_w
    resp_ready = req_valid_w
    resp_hit = hit
    resp_data = hit._select_internal(hit_data, u(int(data_width), 0))
    miss = req_valid_w & (~hit)

    return m.bundle_connector(
        resp_valid=resp_valid,
        resp_ready=resp_ready,
        resp_hit=resp_hit,
        resp_data=resp_data,
        miss=miss,
    )
