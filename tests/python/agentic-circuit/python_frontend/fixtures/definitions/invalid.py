from agentic_circuit import system


@system
def invalid_defaults(lanes: int = object()) -> None:
    return None


@system
def invalid_variadic(*lanes: int) -> None:
    return None
