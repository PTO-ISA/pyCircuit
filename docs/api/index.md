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
    cas,
    compile_cycle_aware,
    mux,
    submodule_input,
    testbench,
    wire_of,
)
```

Use `CycleAwareSignal` for scalar design values. Use `domain.signal()` to infer
state, `domain.next()` to advance logical time, and `wire_of()` only at explicit
I/O boundaries.

## Structural library imports

```python
from pycircuit import Circuit, compile, const, function, module
from pycircuit import ct, hierarchical, lib, logic, spec, wiring
```

The structural surface is supported for explicit hierarchy, compile-time
specialization, reusable library blocks, and static hardware generation. It
does not define a competing timing model.

## Reference documents

- [V6 language specification](../v6_PyCircuit_Specification.md)
- [Frontend API details](../FRONTEND_API.md)
- [Testbench API](../TESTBENCH.md)
- [Primitive reference](../PRIMITIVES.md)
- [IR specification](../IR_SPEC.md)
- [Diagnostics](../DIAGNOSTICS.md)
