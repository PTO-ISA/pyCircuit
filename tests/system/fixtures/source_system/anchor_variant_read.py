import pycircuit as ac
from checks.anchor_variant_provider import Counter


@ac.system
def ReadCounter():
    owner = Counter()

    @ac.rule
    def observe() -> None:
        ac.log("info", "anchor read", owner.read())

    observe()
