import pycircuit as ac
from example_resource_composition.split_stage import SplitStage


@ac.module
def Relay(source: ac.u8, peer: ac.module) -> tuple[ac.u8, ac.u8]:  # noqa: N802
    left, right = SplitStage(source, peer)
    return left, right
