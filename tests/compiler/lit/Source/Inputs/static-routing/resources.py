"""A reference targets an ordinary Module; its Queue and Memory calls are direct."""

# ruff: noqa: F823, F841 -- captured phase state and hardware allocations.
import pycircuit as ac


@ac.module
def Store():
    tokens = ac.queue[ac.u8](depth=2, latency=1)
    memory = ac.byte_mem[ac.u16](depth=4)

    @ac.method
    def fill(value: ac.u8) -> None:
        tokens.push(value)
        memory.write(1, 0x1234, mask=1)

    @ac.method
    def consume() -> tuple[ac.u8, ac.u16]:
        token = tokens.pop()
        previous = memory.read(1)
        memory.write(1, 0xAB00, mask=2)
        return token, previous

    @ac.method
    def read() -> ac.u16:
        return memory.read(1)


@ac.module
def Worker():
    store = ac.instance_ref(Store, "..", "..", "remote", "store")
    phase: ac.u2 = 0

    @ac.rule
    def step() -> None:
        if phase == 0:
            store.fill(13)
            ac.log("info", "resource-fill", 13)
        elif phase == 1:
            token, previous = store.consume()
            assert token == 13, "routed Queue committed head"
            assert previous == 0x34, "routed Memory low-byte write"
            ac.log("info", "resource-consume", token, previous)
        else:
            current = store.read()
            assert current == 0xAB34, "routed Memory high-byte write"
            ac.log("info", "resource-read", current)
        phase = phase + 1

    step()


@ac.module
def Remote():
    store = Store()


@ac.module
def Branch():
    worker = Worker()


@ac.system(samples=((0, 0), (1, 0), (1, 0), (0, 0), (1, 1), (0, 0), (1, 0),
                    (0, 0), (1, 0), (0, 0), (1, 0)))
def Top():
    remote = Remote()
    branch = Branch()
