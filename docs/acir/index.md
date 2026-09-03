# Agentic Circuit and ACIR

Agentic Circuit is the architecture-modeling frontend hosted in the pyCircuit
repository. It retains its own Python distribution, import namespace, command
line interface, MLIR dialect, schemas, and simulator backend while sharing one
repository, release authority, LLVM/MLIR toolchain, and review process with
pyCircuit 6.

## Contract boundaries

- `agentic_circuit` captures architecture, process, resource, and queue models.
- ACPy schema `agentic-circuit-acpy` version `0.1`, contract epoch `0.4`, is the
  stable frontend interchange contract.
- ACIR (`ac`) is an upper-level MLIR dialect. It remains separate from the PYC
  hardware dialect.
- ACSim is the canonical simulator-oriented lowering of ACIR.
- gfsim executes generated ACSim C++ models and remains independent of
  `libpyc6_runtime`.
- The synthesizable ACIR subset lowers through PYC and the normal pyCircuit 6
  `pycc` flow to C++ simulation and Verilog.
- Cycle-Aware Signal remains the canonical pyCircuit 6 hardware-authoring
  model. ACIR-to-PYC adapts to that accepted semantic contract.

## Source layout

Agentic Circuit is integrated into the repository by responsibility:

| Path | Responsibility |
| --- | --- |
| `python/agentic-circuit/` | Python frontend, ACPy, CLI, workspace and JIT APIs |
| `compiler/acir/` | ACIR/ACSim dialects, analysis, transformations and code generation |
| `simulator/gfsim/` | Architecture simulator library |
| `schemas/agentic-circuit/` | Machine-readable public contracts and schemas |
| `tools/agentic-circuit/` | Repository maintenance and contract tools |
| `tests/*/agentic-circuit/` | MLIR, Python, end-to-end and C++ coverage |
| `docs/acir/spec/` | Detailed ACIR and frontend specification |

These module boundaries preserve ACIR semantics without creating a nested
standalone workspace. The source is versioned and released only from
`PTO-ISA/pyCircuit`.

## Public names

The two frontend namespaces are intentionally separate:

```python
import pycircuit
import agentic_circuit
```

Do not re-export Agentic Circuit's generic `module`, `process`, `queue`,
`pipeline`, or `memory` names from `pycircuit`. Both frontends may lower toward
PYC, but they do not share a top-level Python API.

## Verification

AC changes must run the applicable AC G0/G1/G2 lanes documented in
[Testing and Gates](../development/testing-and-gates.md). Integration changes
must also run the full pyCircuit 6 closure because ACIR-to-PYC consumes current
PYC, `pycc`, and `libpyc6_runtime` contracts.

Detailed AC specifications remain under `docs/acir/spec/`. The pyCircuit 6 specification remains
the authority for PYC and Cycle-Aware Signal semantics.
