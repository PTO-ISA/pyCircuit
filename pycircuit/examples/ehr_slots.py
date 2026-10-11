"""Release capacity before allocation, with an atomic EHR guard view."""
from pycircuit import ac


@ac.module
def Slot():
    occupied = ac.ehr[bool](initial=True)
    payload = ac.cell[ac.u32](initial=9)

    @ac.method
    def release():
        occupied.value = False

    @ac.method(After='release')
    def available() -> bool:
        return not occupied.value

    def free(value: ac.u32) -> bool:
        return available()

    @ac.method(After='available', guard=free)
    def allocate(value: ac.u32):
        occupied.value = True
        payload.value = value


@ac.module
def EHRSlots(release_enabled: bool):
    slot = Slot()

    @ac.rule
    def release():
        slot.release()

    @ac.rule
    def allocate():
        slot.allocate(7)

    if release_enabled:
        release()
    allocate()
