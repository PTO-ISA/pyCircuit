from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    cas,
    compile_cycle_aware,
    mux,
    u,
    unsigned,
    wire_of,
)

KEY_ADD = 10
KEY_SUB = 11
KEY_MUL = 12
KEY_DIV = 13
KEY_EQ = 14
KEY_AC = 15

OP_ADD = 0
OP_SUB = 1
OP_MUL = 2
OP_DIV = 3


def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
    key = cas(domain, m.input("key", width=5), cycle=0)
    key_press = cas(domain, m.input("key_press", width=1), cycle=0)

    U5_9 = u(5, 9)
    U64_0 = u(64, 0)
    U64_1 = u(64, 1)
    U64_10 = u(64, 10)
    U1_0 = u(1, 0)
    U1_1 = u(1, 1)

    K_ADD = u(5, KEY_ADD)
    K_SUB = u(5, KEY_SUB)
    K_MUL = u(5, KEY_MUL)
    K_DIV = u(5, KEY_DIV)
    K_EQ = u(5, KEY_EQ)
    K_AC = u(5, KEY_AC)

    OP_ADD_CAS = u(2, OP_ADD)
    OP_SUB_CAS = u(2, OP_SUB)
    OP_MUL_CAS = u(2, OP_MUL)
    OP_DIV_CAS = u(2, OP_DIV)
    ZERO_OP = u(2, 0)

    lhs = domain.signal(width=64, reset_value=0, name="lhs")
    rhs = domain.signal(width=64, reset_value=0, name="rhs")
    op = domain.signal(width=2, reset_value=0, name="op")
    in_rhs = domain.signal(width=1, reset_value=0, name="in_rhs")
    display = domain.signal(width=64, reset_value=0, name="display_r")

    digit_lo = unsigned(wire_of(key[0:4]))
    # Widen 4-bit digit to 64 via literal add (avoids removed .zext helper).
    digit = cas(domain, digit_lo, cycle=0) + u(64, 0)

    is_digit = key_press & (key <= U5_9)
    is_add = key_press & (key == K_ADD)
    is_sub = key_press & (key == K_SUB)
    is_mul = key_press & (key == K_MUL)
    is_div = key_press & (key == K_DIV)
    is_eq = key_press & (key == K_EQ)
    is_ac = key_press & (key == K_AC)

    lhs_n = lhs
    rhs_n = rhs
    op_n = op
    in_rhs_n = in_rhs
    disp_n = display

    rhs_new = rhs_n * U64_10 + digit
    lhs_new = lhs_n * U64_10 + digit
    rhs_a = mux(is_digit & in_rhs_n, rhs_new, rhs_n)
    disp_a = mux(is_digit & in_rhs_n, rhs_new, disp_n)
    lhs_a = mux(is_digit & ~in_rhs_n, lhs_new, lhs_n)
    disp_a = mux(is_digit & ~in_rhs_n, lhs_new, disp_a)

    op_sel = is_add | is_sub | is_mul | is_div
    in_rhs_b = mux(op_sel, U1_1, in_rhs_n)
    rhs_b = mux(op_sel, U64_0, rhs_a)
    lhs_b = lhs_a
    disp_b = disp_a

    op_b = op_n
    op_b = mux(is_add, OP_ADD_CAS, op_b)
    op_b = mux(is_sub, OP_SUB_CAS, op_b)
    op_b = mux(is_mul, OP_MUL_CAS, op_b)
    op_b = mux(is_div, OP_DIV_CAS, op_b)

    rhs_safe = mux(rhs_b != U64_0, rhs_b, U64_1)
    result = lhs_b
    result = mux(op_b == OP_ADD_CAS, lhs_b + rhs_b, result)
    result = mux(op_b == OP_SUB_CAS, lhs_b - rhs_b, result)
    result = mux(op_b == OP_MUL_CAS, lhs_b * rhs_b, result)
    result = mux(
        op_b == OP_DIV_CAS,
        cas(domain, wire_of(lhs_b) // wire_of(rhs_safe), cycle=0),
        result,
    )

    lhs_c = mux(is_eq, result, lhs_b)
    rhs_c = mux(is_eq, U64_0, rhs_b)
    in_rhs_c = mux(is_eq, U1_0, in_rhs_b)
    disp_c = mux(is_eq, result, disp_b)
    op_c = op_b

    lhs_next = mux(is_ac, U64_0, lhs_c)
    rhs_next = mux(is_ac, U64_0, rhs_c)
    op_next = mux(is_ac, ZERO_OP, op_c)
    in_rhs_next = mux(is_ac, U1_0, in_rhs_c)
    disp_next = mux(is_ac, U64_0, disp_c)

    m.output("display", wire_of(display))
    m.output("op_pending", wire_of(op))

    domain.next()

    lhs <<= lhs_next
    rhs <<= rhs_next
    op <<= op_next
    in_rhs <<= in_rhs_next
    display <<= disp_next


build.__pycircuit_name__ = "calculator"


if __name__ == "__main__":
    print(compile_cycle_aware(build, name="calculator", eager=True).emit_mlir())
