# External release identity hard break

Date: 2026-09-19

Decision: 0268

## Result

- Removed contract/freeze epoch identity from ACIR, ACPy, project configuration,
  diagnostics, capabilities, generated inventories, cost reports, and SDK JSON.
- `.ac` is selected-system verified ACIR; release selection is external package
  metadata plus the exact Git source revision.
- Topology closure remains a compiler verification stage and carries no release
  number or compatibility identity.

## Evidence

- Python frontend/CLI/contracts: 458 passed, 1 skipped.
- Root unit tests: 153/153 passed.
- Agentic lit: 206/206 passed.
- Fresh native CTest: 6/6 passed; QueueGraph CodeGen 110/110 and model
  analysis 33/33 are included.
- Primitive selection: 18/18 passed.
- Official Agentic G0/G1 passed in run
  `20260919-external-release-identity-r3`; corrected G2 passed in run
  `20260919-external-release-identity-r4` for five C++/bundle/Verilog cases.
- Repository and SDK contracts, API hygiene, strict MkDocs, diagnostic catalog,
  PYC/ACIR inventories, decision coverage, and diff hygiene passed.
- A fresh build and install prefix contained no ACSim, contract/freeze epoch,
  fingerprint, SHA, checksum, or digest path/field; installed backend tools
  expose ACC rather than the retired QueueGraph command set.

## Boundary

- Schema-local versions describe serialization shape only.
- Git source revision remains an external release/toolchain pin and never enters
  specialization identity.
