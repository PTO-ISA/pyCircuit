"""A filtered transfer: Queue + Cell + Signal outputs, with one outer transaction."""
from pycircuit import ac

@ac.module
class Ledger:

    def __init__() -> None:
        observed = ac.cell[ac.u32](initial=0)

    @ac.rule
    def record(value: ac.u32) -> None:
        observed = value

    def __call__() -> ac.cell[ac.u32]:
        return observed

@ac.module
class Observer:

    def __init__() -> None:
        last = ac.cell[ac.u32](initial=0)

    @ac.rule
    def sample(x: ac.signal[ac.u32]) -> None:
        last = x

    def __call__(value: ac.signal[ac.u32]) -> ac.cell[ac.u32]:
        sample(value)
        return last

@ac.module
class Router:

    def __init__() -> None:
        accepted = ac.queue[ac.u32](capacity=2)
        last_accepted = ac.cell[ac.u32](initial=0)
        result = ac.signal[ac.u32]()

    @ac.rule
    def route(message: ac.queue[ac.u32], take: bool) -> tuple[ac.queue[ac.u32], ac.cell[ac.u32], ac.signal[ac.u32]]:
        value = message
        ledger.record(value)
        if take:
            return (value + 1, value, value)
        return (None, None, value)

    def __call__(source: ac.queue[ac.u32], ledger: Ledger, enabled: bool) -> tuple[ac.queue[ac.u32], ac.cell[ac.u32], ac.signal[ac.u32]]:
        accepted, last_accepted, result = route(source, enabled)
        return (accepted, last_accepted, result)

@ac.module
class MixedPorts:

    def __init__(enabled: bool) -> None:
        source = ac.queue[ac.u32](initial=7)
        ledger = Ledger()
        router = Router()
        observer = Observer()

    def __call__() -> tuple[ac.queue[ac.u32], ac.cell[ac.u32], ac.cell[ac.u32]]:
        ledger()
        router(source, ledger, enabled)
        observer(router.outputs[2])
        return (router.outputs[0], router.outputs[1], observer.outputs)
