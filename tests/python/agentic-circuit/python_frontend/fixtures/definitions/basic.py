from agentic_circuit import Static, module, system


@module
def worker(lanes: Static[int]) -> None:
    return None


@system
def main(lanes: int = 4) -> None:
    return None
