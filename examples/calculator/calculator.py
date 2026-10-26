"""A u64 integer calculator driven by a 5-bit keypad, one key per epoch.

Key codes:
  0..9 digit, 10 add, 11 subtract, 12 multiply, 13 divide, 14 equals, 15 clear.
Pending-op encoding: 0 add, 1 subtract, 2 multiply, 3 divide.
"""

# ruff: noqa: F823 -- nested Rules update captured Module-owned state.

import pycircuit as ac


@ac.struct
class CalcResult:
    display: ac.u64
    op_pending: ac.u2


@ac.module
def Calculator(key: ac.u5, key_press: ac.u1) -> CalcResult:  # noqa: N802
    lhs: ac.u64 = 0
    rhs: ac.u64 = 0
    op: ac.u2 = 0
    in_rhs: ac.u1 = 0
    shown: ac.u64 = 0

    @ac.rule
    def step() -> CalcResult:
        # Outputs retain the committed state while this transaction builds D.
        lhs_q = lhs
        rhs_q = rhs
        op_q = op
        in_rhs_q = in_rhs
        shown_q = shown
        result = CalcResult(display=shown_q, op_pending=op_q)

        digit: ac.u64 = key[0:4]
        is_digit = key_press & (key <= 9)
        is_add = key_press & (key == 10)
        is_sub = key_press & (key == 11)
        is_mul = key_press & (key == 12)
        is_div = key_press & (key == 13)
        is_eq = key_press & (key == 14)
        is_ac = key_press & (key == 15)

        # Candidate reads after each assignment intentionally see prior updates.
        lhs = lhs_q * 10 + digit if is_digit & ~in_rhs_q else lhs_q
        rhs = rhs_q * 10 + digit if is_digit & in_rhs_q else rhs_q
        shown = lhs if is_digit & ~in_rhs_q else shown_q
        shown = rhs if is_digit & in_rhs_q else shown

        op_sel = is_add | is_sub | is_mul | is_div
        in_rhs = 1 if op_sel else in_rhs_q
        rhs = 0 if op_sel else rhs
        op = 0 if is_add else op_q
        op = 1 if is_sub else op
        op = 2 if is_mul else op
        op = 3 if is_div else op

        divisor = 1 if rhs == 0 else rhs
        computed = lhs
        computed = lhs + rhs if op == 0 else computed
        computed = lhs - rhs if op == 1 else computed
        computed = lhs * rhs if op == 2 else computed
        computed = lhs // divisor if op == 3 else computed

        lhs = computed if is_eq else lhs
        rhs = 0 if is_eq else rhs
        in_rhs = 0 if is_eq else in_rhs
        shown = computed if is_eq else shown

        # AC has final priority over entry, operator and equals candidates.
        lhs = 0 if is_ac else lhs
        rhs = 0 if is_ac else rhs
        op = 0 if is_ac else op
        in_rhs = 0 if is_ac else in_rhs
        shown = 0 if is_ac else shown
        return result

    return step
