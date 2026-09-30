# M4 executable-source C++ parts

Status: done for the bounded executable-source profile; independent review APPROVE. Base: `3a54380226519f4e4d22600387356fb9699eb4a3`.
Checkout: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.

## Approved boundary

C3-C (approved via c2-c3-foundation) already specifies source-owned hpp/cpp,
source-derived names, template families and independent translation units.
Current scope is the scalar, empty-static-argument final profile. Current final
materialization drops declaration-only units; do not invent their headers or
read Python to recover them. This package does not produce generated.json,
dut.h ABI, public emit, installed SDK, or the unapproved system/expect B changes.

Read-only architecture: other_agent_arch_review (Astra xhigh), CLEAR. Repo map:
cpp_bundle_explore (Astra low). Independent validation planning:
other_agent_code_review (Sol high). SourceOwner comes from verified ac.module
ac.source_owner, not instance OwnerRef. Existing private monolithic gate names
may remain an internal presentation; one shared structured semantic generator
must serve both renderers. Never generate a whole C++ string and split it.

## Ownership

PM owns this packet, FinalCppSourceParts.h (frozen internal result), test harness,
CMake registration, shared native build, integration and evidence. Implementation
owns FinalEmitCpp* and new internal emission/layout files assigned in dispatch.
Independent tests own a new source-parts system regression. No author reviews
its own change, no recursive delegation, and no edits to other agents' files.

## Acceptance

Real per-source Python compile/link, saved final reparse in a fresh process,
source-owned groups, source-derived C3 namespace/class and local storage/child
names, empty-argument class template specialization, by-value child includes,
separate object compilation and link. Repeated child definitions share a TU but
keep distinct state; alias ports add no registers. Use existing independent
numeric/observation/reset oracle and current C++/RTL behavior.

Guard emission before and after; reject invalid final, unsafe or colliding
source names and unsupported static arguments. Preserve the old private gate
renderer through the same emission core. Archive targeted current-checkout
build/tests, hash binding and independent review. M4 remains open afterward for
full declaration ownership, generated bundle publication and build/run delivery.

## Dispatch details

- Implementation: `unit_pair_fix`, gpt-6-luna/high. Existing FinalEmitCpp* plus
  FinalEmit.cpp, new FinalCppEmission.cpp/.h;
  no CMake/harness/test writes. Private naming layout serves both output forms.
- Independent tests: `cpp_parts_tests`, separate gpt-6-luna/high. Owns
  tests/system/test_cpp_source_parts.py and optional test-only CMake fixture.
- PM: frozen FinalCppSourceParts.h, structural FinalCppSourceParts.cpp renderer,
  noninstalled acir-cpp-source-parts-harness,
  native CMake registration and checkout-local .pycircuit_out/w10-pm/build.
- Independent review: other_agent_code_review, gpt-5.6-sol/high, no authorship.

Architecture advice is CLEAR for internal layout injection: old ordinal names
remain confined to existing private gates; grouped output uses approved source
names/template families. Registry/source ownership and hardware semantics are
not reconstructed from generated C++ or Python. The validated grouped path
must reject legalization/path/scope collisions rather than append ordinals.

The test transport writes JSON to stdout only after the complete verified
in-memory result exists. It is not installed and does not publish generated
bundles, implement a runtime driver, or bypass public role/entry admission.
Source @system fixtures remain existing private testbench fixtures, not newly
approved first-class system definitions.

## Acceptance result

The candidate passed 68 Python/system regressions and 69 native tests, with
no failures/errors/skips. After final formatting, the targeted five new tests
passed again; the independent reviewer also reran those five and approved the
17-file content-bound candidate. The old monolithic emitter matched saved
baseline C++ bytes. See [evidence and exact commands](../gates/logs/20260930-m4-cpp-source-parts/README.md).

Integration ownership expanded explicitly: PM owns FinalEmitCppSystem.cpp/.h,
the renderer and harness; PM fixed LLVM SmallVector inline capacity and the
naming layer's incorrect integer-only PortSlot interpretation. Scalar UnitAttr
and u64 collection ordinals follow the existing MLIR verifier. No IR or runtime
semantics were changed. Existing private monolithic gates still use the same
semantic generator, not a fallback generator.

Remaining M4 work: declaration-only source ownership in common final IR,
source-owned declaration headers, full generated.json/publication delivery and
usable complete build/run flow. New public emit stays assigned to M5. The
source-level multi-assignment reconstruction issue discovered on the baseline
is tracked separately in [the M3 follow-up](m3-generic-multi-assignment-reconstruction.md).
