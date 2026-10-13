"""A fixed shared EHR view used by two independent transactions."""
from pycircuit import ac

@ac.module
class Storage:

    def __init__() -> None:
        state = ac.ehr[ac.u32](initial=1)

    @ac.rule(SB=('read', 'late'))
    def early() -> None:
        state = 7

    @ac.rule(SB='late')
    def read() -> ac.u32:
        return state

    @ac.rule
    def late() -> None:
        state = 9

@ac.module
class EHRShared:

    def __init__() -> None:
        storage = Storage()
        first_seen = ac.cell[ac.u32](initial=0)
        second_seen = ac.cell[ac.u32](initial=0)

    @ac.rule
    def first(value: ac.u32) -> None:
        first_seen = value

    @ac.rule
    def second(value: ac.u32) -> None:
        second_seen = value + 1

    def __call__() -> None:
        storage.late()
        shared = storage.read()
        first(shared)
        second(shared)
        storage.early()
