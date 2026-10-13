"""A Module with methods, an autonomous Rule and control exports a saved Signal."""
from pycircuit import ac

@ac.module
class Producer:

    def __init__() -> None:
        offset = ac.cell[ac.u32](initial=0)
        count = ac.cell[ac.u32](initial=0)
        result = ac.signal[ac.u32]()

    @ac.rule
    def configure(value: ac.u32) -> None:
        offset = value

    @ac.rule
    def peek() -> ac.u32:
        return result

    @ac.rule
    def advance() -> None:
        result = count + offset
        count = count + 1

    def __call__(enabled: bool) -> ac.signal[ac.u32]:
        if enabled and count < 16:
            advance()
        return result

@ac.module
class Reader:

    def __init__() -> None:
        last = ac.cell[ac.u32](initial=0)

    @ac.rule
    def observe(value: ac.u32) -> None:
        last = value

    def __call__(source: ac.signal[ac.u32]) -> ac.cell[ac.u32]:
        saved = source
        adjusted = saved + 1
        ready = adjusted != 0
        if ready:
            observe(adjusted)
        return last

@ac.module
class SignalLink:

    def __init__(enabled: bool) -> None:
        producer = Producer()
        reader = Reader()

    @ac.rule
    def configure() -> None:
        producer.configure(4)

    def __call__() -> tuple[ac.signal[ac.u32], ac.cell[ac.u32]]:
        producer(enabled)
        configure()
        reader(producer.outputs)
        return (producer.outputs, reader.outputs)
