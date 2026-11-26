#!/usr/bin/env python3
"""Walk the Batcher's dispatch chains and a load -> kickstart -> launch sequence.

Whitepaper §6, §12 and §14: the Batcher is the only external entry point, there
are three load-time chains with three different destinations, and a steady-state
launch dispatches activations and a descriptor and nothing else. This script also
shows the shared ``Batcher.mem`` absorbing most of the die-wide cache-miss
traffic, and the scheduling constraints that are correctness rather than taste.

Run:

    python examples/dispatch_and_refill.py
"""

from __future__ import annotations

import json

from wse_model.host import (
    BatcherMem,
    DispatchChain,
    LaunchConstraints,
    Loader,
    VersionSet,
    dispatch_chains,
    dispatch_paths,
)
from wse_model.host.ub_bus import local_dram_over_fabric_ratio
from wse_model.noc.calreg import CalRegImage, CalRegSlot


def main() -> int:
    print("== the UB bus (whitepaper §6.3) ==")
    from wse_model.host import UbBus

    print(json.dumps(UbBus().describe(), indent=2))
    print(
        f"  per-core local DRAM is {local_dram_over_fabric_ratio():.2f}x the fabric, "
        "so weights must reside and only activations cross"
    )

    print("\n== the four Batcher responsibilities ==")
    for path in dispatch_paths():
        print(
            f"  {path.responsibility.direction:3s} {path.frequency.value:22s} "
            f"{path.responsibility.value}"
        )

    print("\n== the three load-time chains ==")
    for chain, payload in dispatch_chains().items():
        print(
            f"  {chain.value:28s} via_batcher={str(payload['via_batcher']):5s} "
            f"-> {payload['destination']}"
        )

    print("\n== shared Batcher.mem absorbs most of the miss traffic ==")
    mem = BatcherMem()
    for label, account in (
        ("I$ 2 KB blocks", mem.instruction_refill(distinct_blocks=3, requesters_per_line=40)),
        ("D$ 64 B lines ", mem.data_refill(distinct_lines=20, requesters_per_line=4)),
    ):
        described = account.describe()
        print(
            f"  {label}: {described['requests']:5d} requests, "
            f"{described['mem_served_requests']:5d} served by Batcher.mem, "
            f"{described['ddr_backed_requests']:5d} reach DDR"
        )

    print("\n== a load -> kickstart -> launch sequence ==")
    loader = Loader()
    loader.install_kernel(rodata_bytes=1280)
    loader.install_weights(shard_bytes=2048)
    loader.install_calreg(CalRegImage(slots=(CalRegSlot(opcode=1, content=b"\x01"),)))
    loader.check_versions(VersionSet(1, 1, 1), VersionSet(1, 1, 1))
    loader.kickstart()
    loader.launch(activation_bytes=4096)
    payload = loader.describe()
    print(json.dumps(payload["state"], indent=2))
    print(
        "  steady state dispatches "
        f"{loader.steady_state_dispatch_bytes(activation_bytes=4096)} B per launch: "
        "activations plus a descriptor, and no Calendar data at all"
    )

    print("\n== scheduling constraints are correctness requirements ==")
    groups = (frozenset(range(8)),)
    try:
        LaunchConstraints.check_wave(
            waves=(frozenset(range(4)), frozenset(range(4, 8))), groups=groups
        )
    except Exception as exc:  # noqa: BLE001 - the point is to show the refusal
        print(f"  splitting a syncing group: refused -> {exc}")
    LaunchConstraints.check_wave(waves=(frozenset(range(8)),), groups=groups)
    print("  a whole group in one wave: accepted")
    print(
        "  blockId from the SPR costs "
        f"{LaunchConstraints.block_id_cost(from_spr=True)['loads']} loads; as an "
        f"argument it costs {LaunchConstraints.block_id_cost(from_spr=False)['loads']}"
    )
    print(
        "  the CalReg chain is the only one that bypasses the Batcher: "
        f"{dispatch_chains()[DispatchChain.CALREG]['destination']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
