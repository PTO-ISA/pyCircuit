from agentic_circuit import Flow, module

class ReadyValid: pass

@module
def Unsupported(request: Flow[int, ReadyValid]) -> None:
    if (
      request
    ):
        return None
