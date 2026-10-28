from pycircuit import dffe, module


@module
def Child(  # noqa: N802 - hardware module definition
    clk: bool, rst: bool, en: bool, data: bool, init: bool
) -> bool:  # noqa: N802
    # Legal low-level primitive: explicit independent enable, XOR feedback and
    # dynamic initialization remain bound to one canonical storage instance.
    state = dffe(T=bool)

    # Primitive pin wiring is Module structure, not an executable callback.
    state(clk=clk, rst=rst, en=en, d=state.q ^ data, init=init)
    return state.q
