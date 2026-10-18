import pycircuit as ac


@ac.rule
def evolve(cells: ac.bits[512], incoming: ac.bits[512], load: ac.u1) -> None:
    if load:
        cells = incoming
    else:
        cells = (cells << 1) ^ (cells >> 1)


@ac.module
def Top(incoming: ac.bits[512], load: ac.u1) -> ac.bits[512]:  # noqa: N802
    cells: ac.bits[512] = 0
    evolve(cells, incoming, load)
    return cells
