import pycircuit as ac


@ac.module
def Writer(counter: ac.module):  # noqa: N802
    @ac.rule
    def write() -> None:
        counter.increment()

    write()


@ac.module
def Reader(counter: ac.module):  # noqa: N802
    @ac.rule
    def observe() -> None:
        value: ac.u8 = counter.read()
        ac.log("info", "count", value)

    observe()
