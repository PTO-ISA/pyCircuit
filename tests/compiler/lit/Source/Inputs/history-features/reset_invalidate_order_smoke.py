from pycircuit import module, rule, struct, u1, u8


@struct
class ResetOut:
    y: u8


@rule
def advance(q, en):
    if en:
        q = q + 1


@module
def ResetInvalidateOrder(en: u1) -> ResetOut:
    q: u8 = 0
    advance(q, en)
    return ResetOut(y=q)
