"""Row-major byte-address source unit for TLOAD/TSTORE transport."""

import pycircuit as ac


@ac.struct
class AddressResponse:
    address: ac.u64
    valid: ac.u1


@ac.module
def RowMajorAddress(  # noqa: N802
    base: ac.u64,
    row: ac.u64,
    column: ac.u64,
    stride: ac.u64,
    element_bytes: ac.u64,
    valid_rows: ac.u64,
    valid_columns: ac.u64,
) -> AddressResponse:
    return AddressResponse(
        address=base + row * stride + column * element_bytes,
        valid=(row < valid_rows) & (column < valid_columns),
    )
