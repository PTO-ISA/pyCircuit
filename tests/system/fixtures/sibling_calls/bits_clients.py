import pycircuit as ac
from checks.bits import BitsOwner


@ac.module
def BitsWriter(owner: BitsOwner, payload: ac.u4, enabled: ac.u1):  # noqa: N802
    @ac.rule
    def write() -> None:
        owner.write(payload, enabled)

    write()


@ac.module
def BitsReader(owner: BitsOwner):  # noqa: N802
    @ac.rule
    def read() -> None:
        value = owner.read()
        ac.log("info", "bits forwarded", value)

    read()
