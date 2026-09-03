"""DMA-style DRAM-to-SRAM copy built from explicit memory endpoints.

The memories are owned by the root scope.  The ``dma`` scope connects a
request Queue to a DRAM read endpoint and feeds that response Queue directly
into an SRAM write endpoint:

    requests -> DRAM read -> dram_responses -> SRAM write -> copy_responses

Each DMA token carries both addresses.  The DRAM response replaces ``data``
with the value read from DRAM; the following SRAM endpoint uses that value as
its write data.  The SRAM response contains SRAM old-data, as required by the
memory contract.
"""

import agentic_circuit as ac


@ac.struct
class DmaRequest:
    dram_address: ac.u4
    sram_address: ac.u4
    data: ac.u16
    tag: ac.u8


@ac.system
def dma() -> None:
    dram = ac.memory(ac.u16, entries=16, init=0, latency=3)
    sram = ac.memory(ac.u16, entries=16, init=0, latency=2)

    # Test/host endpoint used by the harness to put non-zero data in DRAM.
    with ac.scope("host"):
        dram_seed = ac.source(DmaRequest, depth=2, latency=1)
        dram_seed_responses = dram.request(
            dram_seed,
            address=lambda request: request.dram_address,
            write=lambda request: True,
            data=lambda request: request.data,
            result_field="data",
            depth=2,
        )
        ac.sink(dram_seed_responses)

    with ac.scope("dma"):
        requests = ac.source(DmaRequest, depth=4, latency=1)
        dram_responses = dram.request(
            requests,
            address=lambda request: request.dram_address,
            write=lambda request: False,
            data=lambda request: request.data,
            result_field="data",
            depth=2,
        )
        copy_responses = sram.request(
            dram_responses,
            address=lambda response: response.sram_address,
            write=lambda response: True,
            data=lambda response: response.data,
            result_field="data",
            depth=2,
        )
        ac.sink(copy_responses)

    # Independent read endpoint used to verify the copied SRAM value.
    with ac.scope("check"):
        sram_reads = ac.source(DmaRequest, depth=2, latency=1)
        sram_read_responses = sram.request(
            sram_reads,
            address=lambda request: request.sram_address,
            write=lambda request: False,
            data=lambda request: request.data,
            result_field="data",
            depth=2,
        )
        ac.sink(sram_read_responses)
