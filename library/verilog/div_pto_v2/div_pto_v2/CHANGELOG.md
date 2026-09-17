# Changes from supplied DIV candidate

1. Added runtime `word_mode` so one 64-bit divider covers DIV/DIVU/DIVW/DIVUW
   and REM/REMU/REMW/REMUW.
2. Added fixed-word operand normalization for signed and unsigned W forms.
3. Added mandatory sign-extension of every W-form quotient/remainder result.
4. Added PTO zero-divisor bypass: quotient=0, remainder=normalized dividend.
5. Added signed-minimum/-1 bypass: quotient=minimum, remainder=0.
6. Kept one physical BaseJump divider and dual quotient/remainder outputs.
7. Kept `BITS_PER_ITER` as the physical latency/area parameter.
8. Retained `divide_by_zero` only as a non-fault diagnostic output.
9. Added a directed RTL testbench and updated `files.f`.
10. Left all imported BaseJump RTL and its license unchanged.
