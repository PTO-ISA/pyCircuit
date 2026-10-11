"""A Module with methods, an autonomous Rule and control exports a saved Signal."""
from pycircuit import ac


@ac.module
def Producer(enabled: bool):
    offset = ac.cell[ac.u32](initial=0)
    count = ac.cell[ac.u32](initial=0)
    result = ac.signal[ac.u32](initial=0)

    @ac.method
    def configure(value: ac.u32):
        offset.value = value

    @ac.method
    def peek() -> ac.u32:
        return result

    @ac.rule
    def advance():
        result = count.value + offset.value
        count.value = count.value + 1

    if enabled and count.value < 16:
        advance()
    return result


@ac.module
def Reader(source: ac.signal[ac.u32]):
    last = ac.cell[ac.u32](initial=0)
    saved = source  # This local is payload SSA; source keeps its resource identity.
    adjusted = saved + 1
    ready = adjusted != 0

    @ac.rule(guard=ready)
    def observe(value: ac.u32):
        last.value = value

    observe(adjusted)
    return last


@ac.module
def SignalLink(enabled: bool):
    producer = Producer(enabled)

    @ac.rule
    def configure():
        producer.configure(4)

    configure()
    reader = Reader(producer.outputs)  # Pass the initialized resource reference.
    return producer.outputs, reader.outputs
