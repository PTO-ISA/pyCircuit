"""Late source failure discards routed and root-local RAM proposals atomically."""

# ruff: noqa: F823, F841 -- compiler-owned hardware state and allocations.
import pycircuit as ac


@ac.module
def Store():
    memory = ac.byte_mem[ac.u16](depth=4)

    @ac.method
    def read() -> ac.u16:
        return memory.read(1)

    @ac.method
    def write(value: ac.u16) -> None:
        memory.write(1, value)


@ac.module
def Worker():
    store = ac.instance_ref(Store, "..", "..", "remote", "store")
    progress: ac.u8 = 0
    held: ac.u8 = ac.four_state("10xz0011")

    @ac.rule
    def step() -> None:
        previous = store.read()
        store.write(previous + 3)
        progress = progress + 1
        assert progress == 1, "memory_routed_late_failure"
        ac.log("info", "memory-accepted", previous, progress, held)

    step()


@ac.module
def Remote():
    store = Store()


@ac.module
def Branch():
    worker = Worker()


@ac.system(samples=((0, 0), (1, 0), (0, 0)))
def Top():
    remote = Remote()
    branch = Branch()
    other_memory = ac.byte_mem[ac.u16](depth=4)

    @ac.rule
    def advance() -> None:
        previous = other_memory.read(0)
        other_memory.write(0, previous + 5)

    advance()
