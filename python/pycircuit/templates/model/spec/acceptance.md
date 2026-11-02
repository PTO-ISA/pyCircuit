---
doc_id: MODEL-DOC-ACCEPTANCE
status: draft
authority: normative
owner: telemetry-model
---

<!-- markdownlint-disable MD022 -->

# Verification criteria

## Independent packet and state checks {#MODEL-VER-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=MODEL-REQ-001,MODEL-SPEC-001,MODEL-SPEC-002 -->

The verifier MUST compare each functional completion, valid publication order
and final state against independent expected.json vectors in both execution
policies and both backends. Missing tools, failed execution or mismatched values
MUST fail verification. Record commands, source revision, outputs and exit status.
Add independent capacity, latency, reset and failure oracles before claiming full
timing or reset conformance; the supplied vector check does not establish those.
