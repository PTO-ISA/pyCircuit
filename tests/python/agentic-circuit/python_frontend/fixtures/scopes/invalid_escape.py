from agentic_circuit import ResourceRef, module, scope, system


class PrivateState:
    pass


class Owner:
    pass


@module
def pipeline() -> ResourceRef[PrivateState, Owner]:
    with scope("private"):
        private = MakePrivate()
    return private


@system
def main() -> None:
    return None
