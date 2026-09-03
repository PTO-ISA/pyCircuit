from agentic_circuit import module


@module
def Supported(lanes: int, enabled: bool) -> tuple[int, ...]:
    values = tuple(i * 2 for i in range(lanes))
    if enabled:
        return values
    return ()
