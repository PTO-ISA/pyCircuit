"""Compile-time Python declarations for pyCircuit hardware designs.

The compiler captures source syntax without executing Python models. Compile
one source with ``pycircuit compile``; execute generated designs via the runtime.
"""

__all__ = [
    "module",
    "rule",
    "struct",
    "bits",
    "table",
    "queue",
    "encoding",
    "enum_to_bits",
    "enum_from_bits",
    "concat",
    "popcount",
    "count_leading_zeros",
    "count_trailing_zeros",
    "priority_encode",
    "onehot_encode",
    "system",
    "dff",
    "dffe",
    "sync_mem",
    "sync_mem_dp",
    "byte_mem",
    "log",
    "report",
]
__version__ = "6.1.0"


def module(declaration):
    """Declare a hardware module in compiler input."""
    raise RuntimeError("pyCircuit declarations require pycircuit compile")


def rule(declaration):
    """Declare a rule whose computations and state updates are compiler owned."""
    raise RuntimeError("pyCircuit declarations require pycircuit compile")


def struct(declaration):
    """Declare an ordered nominal record in compiler input."""
    raise RuntimeError("pyCircuit declarations require pycircuit compile")


def encoding(**_parameters):
    """Declare a compiler-owned enum encoding without executing the class."""
    raise RuntimeError("pyCircuit declarations require pycircuit compile")


def enum_to_bits(_value):
    """Expose an enum carrier through an explicit compiler-owned conversion."""
    raise RuntimeError("pyCircuit values require pycircuit compile")


def concat(*values):
    """Concatenate fixed-bit values, most significant operand first."""
    raise RuntimeError("pyCircuit values require pycircuit compile")


def popcount(value):
    """Declare a compiler-owned unsigned fixed-bit population count."""
    raise RuntimeError("pyCircuit values require pycircuit compile")


def count_leading_zeros(value):
    """Count consecutive zeros from a fixed-bit value's most significant end."""
    raise RuntimeError("pyCircuit values require pycircuit compile")


def count_trailing_zeros(value):
    """Count consecutive zeros from a fixed-bit value's least significant end."""
    raise RuntimeError("pyCircuit values require pycircuit compile")


def priority_encode(value, *, order="low"):
    """Declare a compiler-owned fixed index and Boolean validity result."""
    raise RuntimeError("pyCircuit values require pycircuit compile")


def onehot_encode(value, *, order="low"):
    """Declare a fixed index and Boolean validity/conflict results."""
    raise RuntimeError("pyCircuit values require pycircuit compile")


class _SourceType:
    """Syntax marker only; type checking and allocation belong to MLIR."""

    def __getitem__(self, _arguments):
        raise RuntimeError("pyCircuit types require pycircuit compile")

    def __call__(self, *_arguments, **_keywords):
        raise RuntimeError("pyCircuit values require pycircuit compile")


bits = _SourceType()
table = _SourceType()
queue = _SourceType()
enum_from_bits = _SourceType()
for _width in range(1, 65):
    _name = f"u{_width}"
    globals()[_name] = _SourceType()
    __all__.append(_name)
del _width, _name


def system(declaration):
    """Declare a system in compiler input; capability checks happen in MLIR."""
    raise RuntimeError("pyCircuit declarations require pycircuit compile")


def _primitive(**_parameters):
    """Reject host execution of a compiler-owned standard hardware leaf."""
    raise RuntimeError("pyCircuit primitives require pycircuit compile")


dff = _primitive
dffe = _primitive
sync_mem = _SourceType()
sync_mem_dp = _SourceType()
byte_mem = _SourceType()


def log(level, event, *items):
    """Declare a compiler-owned observation without host I/O."""
    raise RuntimeError("pyCircuit observations require pycircuit compile")


def report(name, value):
    """Declare a compiler-owned gauge without host state."""
    raise RuntimeError("pyCircuit observations require pycircuit compile")
