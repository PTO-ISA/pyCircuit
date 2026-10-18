from pycircuit import dffe, module, rule


@module
def Child(  # noqa: N802 - hardware module definition
    clk: bool, rst: bool, en: bool, data: bool, init: bool
) -> bool:  # noqa: N802
    # Legal low-level primitive: explicit independent enable, XOR feedback and
    # dynamic initialization remain bound to one canonical storage instance.
    state = dffe(T=bool)

    @rule
    def bind_state():
        state(clk=clk, rst=rst, en=en, d=state.q ^ data, init=init)

    bind_state()
    return state.q
