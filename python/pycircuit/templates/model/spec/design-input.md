---
doc_id: MODEL-DOC-INPUT
status: draft
authority: normative
owner: telemetry-model
---

<!-- markdownlint-disable MD022 -->

# Design input

This starter is an unapproved draft. Replace its purpose, interfaces, constraints
and acceptance criteria for your design; retain clause identities across revisions.

## Purpose {#MODEL-INTENT-001}
<!-- ndf: kind=requirement modality=must refinement=L0 domain=behavior status=draft -->

The project MUST normalize telemetry requests using one hardware source shared
by functional and timing execution and by C++ and Verilog generation.

## Observable behavior {#MODEL-REQ-001}
<!-- ndf: kind=requirement modality=must refinement=L1 domain=behavior status=draft refines=MODEL-INTENT-001 -->

The model MUST preserve accepted request order, reject zero masks without
publishing a response, and expose the accepted, completed and rejected counts
and final checksum. Disabled or inactive requests MUST NOT change that state.
All 16-bit arithmetic MUST wrap modulo 65536.
