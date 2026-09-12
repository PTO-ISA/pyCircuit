"""Nested rules infer typed lexical state while module instances stay isolated."""

import agentic_circuit as ac


@ac.module
def accumulator(incoming: ac.u8) -> ac.u8:
    total: ac.u8 = 0

    @ac.rule
    def add(value):
        total = total + value
        return total

    result = add(incoming)
    return result


@ac.system
def inferred_nested_rule(left: ac.u8, right: ac.u8) -> tuple[ac.u8, ac.u8]:
    left_result = accumulator(left)
    right_result = accumulator(right)
    return left_result, right_result
