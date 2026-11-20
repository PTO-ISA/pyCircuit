# SparseFA NDF authoring contract

This directory is the normative architecture specification for the
SparseAttention accelerator. Read [README.md](README.md) before editing it.

- Preserve frozen L0 records. L1 defines behavior and interfaces; L2 defines modules,
  storage, data paths, resource capacity and sustained resource throughput;
  L3 defines structural and functional conformance criteria.
- Keep precomputed Index input, ordered 64-item batches, shared K=V cache,
  independent head state, gather, buffering, masks and atomic commits explicit.
- Use two bounded Index slots. A validated batch may start KV gather immediately;
  load the next Index batch in parallel. Keep duplicate detection across batches
  and withhold output publication until the complete list succeeds.
- Do not put operator or workload latency, cycle formulas, issue-cycle bounds,
  tile cadence, speedup, or measured timing traces in NDF. Place them in the
  independent timing-model/test documentation. A resource throughput contract
  such as 256 B/cycle vector or 32 FP32 exp elements/cycle is allowed.
- Preserve ordering and handshake constraints needed for correctness, including
  validation of each Index batch before its KV gather, global duplicate checks,
  final output gating and drain before slot reuse.
- When changing an NDF page, remove any legacy timing claim in the affected
  section. Do not turn simulation assumptions into architecture requirements.
