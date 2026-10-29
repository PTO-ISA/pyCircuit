import pycircuit as ac
from checks.saved_anchor_provider import Counter
from pycircuit import rule, system


@system
def ExerciseSavedAnchor():
    owner = Counter()
    held = Counter()

    @rule
    def drive() -> None:
        proposed: ac.u8 = owner.advance(3)
        old: ac.u8 = owner.read()
        ac.log("info", "anchor advance", old, proposed)

    @rule
    def observe() -> None:
        old: ac.u8 = owner.read()
        unchanged: ac.u8 = held.read()
        ac.log("info", "anchor old", old, unchanged)

    @rule
    def suppressed() -> None:
        proposed: ac.u8 = held.advance(1)
        denied: ac.u8 = held.guarded(0)
        ac.log("info", "anchor suppressed", proposed, denied)

    drive()
    observe()
    suppressed()
