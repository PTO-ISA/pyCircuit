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


def test_forward_signal_helpers_share_the_current_read_occurrence() -> None:
    circuit = pycircuit.CycleAwareCircuit("forward_helper_occurrence")
    domain = circuit.create_domain("clk")
    value = domain.signal(width=8, reset_value=0, name="value")
    saved = value.as_cas()

    domain.next()

    direct = value.as_cas()
    unified = pycircuit.CycleAwareSignal.as_cas(value)
    method_priority = value.priority_encode()
    function_priority = pycircuit.priority_encode(value)
    assert saved.cycle == 0
    assert value.cycle == direct.cycle == unified.cycle == 1
    assert method_priority.index.cycle == function_priority.index.cycle == 1
    assert method_priority.valid.cycle == function_priority.valid.cycle == 1
    assert value.popcount().cycle == pycircuit.popcount(value).cycle == 1
    assert (
        value.count_leading_zeros().cycle
        == pycircuit.count_leading_zeros(value).cycle
        == 1
    )
    assert (
        value.count_trailing_zeros().cycle
        == pycircuit.count_trailing_zeros(value).cycle
        == 1
    )
    assert (
        pycircuit.cat(value, value[0]).cycle == domain.cat(value, value[0]).cycle == 1
    )
    assert (
        pycircuit.mux(value[0], value, 0).cycle == value[0].select(value, 0).cycle == 1
    )
    assert "_v6_bal_" not in circuit.emit_mlir()


def test_domain_cycle_returns_source_occurrence_plus_one() -> None:
    circuit = pycircuit.CycleAwareCircuit("explicit_cycle_provenance")
    domain = circuit.create_domain("clk")
    a = pycircuit.cas(domain, circuit.input("a", width=8), cycle=0)

    domain.next()
    domain.next()
    domain.next()
    first = domain.cycle(a, name="first")
    second = domain.cycle(first, name="second")
    b = pycircuit.cas(domain, circuit.input("b", width=8), cycle=3)
    mixed = second + b

    assert isinstance(first, pycircuit.CycleAwareSignal)
    assert isinstance(second, pycircuit.CycleAwareSignal)
    assert first.cycle == 1
    assert second.cycle == 2
    assert mixed.cycle == 3
    mlir = circuit.emit_mlir()
    assert mlir.count("pyc.reg") == 3
    assert "_v6_bal_1" in mlir
    assert "_v6_bal_2" not in mlir


def test_domain_cycle_reads_forward_state_at_current_occurrence() -> None:
    circuit = pycircuit.CycleAwareCircuit("forward_explicit_cycle")
    domain = circuit.create_domain("clk")
    state = domain.signal(width=8, reset_value=0, name="state")
    raw = circuit.input("raw", width=8)
    domain.next()
    domain.next()
    domain.next()

    delayed = domain.cycle(state, name="delayed")
    delayed_raw = domain.cycle(raw, name="delayed_raw")

    assert delayed.cycle == 4
    assert delayed_raw.cycle == 4


def test_submodule_input_fails_closed_for_missing_composed_key() -> None:
    circuit = pycircuit.CycleAwareCircuit("submodule_input_missing")
    domain = circuit.create_domain("clk")
    value = pycircuit.cas(domain, circuit.input("value", width=8), cycle=0)

    with pytest.raises(KeyError, match="missing composed input 'missing'"):
        pycircuit.submodule_input(
            {"value": value},
            "missing",
            circuit,
            domain,
            prefix="child",
            width=8,
        )

    assert "child_missing" not in circuit.emit_mlir()


def test_submodule_input_normalizes_composed_and_standalone_values() -> None:
    circuit = pycircuit.CycleAwareCircuit("submodule_input_normalized")
    domain = circuit.create_domain("clk")
    state = domain.signal(width=8, reset_value=0, name="state")
    domain.next()

    composed = pycircuit.submodule_input(
        {"state": state},
        "state",
        circuit,
        domain,
        prefix="child",
        width=8,
    )
    standalone = pycircuit.submodule_input(
        None,
        "value",
        circuit,
        domain,
        prefix="child",
        width=8,
        cycle=2,
    )

    assert type(composed) is pycircuit.CycleAwareSignal
    assert composed.cycle == 1
    assert type(standalone) is pycircuit.CycleAwareSignal
    assert standalone.cycle == 2

    with pytest.raises(TypeError, match="unexpected type"):
        pycircuit.submodule_input(
            {"bad": object()},
            "bad",
            circuit,
            domain,
            prefix="child",
            width=8,
        )

    with pytest.raises(TypeError, match="width mismatch"):
        pycircuit.submodule_input(
            {"bad_width": state},
            "bad_width",
            circuit,
            domain,
            prefix="child",
            width=4,
        )

    other_circuit = pycircuit.CycleAwareCircuit("other_domain")
    other_domain = other_circuit.create_domain("clk")
    other_value = pycircuit.cas(
        other_domain, other_circuit.input("value", width=8), cycle=0
    )
    with pytest.raises(ValueError, match="domain mismatch"):
        pycircuit.submodule_input(
            {"other": other_value},
            "other",
            circuit,
            domain,
            prefix="child",
            width=8,
        )

    with pytest.raises(TypeError, match="dict or None"):
        pycircuit.submodule_input(  # type: ignore[arg-type]
            [],
            "bad",
            circuit,
            domain,
            prefix="child",
            width=8,
        )


def test_domain_call_rejects_extra_composed_inputs() -> None:
    circuit = pycircuit.CycleAwareCircuit("submodule_input_extra")
    domain = circuit.create_domain("clk")
    value = pycircuit.cas(domain, circuit.input("value", width=8), cycle=0)

    def child(m, child_domain, *, inputs):
        child_value = pycircuit.submodule_input(
            inputs,
            "value",
            m,
            child_domain,
            prefix="child",
            width=8,
        )
        return {"value": child_value}

    with pytest.raises(KeyError, match="unexpected composed inputs: extra"):
        domain.call(child, inputs={"value": value, "extra": value})


def test_flat_nested_domain_call_propagates_consumed_inputs() -> None:
    circuit = pycircuit.CycleAwareCircuit("nested_composed_input")
    domain = circuit.create_domain("clk")
    value = pycircuit.cas(domain, circuit.input("value", width=8), cycle=0)

    def leaf(m, leaf_domain, *, inputs):
        leaf_value = pycircuit.submodule_input(
            inputs,
            "value",
            m,
            leaf_domain,
            prefix="leaf",
            width=8,
        )
        return {"value": leaf_value}

    def wrapper(m, wrapper_domain, *, inputs):
        return wrapper_domain.call(leaf, inputs=inputs)

    outputs = domain.call(wrapper, inputs={"value": value})

    assert outputs["value"] is value


def test_hierarchical_domain_call_rejects_missing_and_extra_inputs() -> None:
    def child(m, domain, *, inputs, prefix="child"):
        lhs = pycircuit.submodule_input(
            inputs, "lhs", m, domain, prefix=prefix, width=8
        )
        rhs = pycircuit.submodule_input(
            inputs, "rhs", m, domain, prefix=prefix, width=8
        )
        return {"value": lhs + rhs}

    def missing_top(m, domain):
        lhs = pycircuit.cas(domain, m.input("lhs", width=8), cycle=0)
        domain.call(child, inputs={"lhs": lhs}, prefix="u_child")

    with pytest.raises(KeyError, match="missing composed input 'rhs'"):
        pycircuit.compile_cycle_aware(missing_top, eager=True, hierarchical=True)

    def extra_top(m, domain):
        lhs = pycircuit.cas(domain, m.input("lhs", width=8), cycle=0)
        rhs = pycircuit.cas(domain, m.input("rhs", width=8), cycle=0)
        domain.call(
            child,
            inputs={"lhs": lhs, "rhs": rhs, "extra": lhs},
            prefix="u_child",
        )

    with pytest.raises(KeyError, match="unexpected composed inputs: extra"):
        pycircuit.compile_cycle_aware(extra_top, eager=True, hierarchical=True)


def test_hierarchical_domain_call_rejects_domain_and_width_mismatches() -> None:
    def child(m, domain, *, inputs, prefix="child"):
        value = pycircuit.submodule_input(
            inputs, "value", m, domain, prefix=prefix, width=8
        )
        return {"value": value}

    other_circuit = pycircuit.CycleAwareCircuit("other_domain")
    other_domain = other_circuit.create_domain("clk")
    other_value = pycircuit.cas(
        other_domain, other_circuit.input("value", width=8), cycle=0
    )

    def wrong_domain_top(m, domain):
        domain.call(child, inputs={"value": other_value}, prefix="u_child")

    with pytest.raises(ValueError, match="domain mismatch"):
        pycircuit.compile_cycle_aware(wrong_domain_top, eager=True, hierarchical=True)

    def wrong_width_top(m, domain):
        value = pycircuit.cas(domain, m.input("value", width=4), cycle=0)
        domain.call(child, inputs={"value": value}, prefix="u_child")

    with pytest.raises(TypeError, match="width mismatch: expected 8, got 4"):
        pycircuit.compile_cycle_aware(wrong_width_top, eager=True, hierarchical=True)
