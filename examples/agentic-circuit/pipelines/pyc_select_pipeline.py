import agentic_circuit as ac


@ac.struct
class SelectControl:
    route: ac.u1


@ac.system
def pyc_select_pipeline() -> None:
    control = ac.source(SelectControl, depth=2, latency=1)
    lanes = ac.array(2, lambda index: ac.source(int, depth=2, latency=1))
    selected = lanes.select(
        control,
        key=lambda item: item.route,
        depth=2,
        latency=1,
    )
    ac.sink(selected)
