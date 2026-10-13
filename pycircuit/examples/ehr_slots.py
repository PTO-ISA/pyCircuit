"""Release capacity before allocation, with an atomic EHR guard view."""
from pycircuit import ac

@ac.module
class Slot:

    def __init__() -> None:
        occupied = ac.ehr[bool](initial=True)
        payload = ac.cell[ac.u32](initial=9)

    @ac.rule(SB='available')
    def release() -> None:
        occupied.value = False

    @ac.rule(SB='allocate')
    def available() -> bool:
        return not occupied.value

    def free(value: ac.u32) -> bool:
        return available()

    @ac.rule(guard=free)
    def allocate(value: ac.u32) -> None:
        occupied.value = True
        payload.value = value

    def __call__() -> None:
        pass

@ac.module
class EHRSlots:

    def __init__(release_enabled: bool) -> None:
        slot = Slot()

    @ac.rule
    def release() -> None:
        slot.release()

    @ac.rule
    def allocate() -> None:
        slot.allocate(7)

    def __call__() -> None:
        slot()
        if release_enabled:
            release()
        allocate()
