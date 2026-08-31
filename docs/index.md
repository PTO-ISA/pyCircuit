# pyCircuit 6 Documentation

pyCircuit 6 is a Python hardware construction language built around
CycleAwareSignal. The frontend tracks logical-cycle provenance, lowers
automatic pipeline balancing to explicit `pyc` MLIR, and emits C++ and Verilog
from the same verified design.

## Start here

- [Install pyCircuit](getting-started/installation.md)
- [Follow the V6 tutorial](v6_PyCircuit_Tutorial.md)
- [Read the V6 language specification](v6_PyCircuit_Specification.md)
- [Understand the software architecture](v6_PyCircuit_Software_Architecture.md)

## Core contracts

- CycleAwareSignal is the canonical scalar signal model.
- `domain.next()` advances the authoring-time logical cycle.
- `domain.signal()` plus `<<=` or `.assign()` infers state.
- Mixed-cycle expressions are balanced with explicit delay registers.
- MLIR defines semantics; C++ and Verilog must remain equivalent.
- TICK-OBS and XFER-OBS define backend-stable observation points.

## Reference

- [Frontend API](FRONTEND_API.md)
- [Testbench API](TESTBENCH.md)
- [Primitive reference](PRIMITIVES.md)
- [IR specification](IR_SPEC.md)
- [Diagnostics](DIAGNOSTICS.md)
- [Sidecar schedule](SIDECAR_SCHEDULE.md)

## Development and governance

- [Development guide](development/index.md)
- [Testing and gates](development/testing-and-gates.md)
- [Repository management](development/repository-management.md)
- [pyCircuit 6 decisions](rfcs/pyc6-decisions.md)
- [pyCircuit 6 evolution plan](pyc6-plan.md)

Historical gate logs and compatibility identifiers may retain earlier version
labels. They are evidence and ABI names, not the current product version.
