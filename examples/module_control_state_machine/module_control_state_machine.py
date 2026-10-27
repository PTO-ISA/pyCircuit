# ruff: noqa: F823 -- Module state captured by Rules is compiler-owned.
import pycircuit as ac


@ac.system
def ModuleControlStateMachine():  # noqa: N802
    # Waiting, working, and done use the committed state at the start of a cycle.
    state: ac.bits[2] = 0
    permit: ac.bits[1] = 0
    completed: ac.bits[8] = 0

    @ac.autorule
    def toggle_permit() -> None:
        permit = permit ^ 1

    @ac.rule(guard=lambda: permit == 1)
    def receive() -> None:
        state = 1
        ac.log("info", "receive", state)

    @ac.rule
    def compute() -> None:
        state = 2
        ac.log("info", "compute", state)

    @ac.rule
    def publish() -> None:
        state = 0
        completed = completed + 1
        ac.log("info", "publish", state)

    waiting = state == 0
    working = state == 1
    if waiting:
        receive()
    elif working:
        compute()
    else:
        publish()

    @ac.autorule
    def observe() -> None:
        ac.log("info", "state", state, permit, completed)
