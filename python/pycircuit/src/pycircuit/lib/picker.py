from __future__ import annotations

from pycircuit.hw import Circuit, Wire
from pycircuit.literals import u


def Picker(
    m: Circuit,
    req: Wire,
    *,
    width: int | None = None,
):
    req_w = req
    if not hasattr(req_w, "ty") or not str(req_w.ty).startswith("i"):
        raise ValueError("Picker.req must be an integer wire")
    w = int(width) if width is not None else int(req_w.width)
    if w <= 0:
        raise ValueError("Picker width must be > 0")

    idx_w = max(1, (w - 1).bit_length())
    grant = req_w & 0
    index = req_w[0:idx_w] & 0
    found = req_w[0] & 0

    for i in range(w):
        take = req_w[i] & ~found
        grant = take._select_internal(u(w, 1 << i), grant)
        index = take._select_internal(u(idx_w, i), index)
        found = found | req_w[i]

    return m.bundle_connector(
        valid=found,
        grant=grant,
        index=index,
    )
