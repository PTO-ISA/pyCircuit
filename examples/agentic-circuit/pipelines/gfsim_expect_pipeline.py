import agentic_circuit as ac


@ac.struct
class ExpectedToken:
    value: ac.u16


@ac.system
def gfsim_expect_pipeline() -> None:
    incoming = ac.source(ExpectedToken, depth=2, latency=1)
    ac.expect(
        incoming,
        predicate=lambda item: item.value > 0,
        message="value must be positive",
    )
    ac.sink(incoming)
