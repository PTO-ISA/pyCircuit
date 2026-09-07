import agentic_circuit as ac

from contracts import ReadRequest, ReadResult, WriteAck, WriteRequest
from leaf import typed_state_leaf


@ac.system
def typed_system(
    read_request: ReadRequest,
    write_request: WriteRequest,
    *,
    owner_generation: ac.const[int],
) -> tuple[ReadResult, WriteAck]:
    return typed_state_leaf(
        read_request,
        write_request,
        owner_generation=owner_generation,
    )
