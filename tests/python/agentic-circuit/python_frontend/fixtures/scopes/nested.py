from agentic_circuit import Endpoint, Flow, module, scope, system


class ReadyValid:
    pass


class MemoryPort:
    pass


class Target:
    pass


@module
def pipeline(
    requests: Flow[int, ReadyValid],
    memory: Endpoint[MemoryPort, Target],
) -> Flow[int, ReadyValid]:
    with scope("backend"):
        scheduled = Schedule(requests)
        with scope("memory"):
            stored = Store(scheduled, memory)
    return stored


@system
def main() -> None:
    return None
