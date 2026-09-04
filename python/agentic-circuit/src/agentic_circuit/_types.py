"""Public annotation categories and frontend-only symbolic values."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Generic, Never, TypeVar


T = TypeVar("T")
P = TypeVar("P")
InterfaceT = TypeVar("InterfaceT")
R = TypeVar("R")


@dataclass(frozen=True, slots=True)
class ScalarType:
    width: int
    signed: bool = False

    def __post_init__(self) -> None:
        if type(self.width) is not int or not 1 <= self.width <= 64:
            raise ValueError("ACPY-TYPE-001: bit width must be in [1, 64]")
        if type(self.signed) is not bool:
            raise TypeError("ACPY-TYPE-001: signed must be bool")


UNSIGNED_WIDTHS = tuple(range(1, 65))
for _width in UNSIGNED_WIDTHS:
    globals()[f"u{_width}"] = ScalarType(_width)
del _width
s8 = ScalarType(8, True)
s16 = ScalarType(16, True)
s32 = ScalarType(32, True)
s64 = ScalarType(64, True)


class Static(Generic[T]):
    """Mark an elaboration-time specialization parameter."""


# The Queue frontend uses the lower-case spelling to make the
# specialization boundary read like a value category rather than a runtime
# container.  Keep ``Static`` available for the existing component frontend;
# both annotations have the same runtime origin.
const = Static


class Flow(Generic[T, P]):
    """Describe a typed logical dataflow edge using protocol ``P``."""


class Endpoint(Generic[InterfaceT, R]):
    """Describe interface ``I`` bound in role ``R``."""


@dataclass(frozen=True, slots=True, eq=False)
class SymbolicValue:
    """Frontend identity for an architecture value without a Python value."""

    stable_name: str
    annotation: object

    def __repr__(self) -> str:
        return f"SymbolicValue({self.stable_name!r})"

    def _reject(self, operation: str) -> Never:
        raise TypeError(
            f"ACPY-STATIC-002: {self.stable_name!r} cannot be used for {operation}"
        )

    def __bool__(self) -> Never:
        return self._reject("truth testing")

    def __int__(self) -> Never:
        return self._reject("integer conversion")

    def __hash__(self) -> Never:
        return self._reject("hashing")

    def __iter__(self) -> Never:
        return self._reject("iteration")

    def __eq__(self, other: object) -> Never:
        return self._reject("equality")

    def __ne__(self, other: object) -> Never:
        return self._reject("equality")


@dataclass(frozen=True, slots=True, eq=False, repr=False)
class ResourceRef(SymbolicValue, Generic[T, R]):
    """A typed resource capability bound in a declared role."""

    role: object

    @property
    def resource_type(self) -> object:
        return self.annotation

    def __repr__(self) -> str:
        return f"ResourceRef({self.stable_name!r})"


def _test_symbolic(stable_name: str, annotation: object) -> SymbolicValue:
    """Create a symbolic value for contract tests without elaboration state."""

    return SymbolicValue(stable_name=stable_name, annotation=annotation)
