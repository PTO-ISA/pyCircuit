"""A reusable two-input Queue adder for Agentic Circuit and gfsim."""

import agentic_circuit as ac


@ac.rule
def add_pair(left: ac.u32, right: ac.u32) -> ac.u32:
    return left + right


@ac.module_decl(source="queue_adder.py")
def queue_adder(left: ac.u32, right: ac.u32) -> ac.u32:
    ...


queue_adder_decl = queue_adder


@ac.module(declaration=queue_adder_decl)
def queue_adder(left: ac.u32, right: ac.u32) -> ac.u32:
    result = add_pair(left, right)
    return result
