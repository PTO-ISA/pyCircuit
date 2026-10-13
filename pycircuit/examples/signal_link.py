"""A controlled producer and reader share a Signal declared by their parent."""
from pycircuit import ac

@ac.module
class Producer:

    def __init__() -> None:
        offset = ac.cell[ac.u32](initial=0)
        count = ac.cell[ac.u32](initial=0)

    @ac.rule
    def configure(value: ac.u32) -> None:
        offset = value

    @ac.rule
    def peek(source: ac.signal[ac.u32]) -> ac.u32:
        return source

    @ac.rule
    def advance() -> ac.signal[ac.u32]:
        value = count + offset
        count = count + 1
        return value

    def __call__(enabled: bool) -> ac.signal[ac.u32]:
        if enabled and count < 16:
            return advance()

@ac.module
class Reader:

    def __init__() -> None:
        last = ac.cell[ac.u32](initial=0)

    @ac.rule
    def observe(value: ac.u32) -> None:
        last = value

    def __call__(source: ac.signal[ac.u32]) -> None:
        saved = source
        adjusted = saved + 1
        ready = adjusted != 0
        if ready:
            observe(adjusted)

@ac.module
class SignalLink:

    def __init__(enabled: bool) -> None:
        result = ac.signal[ac.u32]()
        producer = Producer()
        reader = Reader()

    @ac.rule
    def configure() -> None:
        producer.configure(4)

    def __call__() -> None:
        result = producer(enabled)
        configure()
        reader(result)
