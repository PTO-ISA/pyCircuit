from __future__ import annotations

import importlib.util
import inspect
import re
import subprocess
from pathlib import Path

import pycircuit
import pycircuit.v6 as pyc6
import pytest
from pycircuit.design import Design

pytestmark = pytest.mark.unit


def _domain_factory_build(m, domain):
    value = domain.create_signal("value", width=8)
    constant = domain.create_const(3, width=8)
    reset = domain.create_reset()
    m.output("result", pycircuit.wire_of(pycircuit.mux(reset, value, constant)))


def test_cycle_aware_frontend_is_the_pyc6_surface() -> None:
    assert pycircuit.CycleAwareSignal is pyc6.CycleAwareSignal
    assert pycircuit.CycleAwareDomain is pyc6.CycleAwareDomain
    assert pycircuit.compile_cycle_aware is pyc6.compile_cycle_aware
    assert pycircuit.build_cycle_aware is pyc6.build_cycle_aware
    assert not hasattr(pycircuit, "StateSignal")
    assert not hasattr(pycircuit, "priority_mux")


def test_tutorial_facade_is_not_part_of_the_public_surface() -> None:
    root = Path(__file__).resolve().parents[2]
    expected = set(
        (root / "tests/goldens/pyc6_public_api.txt")
        .read_text(encoding="utf-8")
        .splitlines()
    )
    removed = {
        "log",
        "pyc_CircuitLogger",
        "pyc_CircuitModule",
        "pyc_ClockDomain",
        "pyc_Signal",
        "signal",
    }

    assert set(pycircuit.__all__) == expected
    assert removed.isdisjoint(pycircuit.__all__)
    for name in removed:
        assert not hasattr(pycircuit, name)


def test_public_exception_families_share_one_runtime_base() -> None:
    for error in (
        pycircuit.ConnectorError,
        pycircuit.DesignError,
        pycircuit.DiagnosticError,
        pycircuit.JitError,
        pycircuit.ProbeError,
        pycircuit.TbError,
        pycircuit.TraceConfigError,
    ):
        assert issubclass(error, pycircuit.PyCircuitError)
    assert issubclass(pycircuit.ConnectorError, TypeError)


@pytest.mark.parametrize(
    ("error_type", "code"),
    [
        (pycircuit.ConnectorError, "PYC-PY-CONNECTOR"),
        (pycircuit.DesignError, "PYC-PY-DESIGN"),
        (pycircuit.JitError, "PYC-PY-JIT"),
        (pycircuit.ProbeError, "PYC-PY-PROBE"),
        (pycircuit.TbError, "PYC-PY-TB"),
        (pycircuit.TraceConfigError, "PYC-PY-TRACE"),
    ],
)
def test_public_exception_families_have_stable_structured_codes(
    error_type: type[pycircuit.PyCircuitError], code: str
) -> None:
    error = error_type("invalid public API input")

    assert error.code == code
    assert error.diagnostic.code == code
    assert error.diagnostic.message == "invalid public API input"


@pytest.mark.parametrize(
    ("invoke", "builtin_type", "code"),
    [
        (lambda: pycircuit.wire_of(object()), TypeError, "PYC-PY-TYPE"),
        (lambda: pycircuit.cat(), ValueError, "PYC-PY-VALUE"),
        (
            lambda: pycircuit.submodule_input(
                {}, "missing", None, None, prefix="sub", width=1
            ),
            KeyError,
            "PYC-PY-KEY",
        ),
    ],
)
def test_cycle_aware_public_errors_preserve_builtin_compatibility_and_diagnostics(
    invoke, builtin_type: type[Exception], code: str
) -> None:
    with pytest.raises(pycircuit.PyCircuitError) as exc_info:
        invoke()

    error = exc_info.value
    assert isinstance(error, builtin_type)
    assert error.code == code
    assert error.diagnostic.code == code
    assert error.diagnostic.message
    assert error.location == "<unknown>"
    assert f"[{code}]" in str(error)


def test_cycle_aware_compile_entrypoints_have_stable_modes_and_types() -> None:
    compile_sig = inspect.signature(pycircuit.compile_cycle_aware)
    build_sig = inspect.signature(pycircuit.build_cycle_aware)
    assert list(compile_sig.parameters) == ["fn", "name", "domain_name", "jit_params"]
    assert list(build_sig.parameters) == [
        "fn",
        "name",
        "domain_name",
        "hierarchical",
        "build_params",
    ]
    assert compile_sig.parameters["jit_params"].kind is inspect.Parameter.VAR_KEYWORD
    assert build_sig.parameters["build_params"].kind is inspect.Parameter.VAR_KEYWORD

    def build(m, domain, offset=1):
        value = pycircuit.cas(domain, m.input("value", width=8), cycle=0)
        m.output("result", pycircuit.wire_of(value + offset))

    compiled = pycircuit.compile_cycle_aware(build, name="compiled", offset=3)
    elaborated = pycircuit.build_cycle_aware(build, name="elaborated", offset=3)

    assert isinstance(compiled, Design)
    assert isinstance(elaborated, pycircuit.CycleAwareCircuit)
    for mlir in (compiled.emit_mlir(), elaborated.emit_mlir()):
        assert 'pyc.frontend.contract = "pycircuit"' in mlir
        assert 'pyc.kind = "module"' in mlir
        assert '\\"offset\\":3' in mlir
        assert "pyc.add" in mlir


@pytest.mark.parametrize("blank_name", ["", "   "])
def test_cycle_aware_compile_entrypoints_normalize_blank_names(
    blank_name: str,
) -> None:
    def named_build(m, domain):
        m.output("result", pycircuit.wire_of(domain.create_const(0, width=1)))

    compiled = pycircuit.compile_cycle_aware(named_build, name=blank_name)
    elaborated = pycircuit.build_cycle_aware(named_build, name=blank_name)

    assert compiled.top == "named_build"
    assert "pyc.top = @named_build" in elaborated.emit_mlir()


@pytest.mark.parametrize(
    "removed",
    ["design_ctx", "eager", "hierarchical", "structural", "value_params"],
)
def test_compile_cycle_aware_rejects_removed_mode_parameters(removed: str) -> None:
    def build(m, domain):
        _ = (m, domain)

    with pytest.raises(TypeError, match=f"no longer accepts {removed}"):
        pycircuit.compile_cycle_aware(build, **{removed: True})


@pytest.mark.parametrize(
    "removed", ["design_ctx", "eager", "structural", "value_params"]
)
def test_build_cycle_aware_rejects_removed_mode_parameters(removed: str) -> None:
    def build(m, domain):
        _ = (m, domain)

    with pytest.raises(TypeError, match=f"no longer accepts {removed}"):
        pycircuit.build_cycle_aware(build, **{removed: True})


def test_build_cycle_aware_preserves_structural_and_rejects_value_params() -> None:
    @pycircuit.module(structural=True)
    def structural(m, domain):
        m.output("result", pycircuit.wire_of(domain.create_const(0, width=1)))

    structural_mlir = pycircuit.build_cycle_aware(structural).emit_mlir()
    assert 'pyc.emit.structural = "true"' in structural_mlir

    @pycircuit.module(value_params={"gain": "i8"})
    def dynamic(m, domain, gain):
        _ = (m, domain, gain)

    with pytest.raises(TypeError, match="does not support runtime value_params"):
        pycircuit.build_cycle_aware(dynamic)


def test_build_cycle_aware_emits_hardened_hierarchy() -> None:
    def child(m, domain, *, inputs, prefix="child"):
        value = pycircuit.submodule_input(
            inputs, "value", m, domain, prefix=prefix, width=8
        )
        result = value + 1
        m.output("result", pycircuit.wire_of(result))
        return {"result": result}

    def top(m, domain):
        value = pycircuit.cas(domain, m.input("value", width=8), cycle=0)
        result = domain.call(child, inputs={"value": value}, prefix="u_child")
        m.output("result", pycircuit.wire_of(result["result"]))

    circuit = pycircuit.build_cycle_aware(top, hierarchical=True)
    mlir = circuit.emit_mlir()

    assert mlir.count("func.func @") == 2
    assert "pyc.instance " in mlir and "callee = @child" in mlir
    assert 'pyc.frontend.contract = "pycircuit"' in mlir


def test_build_cycle_aware_names_hierarchical_specializations_by_params() -> None:
    def child(m, domain, *, inputs, prefix="child", increment=1):
        value = pycircuit.submodule_input(
            inputs, "value", m, domain, prefix=prefix, width=8
        )
        result = value + increment
        m.output("result", pycircuit.wire_of(result))
        return {"result": result}

    def top(m, domain):
        value = pycircuit.cas(domain, m.input("value", width=8), cycle=0)
        first = domain.call(
            child, inputs={"value": value}, prefix="u_first", increment=1
        )
        second = domain.call(
            child, inputs={"value": value}, prefix="u_second", increment=2
        )
        m.output("first", pycircuit.wire_of(first["result"]))
        m.output("second", pycircuit.wire_of(second["result"]))

    mlir = pycircuit.build_cycle_aware(top, hierarchical=True).emit_mlir()
    specializations = set(re.findall(r"func\.func @(child__p[0-9a-f]{8})", mlir))

    assert len(specializations) == 2
    for specialization in specializations:
        assert f"callee = @{specialization}" in mlir


def test_cycle_aware_bitwise_or_rejects_description_strings() -> None:
    circuit = pycircuit.CycleAwareCircuit("description_string")
    domain = circuit.create_domain("clk")
    value = pycircuit.cas(domain, circuit.input("value", width=8), cycle=0)
    forward = domain.signal(width=8, name="forward")

    with pytest.raises(TypeError, match="unsupported operand"):
        _ = value | "description"
    with pytest.raises(TypeError, match="unsupported operand"):
        _ = forward | "description"


def test_pyc6_data_model_is_scalar_only() -> None:
    from pycircuit.data import Data

    with pytest.raises(ValueError, match="unsupported type literal"):
        Data.from_str("vector<2xi8>")


def test_cycle_aware_domain_factories_return_current_cycle_signals() -> None:
    circuit = pycircuit.CycleAwareCircuit("domain_factories")
    domain = circuit.create_domain("clk")
    domain.next()

    value = domain.create_signal("value", width=8)
    constant = domain.create_const(3, width=8)
    reset = domain.create_reset()
    via_circuit = circuit.input_signal("other", 8, domain)
    const_via_circuit = circuit.const_signal(4, 8, domain)

    for signal in (value, constant, reset, via_circuit, const_via_circuit):
        assert type(signal) is pycircuit.CycleAwareSignal
        assert signal.cycle == 1

    compiled = pycircuit.compile_cycle_aware(_domain_factory_build)
    compiled_mlir = compiled.emit_mlir()
    assert 'pyc.frontend.contract = "pycircuit"' in compiled_mlir
    assert "pyc.reset_active" in compiled_mlir
    assert "pyc.select" in compiled_mlir


def test_cycle_aware_constructors_reject_unrepresented_metadata() -> None:
    assert list(inspect.signature(pycircuit.Circuit.create_domain).parameters) == [
        "self",
        "name",
    ]
    assert list(
        inspect.signature(pycircuit.CycleAwareCircuit.create_domain).parameters
    ) == ["self", "name"]
    assert list(
        inspect.signature(pycircuit.CycleAwareDomain.create_const).parameters
    ) == [
        "self",
        "value",
        "width",
        "signed",
    ]

    circuit = pycircuit.CycleAwareCircuit("phantom_parameters")
    with pytest.raises(TypeError, match="frequency_desc"):
        circuit.create_domain("clk", frequency_desc="100MHz")  # type: ignore[call-arg]
    with pytest.raises(TypeError, match="reset_active_high"):
        circuit.create_domain("clk", reset_active_high=True)  # type: ignore[call-arg]
    domain = circuit.create_domain("clk")
    with pytest.raises(TypeError, match="name"):
        domain.create_const(1, width=1, name="one")  # type: ignore[call-arg]


def test_cycle_aware_and_structural_mux_have_stable_return_types() -> None:
    circuit = pycircuit.CycleAwareCircuit("mux_boundaries")
    domain = circuit.create_domain("clk")
    cond = circuit.input("cond", width=1)
    lhs = circuit.input("lhs", width=8)
    rhs = circuit.input("rhs", width=8)

    selected_wire = pycircuit.structural.mux(cond, lhs, rhs)
    with pytest.raises(TypeError, match="pycircuit.structural.mux"):
        pycircuit.mux(cond, lhs, rhs)
    selected_cas = pycircuit.mux(pycircuit.cas(domain, cond), lhs, rhs)

    assert type(selected_wire) is pycircuit.Wire
    assert type(selected_cas) is pycircuit.CycleAwareSignal


def test_priority_encode_is_vendor_neutral_on_the_public_pyc6_surface() -> None:
    circuit = pycircuit.CycleAwareCircuit("priority")
    domain = circuit.create_domain("clk")
    mask = domain.create_signal("mask", width=13)

    result = pycircuit.priority_encode(mask, order="high")

    assert result.index.width == 4
    assert result.valid.width == 1
    assert result.index.cycle == result.valid.cycle == domain.cycle_index
    mlir = circuit.emit_mlir()
    assert "pyc.priority_encode" in mlir
    assert 'order = "high"' in mlir
    assert "basejump" not in mlir.lower()


def test_priority_encode_rejects_noncanonical_order() -> None:
    circuit = pycircuit.CycleAwareCircuit("bad_priority")
    domain = circuit.create_domain("clk")
    mask = domain.create_signal("mask", width=4)

    with pytest.raises(ValueError, match="'low' or 'high'"):
        pycircuit.priority_encode(mask, order="middle")


def test_popcount_is_exact_width_and_vendor_neutral_on_pyc6() -> None:
    circuit = pycircuit.CycleAwareCircuit("popcount")
    domain = circuit.create_domain("clk")
    value = domain.create_signal("value", width=13)

    count = pycircuit.popcount(value)

    assert count.width == 4
    assert count.cycle == domain.cycle_index
    mlir = circuit.emit_mlir()
    assert "pyc.popcount" in mlir
    assert "bsg_" not in mlir.lower()


def test_structural_circuit_popcount_emits_the_same_semantic_op() -> None:
    circuit = pycircuit.Circuit("structural_popcount")
    value = circuit.input("value", width=13)

    count = circuit.popcount(value)
    circuit.output("count", count)

    assert count.width == 4
    mlir = circuit.emit_mlir()
    assert "pyc.popcount" in mlir
    assert "bsg_" not in mlir.lower()


def test_count_leading_zeros_is_exact_width_and_cycle_aware() -> None:
    circuit = pycircuit.CycleAwareCircuit("count_leading_zeros")
    domain = circuit.create_domain("clk")
    value = domain.create_signal("value", width=13)

    count = pycircuit.count_leading_zeros(value)

    assert count.width == 4
    assert count.cycle == domain.cycle_index
    mlir = circuit.emit_mlir()
    assert "pyc.count_zeros" in mlir
    assert 'direction = "leading"' in mlir
    assert "lzc" not in mlir.lower()


def test_structural_count_leading_zeros_emits_the_same_semantic_op() -> None:
    circuit = pycircuit.Circuit("structural_count_leading_zeros")
    value = circuit.input("value", width=13)

    count = circuit.count_leading_zeros(value)
    circuit.output("count", count)

    assert count.width == 4
    mlir = circuit.emit_mlir()
    assert "pyc.count_zeros" in mlir
    assert 'direction = "leading"' in mlir


def test_count_trailing_zeros_uses_the_same_parameterized_semantic_family() -> None:
    circuit = pycircuit.CycleAwareCircuit("count_trailing_zeros")
    domain = circuit.create_domain("clk")
    value = domain.create_signal("value", width=13)

    count = pycircuit.count_trailing_zeros(value)

    assert count.width == 4
    assert count.cycle == domain.cycle_index
    mlir = circuit.emit_mlir()
    assert "pyc.count_zeros" in mlir
    assert 'direction = "trailing"' in mlir


def test_pyc5_module_is_not_shipped_as_a_compatibility_surface() -> None:
    assert importlib.util.find_spec("pycircuit.v5") is None


def test_runtime_and_trace_identifiers_are_pyc6_only() -> None:
    root = Path(__file__).resolve().parents[2]
    contract_files = (
        root / "CMakeLists.txt",
        root / "library/cpp/CMakeLists.txt",
        root / "library/cpp/pyc_runtime.cpp",
        root / "library/cpp/pyc_trace_bin.hpp",
        root / "python/pycircuit/src/pycircuit/cli.py",
        root / "compiler/mlir/tools/pycc.cpp",
        root / "flows/tools/gen_cmake_from_manifest.py",
        root / "flows/tools/dump_pyctrace.py",
    )
    text = "\n".join(path.read_text(encoding="utf-8") for path in contract_files)

    assert "pyc6_runtime" in text
    assert "PYC6TRC3" in text
    assert "pyc4_runtime" not in text
    assert "PYC4TRC2" not in text
    assert "PYC4TRC3" not in text


def test_repository_flow_pythonpath_includes_the_shared_semantic_core() -> None:
    root = Path(__file__).resolve().parents[2]
    completed = subprocess.run(
        ["bash", "-c", "source flows/scripts/lib.sh; pyc_pythonpath"],
        cwd=root,
        check=True,
        capture_output=True,
        text=True,
    )
    entries = completed.stdout.strip().split(":")
    assert str(root / "python/semantic-core/src") in entries
    assert str(root / "python/pycircuit/src") in entries
