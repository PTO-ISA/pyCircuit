# ACIR-to-gfsim Reference Generation Contract

## Purpose

The DavinciOO snapshot is an executable target shape for future ACIR lowering.
It is not the implementation template itself. The generator should reproduce
the model's static topology and observable scheduling contracts using Agentic
Circuit's supported gfsim runtime rather than copying source text from this
directory.

## Generation boundary

| Reference area | Future ownership | Required ACIR information |
| --- | --- | --- |
| `model_top/core.*` | Generated topology and wiring | module instances, arrays, queue endpoints, capacities, latencies, stable paths |
| `model_top/core_system.*` | Generated system wrapper | root module, clock/time domain, reset, termination and observation policy |
| `frontend/trace_source_module.*` | Generated component instantiation around a reusable provider | one typed trace owner, output protocol and backpressure |
| `backend/dispatch.*`, `rename.*` | Reusable component providers selected by static bindings | typed ports, resource parameters, dependency/rename policy identity |
| `backend/issue_queue.*`, engine classes and `rob.*` | Reusable component providers selected by static bindings | queue/resource parameters, arbitration order, latency policy, activation edges |
| `include/davincioo/model/pto_inst.hpp` | Typed packet/value schema or provider-local ABI | opcode, operands, engine class, identity and committed timestamps |
| `model_top/cli.*`, trace parsing and trace exporters | Harness/tooling, not model topology | run manifest, canonical trace input and observation output contracts |
| `include/davincioo/model/framework.hpp` | Must not be generated | replaced by the repository `gfsim` runtime ABI |

## Invariants to preserve

Generated output must preserve these semantic properties rather than the
reference model's filenames or class spellings:

1. The object graph, queue ownership, capacities, latencies and engine counts
   are fully static after construction.
2. Trace records enter through exactly one typed trace-source owner.
3. Rename and readiness are committed before dependent issue becomes visible.
4. Each issue queue uses deterministic oldest-ready arbitration.
5. Scalar, vector, cube and TMA engines can execute independent instructions
   concurrently while observing finite capacity.
6. Completion makes destinations ready; retirement remains ordered by the
   architectural sequence contract.
7. Work, arbitration and Xfer state remain separated, and observations describe
   committed state only.
8. Generated model identity, hierarchy, results, statistics and event ordering
   are independent of pointers, allocation order and host scheduling.

## Comparison strategy

The first generator milestone lowers
[`davincioo_queue_model.py`](../../../examples/agentic-circuit/pipelines/davincioo_queue_model.py)
through Queue/Var ACIR into normal Agentic Circuit generated C++. Its checked-in
projection consumes this snapshot's 15-record softmax trace and compares opcode
counts, out-of-order completion, in-order retirement, architectural values and
the 453-cycle completion contract. The same frozen ACIR also lowers through
PYC C++ and Verilog, where cycle-level equivalence is checked independently.

The generated model now uses explicit `ac.dependency` predecessor tracking,
per-resource reservation, and execution countdown; dependency-wait time is not
folded into token latency.
The projection retains a fixed 5-cycle ingress and 4-cycle drain compensation
for the different Queue boundaries. It therefore proves contract-equivalent
committed behavior for this bounded trace, not internal per-unit issue/ROB
occupancy equivalence. Later milestones may add memory and credit blocks without
changing the trace or output contract. Byte-identical C++ is not
required; contract-equivalent committed output is the acceptance criterion.

The imported source is frozen by [`UPSTREAM_FILES.sha256`](UPSTREAM_FILES.sha256),
so changes in the comparison oracle are explicit and reviewable.
