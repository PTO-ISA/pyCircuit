import pycircuit as ac
from checks.method_nominal_provider import NominalCounter


@ac.system
def ExerciseNominalMethods():
    owner = NominalCounter()
    held = NominalCounter()

    @ac.rule
    def drive() -> None:
        first = owner.advance(0, 1)
        second = owner.advance(1, 2)
        saved = first.packet
        first.packet.left = second.packet.right
        ac.log(
            "info",
            "nominal first",
            saved.left.data,
            saved.right.data,
            first.saved.left.data,
            ac.enum_to_bits(first.direct),
        )
        ac.log(
            "info",
            "nominal second",
            second.packet.left.data,
            second.packet.right.data,
            second.saved.left.data,
            ac.enum_to_bits(second.direct),
        )
        ac.report("nominal count", owner.read())

    @ac.rule
    def observe() -> None:
        ac.log("info", "nominal old", owner.read(), held.read())

    @ac.rule
    def suppressed() -> None:
        pending = held.advance(1, 1)
        denied = held.guarded(0)
        ac.log("info", "nominal suppressed", pending.packet.left.data, denied.left.data)

    drive()
    observe()
    suppressed()
