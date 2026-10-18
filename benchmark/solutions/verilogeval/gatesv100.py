import pycircuit as ac


@ac.struct
class NeighborRelations:
    both: ac.bits[99]
    any: ac.bits[99]
    different: ac.bits[100]


@ac.module
def Top(incoming: ac.bits[100]) -> NeighborRelations:  # noqa: N802
    lower = incoming[:99]
    upper = incoming[1:]
    left_neighbors = ac.concat(incoming[:1], upper)
    return NeighborRelations(
        both=lower & upper,
        any=lower | upper,
        different=incoming ^ left_neighbors,
    )
