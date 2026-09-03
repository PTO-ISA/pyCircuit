from agentic_circuit import Flow, module, system


class ReadyValid:
    pass


@module
def pipeline(request: Flow[int, ReadyValid]) -> Flow[int, ReadyValid]:
    decoded = Decode(request)
    return decoded


@system
def main() -> None:
    return None
