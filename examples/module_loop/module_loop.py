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
) -> {"left": bool, "right": bool}:  # noqa: F821 - named hardware ports
    left_cell = Child()
    right_cell = Child()

    @rule
    def bind_left():
        left_cell(clk=clk, rst=rst, en=en_left, data=data_left, init=init_left)

    @rule
    def bind_right():
        right_cell(clk=clk, rst=rst, en=en_right, data=data_right, init=init_right)

    bind_left()
    bind_right()
    return {"left": left_cell.q, "right": right_cell.q}
