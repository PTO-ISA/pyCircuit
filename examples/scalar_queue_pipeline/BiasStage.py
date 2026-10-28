import pycircuit as ac


@ac.module
def BiasStage(source: ac.bits[8]) -> ac.bits[8]:  # noqa: N802
    @ac.rule
    def bias(value: ac.bits[8]) -> ac.bits[8]:
        return value + 1

    result = bias(source)
    return result
