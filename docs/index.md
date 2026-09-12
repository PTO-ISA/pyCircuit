# pyCircuit 6 Documentation

pyCircuit 6 is a Python hardware construction language built around
CycleAwareSignal. The frontend tracks logical-cycle provenance, lowers
automatic pipeline balancing to explicit `pyc` MLIR, and emits C++ and Verilog
from the same verified design.

The repository also hosts the separate `agentic_circuit` architecture-modeling
frontend, ACIR/ACSim dialects, and gfsim runtime. Synthesizable ACIR models join
the pyCircuit 6 flow at verified PYC; the Python and MLIR frontend namespaces do
not collapse into one API.

## Documentation map

| Directory | Audience and content |
| --- | --- |
| `getting-started/` | Installation, frontend selection, quickstart, and tutorial |
| `reference/` | Normative language, API, PYC IR, diagnostics, and testbench contracts |
| `architecture/` | Compiler, cycle balancing, and simulation design |
| `acir/` | Agentic Circuit and ACIR specifications, designs, and historical provenance |
| `development/` | Contributor workflows, gates, release contracts, and inventories |
| `rfcs/` | Accepted decisions and active proposals |
| `research/` | Comparative studies that inform, but do not define, the product contract |
| `gates/` | Decision status plus immutable validation evidence |
| `legal/` | Licensing and provenance records |

Only `index.md` and the active `pyc6-plan.md` remain at the documentation root.
Historical gate logs retain their original paths.

## Start here

- [Install pyCircuit](getting-started/installation.md)
- [Choose between `pycircuit` and `agentic_circuit`](getting-started/choose-a-frontend.md)
- [Follow the V6 tutorial](getting-started/tutorial.md)
- [Read the V6 language specification](reference/language.md)
- [Understand the software architecture](architecture/overview.md)

## Core contracts

- CycleAwareSignal is the canonical scalar signal model.
- `domain.next()` advances the authoring-time logical cycle.
- `domain.signal()` plus `<<=` or `.assign()` infers state.
- Mixed-cycle expressions are balanced with explicit delay registers.
- MLIR defines semantics; C++ and Verilog must remain equivalent.
- TICK-OBS and XFER-OBS define backend-stable observation points.

## Reference

- [Frontend API](reference/frontend-api.md)
- [Testbench API](reference/testbench.md)
- [Primitive reference](reference/primitives.md)
- [IR specification](reference/pyc-ir.md)
- [Diagnostics](reference/diagnostics.md)
- [Sidecar schedule](reference/sidecar-schedule.md)
- [Agentic Circuit and ACIR](acir/index.md)
- [Historical repository record](acir/spec/refs/history.md)

## Development and governance

- [Development guide](development/index.md)
- [Testing and gates](development/testing-and-gates.md)
- [Repository management](development/repository-management.md)
- [Repository layout](development/repository-layout.md)
- [pyCircuit 6 decisions](rfcs/pyc6-decisions.md)
- [pyCircuit 6 evolution plan](pyc6-plan.md)

Historical gate logs and compatibility identifiers may retain earlier version
labels. They are evidence and ABI names, not the current product version.
