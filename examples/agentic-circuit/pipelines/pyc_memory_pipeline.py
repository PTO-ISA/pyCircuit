import agentic_circuit as ac


@ac.struct
class MemoryRequest:
    address: ac.u4
    write: ac.u1
    data: ac.u16
    tag: ac.u8


@ac.system
def pyc_memory_pipeline() -> None:
    sram = ac.memory(ac.u16, entries=16, init=0, latency=3)
    requests = ac.source(MemoryRequest, depth=4, latency=1)
    responses = sram.request(
        requests,
        address=lambda item: item.address,
        write=lambda item: item.write,
        data=lambda item: item.data,
        result_field="data",
        depth=4,
    )
    ac.sink(responses)
