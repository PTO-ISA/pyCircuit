from pycircuit import dffe, module, rule


@module
def Child(  # noqa: N802 - hardware module definition
    clk: bool, rst: bool, en: bool, data: bool, init: bool
) -> {"q": bool}:  # noqa: N802, F821 - compiler-owned module/port syntax
    state = dffe(T=bool)

    @rule
    def bind_state():
        state(clk=clk, rst=rst, en=en, d=state.q ^ data, init=init)

    bind_state()
    return {"q": state.q}
