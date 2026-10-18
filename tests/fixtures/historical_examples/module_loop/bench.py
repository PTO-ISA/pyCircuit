# ruff: noqa: F823 -- captured System state proposal.
"""Generated System for the supported normal-state loop variant."""

import pycircuit as ac
from example_loop.module_loop import LoopReg


@ac.system
def ExerciseModuleLoop():  # noqa: N802
    enable: bool = False
    loop = LoopReg(enable, enable)

    @ac.rule
    def observe() -> None:
        ac.log("info", "module loop", loop, enable)

    @ac.rule
    def tick() -> None:
        enable = not enable

    observe()
    tick()
