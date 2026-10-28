# ruff: noqa: F823 -- captured module-owned state proposal.
from pycircuit import module, rule
from example_loop.child import Child


@module
def Top(  # noqa: N802 - hardware module definition
    clk: bool,
    rst: bool,
    en_left: bool,
    en_right: bool,  # noqa: N802
    data_left: bool,
    data_right: bool,
    init_left: bool,
    init_right: bool,
) -> tuple[bool, bool]:
    left = Child(clk, rst, en_left, data_left, init_left)
    right = Child(clk, rst, en_right, data_right, init_right)
    return left, right


@module
def LoopReg(enable: bool, feedback: bool) -> bool:  # noqa: N802
    # Supported normal-state form of the same XOR feedback and independent
    # enable, used by the generated System below.
    value: bool = False

    @rule
    def advance() -> None:
        if enable:
            value = value ^ feedback

    @rule
    def read() -> bool:
        return value

    return read
