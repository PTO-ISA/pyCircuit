# API Reference

The pyCircuit 6 product API is centered on CycleAwareSignal and lowers to the
same `pyc` MLIR used by the structural module API.

## Cycle-aware design imports

```python
from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    CycleAwareSignal,
    CycleAwareTb,
    ForwardSignal,
    Tb,
    build_cycle_aware,
    cas,
    compile_cycle_aware,
    mux,
    submodule_input,
    wire_of,
)
from pycircuit.design import probe, testbench
```

Use `CycleAwareSignal` for scalar design values. Use `domain.signal()` to infer
state, `domain.next()` to advance logical time, and `wire_of()` only at explicit
I/O boundaries. `compile_cycle_aware()` always JIT-compiles to a hardened
`Design`; `build_cycle_aware()` directly executes Python elaboration and returns
a `CycleAwareCircuit` whose MLIR carries the same frontend contract.

## Structural library imports

```python
from pycircuit import Circuit, compile, const, function, module
from pycircuit import ct, hierarchical, lib, logic, spec, structural, wiring
```

The structural surface is supported for explicit hierarchy, compile-time
specialization, reusable library blocks, and static hardware generation. It
does not define a competing timing model. Use `structural.mux()` for raw Wire
selection; top-level `mux()` is CycleAware and always returns a CAS.

## Reference documents

- [V6 language specification](language.md)
- [Frontend API details](frontend-api.md)
- [Testbench API](testbench.md)
- [Primitive reference](primitives.md)
- [IR specification](pyc-ir.md)
- [Diagnostics](diagnostics.md)
