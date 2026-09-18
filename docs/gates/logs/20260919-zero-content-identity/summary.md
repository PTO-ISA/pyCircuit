# ACC structural-identity hard break

Date: 2026-09-19

Decision: 0267

## Result

- Removed the ACSim dialect, conversion, C++ generator, build tools, process
  planner, binding registry, public compile/build/model commands, and their
  compatibility tests.
- Verified ACIR and QueueGraph specialization reuse now use explicit definition
  symbols plus canonical typed static arguments.
- Generated module files use source definition names. Generated classes append
  readable parameter names and values only when static arguments differ.
- Removed persistent content-addressed caches and byte-derived identities from
  the Python frontend, ACIR, PYC RTL selection, SDK/release inventories, and
  generated model flow.
- `acc.py -> .ac -> acc` is the only Agentic backend path. C++, bundle, and
  Verilog publication remain transactional.

## Evidence

- Agentic lit: 206/206 passed.
- QueueGraph CodeGen: 110/110 passed.
- Compiler driver/diagnostics: 3/3 passed.
- Model analysis: 33/33 passed.
- Python frontend/CLI/contracts: 458 passed, 1 skipped.
- Root unit tests: 153/153 passed.
- Primitive selection: 18/18 passed.
- Fresh native CTest: 6/6 passed.
- Repository and SDK contracts passed.
- Strict MkDocs, API hygiene, diagnostic catalog, PYC inventory, decision
  coverage, formatting, and diff hygiene passed.
- Active-tree zero-content-identity search returned no matches outside the
  historical evidence directories.

## Boundary

- Git revisions remain exact source-version pins.
- Historical RFC and gate logs remain evidence of the removed architecture;
  they are not active compiler contracts.
- Cross-invocation content-addressed caching is intentionally unavailable.
