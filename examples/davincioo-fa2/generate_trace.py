#!/usr/bin/env python3
"""Generate a tiled FlashAttention-2 PTO JSONL trace.

Shape: B=1, H=1, S=128, D=64, Br=32, Bc=64.
The sequence follows online softmax: QK^T, row-max, exp, row-sum, PV, rescale.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

B = 1
H = 1
S = 128
D = 64
BR = 32
BC = 64
DTYPE = "float16"


def tile(address: int, shape: list[int]) -> dict:
    return {
        "address": hex(address),
        "dtype": DTYPE,
        "shape": shape,
    }


def record(sequence: int, opcode: str, inputs: list[dict],
           outputs: list[dict]) -> dict:
    return {
        "sequence_id": sequence,
        "opcode": opcode,
        "input_tiles": inputs,
        "output_tiles": outputs,
    }


def generate() -> list[dict]:
    n_q = S // BR
    n_kv = S // BC
    records: list[dict] = []
    seq = 0

    def emit(opcode: str, inputs: list[dict], outputs: list[dict]) -> None:
        nonlocal seq
        records.append(record(seq, opcode, inputs, outputs))
        seq += 1

    q_base, k_base, v_base = 0x100000, 0x200000, 0x300000
    s_addr, p_addr, pv_addr = 0x400000, 0x410000, 0x420000
    m_addr, l_addr, o_addr = 0x430000, 0x440000, 0x450000
    local_max, new_max = 0x460000, 0x470000
    alpha_src, alpha, l_scaled = 0x480000, 0x490000, 0x4A0000
    local_sum, inv, p_acc = 0x4B0000, 0x4C0000, 0x4D0000
    stride = 0x10000

    for i in range(n_q):
        q = tile(q_base + i * stride, [BR, D])
        m = tile(m_addr + i * stride, [BR])
        l = tile(l_addr + i * stride, [BR])
        o = tile(o_addr + i * stride, [BR, D])
        emit("TLOAD", [], [q])
        emit("TEXPANDS", [], [m])
        emit("TEXPANDS", [], [l])
        emit("TEXPANDS", [], [o])
        for j in range(n_kv):
            k = tile(k_base + j * stride, [D, BC])
            v = tile(v_base + j * stride, [BC, D])
            scores = tile(s_addr, [BR, BC])
            p = tile(p_addr, [BR, BC])
            p_cube = tile(p_acc, [BR, BC])
            pv = tile(pv_addr, [BR, D])
            row_max = tile(local_max, [BR])
            nxt_max = tile(new_max, [BR])
            diff = tile(alpha_src, [BR])
            scale = tile(alpha, [BR])
            l_old = tile(l_scaled, [BR])
            row_sum = tile(local_sum, [BR])
            emit("TLOAD", [], [k])
            emit("TLOAD", [], [v])
            emit("TMATMUL", [q, k], [scores])
            emit("TCVT", [scores], [scores])
            emit("TMULS", [scores], [scores])
            emit("TCOLMAX", [scores], [row_max])
            emit("TMAX", [m, row_max], [nxt_max])
            emit("TSUB", [m, nxt_max], [diff])
            emit("TEXP", [diff], [scale])
            emit("TMUL", [l, scale], [l_old])
            emit("TCOLEXPANDSUB", [scores, nxt_max], [p])
            emit("TEXP", [p], [p])
            emit("TCOLSUM", [p], [row_sum])
            emit("TADD", [l_old, row_sum], [l])
            emit("TCVT", [p], [p_cube])
            emit("TMOV", [p_cube], [p_cube])
            if j == 0:
                emit("TMATMUL", [p_cube, v], [pv])
                emit("TCVT", [pv], [o])
            else:
                emit("TCOLEXPANDMUL", [o, scale], [o])
                emit("TMATMUL", [p_cube, v], [pv])
                emit("TCVT", [pv], [pv])
                emit("TADD", [o, pv], [o])
            emit("TMOV", [nxt_max], [m])
        recip = tile(inv, [BR])
        emit("TRECIP", [l], [recip])
        emit("TCOLEXPANDMUL", [o, recip], [o])
        emit("TCVT", [o], [o])
        emit("TSTORE", [o], [])
    return records


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    records = generate()
    args.output.write_text("".join(json.dumps(row, separators=(",", ":")) + "\n"
                                   for row in records))
    print(f"wrote {len(records)} records to {args.output}")


if __name__ == "__main__":
    main()
