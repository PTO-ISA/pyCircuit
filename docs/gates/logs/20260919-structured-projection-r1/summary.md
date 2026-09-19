# Structured module projection and external release identity closure

Date: 2026-09-19

Decisions: 0249, 0250, 0264, 0267, 0268

## Contract

- Structured `@ac.system` calls may pass `Name.field...` values to reusable
  modules.
- The frontend emits a compiler-owned, readable projection module and reuses it
  for equal input specialization and field path.
- Multi-consumer sources use verifier-visible strict broadcast.
- Generated projection modules retain Python source and parent-system NDF
  metadata.
- Python names beginning with `__ac_` are rejected as compiler-owned.
- Module specialization cannot implicitly capture an outer static type root.
- SDK JSON carries package/ABI versions and exact source revision externally;
  it carries no embedded IR/frontend release selector.

## Evidence

```text
python frontend: 395 passed, 1 skipped
unit: 153 passed
primitive selection: 18 passed
repository contracts: OK
API hygiene: OK
MkDocs strict: passed
pre-commit changed-file set: passed
```

Native focused flow:

```text
raw ACIR -> topology closure -> acc -emit-cpp: passed
raw ACIR -> topology closure -> acc -emit-cpp-bundle: passed
all focused bundle translation units -fsyntax-only: passed
```

DavinciOO consumer source closure against the current checkout:

```text
ACIR bytes: 21005670
unique generated projection modules: 59
projection instances: 92
system NDF metadata present: yes
```

`acc -emit-verilog` remains fail-closed for module-preserving QueueGraph input
with `ACLOWER-PYC: module-preserving QueueGraph PYC lowering is not implemented`.
This gap is not hidden by flattening or a backend-only compatibility path.
