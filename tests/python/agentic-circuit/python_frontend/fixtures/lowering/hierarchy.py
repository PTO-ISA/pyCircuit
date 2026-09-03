from __future__ import annotations

from agentic_circuit import Flow, module, system


class ReadyValid:
    pass


@module
def pipeline(request: Flow[int, ReadyValid]) -> Flow[int, ReadyValid]:
    refined = Refine(request)
    return refined


@system(root="pipeline")
def main() -> None:
    return None
