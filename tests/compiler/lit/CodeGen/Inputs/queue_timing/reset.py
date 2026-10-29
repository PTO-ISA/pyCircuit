"""Reset a full Queue, then replay its complete fill, replace and drain sequence."""

import pycircuit as ac
from queue_timing.design import Timing


@ac.system(
    samples=(
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 1),
        (1, 1),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
    )
)
def Top():  # noqa: N802
    Timing()
