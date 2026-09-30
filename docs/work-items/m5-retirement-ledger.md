# M5 retirement ledger (candidate, not accepted)

Baseline: a21bb596. Supported profile is the revision-8 scalar function-module
hierarchy, default clock, empty static arguments, serial executor and existing
checks/observations. No old route may fill unsupported capability gaps.

| Removed route/oracle | Surviving evidence or explicit follow-up |
| --- | --- |
| CAS/JIT/builder, PYC dialect/CppEmitter/pycc, separate Agentic capture/QueueGraph and semantic-core | source_capture/source_transport/source_compile, source numeric/module/namespace tests, final_scalar_declarations, cpp_source_parts, native ACIRSource*/Final*/ExecutableBackend tests |
| Old current/next builder tests | unified_register_backends, masked_next_register, source_numeric_next, native RegContracts/RegRuntime/ProposalContracts |
| Old eager frontend import, CLI build/emit and static specialization rejection tests | driver_compile_link, driver_commands, independent M5 public emit/installed-route tests; nonempty static arguments still reject in current profile |
| Old generic diagnostics fixtures | source arithmetic/check negative tests and native source contracts; exact legacy DSL diagnostics are retired |
| Queue runtime/event scheduler and source-map v0.1 | SimDFF/SimSystem/SimExecutor, new ObservationSlots oracle, C3-SM inventory tests; no fabricated QueueGraph map |
| Old register-file/SRAM/multiclock/NEON/dodgeball tests | not delivered in this profile; M3 capability backlog must supply new common-IR implementation and fresh independent oracle before support claims |
| Legacy Agentic optimizer/codegen/compiler-only tests | new source/final native tests cover accepted current semantics; specialized queues/resources/memory/performance remain M3/M6 backlog |

This ledger records retirement scope, not a claim that deleted tests passed.
Candidate acceptance requires public same-final C++/Verilog behavior, installed
Runtime/CompilerDev consumers, source map mutations, and static plus dynamic
retirement checks. Historical baseline remains available in Git.

## Verification-discovered integration fixes

- Runtime archive symbols previously leaked through the generated DUT shared
  library. Hidden visibility on the one runtime now preserves the exact v1
  `agentic_model_query_v1` export; external SDK smoke caught this and an
  independent public-emission regression covers it.
- CompilerDev exported headers needed the matching internal Dialect header
  closure and imported LLVM/MLIR include paths. Its C++-only consumer also
  exposed LLVM's C-language configuration probes; only CompilerDev enables C.
- Ninja creates parent directories for file outputs. The example reuses the
  existing strict empty-directory preparation helper before managed publication;
  it never deletes nonempty or symlink destinations.
- C1 syntax-capture fixtures remain as non-executing parser oracles. Old
  class/self executable module tests are migrated to function-module source by
  an independent test author; unsupported class authoring remains a negative.
- Source-tree test setup no longer adds the retired prebuilt Agentic Python
  directory or semantic-core roots, preventing accidental reuse of old tooling.

M6 retains the two V44 schedule-permutation/source-reorder acceptance cases.
They are explicitly deselected in the serial-profile gate exactly as in M4;
this is not evidence of parallel scheduling support. Native source-owned and
cross-backend register tests remain active.

The old emitted-cost schema/example and its SDK installation/validation entry
are retired with QueueGraph/PYC; no cost emitter exists in the current profile.
New scale/cost measurement contracts belong to M6 and must not reuse those old
verification-stage claims.

The unregistered legacy `tests/mlir` lit runner and its PYC/QueueGraph inputs
are retired, not counted as passing tests. Current source/final MLIR contracts
run in the registered ACIR CTest and source-transport suites. Historical lit
inputs remain retrievable at the pinned pre-cutover Git revision; unsupported
optimizer/library capabilities need new M3/M6 oracles before admission.
