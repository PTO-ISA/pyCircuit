"""Static-only contracts for generic vector frontend APIs.

Run with ``MYPYPATH=python/pycircuit/src python -m mypy tests/typecheck/test_vector_api.py``.
"""

from typing import TYPE_CHECKING

from pycircuit import Circuit, Wire
from pycircuit.data import Bits, Data, Vector

if TYPE_CHECKING:
    m = Circuit("vector_type_contract")
    selector: Wire[Vector[Bits]] = m.vec(
        [m.input("sel0", width=1), m.input("sel1", width=1)]
    )
    values: Wire[Vector[Bits]] = m.vec(
        [m.input("value0", width=8), m.input("value1", width=8)]
    )

    lane: Wire[Bits] = values[0]
    selected: Wire[Bits] = m.priority_mux(selector, values)
    all_active: Wire[Bits] = selector.reduce_or(dim=None)
    partial_active: Wire[Data] = selector.reduce_or(dim=0)
    expanded: Wire[Vector[Data]] = values.broadcast(size=2, dim=0)
