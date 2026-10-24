"""Nominal idle authentication with no zero-valued Enum member."""


def source(shape=1, profile="enum", repeated=False):
    assert shape in (0, 1)
    assert profile == "enum" and not repeated
    receiver = '".."' if shape == 0 else '"..", "store"'
    nested_worker = "    worker = Worker()\n" if shape == 0 else ""
    top_worker = "    worker = Worker()\n" if shape == 1 else ""
    return '''from enum import Enum
import pycircuit as ac

@ac.encoding(width=8)
class Mark(Enum):
    FIRST = 5
    SECOND = 9

@ac.module
def Store():
    memory = ac.byte_mem[Mark](depth=2)

    @ac.method
    def write(value: Mark) -> None:
        memory.write(0, value, mask=1)

    @ac.method
    def read() -> Mark:
        return memory.read(0)

''' + nested_worker + '''
@ac.module
def Worker():
    store = ac.instance_ref(Store, ''' + receiver + ''')

    @ac.rule
    def step() -> None:
        before = store.read()
        store.write(Mark.SECOND)
        after = store.read()
        ac.log("info", "enum-memory", ac.enum_to_bits(before), ac.enum_to_bits(after))

    step()

@ac.system(samples=((0, 0), (1, 0), (0, 0), (1, 0)))
def Top():
    store = Store()
''' + top_worker
