# Examples

Examples are grouped by purpose. Product releases version the repository; the
source tree does not keep parallel versioned or phase-numbered example sets.

- [`pipelines`](pipelines/README.md): Queue/Var composition and control flow.
- [`memory`](memory/README.md): explicit memory, banking, latency, and DMA.
- [`blocks`](blocks/README.md): parameterized reusable building blocks.
- [`architecture`](architecture/README.md): complete generated architecture models.
- [`state`](state/README.md): committed Table state plus the epoch 0.5
  `@ac.rule` bounded-retirement example.
- [`workspaces`](workspaces/README.md): CLI build/run/replay workspaces with goldens.

Imported upstream source is not an example. It lives under [`references`](../../third_party/references/README.md).
Test-owned generated output lives under [`tests/goldens`](../../tests/goldens/agentic-circuit/README.md).
