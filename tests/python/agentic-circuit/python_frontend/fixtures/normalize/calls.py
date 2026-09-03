from agentic_circuit import Flow, module, system


class ReadyValid:
    pass


@module
def pipeline(
    request: Flow[int, ReadyValid],
) -> tuple[Flow[int, ReadyValid], Flow[int, ReadyValid]]:
    decoded, accepted = Decode(request, mode="strict")
    decoded = Refine(decoded)
    return decoded, accepted


@system
def main() -> None:
    return None
