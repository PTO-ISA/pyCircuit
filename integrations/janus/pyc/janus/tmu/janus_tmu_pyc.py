from __future__ import annotations

import os
from dataclasses import dataclass

from pycircuit import Circuit, Reg, Wire, function, module, mux, u, zext
from pycircuit.hw import cat

RING_ORDER = [0, 1, 3, 5, 7, 6, 4, 2]
NODE_COUNT = 8


@function
def _mux_by_uindex(
    m: Circuit, *, idx: Wire, items: list[Wire | Reg], default: Wire
) -> Wire:
    """Select an item by unsigned index using a balanced mux tree."""

    _ = m
    if not items:
        return default

    values: list[Wire] = []
    for item in items:
        values.append(item if isinstance(item, Wire) else item.out())
    if len(values) != NODE_COUNT:
        raise ValueError(f"expected {NODE_COUNT} mux inputs")

    stage = values
    for bit in range(3):
        next_stage: list[Wire] = []
        for pos in range(0, len(stage), 2):
            next_stage.append(mux(idx[bit], stage[pos + 1], stage[pos]))
        stage = next_stage
    return stage[0]


@function
def _pack_req_meta(
    m: Circuit,
    write: Wire,
    src: Wire,
    dst: Wire,
    tag: Wire,
    addr: Wire,
) -> Wire:
    _ = m
    return zext(cat(addr, tag, dst, src, write), width=64)


@function
def _pack_rsp_meta(m: Circuit, write: Wire, src: Wire, dst: Wire, tag: Wire) -> Wire:
    _ = m
    return zext(cat(tag, dst, src, write), width=64)


def _build_cw_pref() -> list[list[int]]:
    order = RING_ORDER
    n = len(order)
    pos = {node: i for i, node in enumerate(order)}
    prefs: list[list[int]] = [[0 for _ in range(n)] for _ in range(n)]
    for s in range(n):
        for d in range(n):
            if s == d:
                prefs[s][d] = 1
                continue
            s_pos = pos[s]
            d_pos = pos[d]
            cw = (d_pos - s_pos) % n
            cc = (s_pos - d_pos) % n
            prefs[s][d] = 1 if cw <= cc else 0
    return prefs


CW_PREF = _build_cw_pref()


@function
def _dir_cw(m: Circuit, *, src: int, dst: Wire) -> Wire:
    c = m.const
    items = [c(1 if CW_PREF[src][i] else 0, width=1) for i in range(NODE_COUNT)]
    return _mux_by_uindex(m, idx=dst, items=items, default=c(1, width=1))


@function
def _field(m: Circuit, w: Wire, *, lsb: int, width: int) -> Wire:
    _ = m
    return w.slice(lsb=lsb, width=width)


@function
def _and_all(m: Circuit, items: list[Wire]) -> Wire:
    out = u(1, 1)
    for it in items:
        out = out & it
    return out


@function
def _select_words(
    m: Circuit, sel: Wire, a_words: list[Wire], b_words: list[Wire]
) -> list[Wire]:
    _ = m
    out: list[Wire] = []
    for index in range(len(a_words)):
        out.append(mux(sel, a_words[index], b_words[index]))
    return out


@function
def _select4_words(
    m: Circuit,
    sel_a: Wire,
    sel_b: Wire,
    sel_c: Wire,
    sel_d: Wire,
    wa: list[Wire],
    wb: list[Wire],
    wc: list[Wire],
    wd: list[Wire],
) -> list[Wire]:
    _ = m
    out: list[Wire] = []
    for index in range(len(wa)):
        out.append(
            mux(
                sel_a,
                wa[index],
                mux(sel_b, wb[index], mux(sel_c, wc[index], wd[index])),
            )
        )
    return out


@dataclass(frozen=True)
class BundleFifo:
    in_ready: Wire
    out_valid: Wire
    out_meta: Wire
    out_data: list[Wire]


@module
def _build_bundle_fifo(
    m: Circuit,
    *,
    clk: Wire,
    rst: Wire,
    in_valid: Wire,
    in_meta: Wire,
    in_data: Wire,
    out_ready: Wire,
    depth: int,
    line_words: int,
) -> None:
    push = m.named_wire("push", width=1)
    pop = m.named_wire("pop", width=1)

    meta_in_ready, meta_out_valid, meta_out_data = m.fifo(
        clk,
        rst,
        in_valid=push,
        in_data=in_meta,
        out_ready=pop,
        depth=depth,
    )

    data_in_ready: list[Wire] = []
    data_out_valid: list[Wire] = []
    data_out_data: list[Wire] = []

    for wi in range(line_words):
        word = in_data[wi * 64 : (wi + 1) * 64]
        in_ready_w, out_valid_w, out_data_w = m.fifo(
            clk,
            rst,
            in_valid=push,
            in_data=word,
            out_ready=pop,
            depth=depth,
        )
        data_in_ready.append(in_ready_w)
        data_out_valid.append(out_valid_w)
        data_out_data.append(out_data_w)

    bundle_in_ready = meta_in_ready & _and_all(m, data_in_ready)
    bundle_out_valid = meta_out_valid & _and_all(m, data_out_valid)

    m.assign(push, in_valid & bundle_in_ready)
    m.assign(pop, out_ready & bundle_out_valid)

    m.output("in_ready", bundle_in_ready)
    m.output("out_valid", bundle_out_valid)
    m.output("out_meta", meta_out_data)
    packed_out_data = data_out_data[0]
    for wi in range(1, line_words):
        packed_out_data = cat(data_out_data[wi], packed_out_data)
    m.output("out_data", packed_out_data)


@dataclass(frozen=True)
class NodeIo:
    req_valid: Wire
    req_write: Wire
    req_addr: Wire
    req_tag: Wire
    req_data_words: list[Wire]
    req_ready: Wire
    resp_ready: Wire
    resp_valid: Wire
    resp_tag: Wire
    resp_data_words: list[Wire]
    resp_is_write: Wire


@module
def build(
    m: Circuit,
    *,
    tile_bytes: int | None = None,
    tag_bits: int = 8,
    spb_depth: int = 4,
    mgb_depth: int = 4,
) -> None:
    if tile_bytes is None:
        tile_bytes = int(os.getenv("JANUS_TMU_TILE_BYTES", 1 << 20))
    if tile_bytes <= 0:
        raise ValueError("tile_bytes must be > 0")

    line_bytes = 256
    line_words = line_bytes // 8
    pipe_count = NODE_COUNT

    if tile_bytes % (pipe_count * line_bytes) != 0:
        raise ValueError("tile_bytes must be divisible by 8 * 256")

    addr_bits = (tile_bytes - 1).bit_length()
    offset_bits = (line_bytes - 1).bit_length()
    pipe_bits = (pipe_count - 1).bit_length()
    if addr_bits < offset_bits + pipe_bits:
        raise ValueError("tile_bytes too small for pipe addressing")

    index_bits = addr_bits - offset_bits - pipe_bits
    lines_per_pipe = tile_bytes // (pipe_count * line_bytes)

    c = m.const
    node_bits = pipe_bits

    clk = m.clock("clk")
    rst = m.reset("rst")

    # Meta layouts (packed into 64-bit).
    req_write_lsb = 0
    req_src_lsb = req_write_lsb + 1
    req_dst_lsb = req_src_lsb + node_bits
    req_tag_lsb = req_dst_lsb + node_bits
    req_addr_lsb = req_tag_lsb + tag_bits

    rsp_write_lsb = 0
    rsp_src_lsb = rsp_write_lsb + 1
    rsp_dst_lsb = rsp_src_lsb + node_bits
    rsp_tag_lsb = rsp_dst_lsb + node_bits

    # --- Node IOs ---
    nodes: list[NodeIo] = []
    for i in range(NODE_COUNT):
        req_valid = m.input(f"n{i}_req_valid", width=1)
        req_write = m.input(f"n{i}_req_write", width=1)
        req_addr = m.input(f"n{i}_req_addr", width=addr_bits)
        req_tag = m.input(f"n{i}_req_tag", width=tag_bits)
        req_data_words = [
            m.input(f"n{i}_req_data_w{wi}", width=64) for wi in range(line_words)
        ]
        resp_ready = m.input(f"n{i}_resp_ready", width=1)

        req_ready = m.named_wire(f"n{i}_req_ready", width=1)
        resp_valid = m.named_wire(f"n{i}_resp_valid", width=1)
        resp_tag = m.named_wire(f"n{i}_resp_tag", width=tag_bits)
        resp_data_words = [
            m.named_wire(f"n{i}_resp_data_w{wi}", width=64) for wi in range(line_words)
        ]
        resp_is_write = m.named_wire(f"n{i}_resp_is_write", width=1)

        nodes.append(
            NodeIo(
                req_valid=req_valid,
                req_write=req_write,
                req_addr=req_addr,
                req_tag=req_tag,
                req_data_words=req_data_words,
                req_ready=req_ready,
                resp_ready=resp_ready,
                resp_valid=resp_valid,
                resp_tag=resp_tag,
                resp_data_words=resp_data_words,
                resp_is_write=resp_is_write,
            )
        )

    # --- Build SPB bundles per node (cw/cc) ---
    spb_cw: list[BundleFifo] = []
    spb_cc: list[BundleFifo] = []
    spb_cw_out_ready: list[Wire] = []
    spb_cc_out_ready: list[Wire] = []

    req_meta: list[Wire] = []
    req_words: list[list[Wire]] = []
    req_dir_cw: list[Wire] = []

    for i, node in enumerate(nodes):
        dst = node.req_addr.slice(lsb=offset_bits, width=pipe_bits)
        src = c(i, width=node_bits)
        meta = _pack_req_meta(m, node.req_write, src, dst, node.req_tag, node.req_addr)
        req_meta.append(meta)
        words = node.req_data_words
        packed_words = words[0]
        for wi in range(1, line_words):
            packed_words = cat(words[wi], packed_words)
        req_words.append(words)

        dir_cw = _dir_cw(m, src=i, dst=dst)
        req_dir_cw.append(dir_cw)

        in_valid_cw = node.req_valid & dir_cw
        in_valid_cc = node.req_valid & (~dir_cw)

        cw_ready = m.named_wire(f"spb{i}_cw_out_ready", width=1)
        cc_ready = m.named_wire(f"spb{i}_cc_out_ready", width=1)
        spb_cw_out_ready.append(cw_ready)
        spb_cc_out_ready.append(cc_ready)

        cw_fifo = _build_bundle_fifo(
            m,
            clk=clk,
            rst=rst,
            in_valid=in_valid_cw,
            in_meta=meta,
            in_data=packed_words,
            out_ready=cw_ready,
            depth=spb_depth,
            line_words=line_words,
        )
        cc_fifo = _build_bundle_fifo(
            m,
            clk=clk,
            rst=rst,
            in_valid=in_valid_cc,
            in_meta=meta,
            in_data=packed_words,
            out_ready=cc_ready,
            depth=spb_depth,
            line_words=line_words,
        )
        spb_cw.append(
            BundleFifo(
                in_ready=cw_fifo["in_ready"],
                out_valid=cw_fifo["out_valid"],
                out_meta=cw_fifo["out_meta"],
                out_data=[
                    cw_fifo["out_data"][wi * 64 : (wi + 1) * 64]
                    for wi in range(line_words)
                ],
            )
        )
        spb_cc.append(
            BundleFifo(
                in_ready=cc_fifo["in_ready"],
                out_valid=cc_fifo["out_valid"],
                out_meta=cc_fifo["out_meta"],
                out_data=[
                    cc_fifo["out_data"][wi * 64 : (wi + 1) * 64]
                    for wi in range(line_words)
                ],
            )
        )

        m.assign(node.req_ready, mux(dir_cw, spb_cw[i].in_ready, spb_cc[i].in_ready))

    # --- Ring link registers (request + response, cw/cc) ---
    req_cw_link_valid: list[Reg] = []
    req_cw_link_meta: list[Reg] = []
    req_cw_link_data: list[list[Reg]] = []
    req_cc_link_valid: list[Reg] = []
    req_cc_link_meta: list[Reg] = []
    req_cc_link_data: list[list[Reg]] = []

    rsp_cw_link_valid: list[Reg] = []
    rsp_cw_link_meta: list[Reg] = []
    rsp_cw_link_data: list[list[Reg]] = []
    rsp_cc_link_valid: list[Reg] = []
    rsp_cc_link_meta: list[Reg] = []
    rsp_cc_link_data: list[list[Reg]] = []

    with m.scope("req_ring"):
        for i in range(NODE_COUNT):
            req_cw_link_valid.append(
                m.out(f"cw_v{i}", clk=clk, rst=rst, width=1, init=0, en=1)
            )
            req_cw_link_meta.append(
                m.out(f"cw_m{i}", clk=clk, rst=rst, width=64, init=0, en=1)
            )
            req_cw_link_data.append(
                [
                    m.out(f"cw_d{i}_w{wi}", clk=clk, rst=rst, width=64, init=0, en=1)
                    for wi in range(line_words)
                ]
            )
            req_cc_link_valid.append(
                m.out(f"cc_v{i}", clk=clk, rst=rst, width=1, init=0, en=1)
            )
            req_cc_link_meta.append(
                m.out(f"cc_m{i}", clk=clk, rst=rst, width=64, init=0, en=1)
            )
            req_cc_link_data.append(
                [
                    m.out(f"cc_d{i}_w{wi}", clk=clk, rst=rst, width=64, init=0, en=1)
                    for wi in range(line_words)
                ]
            )

    with m.scope("rsp_ring"):
        for i in range(NODE_COUNT):
            rsp_cw_link_valid.append(
                m.out(f"cw_v{i}", clk=clk, rst=rst, width=1, init=0, en=1)
            )
            rsp_cw_link_meta.append(
                m.out(f"cw_m{i}", clk=clk, rst=rst, width=64, init=0, en=1)
            )
            rsp_cw_link_data.append(
                [
                    m.out(f"cw_d{i}_w{wi}", clk=clk, rst=rst, width=64, init=0, en=1)
                    for wi in range(line_words)
                ]
            )
            rsp_cc_link_valid.append(
                m.out(f"cc_v{i}", clk=clk, rst=rst, width=1, init=0, en=1)
            )
            rsp_cc_link_meta.append(
                m.out(f"cc_m{i}", clk=clk, rst=rst, width=64, init=0, en=1)
            )
            rsp_cc_link_data.append(
                [
                    m.out(f"cc_d{i}_w{wi}", clk=clk, rst=rst, width=64, init=0, en=1)
                    for wi in range(line_words)
                ]
            )

    # --- Pipe request wires ---
    pipe_req_valid: list[Wire] = [c(0, width=1) for _ in range(NODE_COUNT)]
    pipe_req_meta: list[Wire] = [c(0, width=64) for _ in range(NODE_COUNT)]
    pipe_req_data: list[list[Wire]] = [
        [c(0, width=64) for _ in range(line_words)] for _ in range(NODE_COUNT)
    ]

    # --- Request ring traversal + ejection to pipes ---
    for pos in range(NODE_COUNT):
        nid = RING_ORDER[pos]
        node_const = c(nid, width=node_bits)

        prev_pos = (pos - 1) % NODE_COUNT
        next_pos = (pos + 1) % NODE_COUNT

        cw_in_valid = req_cw_link_valid[prev_pos].out()
        cw_in_meta = req_cw_link_meta[prev_pos].out()
        cw_in_data = [r.out() for r in req_cw_link_data[prev_pos]]

        cc_in_valid = req_cc_link_valid[next_pos].out()
        cc_in_meta = req_cc_link_meta[next_pos].out()
        cc_in_data = [r.out() for r in req_cc_link_data[next_pos]]

        cw_in_dst = _field(m, cw_in_meta, lsb=req_dst_lsb, width=node_bits)
        cc_in_dst = _field(m, cc_in_meta, lsb=req_dst_lsb, width=node_bits)

        ring_cw_local = cw_in_valid & (cw_in_dst == node_const)
        ring_cc_local = cc_in_valid & (cc_in_dst == node_const)

        spb_cw_head_meta = spb_cw[nid].out_meta
        spb_cc_head_meta = spb_cc[nid].out_meta
        spb_cw_head_data = spb_cw[nid].out_data
        spb_cc_head_data = spb_cc[nid].out_data

        spb_cw_dst = _field(m, spb_cw_head_meta, lsb=req_dst_lsb, width=node_bits)
        spb_cc_dst = _field(m, spb_cc_head_meta, lsb=req_dst_lsb, width=node_bits)

        spb_cw_local = spb_cw[nid].out_valid & (spb_cw_dst == node_const)
        spb_cc_local = spb_cc[nid].out_valid & (spb_cc_dst == node_const)

        sel_ring_cw = ring_cw_local
        sel_ring_cc = (~sel_ring_cw) & ring_cc_local
        sel_spb_cw = (~sel_ring_cw) & (~sel_ring_cc) & spb_cw_local
        sel_spb_cc = (~sel_ring_cw) & (~sel_ring_cc) & (~sel_spb_cw) & spb_cc_local

        pipe_req_valid[nid] = sel_ring_cw | sel_ring_cc | sel_spb_cw | sel_spb_cc
        pipe_req_meta[nid] = mux(
            sel_ring_cw,
            cw_in_meta,
            mux(
                sel_ring_cc,
                cc_in_meta,
                mux(sel_spb_cw, spb_cw_head_meta, spb_cc_head_meta),
            ),
        )
        pipe_req_data[nid] = _select4_words(
            m,
            sel_ring_cw,
            sel_ring_cc,
            sel_spb_cw,
            sel_spb_cc,
            cw_in_data,
            cc_in_data,
            spb_cw_head_data,
            spb_cc_head_data,
        )

        cw_forward_valid = cw_in_valid & (~sel_ring_cw)
        cw_can_inject = ~cw_forward_valid
        cw_inject_valid = spb_cw[nid].out_valid & (~spb_cw_local) & cw_can_inject
        cw_out_valid = cw_forward_valid | cw_inject_valid
        cw_out_meta = mux(cw_forward_valid, cw_in_meta, spb_cw_head_meta)
        cw_out_data = _select_words(m, cw_forward_valid, cw_in_data, spb_cw_head_data)

        cc_forward_valid = cc_in_valid & (~sel_ring_cc)
        cc_can_inject = ~cc_forward_valid
        cc_inject_valid = spb_cc[nid].out_valid & (~spb_cc_local) & cc_can_inject
        cc_out_valid = cc_forward_valid | cc_inject_valid
        cc_out_meta = mux(cc_forward_valid, cc_in_meta, spb_cc_head_meta)
        cc_out_data = _select_words(m, cc_forward_valid, cc_in_data, spb_cc_head_data)

        req_cw_link_valid[pos].set(cw_out_valid)
        req_cw_link_meta[pos].set(cw_out_meta)
        for wi in range(line_words):
            req_cw_link_data[pos][wi].set(cw_out_data[wi])

        req_cc_link_valid[pos].set(cc_out_valid)
        req_cc_link_meta[pos].set(cc_out_meta)
        for wi in range(line_words):
            req_cc_link_data[pos][wi].set(cc_out_data[wi])

        m.assign(spb_cw_out_ready[nid], sel_spb_cw | cw_inject_valid)
        m.assign(spb_cc_out_ready[nid], sel_spb_cc | cc_inject_valid)

    # --- Pipe stage regs ---
    pipe_stage_valid: list[Reg] = []
    pipe_stage_meta: list[Reg] = []
    pipe_stage_data: list[list[Reg]] = []

    for p in range(pipe_count):
        with m.scope(f"pipe{p}_stage"):
            pipe_stage_valid.append(m.out("v", clk=clk, rst=rst, width=1, init=0, en=1))
            pipe_stage_meta.append(m.out("m", clk=clk, rst=rst, width=64, init=0, en=1))
            pipe_stage_data.append(
                [
                    m.out(f"d_w{wi}", clk=clk, rst=rst, width=64, init=0, en=1)
                    for wi in range(line_words)
                ]
            )

        pipe_stage_valid[p].set(pipe_req_valid[p])
        pipe_stage_meta[p].set(pipe_req_meta[p])
        for wi in range(line_words):
            pipe_stage_data[p][wi].set(pipe_req_data[p][wi])

    # --- Response inject bundles (per pipe, cw/cc) ---
    rsp_cw: list[BundleFifo] = []
    rsp_cc: list[BundleFifo] = []
    rsp_cw_out_ready: list[Wire] = []
    rsp_cc_out_ready: list[Wire] = []

    for p in range(pipe_count):
        st_valid = pipe_stage_valid[p].out()
        st_meta = pipe_stage_meta[p].out()
        st_data_words = [r.out() for r in pipe_stage_data[p]]

        st_write = _field(m, st_meta, lsb=req_write_lsb, width=1)
        st_src = _field(m, st_meta, lsb=req_src_lsb, width=node_bits)
        st_tag = _field(m, st_meta, lsb=req_tag_lsb, width=tag_bits)
        st_addr = _field(m, st_meta, lsb=req_addr_lsb, width=addr_bits)

        line_idx = st_addr.slice(lsb=offset_bits + pipe_bits, width=index_bits)
        byte_addr = cat(line_idx, c(0, width=3))
        depth_bytes = lines_per_pipe * 8

        read_words: list[Wire] = []
        wvalid = st_valid & st_write
        wstrb = c(0xFF, width=8)

        for wi in range(line_words):
            rdata = m.byte_mem(
                clk=clk,
                rst=rst,
                raddr=byte_addr,
                wvalid=wvalid,
                waddr=byte_addr,
                wdata=st_data_words[wi],
                wstrb=wstrb,
                depth=depth_bytes,
                name=f"tmu_p{p}_w{wi}",
            )
            read_words.append(rdata)

        rsp_meta = _pack_rsp_meta(m, st_write, c(p, width=node_bits), st_src, st_tag)
        rsp_words = [
            mux(st_write, st_data_words[wi], read_words[wi]) for wi in range(line_words)
        ]
        packed_rsp_words = rsp_words[0]
        for wi in range(1, line_words):
            packed_rsp_words = cat(rsp_words[wi], packed_rsp_words)

        rsp_dir = _dir_cw(m, src=p, dst=st_src)
        in_valid_cw = st_valid & rsp_dir
        in_valid_cc = st_valid & (~rsp_dir)

        cw_ready = m.named_wire(f"rsp{p}_cw_out_ready", width=1)
        cc_ready = m.named_wire(f"rsp{p}_cc_out_ready", width=1)
        rsp_cw_out_ready.append(cw_ready)
        rsp_cc_out_ready.append(cc_ready)

        cw_fifo = _build_bundle_fifo(
            m,
            clk=clk,
            rst=rst,
            in_valid=in_valid_cw,
            in_meta=rsp_meta,
            in_data=packed_rsp_words,
            out_ready=cw_ready,
            depth=spb_depth,
            line_words=line_words,
        )
        cc_fifo = _build_bundle_fifo(
            m,
            clk=clk,
            rst=rst,
            in_valid=in_valid_cc,
            in_meta=rsp_meta,
            in_data=packed_rsp_words,
            out_ready=cc_ready,
            depth=spb_depth,
            line_words=line_words,
        )
        rsp_cw.append(
            BundleFifo(
                in_ready=cw_fifo["in_ready"],
                out_valid=cw_fifo["out_valid"],
                out_meta=cw_fifo["out_meta"],
                out_data=[
                    cw_fifo["out_data"][wi * 64 : (wi + 1) * 64]
                    for wi in range(line_words)
                ],
            )
        )
        rsp_cc.append(
            BundleFifo(
                in_ready=cc_fifo["in_ready"],
                out_valid=cc_fifo["out_valid"],
                out_meta=cc_fifo["out_meta"],
                out_data=[
                    cc_fifo["out_data"][wi * 64 : (wi + 1) * 64]
                    for wi in range(line_words)
                ],
            )
        )

    # --- Response ring traversal + MGB buffers ---
    for pos in range(NODE_COUNT):
        nid = RING_ORDER[pos]
        node_const = c(nid, width=node_bits)

        prev_pos = (pos - 1) % NODE_COUNT
        next_pos = (pos + 1) % NODE_COUNT

        cw_in_valid = rsp_cw_link_valid[prev_pos].out()
        cw_in_meta = rsp_cw_link_meta[prev_pos].out()
        cw_in_data = [r.out() for r in rsp_cw_link_data[prev_pos]]

        cc_in_valid = rsp_cc_link_valid[next_pos].out()
        cc_in_meta = rsp_cc_link_meta[next_pos].out()
        cc_in_data = [r.out() for r in rsp_cc_link_data[next_pos]]

        cw_in_dst = _field(m, cw_in_meta, lsb=rsp_dst_lsb, width=node_bits)
        cc_in_dst = _field(m, cc_in_meta, lsb=rsp_dst_lsb, width=node_bits)

        ring_cw_local = cw_in_valid & (cw_in_dst == node_const)
        ring_cc_local = cc_in_valid & (cc_in_dst == node_const)

        rsp_cw_head_meta = rsp_cw[nid].out_meta
        rsp_cc_head_meta = rsp_cc[nid].out_meta
        rsp_cw_head_data = rsp_cw[nid].out_data
        rsp_cc_head_data = rsp_cc[nid].out_data

        rsp_cw_dst = _field(m, rsp_cw_head_meta, lsb=rsp_dst_lsb, width=node_bits)
        rsp_cc_dst = _field(m, rsp_cc_head_meta, lsb=rsp_dst_lsb, width=node_bits)

        rsp_cw_local = rsp_cw[nid].out_valid & (rsp_cw_dst == node_const)
        rsp_cc_local = rsp_cc[nid].out_valid & (rsp_cc_dst == node_const)

        cw_local_valid = ring_cw_local | rsp_cw_local
        cc_local_valid = ring_cc_local | rsp_cc_local
        cw_local_meta = mux(ring_cw_local, cw_in_meta, rsp_cw_head_meta)
        cc_local_meta = mux(ring_cc_local, cc_in_meta, rsp_cc_head_meta)
        cw_local_data = _select_words(m, ring_cw_local, cw_in_data, rsp_cw_head_data)
        cc_local_data = _select_words(m, ring_cc_local, cc_in_data, rsp_cc_head_data)

        # MGB buffers.
        mgb_cw_ready = m.named_wire(f"mgb{nid}_cw_out_ready", width=1)
        mgb_cc_ready = m.named_wire(f"mgb{nid}_cc_out_ready", width=1)

        packed_cw_local_data = cw_local_data[0]
        packed_cc_local_data = cc_local_data[0]
        for wi in range(1, line_words):
            packed_cw_local_data = cat(cw_local_data[wi], packed_cw_local_data)
            packed_cc_local_data = cat(cc_local_data[wi], packed_cc_local_data)

        mgb_cw_inst = _build_bundle_fifo(
            m,
            clk=clk,
            rst=rst,
            in_valid=cw_local_valid,
            in_meta=cw_local_meta,
            in_data=packed_cw_local_data,
            out_ready=mgb_cw_ready,
            depth=mgb_depth,
            line_words=line_words,
        )
        mgb_cc_inst = _build_bundle_fifo(
            m,
            clk=clk,
            rst=rst,
            in_valid=cc_local_valid,
            in_meta=cc_local_meta,
            in_data=packed_cc_local_data,
            out_ready=mgb_cc_ready,
            depth=mgb_depth,
            line_words=line_words,
        )
        mgb_cw = BundleFifo(
            in_ready=mgb_cw_inst["in_ready"],
            out_valid=mgb_cw_inst["out_valid"],
            out_meta=mgb_cw_inst["out_meta"],
            out_data=[
                mgb_cw_inst["out_data"][wi * 64 : (wi + 1) * 64]
                for wi in range(line_words)
            ],
        )
        mgb_cc = BundleFifo(
            in_ready=mgb_cc_inst["in_ready"],
            out_valid=mgb_cc_inst["out_valid"],
            out_meta=mgb_cc_inst["out_meta"],
            out_data=[
                mgb_cc_inst["out_data"][wi * 64 : (wi + 1) * 64]
                for wi in range(line_words)
            ],
        )

        rr = m.out(f"mgb{nid}_rr", clk=clk, rst=rst, width=1, init=0, en=1)

        any_cw = mgb_cw.out_valid
        any_cc = mgb_cc.out_valid
        both = any_cw & any_cc
        pick_cw = (any_cw & (~any_cc)) | (both & (~rr.out()))
        pick_cc = (any_cc & (~any_cw)) | (both & rr.out())

        resp_ready = nodes[nid].resp_ready
        resp_fire = (pick_cw | pick_cc) & resp_ready

        m.assign(mgb_cw_ready, pick_cw & resp_ready)
        m.assign(mgb_cc_ready, pick_cc & resp_ready)

        rr_next = rr.out()
        rr_next = mux(resp_fire, ~rr_next, rr_next)
        rr.set(rr_next)

        resp_meta = mux(pick_cw, mgb_cw.out_meta, mgb_cc.out_meta)
        resp_words = _select_words(m, pick_cw, mgb_cw.out_data, mgb_cc.out_data)

        m.assign(nodes[nid].resp_valid, resp_fire)
        m.assign(
            nodes[nid].resp_tag, _field(m, resp_meta, lsb=rsp_tag_lsb, width=tag_bits)
        )
        m.assign(
            nodes[nid].resp_is_write, _field(m, resp_meta, lsb=rsp_write_lsb, width=1)
        )
        for wi in range(line_words):
            m.assign(nodes[nid].resp_data_words[wi], resp_words[wi])

        # Forward or inject on response cw lane.
        cw_forward_valid = cw_in_valid & (~ring_cw_local)
        cc_forward_valid = cc_in_valid & (~ring_cc_local)

        cw_can_inject = ~cw_forward_valid
        cc_can_inject = ~cc_forward_valid

        cw_inject_valid = rsp_cw[nid].out_valid & (~rsp_cw_local) & cw_can_inject
        cc_inject_valid = rsp_cc[nid].out_valid & (~rsp_cc_local) & cc_can_inject

        cw_out_valid = cw_forward_valid | cw_inject_valid
        cc_out_valid = cc_forward_valid | cc_inject_valid

        cw_out_meta = mux(cw_forward_valid, cw_in_meta, rsp_cw_head_meta)
        cc_out_meta = mux(cc_forward_valid, cc_in_meta, rsp_cc_head_meta)
        cw_out_data = _select_words(m, cw_forward_valid, cw_in_data, rsp_cw_head_data)
        cc_out_data = _select_words(m, cc_forward_valid, cc_in_data, rsp_cc_head_data)

        rsp_cw_link_valid[pos].set(cw_out_valid)
        rsp_cw_link_meta[pos].set(cw_out_meta)
        for wi in range(line_words):
            rsp_cw_link_data[pos][wi].set(cw_out_data[wi])

        rsp_cc_link_valid[pos].set(cc_out_valid)
        rsp_cc_link_meta[pos].set(cc_out_meta)
        for wi in range(line_words):
            rsp_cc_link_data[pos][wi].set(cc_out_data[wi])

        rsp_cw_local_pop = rsp_cw_local & (~ring_cw_local) & mgb_cw.in_ready
        rsp_cc_local_pop = rsp_cc_local & (~ring_cc_local) & mgb_cc.in_ready
        m.assign(rsp_cw_out_ready[nid], rsp_cw_local_pop | cw_inject_valid)
        m.assign(rsp_cc_out_ready[nid], rsp_cc_local_pop | cc_inject_valid)

    # --- Debug ring metadata outputs (for visualization) ---
    for pos in range(NODE_COUNT):
        nid = RING_ORDER[pos]
        req_meta = (
            req_cw_link_meta[pos].out().slice(lsb=0, width=req_addr_lsb + addr_bits)
        )
        req_meta_cc = (
            req_cc_link_meta[pos].out().slice(lsb=0, width=req_addr_lsb + addr_bits)
        )
        rsp_meta = (
            rsp_cw_link_meta[pos].out().slice(lsb=0, width=rsp_tag_lsb + tag_bits)
        )
        rsp_meta_cc = (
            rsp_cc_link_meta[pos].out().slice(lsb=0, width=rsp_tag_lsb + tag_bits)
        )
        m.output(f"dbg_req_cw_v{nid}", req_cw_link_valid[pos].out())
        m.output(f"dbg_req_cc_v{nid}", req_cc_link_valid[pos].out())
        m.output(f"dbg_req_cw_meta{nid}", req_meta)
        m.output(f"dbg_req_cc_meta{nid}", req_meta_cc)
        m.output(f"dbg_rsp_cw_v{nid}", rsp_cw_link_valid[pos].out())
        m.output(f"dbg_rsp_cc_v{nid}", rsp_cc_link_valid[pos].out())
        m.output(f"dbg_rsp_cw_meta{nid}", rsp_meta)
        m.output(f"dbg_rsp_cc_meta{nid}", rsp_meta_cc)

    for i, node in enumerate(nodes):
        m.output(f"n{i}_req_ready", node.req_ready)
        m.output(f"n{i}_resp_valid", node.resp_valid)
        m.output(f"n{i}_resp_tag", node.resp_tag)
        for wi in range(line_words):
            m.output(f"n{i}_resp_data_w{wi}", node.resp_data_words[wi])
        m.output(f"n{i}_resp_is_write", node.resp_is_write)


build.__pycircuit_name__ = "janus_tmu_pyc"
