from typing import Annotated

from pycircuit import dff, module, rule
from locals.local_binding_child import Word


@module
def Bindings(clk: bool, rst: bool, a: Annotated[int, range(1 << 8)], b: Annotated[int, range(1 << 8)], wide: Annotated[int, range(1 << 65)], flag: bool, one: Annotated[int, range(1 << 1)]) -> {
    "result": Annotated[int, range(1 << 8)],
    "q": Annotated[int, range(1 << 8)],
    "signed_mask": Annotated[int, range(1 << 8)],
    "signed_q": Annotated[int, range(1 << 8)],
    "zero_q": Annotated[int, range(1 << 1)],
    "selected": Annotated[int, range(1 << 9)],
    "input_alias": Annotated[int, range(1 << 8)],
    "child_alias": Annotated[int, range(1 << 8)],
    "bool_alias": bool,
    "integer_plus": Annotated[int, range(1 << 2)],
    "original_xor": Annotated[int, range(1 << 8)],
    "original_or": Annotated[int, range(1 << 8)],
    "original_equal": bool,
    "wide_alias": Annotated[int, range(1 << 65)],
    "wide_carry": Annotated[int, range(1 << 1)],
    "wide_singleton": Annotated[int, range(1 << 64)]}:
    state = dff(T=Annotated[int, range(1 << 8)])
    signed_state = dff(T=Annotated[int, range(1 << 8)])
    zero_state = dff(T=Annotated[int, range(1 << 1)])
    child = Word()
    old = state.q
    candidate = (a + 1) & 255
    chain = candidate
    complete = chain
    input_alias = a
    bool_alias = flag
    integer_alias = one
    child_alias = child.q
    signed_value = a - b
    signed_mask = signed_value & 255
    wide_alias = wide
    widened = wide_alias + 1
    wide_carry = widened >> 65
    singleton = wide_alias * 0
    wide_singleton = singleton >> 1
    selected = (a + 1) if bool_alias else 256

    @rule
    def update():
        state(clk=clk, rst=rst, d=late, init=0)

    @rule
    def update_signed():
        signed_state(clk=clk, rst=rst, d=signed_mask, init=0)

    @rule
    def update_zero():
        zero_state(clk=clk, rst=rst, d=singleton >> 0, init=0)

    @rule
    def update_child():
        child(a=input_alias)

    update()
    update_signed()
    update_zero()
    update_child()
    late = complete
    result = late
    return {"result": result, "q": old, "signed_mask": signed_mask,
            "signed_q": signed_state.q, "zero_q": zero_state.q,
            "selected": selected, "input_alias": input_alias,
            "child_alias": child_alias, "bool_alias": bool_alias,
            "integer_plus": integer_alias + 1,
            "original_xor": input_alias ^ 1, "original_or": input_alias | 1,
            "original_equal": input_alias == 0, "wide_alias": wide_alias,
            "wide_carry": wide_carry, "wide_singleton": wide_singleton}
