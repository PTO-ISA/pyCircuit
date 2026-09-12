# Changelog

All notable user-facing changes to pyCircuit and the integrated Agentic Circuit
toolchain are recorded here. Release artifacts and immutable source tags are
published through the repository's release workflow.

## Unreleased

- Add a Pythonic frontend guide for coding agents, covering Cycle-Aware Signal,
  structural modules, Agentic rules, complex-design decomposition, maintained
  examples, failure modes, validation, and handoff evidence.
- Professionalize repository navigation and onboarding: separate pyCircuit
  tools from flow internals, retire completed migration pages and obsolete
  design docs, remove name-based ACIR-to-ACSim device inference, rename the
  active generated dispatch ABI, and refresh README/getting-started content.
- Consolidate FastFwd performance harnesses and design-space exploration under
  `benchmarks/pycircuit/fastfwd`, remove the ambiguous `contrib/fastfwd` root,
  and tighten repository layout checks for public examples and benchmarks
  (Decision 0157).
- Preserve rule, state, port, local, and project-relative source provenance in
  QueueGraph/GFSim output and render state transitions through reviewable named
  components without changing runtime behavior or ABI (Decision 0244, issue
  #106).
- Add typed pure ACPy helpers for rule and Queue expressions, including explicit
  `@ac.inline`, bounded conditional/local lowering, ordinary GFSim functions,
  and call-free PYC legalization (Decision 0243, issue #104).
- Classify every public pyCircuit example as a basic, feature, or application;
  gate all 28 examples; move large framework fixtures under integration tests;
  and separate benchmark sources from generated profiles.
- Reorganize documentation by audience and contract type.
- Close repository consistency gaps in the optimizer driver, source-resource
  discovery, CLI exit policy, examples, and verification coverage.

## 6.0.0 - 2026-09-10

- Establish Cycle-Aware Signal as the primary pyCircuit 6 authoring model.
- Unify structural and cycle-aware authoring on verified PYC semantics and the
  `libpyc6_runtime` C++/Verilog backend contract.
- Integrate Agentic Circuit as the separate ACPy/ACIR frontend and gfsim model
  runtime within the canonical PTO-ISA/pyCircuit repository.
- Add `pyc.concat` lowering for readable `{a, b, c}` packed concatenations in
  generated Verilog and C++.
- Improve generated identifier readability and traceability with scope and
  source-location name mangling.
- Add hierarchical instance input-change caching, including commit preservation
  for combinational-only callees and opt-out controls.
- Add simulator performance controls and optional SCC worklist scheduling.
- Add versioned input caching and unchanged-input/output fast paths for runtime
  memory and FIFO primitives.
