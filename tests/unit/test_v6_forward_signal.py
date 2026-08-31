from __future__ import annotations

import pycircuit
import pytest

pytestmark = pytest.mark.unit


def test_forward_signal_reads_rebase_to_current_occurrence() -> None:
    circuit = pycircuit.CycleAwareCircuit("rebased_state")
    domain = circuit.create_domain("clk")
    counter = domain.signal(width=8, reset_value=0, name="counter")

    domain.next()
    expr = counter + 1

    assert expr.cycle == domain.cycle_index


def test_forward_signal_feedback_does_not_insert_balance_registers() -> None:
    circuit = pycircuit.CycleAwareCircuit("counter_feedback")
    domain = circuit.create_domain("clk")
    counter = domain.signal(width=8, reset_value=0, name="counter")

    domain.next()
    counter <<= counter + 1

    mlir = circuit.emit_mlir()
    assert mlir.count("pyc.reg") == 1
    assert "_v6_bal_" not in mlir


def test_forward_signal_slice_reads_rebase_to_current_occurrence() -> None:
    circuit = pycircuit.CycleAwareCircuit("rebased_slice")
    domain = circuit.create_domain("clk")
    counter = domain.signal(width=8, reset_value=0, name="counter")

    domain.next()
    expr = counter[0]

    assert expr.cycle == domain.cycle_index


def test_cycle_aware_reverse_subtraction_compiles_through_jit() -> None:
    def build(m, domain) -> None:
        counter = domain.signal(width=8, reset_value=0, name="counter")
        domain.next()
        result = 1 - counter
        m.output("result", pycircuit.wire_of(result))

    design = pycircuit.compile_cycle_aware(build, name="reverse_sub_smoke")
    mlir = design.emit_mlir()

    assert "_v6_bal_" not in mlir
