"""A wide finite carrier for bounded nominal-idle authentication tests."""


def source(shape=1, profile="wide", repeated=False):
    assert shape == 1 and profile == "wide" and not repeated
    return '''import pycircuit as ac

@ac.struct
class Packet:
    wide: ac.bits[8192]
    tag: ac.u8

@ac.module
def Store():
    memory = ac.byte_mem[Packet](depth=2)

    @ac.method
    def write(value: Packet) -> None:
        memory.write(0, value)

@ac.module
def Worker():
    store = ac.instance_ref(Store, "..", "store")

    @ac.rule
    def step() -> None:
        store.write(Packet(wide=0, tag=0))

    step()

@ac.system(samples=((0, 0), (1, 0), (0, 0), (1, 0)))
def Top():
    store = Store()
    worker = Worker()
'''
