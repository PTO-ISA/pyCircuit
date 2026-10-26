import pycircuit as ac


@ac.module
def RuleQueue():
    tokens = ac.queue[ac.u8](depth=1, latency=1)

    @ac.method
    def send(value: ac.u8) -> None:
        tokens.push(value)

    @ac.method
    def replace(value: ac.u8) -> ac.u8:
        old = tokens.pop()
        tokens.push(value)
        return old

    @ac.method
    def take() -> ac.u8:
        return tokens.pop()


@ac.module
def AtomicQueuePair():
    left = ac.queue[ac.u8](depth=1, latency=1)
    right = ac.queue[ac.u8](depth=1, latency=1)
    unused = ac.queue[ac.u8](depth=1, latency=1)

    @ac.method
    def seed_left() -> None:
        left.push(11)

    @ac.method
    def seed_right() -> None:
        right.push(7)

    @ac.method
    def exchange() -> ac.u8:
        old = left.pop()
        extra = right.pop()
        left.push(old + extra)
        return old

    @ac.method(guard=lambda enabled: enabled)
    def require(enabled: ac.u1) -> None:
        pass

    @ac.method
    def denied() -> ac.u8:
        old = left.pop()
        require(0)
        return old

    @ac.method
    def pick(select: ac.u1) -> ac.u8:
        if select:
            value = left.pop()
        else:
            value = unused.pop()
        return value
