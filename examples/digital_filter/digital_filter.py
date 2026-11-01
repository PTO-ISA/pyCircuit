# ruff: noqa: F821, F823, F842 -- Module state captured by Rules is compiler-owned.
"""Four-tap signed FIR, with history advancing only on valid samples."""

import pycircuit as ac


@ac.struct
class FilterResult:
    y_out: ac.u34
    y_valid: ac.u1


@ac.module
def FilterStorage() -> None:  # noqa: N802
    delay1: ac.u16 = 0
    delay2: ac.u16 = 0
    delay3: ac.u16 = 0
    y_out: ac.u34 = 0
    y_valid: ac.u1 = 0

    @ac.method
    def filter_sample(x_in: ac.u16, x_valid: ac.u1) -> FilterResult:
        result = FilterResult(y_out=y_out, y_valid=y_valid)

        # Form 34-bit two's-complement samples before the modular FIR arithmetic.
        sample: ac.u34 = x_in
        sample_sign: ac.u34 = 0x3FFFF0000 if x_in[15:16] else 0
        tap1: ac.u34 = delay1
        tap1_sign: ac.u34 = 0x3FFFF0000 if delay1[15:16] else 0
        tap2: ac.u34 = delay2
        tap2_sign: ac.u34 = 0x3FFFF0000 if delay2[15:16] else 0
        tap3: ac.u34 = delay3
        tap3_sign: ac.u34 = 0x3FFFF0000 if delay3[15:16] else 0
        total = (
            (sample | sample_sign) * 1
            + (tap1 | tap1_sign) * 2
            + (tap2 | tap2_sign) * 3
            + (tap3 | tap3_sign) * 4
        )

        # Every history read observes committed old Q.
        # Conditional data selection also preserves the original X/Z-valid merges.
        delay3 = delay2 if x_valid else delay3
        delay2 = delay1 if x_valid else delay2
        delay1 = x_in if x_valid else delay1
        y_out = total if x_valid else y_out
        y_valid = x_valid
        return result


@ac.module
def DigitalFilter(x_in: ac.u16, x_valid: ac.u1) -> FilterResult:  # noqa: N802
    storage: FilterStorage

    @ac.rule
    def filter_sample() -> FilterResult:
        return storage.filter_sample(x_in, x_valid)

    return filter_sample()
