# ruff: noqa: F821, F823, F841, N802 -- compiler-owned captured state and published System names.
"""Independent layout boundary cases for the tensor scheduler."""
import pycircuit as ac
from fa2.fa2_scheduler import TensorLayoutValid


@ac.system
def TensorLayoutSystem():
    regular = TensorLayoutValid(4096, 128, 32, 6, 2, 2, 2, 2, 0, 0)
    input_broadcast = TensorLayoutValid(4096, 0, 0, 0, 2, 2, 2, 2, 0, 1)
    output_alias = TensorLayoutValid(4096, 0, 0, 0, 2, 2, 2, 2, 0, 0)
    row_alias = TensorLayoutValid(4096, 128, 32, 2, 2, 2, 2, 4, 0, 0)
    head_alias = TensorLayoutValid(4096, 128, 8, 10, 2, 2, 2, 2, 0, 0)
    batch_alias = TensorLayoutValid(4096, 32, 44, 10, 2, 2, 2, 2, 0, 0)
    last_legal = TensorLayoutValid(18446744073709551614, 0, 0, 0, 1, 1, 1, 2, 0, 0)
    overflowing = TensorLayoutValid(18446744073709551614, 0, 0, 0, 1, 1, 1, 4, 0, 0)
    product_overflow = TensorLayoutValid(0, 18446744073709551614, 0, 0, 3, 1, 1, 2, 0, 0)
    unaligned = TensorLayoutValid(4097, 128, 32, 6, 2, 2, 2, 2, 0, 0)
    metadata_unaligned = TensorLayoutValid(4098, 0, 0, 0, 1, 1, 1, 4, 1, 0)

    @ac.rule
    def check() -> None:
        assert regular == 1, "regular layout"
        assert input_broadcast == 1, "read-only broadcast"
        assert output_alias == 0, "output broadcast rejected"
        assert row_alias == 0, "row overlap rejected"
        assert head_alias == 0, "head overlap rejected"
        assert batch_alias == 0, "batch overlap rejected"
        assert last_legal == 1, "last aligned scalar address"
        assert overflowing == 0, "end overflow"
        assert product_overflow == 0, "stride product overflow"
        assert unaligned == 0, "tensor alignment"
        assert metadata_unaligned == 0, "metadata alignment"
        ac.log("info", "tensor_layout_pass")
    check()
