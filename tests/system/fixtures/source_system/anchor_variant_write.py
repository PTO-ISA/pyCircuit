import pycircuit as ac
from checks.anchor_variant_provider import Counter


@ac.system
def WriteCounter():
    owner = Counter()
    held = Counter()

    @ac.rule
    def advance() -> None:
        before = owner.read()
        after = owner.add(3)
        ac.log("info", "anchor write", before, after, held.read())

    advance()
