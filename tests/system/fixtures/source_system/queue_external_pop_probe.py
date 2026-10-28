# ruff: noqa: F823, F841 -- captured module-owned state proposal.
import pycircuit as ac


@ac.system
def ExternalPopProbe():
    allow: ac.u1 = 1
    take: ac.u1 = 0
    count: ac.u8 = 0
    tokens = ac.queue[ac.u8](depth=1, latency=1)

    @ac.rule
    def produce() -> None:
        tokens.push(23)
        ac.log("info", "external pop pushed", 23)

    @ac.rule(guard=lambda: allow)
    def consume() -> None:
        if take:
            value = tokens.pop()
            ac.log("info", "external pop retired", value)

    @ac.rule
    def tick() -> None:
        count = count + 1
