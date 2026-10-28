import pycircuit as ac


@ac.module
def Receiver():
    total: ac.u16 = 0

    @ac.method
    def accept(x: ac.u8) -> None:
        if x == 99:
            assert x == 0
            ac.log("info", "unreachable", x)
        elif x == 3:
            contribution = x
            total = total + contribution
        else:
            total = total + x

    @ac.method(guard=lambda: total == 99)
    def blocked(x: ac.u8) -> None:
        total = total + x

    @ac.autorule
    def observe() -> None:
        ac.log("info", "total", total)


@ac.system
def Top():
    receiver: Receiver
    phase: ac.u8 = 0
    accepted: ac.u8 = 0

    @ac.method
    def deliver(value: ac.u8) -> None:
        if phase == 2:
            receiver.blocked(value)
        else:
            receiver.accept(value)

    @ac.rule
    def consume() -> None:
        value: ac.u8 = 3 if phase == 0 else 5
        accepted = accepted + 1
        deliver(value)
        ac.log("info", "consume", value)

    consume()

    @ac.autorule
    def tick() -> None:
        phase = phase + 1
        ac.log("info", "progress", phase, accepted)
