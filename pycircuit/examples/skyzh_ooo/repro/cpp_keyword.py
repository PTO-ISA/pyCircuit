"""A legal Python identifier must not become a reserved C++ identifier."""
from pycircuit import ac


@ac.module
def Probe(unsigned: bool):
    result = ac.queue[bool](initial=False)

    @ac.rule
    def update():
        result.value = unsigned

    @ac.work
    def work():
        update()
