import pycircuit as ac


@ac.module
def RuleQueue():
    tokens = ac.queue[ac.u8](depth=1, latency=1)

    @ac.rule
    def send(value: ac.u8) -> None:
        tokens.push(value)

    @ac.rule
    def replace(value: ac.u8) -> ac.u8:
        old = tokens.pop()
        tokens.push(value)
        return old

    @ac.rule
    def take() -> ac.u8:
        return tokens.pop()


@ac.module
def AtomicQueuePair():
    left = ac.queue[ac.u8](depth=1, latency=1)
    right = ac.queue[ac.u8](depth=1, latency=1)
    unused = ac.queue[ac.u8](depth=1, latency=1)

    @ac.rule
    def seed_left() -> None:
        left.push(11)

    @ac.rule
    def seed_right() -> None:
        right.push(7)

    @ac.rule
    def exchange() -> ac.u8:
        old = left.pop()
        extra = right.pop()
        left.push(old + extra)
        return old

    @ac.rule(guard=lambda enabled: enabled)
    def require(enabled: ac.u1) -> None:
        pass

    @ac.rule
    def denied() -> ac.u8:
        old = left.pop()
        require(0)
        return old

    @ac.rule
    def pick(select: ac.u1) -> ac.u8:
        if select:
            value = left.pop()
        else:
            value = unused.pop()
        return value
