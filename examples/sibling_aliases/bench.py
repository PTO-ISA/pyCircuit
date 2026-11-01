import pycircuit as ac
from example_sibling_aliases.sibling_aliases import Counter, Relay


@ac.system
def SiblingAliases():  # noqa: N802
    shared = Counter()
    independent = Counter()
    Relay(shared)

    @ac.rule
    def observe_independent() -> None:
        ac.log("info", "independent count", independent.read())

    observe_independent()
