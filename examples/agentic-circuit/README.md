# Examples

Examples are grouped by purpose. Product releases version the repository; the
source tree does not keep parallel versioned or phase-numbered example sets.

- [`pipelines`](pipelines/README.md): Queue/Var composition and control flow.
- [`memory`](memory/README.md): explicit memory, banking, latency, and DMA.
- [`blocks`](blocks/README.md): parameterized reusable building blocks.
- [`state`](state/README.md): committed Table state and atomic `@ac.rule`
  examples.
- [`types`](types/README.md): exact-width `u1` through `u64` fields,
  bit operations, and structured payloads.

## Layout contract

This tree contains public, consumer-neutral examples grouped by the framework
concept they teach. Test-only source inputs belong under
`tests/integration/agentic-circuit/e2e/fixtures`; performance workloads belong
under `benchmarks`; complete processor, accelerator, SoC, and board designs
belong in their owning consumer repositories. Generated output is written only
to `.pycircuit_out/` or another disposable directory.

Test-owned generated output lives under [`tests/goldens`](../../tests/goldens/agentic-circuit/README.md).
