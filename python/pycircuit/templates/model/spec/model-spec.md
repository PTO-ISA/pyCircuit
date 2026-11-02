---
doc_id: MODEL-DOC-SPEC
status: draft
authority: normative
owner: telemetry-model
---

<!-- markdownlint-disable MD022 -->

# Implementation specification

## State and transformation {#MODEL-SPEC-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=behavior status=draft refines=MODEL-REQ-001 -->

Worker MUST compute value XOR mask for accepted requests. In the compute
variant the checksum MUST become that normalized value; in stateful and
backpressure variants it MUST accumulate normalized values. A zero mask MUST
increment rejected and completed but leave the checksum unchanged. Worker MUST
own these state updates; the Pipeline MUST own accepted count and transport.

## Execution and transport {#MODEL-SPEC-002}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=behavior status=draft refines=MODEL-REQ-001 -->

Pipeline MUST accept enabled requests, invoke Worker exactly once per accepted
request and publish valid responses in order. Timing execution MUST retain the
declared queue capacity and latency; functional execution MAY fold only waits
proven by the compiler's supported execution profile. Backend selection MUST
consume the same verified final artifact for each execution policy.
