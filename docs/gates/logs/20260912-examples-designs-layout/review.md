# Examples and designs layout closure

The public pyCircuit catalog is now organized by user intent rather than by a
flat list of unrelated designs: `basics` teaches the smallest concepts,
`features` demonstrates one focused framework capability, and `applications`
contains complete but still framework-owned examples. Discovery records and
validates that category, while unit coverage fixes the public inventory at 28
examples and requires every one to emit canonical PYC.

The former `designs/` root mixed reusable regressions, benchmarks, generated
profiles, and user-facing examples. Generic integration workloads now live in
`tests/integration/pycircuit/fixtures/`, the RegisterFile performance harness
lives in `benchmarks/pycircuit/register_file/`, and tracked profile data and
obsolete one-off build scripts are removed. Release-layout checks reject a
future `designs/` root and generated artifacts in active source trees.

All 28 public designs compile to C++, all 26 normal examples and three moved
fixtures pass the C++/Verilator simulation lane, and the three semantic smoke
designs pass both backends. The slow `bypass_unit` fixture completed after the
per-case compile timeout was raised from 300 to 900 seconds; it showed no
semantic error.
