"""A filtered transfer: Queue + Cell + Signal outputs, with one outer transaction."""
from pycircuit import ac


@ac.module
def Ledger():
    observed = ac.cell[ac.u32](initial=0)

    @ac.method
    def record(value: ac.u32):
        observed.value = value

    return observed


@ac.module
def Observer(value: ac.signal[ac.u32]):
    last = ac.cell[ac.u32](initial=0)

    @ac.rule
    def sample(x: ac.signal[ac.u32]):
        last.value = x

    # Observe the current saved result, including when route does not commit.
    sample(value)
    return last


@ac.module
def Router(source: ac.queue[ac.u32], ledger, enabled: bool):
    accepted = ac.queue[ac.u32](capacity=2)
    last_accepted = ac.cell[ac.u32](initial=0)
    result = ac.signal[ac.u32](initial=0)

    @ac.rule
    def route(message: ac.queue[ac.u32], take: bool) -> tuple[
        ac.queue[ac.u32], ac.cell[ac.u32], ac.signal[ac.u32]
    ]:
        value = message.value
        ledger.record(value)  # This method's write belongs to route.
        if take:
            return value + 1, value, value
        # Consume and record the message, but omit Queue/Cell output effects.
        return None, None, value

    accepted, last_accepted, result = route(source, enabled)
    return accepted, last_accepted, result


@ac.module
def MixedPorts(enabled: bool):
    source = ac.queue[ac.u32](initial=7)
    ledger = Ledger()
    router = Router(source, ledger, enabled)
    observer = Observer(router.outputs[2])
    return router.outputs[0], router.outputs[1], observer.outputs
