import pycircuit as ac
from example_sibling_transitive.sibling_transitive import Counter, Reader, Writer


@ac.system
def SiblingTransitive():  # noqa: N802
    counter = Counter()
    Writer(counter)
    Reader(counter)

    @ac.rule
    def observe_committed() -> None:
        ac.log("info", "transitive committed", counter.committed())

    observe_committed()
