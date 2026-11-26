#!/usr/bin/env python3
"""Quantify the flit header, the price of a stateless NoC.

Calendar §1.8 and whitepaper §5.4: because the NoC keeps no routing state, every
flit replicates the full ``{routeBits, opcode, redOp, epochTag}`` header. This
script prints that cost and shows what open item ``Q1`` does to it — the
difference between the Calendar baseline's 40 nodes and the hardware chapter's
48.

Run:

    python examples/header_cost.py
"""

from __future__ import annotations

import json

from wse_model.analysis import BANDWIDTHS, flit_header_cost, local_dram_vs_noc_ratio
from wse_model.topology import PROFILES


def main() -> int:
    print("== link bandwidths (whitepaper §4.4) ==")
    for name, bandwidth in sorted(BANDWIDTHS.items()):
        print(f"  {name:12s} {bandwidth.gb_per_s:8.1f} GB/s   {bandwidth.source}")
    print(
        f"\nper-core local DRAM is {local_dram_vs_noc_ratio():.2f}x one NoC link: "
        "reading weights on core is cheap, moving them between cores is not."
    )

    print("\n== flit header cost per topology reading ==")
    for name in sorted(PROFILES):
        topology = PROFILES[name].topology
        header = flit_header_cost(node_count=topology.node_count)
        print(
            f"  {name:20s} nodes={topology.node_count:2d} "
            f"routeBits={header.route_bits_bytes:2d} B "
            f"header={header.header_bytes:2d} B "
            f"payload/flit={header.payload_bytes:2d} B "
            f"overhead={100.0 * header.overhead_fraction:5.2f}%"
        )

    print("\n== open item Q1 ==")
    forty = flit_header_cost(node_count=40)
    forty_eight = flit_header_cost(node_count=48)
    print(
        f"  Q1 asks whether a die has 40 or 48 NoC nodes. The header changes from "
        f"{forty.header_bytes} B ({100.0 * forty.overhead_fraction:.2f}%) to "
        f"{forty_eight.header_bytes} B ({100.0 * forty_eight.overhead_fraction:.2f}%), "
        "and a 48-node entry would need 18 B and no longer fit the 16 B one-GPR-pair "
        "layout of Calendar §2.6."
    )
    print(
        "\n"
        + json.dumps(
            {
                "calendar-40": flit_header_cost(node_count=40).describe(),
                "whitepaper-48": flit_header_cost(node_count=48).describe(),
            },
            indent=2,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
