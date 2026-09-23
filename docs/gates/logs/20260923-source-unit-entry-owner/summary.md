# Source-unit implementation provenance

Decisions 0275–0278 require a declaration-owned finite module family and a
source-owned implementation body. A separated `@ac.module_decl` may sort before
its same-name implementation in the import closure. The capture worker formerly
kept the declaration's first location and rejected distinct declaration versus
implementation NDF markers. The emitted implementation then appeared to belong
to the interface source and failed AC package verification with `ACIR-EMIT-002`.

The entry implementation now takes precedence for definition location,
source-node provenance, and NDF metadata. Conflicting metadata among dependency
declarations still fails closed.

## Focused evidence

- The new regression failed before the fix with `ACPY-NDF-001` and passed after.
- `test_source_closure_entry_owner.py` and `test_multi_unit_package.py`: 9 passed.
- Agentic Python suite, with its tool-directory partition excluded and
  `AC_GATE_BUILD_ROOT` pointed at this checkout's native build: 486 passed,
  7 skipped.
- `pytest tests/unit -m unit`: 217 passed.
- `tools/agentic-circuit/check-contracts.py`: passed.
- API hygiene check: passed; `mkdocs build --strict`: passed.
- A separate interface/implementation fixture with different NDF markers
  compiled its implementation and core as independent AC units; native
  `acc -c package -verify` passed using this checkout's built toolchain.
- `git diff --check`: passed.

The fixture is design-neutral. This change does not restore removed CLI flags
or infer specialization cases from callers.
