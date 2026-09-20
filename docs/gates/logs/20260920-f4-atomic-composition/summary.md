# F4 atomic pointer-owned composition and finite-family cutover

## Scope and verdict

This run records the pyCircuit F4 implementation evidence for Decisions 0274-0278:
atomic Queue/state publication, pointer-owned generated C++ composition,
source-owned AC and C++ units, closed finite module families, and the typed PYC
logical-to-physical carrier. Decision 0273 is the separate F6 four-state/SRAM
verification contract and is not promoted by this evidence.

No consumer-specific SuperScalarModel behavior was added. No generated AC,
C++, or Verilog artifact was edited to repair semantics.

## Implemented contracts

- One firing checks and borrows all required inputs, checks only present
  outputs, prepares one commit group, and publishes Queue/Table/Slot/state
  effects atomically at Xfer. Failed preparation cancels the complete group.
- Parent modules own concrete `SimQueue<T>` storage. Repeated child instances
  are distinct `std::unique_ptr` objects wired through non-owning Queue
  pointers and stable instance records.
- Packed payloads at 64 bits remain value Queue elements. Packed payloads at
  65 bits use `std::shared_ptr<const T>`; blocked output performs no allocation,
  cancel/reset/destruction releases held references, and final pop releases the
  Queue reference exactly once.
- One Python source owns one readable interface AC shard containing its nominal
  declarations and module imports, one implementation AC unit, and one
  generated `.hpp`/`.cpp` source group. Package tests compile separate generated
  sources and link the executable DUT.
- Static family identity is exactly `(family definition,
  StaticArgumentsAttr)`. Typed Boolean, fixed-integer, enum, config, dependent
  type, layout, projection, clock/reset, Queue lane, and shared-ready mappings
  survive ACIR, QueueGraph, PYC, C++, and Verilog.
- Parameterized Python ports use explicit `Queue[payload, lanes, rate]`
  annotations. Dependent lane/rate expressions materialize per admitted case;
  omitted, non-positive, or `rate > lanes` shapes reject before publication.
- C++ emits one readable template family with explicit cases. Verilog emits one
  typed parameterized module with exact admitted generate branches and rejects
  unadmitted cases.
- Public CycleAware examples expose zero-parameter source-owned entries. Their
  fixed example configurations live in the source body; the former
  `DEFAULT_PARAMS` specialization channel is empty and cannot recreate
  caller-inferred cases.
- Push proposal storage is reserved during prepare. Allocation failure on a
  later output cancels every earlier reservation before any proposal is
  published; publication performs no vector allocation.

## Hard-break deletions

- Deleted `python/agentic-circuit/src/agentic_circuit/_jit.py` and
  `_lower_acir.py`.
- Deleted legacy `check`, `elaborate`, and `inspect` command implementations and
  their obsolete frontend/JIT tests.
- Removed `--specializations-json`, specialization sidecars, direct-body module
  compatibility, readable specialization suffixes, generated-artifact
  splitting, and structural `moduleSpecializations`/`specializationKey`
  authority.
- Removed the parallel `interfaces/modules` and `types.ac` layouts plus untyped
  static type/config metadata dictionaries. Source interfaces now have one
  source-stem identity.
- Removed compiler-generated double-underscore identities, including legacy
  `__ac_*`, `__fanout`, `__local`, `__capture`, `__pyc_*`, and opaque `__p...`
  payload spellings. The repository negative gate prevents their return.

## Focused evidence

Commands were run from the current checkout using LLVM/MLIR 22.1.8.

| Evidence | Result |
| --- | --- |
| `cmake --build .pycircuit_out/build-llvm22 --target check-acir` | 227/227 passed |
| `cmake --build .pycircuit_out/build-llvm22 --target check-pyc` | 3/3 passed |
| focused ACC composite/package/Python driver lit set | 4/4 passed |
| `family-config-backends.mlir` | 1/1; multi-lane PYC, C++ syntax, Verilog, Verilator passed |
| `family-python-multilane-backends.mlir` | 1/1; direct source units, package verify, PYC, C++ syntax, Verilog, Verilator passed |
| `pycircuit-hierarchical-family.mlir` | 1/1; direct pyCircuit hierarchy, PYC, C++ syntax, Verilog, Verilator passed |
| `.pycircuit_out/build-llvm22/bin/CompilerTests` | 10/10 passed |
| `.pycircuit_out/build-llvm22/bin/ACIRModelAnalysisTests` | 36/36 passed |
| `.pycircuit_out/build-llvm22/bin/CodeGenTests` | 110/110 passed |
| `.pycircuit_out/build-llvm22/bin/ACIROpsTests` | 1843/1843 passed |
| `.pycircuit_out/build-llvm22/bin/GfsimTests` | 268/268 passed |
| ASAN `GfsimTests` with `halt_on_error=1` | 268/268 passed |
| reduced Agentic contracts/CLI/frontend pytest suite | 381/381 passed |
| Agentic integration e2e pytest suite | 1/1 passed |
| focused pyCircuit hierarchy/surface pytest suite | 67/67 passed |
| `pytest tests/unit -m unit` | 154/154 passed |
| `python3 flows/tools/check_api_hygiene.py ...` | passed |
| `.pycircuit_out/ac-venv/bin/python tools/agentic-circuit/check-contracts.py` | passed |
| `mkdocs build --strict` | passed |
| `ruff check --select E9,F63,F7,F82` on product and changed tests | passed |
| hard-break product-code absence scan | passed |
| source-map, emitted-cost, diagnostics, and PYC coverage JSON parse | passed |
| `git diff --check` | passed |

The 64/65-bit ownership boundary is covered by
`QueueGraphPlanTest.StructuredQueueStorageUsesValueAt64BitsAndImmutableSharedAt65Bits`.
Wide-payload address preservation, final release, cancel/reset/destruction,
blocked-output zero-allocation, payload-allocation cancellation, and injected
second-output proposal-storage allocation failure are covered by the focused
gfsim runtime tests in `QueueBlocksTest` and the ASAN run.

The typed family/PYC matrix is covered by configured family AttrDef/parser/
verifier lit tests, recursive scalar/struct/tuple/array/nominal mapping checks,
ordered dependent nominal-argument validation,
`QueueGraphPlanTest` family cases, direct C++ syntax compilation, and Verilator
checks for Boolean/integer, enum/config, nested-lane, unused case, and repeated
instances. The multi-lane family test executes generated C++ and Verilator with
the same two-lane/backpressure stimulus and compares observations. Package lit
proves source-owned AC units and generated source groups remain separate through
link and execution.

## Determinism and absence

- Canonical family/case order, typed arguments, source maps, and bundle
  inventories are checked by byte-stable re-emission and caller-order reversal
  tests.
- `family-hard-break-absence.mlir` rejects structural specialization JSON,
  compatibility names, double-underscore compiler names, and opaque payload
  suffixes in product compiler/frontend sources.
- A repository scan found no removed JIT entry, specialization sidecar,
  structural specialization field, or generated double-underscore name in the
  positive Agentic product/test/documentation surfaces.

## Remaining release risk

This is implementation/PR evidence, not release promotion. The complete
release-only scripts were not rerun in this bounded F4 pass. Decision 0273
remains `gap-in-scope`, and Decision 0272 remains `implemented-unverified`;
neither is reclassified by this evidence.
Consumer validation and pinning remain in the owning consumer checkout.
