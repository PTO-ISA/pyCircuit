# Repository layout follow-up

The repository now keeps four distinct product roles separate: public examples
teach supported authoring, integration fixtures prove correctness, benchmarks
measure performance, and consumer designs remain out of tree. This follows the
same separation visible in CIRCT (`examples`, `test`, `unittests`, and
`integration_test`), MLIR (`examples`, `benchmark`, `test`, and `unittests`),
and mature Python projects such as PyTorch and scikit-learn, which keep examples,
tests, tools, and benchmarks in distinct roots.

FastFwd's public design and short testbench remain under
`examples/pycircuit/features/fastfwd`. Its generated-C++ workload, performance
metrics, and deterministic parameter sweep now live together under
`benchmarks/pycircuit/fastfwd`. The former `contrib/fastfwd` RTL and SystemVerilog
stubs were unreferenced, described an external exam integration, and were
removed rather than preserved as a second design surface.

Review also found that the public Agentic memory harnesses and documentation had
drifted from exact-width `gfsim::UInt`, one-proposal-per-epoch Queue semantics,
and the integrated repository paths. All four documented examples now generate,
compile, and run from the current checkout. Release-layout and unit checks lock
the new roots and reject restoration of the deprecated FastFwd locations.
