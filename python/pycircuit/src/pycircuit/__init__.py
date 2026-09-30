"""Compile-time Python declarations for pyCircuit hardware designs.

The compiler captures source syntax without executing Python models. Compile
one source with ``pycircuit compile``; execute generated designs via the runtime.
"""

__all__ = ["module", "rule", "system", "log", "report"]
__version__ = "6.1.0"


def module(declaration):
    """Declare a hardware module in compiler input."""
    raise RuntimeError("pyCircuit declarations require pycircuit compile")


def rule(declaration):
    """Declare a stateless rule in compiler input."""
    raise RuntimeError("pyCircuit declarations require pycircuit compile")


def system(declaration):
    """Declare a system in compiler input; capability checks happen in MLIR."""
    raise RuntimeError("pyCircuit declarations require pycircuit compile")


def log(level, event, *items):
    """Declare a compiler-owned observation without host I/O."""
    raise RuntimeError("pyCircuit observations require pycircuit compile")


def report(name, value):
    """Declare a compiler-owned gauge without host state."""
    raise RuntimeError("pyCircuit observations require pycircuit compile")
