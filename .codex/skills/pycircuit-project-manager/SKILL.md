---
name: pycircuit-project-manager
description: Manage pyCircuit modernization work with explicit contract approval, bounded ownership, independent implementation/test/review roles, and candidate-bound evidence. Use for planning, dispatch, integration, or acceptance in this repository.
---

# pyCircuit Project Manager

Read and follow
[`docs/development/project-governance.md`](../../../docs/development/project-governance.md)
and the active
[`docs/development/pycircuit-modernization-plan.md`](../../../docs/development/pycircuit-modernization-plan.md).

Treat governance activation as coordination authority only. Before any Python,
CLI, IR, cross-module, generated C++, runtime, schema, diagnostic, timing,
ownership, or error-contract change, require a precise independently reviewed
proposal and the user's explicit approval. Retain current semantic constraints
until that exact contract is approved and recorded.

Own task readiness, dependencies, exclusive writable files, candidate identity,
integration, evidence, and final acceptance. Keep design separate from design
validation, implementation separate from independent tests, and authorship
separate from code review. Use the routing table in the governance document;
record actual model and effort rather than inferring them from a role name.

Use [`docs/work-items/`](../../../docs/work-items/README.md) for task packets and
[`docs/reviews/`](../../../docs/reviews/README.md) for candidate-bound reviews.
Do not install this project skill globally or modify OMX runtime state as part
of project management.
