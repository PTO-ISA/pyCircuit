# Per-module ACIR and gfsim multi-TU closure

Date: 2026-09-16

Decision: 0264

## Result

- Frozen ACIR remains the whole-program semantic authority. The compiler also
  emits sorted `modules/<symbol>.ac.mlir` inspection units containing one
  `ac.module`, the shared type scope, and no selected system operation.
- Structured QueueGraph bundles emit one header per nominal enum/payload, a
  declaration-only helper header plus helper implementation source, one class
  declaration header and out-of-line implementation source per concrete module
  specialization, and root QueueGraph/runtime sources.
- Every generated `.cpp` is compiled separately to an object. The regression
  links the objects with gfsim and executes a reused stateful module model;
  independent instances preserve their values and scheduler behavior.
- Model plan fixes the exact sorted generator inventory. Emit must reproduce
  it exactly, and CMake, depfile, and manifest derive from the same list. A
  byte-identical repeated emit preserves file mtimes.
- Scalar lexical reset images survive frontend capture, `ac.var` storage
  selection, Table verification/planning, and gfsim initialization, including
  the complete unsigned 64-bit bit pattern. JIT/source specialization and
  QueueGraph structural specialization are carried and checked separately.

## Evidence

- LLVM/MLIR 22 current-checkout build passed.
- `CodeGenTests`: 172/172 passed. The suite includes deterministic inventory,
  real out-of-line symbols, per-source `-c`, object link, and runtime execution.
- `CompilerTests`: 11/11 passed, including deterministic per-module ACIR
  artifacts and content hashes.
- Focused Agentic lit: 5/5 passed for module ACIR, multi-TU C++, non-zero/u64
  reset images, Table negatives, and CLI compile artifacts.
- Agentic frontend: 239/239 passed.
- Focused model plan/emit: 5/5 passed.
- Root unit suite: 147/147 passed.
- Contract checker, diagnostic catalog, API hygiene, strict MkDocs, C++/Python
  formatting checks, and diff hygiene passed.

The checkout-wide `ACIROpsTests` run passed 1840/1842. Its two failures are the
existing exact operation-registry inventory/count baseline (`152` registered
operations versus the stale expected `151`), unrelated to the files or
semantics changed here.

## Boundary

The per-module `.ac.mlir` files are reviewable inspection units, not independent
ACIR link inputs. Independent ACIR compilation requires a future verified
import/signature and module-link contract; this change does not bypass the
whole-program freeze or claim that contract.
