from agentic_circuit import module, system


@module
def Worker(request: int) -> int:
    return request


@system
def Architecture(lanes: int = 2) -> int:
    return Worker(lanes)
