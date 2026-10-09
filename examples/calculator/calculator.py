"""A u64 integer calculator driven by a 5-bit keypad, one key per epoch.

Key codes:
  0..9 digit, 10 add, 11 subtract, 12 multiply, 13 divide, 14 equals, 15 clear.
Pending-op encoding: 0 add, 1 subtract, 2 multiply, 3 divide.
"""

import pycircuit as ac


@ac.struct
class CalcResult:
    display: ac.u64
    op_pending: ac.u2


@ac.rule
def step(lhs, rhs, op, in_rhs, shown, key, key_press) -> CalcResult:
    # Outputs observe the committed state, not this epoch's proposal.
    result = CalcResult(display=shown, op_pending=op)

    digit: ac.u64 = key[0:4]
    is_digit = key_press & (key <= 9)
    is_add = key_press & (key == 10)
    is_sub = key_press & (key == 11)
    is_mul = key_press & (key == 12)
    is_div = key_press & (key == 13)
    is_eq = key_press & (key == 14)
    is_ac = key_press & (key == 15)

    # A digit extends whichever operand is currently being entered.
    lhs_typed = lhs * 10 + digit
    rhs_typed = rhs * 10 + digit
    lhs_entry = lhs
    rhs_entry = rhs
    shown_entry = shown
    if is_digit & in_rhs:
        rhs_entry = rhs_typed
        shown_entry = rhs_typed
    if is_digit & ~in_rhs:
        lhs_entry = lhs_typed
        shown_entry = lhs_typed

    # An operator key latches the pending op and starts a fresh right operand.
    op_sel = is_add | is_sub | is_mul | is_div
    in_rhs_entry = in_rhs
    rhs_ready = rhs_entry
    if op_sel:
        in_rhs_entry = 1
        rhs_ready = 0

    op_entry = op
    op_entry = 0 if is_add else op_entry
    op_entry = 1 if is_sub else op_entry
    op_entry = 2 if is_mul else op_entry
    op_entry = 3 if is_div else op_entry

    # Division by zero keeps the numerator by replacing the divisor with one.
    divisor = rhs_ready
    if rhs_ready == 0:
        divisor = 1

    # The op-mux chain starts from the left operand, so a retained op passes the
    # left operand through rather than inventing a fifth operation.
    computed = lhs_entry
    if op_entry == 0:
        computed = lhs_entry + rhs_ready
    if op_entry == 1:
        computed = lhs_entry - rhs_ready
    if op_entry == 2:
        computed = lhs_entry * rhs_ready
    if op_entry == 3:
        computed = lhs_entry // divisor

    # EQ commits the computed value into the left operand and re-arms entry.
    lhs_committed = computed if is_eq else lhs_entry
    rhs_committed = 0 if is_eq else rhs_ready
    in_rhs_committed = 0 if is_eq else in_rhs_entry
    shown_committed = computed if is_eq else shown_entry

    # AC clears everything.
    lhs = 0 if is_ac else lhs_committed
    rhs = 0 if is_ac else rhs_committed
    op = 0 if is_ac else op_entry
    in_rhs = 0 if is_ac else in_rhs_committed
    shown = 0 if is_ac else shown_committed
    return result


@ac.module
def Calculator(key: ac.u5, key_press: ac.u1) -> CalcResult:  # noqa: N802
    lhs: ac.u64 = 0
    rhs: ac.u64 = 0
    op: ac.u2 = 0
    in_rhs: ac.u1 = 0
    shown: ac.u64 = 0
    return step(lhs, rhs, op, in_rhs, shown, key, key_press)
