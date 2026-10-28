import pycircuit as ac


@ac.module
def Observer(shared: ac.module, independent: ac.module):  # noqa: N802
    @ac.autorule
    def observe() -> None:
        shared_count: ac.u8 = shared.read()
        independent_count: ac.u8 = independent.read()
        ac.log("info", "counters", shared_count, independent_count)
