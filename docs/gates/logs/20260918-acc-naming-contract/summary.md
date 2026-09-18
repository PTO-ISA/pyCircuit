# ACC two-stage compiler and stable naming closure

Date: 2026-09-18

Decision: 0266

## Result

- Added the wheel-installed `acc.py` source compiler. It reuses the canonical
  Agentic compile pipeline, emits complete selected-system Frozen ACIR, and
  atomically publishes one `.ac` file.
- Added the installed native `acc` compiler. It consumes verified Frozen ACIR
  and emits one concatenated C++ file, the deterministic `multi-tu-v1` model
  bundle, or Verilog through canonical QueueGraph PYC and sibling `pycc`.
- Structured QueueGraph specialization classes always use
  `Module_<ReadableDefinition>_s<16-hex>`, while file names are readable and
  hash-free. All specializations of one definition share
  `Module_<ReadableDefinition>.h/.cpp`; ambiguous readable stems fail closed.
- Adjacent Python `# ndf:` and `# ndf:requires` comments survive source-closure
  flattening as non-semantic ACIR/QueueGraph metadata and appear beside exact
  Python source provenance in generated C++.
- Published the Python-to-ACIR and ACIR-to-C++ naming rules, including the
  boundary between semantic names, complete identities, shortened target
  spellings, compiler-owned wrappers, and local implementation identifiers.
- SDK schemas, examples, relocated-candidate smoke, and developer documentation
  include both launchers while retaining `model plan` and `model emit-cpp` as
  the installed production authority.

## Evidence

- `CodeGenTests`: 172/172 passed, including generated C++ execution, reused
  stateful instances, multi-TU object compilation/link, and permanent
  specialization suffixes.
- Focused `acc` lit: 2/2 passed. The first case covers single C++, bundle and
  Verilog output, invalid mode, unverified input, injected `pycc` failure,
  existing-output preservation, and no partial publication. The second executes
  the real `acc.py -> .ac -> acc -> C++ DUT` and verifies Python rule behavior.
- Python frontend/JIT/CLI/plan tests: 248/248 passed, including single-file and
  multi-file NDF preservation.
- Root unit suite: 153/153 passed.
- Verilog primitive-selection system suite: 18/18 passed.
- SDK release-contract tests: 9/9 passed.
- Repository contracts, diagnostic catalog, API hygiene, strict MkDocs,
  clang-format, Python syntax, and diff hygiene passed.
- A direct command smoke completed C++, bundle, and Verilog paths. The generated
  C++ DUT executed, and the Verilog passed Verilator lint.
- The parser extracted `DAV-SPE-IEX-ALU-0001` and its required NDF set from the
  freshly pulled SuperScalarModel `alu_pipeline` source without adapting its
  existing comment style.

## Boundary and limits

- `.ac` is whole-system Frozen ACIR. Decision 0264 per-module ACIR dumps remain
  inspection artifacts, not independently linkable inputs.
- `.ac` authenticates Frozen ACIR semantics but carries no producer SDK
  manifest. Portable installed-consumer generation remains on the manifest-bound
  `model plan` / `model emit-cpp` path.
- Nominal payload/type spellings are unchanged because existing consumers may
  refer to them. This change stabilizes structured module implementation names
  and files.
- Verilog emission accepts the current flat QueueGraph PYC profile. Structured
  module-preserving QueueGraph input is covered by an explicit no-output
  rejection until canonical PYC hierarchy lowering is implemented.
- The full model-plan negative suite's native-extension tamper case could not
  run in this local mixed-Python installation: the installed SDK extension is
  under Python 3.14 while that test class requires Python 3.11. The positive
  structured plan/emit case and all inventory tests passed under Python 3.11.
- Checkout-wide Agentic lit completed 241/250. The nine remaining failures are
  existing tool/fixture baseline gaps outside this change: hierarchy/memory and
  process-model verifier fixtures, three PYC provenance tests whose configured
  `pyc-opt` lacks the requested passes, freeze-topology, and verify-model. All
  eleven changed ACC/module-generation lit cases passed.
- The freshly pulled SuperScalarModel runner still targets a retired port map,
  and its CLI runtime suite is intentionally skipped upstream. Framework gates,
  not that skipped consumer suite, provide the executable evidence here.
