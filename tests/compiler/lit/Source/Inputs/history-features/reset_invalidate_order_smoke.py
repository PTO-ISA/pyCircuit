from pycircuit import module, rule, u1, u8


@rule
def advance(q, en):
    if en:
        q = q + 1


@module
def ResetInvalidateOrder(en: u1) -> {"y": u8}:
    q: u8 = 0
    advance(q, en)
    return {"y": q}
