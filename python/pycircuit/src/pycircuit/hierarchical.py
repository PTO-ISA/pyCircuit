"""
Hierarchical Module and Spec Inference for pyCircuit

This module provides:
1. Hierarchical module construction - build complex hierarchies from submodules
2. Spec inference - automatically infer types/signatures from existing hardware
"""

from __future__ import annotations

from collections.abc import Callable, Iterable, Mapping
from dataclasses import dataclass, field
from typing import Any

from .dsl import Module, Signal
from .spec.types import (
    BundleSpec,
    FieldSpec,
    ModuleFamilySpec,
    ParamSet,
    SigLeafSpec,
    SignatureSpec,
    StructFieldSpec,
    StructSpec,
)

# =============================================================================
# Hierarchical Module Support
# =============================================================================


@dataclass
class HierarchicalModuleSpec:
    """Specification for a hierarchical module with submodules."""

    name: str
    submodules: dict[str, HierarchicalModuleSpec | Callable] = field(
        default_factory=dict
    )
    connections: list[tuple[str, str]] = field(default_factory=list)
    params: dict[str, Any] = field(default_factory=dict)

    def add_submodule(self, name: str, spec: HierarchicalModuleSpec | Callable) -> None:
        """Add a submodule to this hierarchical module."""
        self.submodules[name] = spec

    def add_connection(self, src: str, dst: str) -> None:
        """Add a connection between submodules (src -> dst)."""
        self.connections.append((src, dst))


def hierarchical_module(
    name: str,
    *,
    submodules: Mapping[str, Callable] | None = None,
) -> Callable:
    """
    Decorator for creating hierarchical modules.

    Example:
        @hierarchical_module("top", submodules={"alu": alu_module, "reg": reg_module})
        def top(m, domain):
            ...
    """

    def decorator(fn: Callable) -> Callable:
        fn._pyc_hierarchical = True
        fn._pyc_hierarchical_name = name
        fn._pyc_submodules = dict(submodules) if submodules else {}
        return fn

    return decorator


def is_hierarchical_module(fn: Callable) -> bool:
    """Check if a function is a hierarchical module."""
    return getattr(fn, "_pyc_hierarchical", False)


def get_hierarchical_submodules(fn: Callable) -> dict[str, Callable]:
    """Get the submodules of a hierarchical module."""
    return getattr(fn, "_pyc_submodules", {})


# =============================================================================
# Spec Inference
# =============================================================================


def infer_signature_from_module(
    m: Module,
    port_names: Iterable[str] | None = None,
) -> SignatureSpec:
    """
    Automatically infer a SignatureSpec from a module's ports.

    Args:
        m: The module to infer from
        port_names: Optional list of port names to include (None = all ports)

    Returns:
        SignatureSpec inferred from the module

    Example:
        sig = infer_signature_from_module(alu_module)
    """
    leaves: list[SigLeafSpec] = []

    ports = m.port_signals()
    if port_names is not None:
        port_names_set = set(port_names)
        ports = {k: v for k, v in ports.items() if k in port_names_set}

    for name, sig in ports.items():
        direction = "in" if m.is_input(name) else "out"
        width = _get_signal_width(sig)
        signed = _is_signed(sig)
        leaves.append(
            SigLeafSpec(path=name, direction=direction, width=width, signed=signed)
        )

    if not leaves:
        raise ValueError(f"Cannot infer signature from module {m.name}: no ports found")

    return SignatureSpec(name=f"{m.name}_sig", leaves=tuple(leaves))


def infer_struct_from_signals(
    signals: Mapping[str, Signal],
    name: str = "inferred_struct",
) -> StructSpec:
    """
    Automatically infer a StructSpec from a dict of signals.

    Args:
        signals: Dict of signal name -> Signal
        name: Name for the inferred struct

    Returns:
        StructSpec inferred from the signals

    Example:
        struct = infer_struct_from_signals({"data": data_sig, "valid": valid_sig})
    """
    fields: list[StructFieldSpec] = []

    for sig_name, sig in signals.items():
        width = _get_signal_width(sig)
        signed = _is_signed(sig)
        fields.append(StructFieldSpec(name=sig_name, width=width, signed=signed))

    if not fields:
        raise ValueError("Cannot infer struct: no signals provided")

    return StructSpec(name=name, fields=tuple(fields))


def infer_bundle_from_wire(
    wire,
    name: str = "inferred_bundle",
) -> BundleSpec:
    """
    Automatically infer a BundleSpec from a wire/connector.

    Args:
        wire: The wire or connector to infer from
        name: Name for the inferred bundle

    Returns:
        BundleSpec inferred from the wire

    Example:
        bundle = infer_bundle_from_wire(alu_result)
    """
    # This is a placeholder - the actual implementation would
    # inspect the wire's internal structure
    width = getattr(wire, "width", 32)
    return BundleSpec(
        name=name, fields=(FieldSpec(name="data", width=width, signed=False),)
    )


def infer_param_set_from_fn(
    fn: Callable,
    defaults: Mapping[str, Any] | None = None,
) -> ParamSet:
    """
    Automatically infer a ParamSet from a function's parameters.

    Args:
        fn: The function to infer from
        defaults: Optional default values

    Returns:
        ParamSet inferred from function parameters

    Example:
        params = infer_param_set_from_fn(alu, {"width": 32, "stages": 2})
    """
    import inspect

    sig = inspect.signature(fn)
    values: list[tuple[str, Any]] = []

    for param_name, param in sig.parameters.items():
        if param_name in ("m", "domain", "self"):
            continue

        if defaults and param_name in defaults:
            values.append((param_name, defaults[param_name]))
        elif param.default is not inspect.Parameter.empty:
            values.append((param_name, param.default))
        else:
            # Try to infer from annotation
            ann = fn.__annotations__.get(param_name)
            if ann == int:
                values.append((param_name, 0))
            elif ann == bool:
                values.append((param_name, False))
            elif ann == str:
                values.append((param_name, ""))
            else:
                values.append((param_name, 0))

    return ParamSet(tuple(values))


# =============================================================================
# Helper Functions
# =============================================================================


def _get_signal_width(sig: Signal) -> int:
    """Get the width of a signal."""
    if hasattr(sig, "width"):
        return int(sig.width)
    # Try to parse from type
    ty = str(getattr(sig, "ty", "i32"))
    if ty.startswith("i"):
        return int(ty[1:])
    return 32  # default


def _is_signed(sig: Signal) -> bool:
    """Check if a signal is signed."""
    return getattr(sig, "signed", False)


def infer_module_family(
    module_fn: Callable,
    *,
    name: str | None = None,
    params: Mapping[str, Any] | None = None,
) -> ModuleFamilySpec:
    """
    Create a ModuleFamilySpec from a module function.

    Args:
        module_fn: The module function
        name: Optional name (defaults to function name)
        params: Optional parameter defaults

    Returns:
        ModuleFamilySpec for the module

    Example:
        family = infer_module_family(alu_module, name="alu", params={"width": 32})
    """
    fn_name = name or getattr(module_fn, "__name__", module_fn.__class__.__name__)
    param_set = infer_param_set_from_fn(module_fn, params) if params else None

    return ModuleFamilySpec(
        name=fn_name,
        module=module_fn,
        params=param_set,
    )


# =============================================================================
# Auto-connection for Hierarchical Modules
# =============================================================================


@dataclass
class AutoConnect:
    """Helper for automatic wiring in hierarchical modules."""

    module: Module
    instances: dict[str, Any] = field(default_factory=dict)

    def instance(self, name: str, module_fn: Callable, **params) -> Any:
        """Create an instance of a submodule."""
        inst = self.module.instance(module_fn, name, **params)
        self.instances[name] = inst
        return inst

    def connect(self, src: str, dst: str) -> None:
        """Connect two signals by name."""
        src_parts = src.split(".")
        dst_parts = dst.split(".")

        if len(src_parts) < 2 or len(dst_parts) < 2:
            raise ValueError("Connection must be in format: instance.signal")

        inst_src, sig_src = ".".join(src_parts[:-1]), src_parts[-1]
        inst_dst, sig_dst = ".".join(dst_parts[:-1]), dst_parts[-1]

        if inst_src not in self.instances:
            raise ValueError(f"Unknown instance: {inst_src}")
        if inst_dst not in self.instances:
            raise ValueError(f"Unknown instance: {inst_dst}")

        # This would connect the actual signals
        # The actual implementation depends on the IR

    def connect_all(self, connections: Iterable[tuple[str, str]]) -> None:
        """Connect multiple signal pairs."""
        for src, dst in connections:
            self.connect(src, dst)


# =============================================================================
# Type Inference Utilities
# =============================================================================


def infer_width_from_value(value: int) -> int:
    """Infer minimum bit width to represent a value."""
    if value < 0:
        # For negative values, we need sign bit + magnitude
        value = -value - 1
    width = 1
    while (1 << width) <= value:
        width += 1
    return width


def infer_type_from_annotation(annotation: Any) -> tuple[int, bool] | None:
    """
    Infer (width, signed) from a type annotation.

    Returns None if cannot infer.
    """
    ann_str = str(annotation)

    # Handle common patterns
    if ann_str.startswith("i") and ann_str[1:].isdigit():
        return (int(ann_str[1:]), False)
    if ann_str.startswith("s") and ann_str[1:].isdigit():
        return (int(ann_str[1:]), True)

    # Handle Optional, Union, etc.
    if "int" in ann_str.lower():
        return (32, False)
    if "bool" in ann_str.lower():
        return (1, False)

    return None
