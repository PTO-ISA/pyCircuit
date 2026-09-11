from __future__ import annotations

import pytest
from pycircuit.diagnostics import (
    PYTHON_DIAGNOSTIC_CODES,
    Diagnostic,
    DiagnosticError,
    PyCircuitError,
    PyCircuitKeyError,
    PyCircuitTypeError,
    PyCircuitValueError,
    make_diagnostic,
)

pytestmark = pytest.mark.unit


@pytest.mark.parametrize(
    ("error_type", "builtin_type", "code"),
    [
        (PyCircuitTypeError, TypeError, "PYC-PY-TYPE"),
        (PyCircuitValueError, ValueError, "PYC-PY-VALUE"),
        (PyCircuitKeyError, KeyError, "PYC-PY-KEY"),
    ],
)
def test_builtin_compatible_errors_carry_structured_diagnostics(
    error_type: type[PyCircuitError], builtin_type: type[Exception], code: str
) -> None:
    error = error_type("invalid authoring input", path="design.py", line=7, col=3)

    assert isinstance(error, PyCircuitError)
    assert isinstance(error, builtin_type)
    assert isinstance(error.diagnostic, Diagnostic)
    assert error.code == code
    assert error.message == "invalid authoring input"
    assert error.location == "design.py:7:3"
    assert str(error) == (
        f"design.py:7:3: [{code}] invalid authoring input\nstage=python"
    )


def test_diagnostic_error_preserves_supplied_diagnostic() -> None:
    diagnostic = Diagnostic(
        code="PYC430",
        stage="jit",
        message="unsupported authoring construct",
        path="design.py",
        line=11,
        col=5,
    )

    error = DiagnosticError(diagnostic)

    assert error.diagnostic is diagnostic
    assert error.code == "PYC430"
    assert error.message == "unsupported authoring construct"
    assert error.location == "design.py:11:5"


def test_python_diagnostic_registry_is_closed() -> None:
    assert len(PYTHON_DIAGNOSTIC_CODES) == 11
    with pytest.raises(ValueError, match="unregistered pyCircuit Python diagnostic"):
        make_diagnostic(
            code="PYC-PY-UNKNOWN",
            stage="python",
            message="unknown diagnostic family",
        )
