# M4 acceptance — 2026-09-30

Status: **M4 DONE for the revision-8 macOS source-tree preview**.
[Use the workflow](../../../development/migration-preview.md).
[Work item](../../../work-items/m4-completion-workflow.md).
Base: `ce4fbbff6d3a3ae039b502905c7126d6ef06ee29`; build/test checkout is
`/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.

## Delivered exit

A checked-in Python design with separate declarations and child modules builds
through public per-source compile and link into design_top.ac. Native emission
consumes one saved verified final snapshot; C++ groups compile independently,
including header-only declaration owners. Generated standard runners execute
C++ and Verilator through the same SimExecutor/SystemRunner. The simulation-only
RTL adapter controls edges, never source algorithms or a second stop policy.
The external oracle, not the hardware, owns expected values and test verdicts.

Explicit event sinks deliver committed Event records then one terminal Result;
the user-approved default is silent. The workflow protects existing sinks and
managed outputs, rejects invalid finals/unsupported capabilities, and handles
managed/unmanaged input races. Shared MLIR observation checks now statically
reject duplicate report names within one actual module instance.

## Evidence

| Lane | Result |
| --- | --- |
| Independent end-to-end workflow | 17 passed, no skips |
| Independent JSONL protocol oracle | 10 passed, no skips |
| Existing targeted regressions | 331 passed, 3 Windows-only skips |
| Native regressions | 116 passed across seven binaries |
| Clean standalone compiler build | 106 steps, success |
| Documented fresh-output build/run | C++ and RTL success; byte-identical 13-line captures; external oracle PASS |
| Strict MkDocs and applicable pre-commit | Passed |

See [commands](commands.md), final.xml, existing-regression.xml and
native-regression.xml, plus the raw compiler/build/runner captures.
The three skipped existing filesystem tests require real Windows APIs; no
Windows certification is implied. Toolchain is LLVM/MLIR 22.1.8, C++20,
Verilator 5.044 on macOS arm64. Tests used the Python/pytest versions preserved
in their logs. No compiler or library was copied from another checkout.

The M4 gate proves ordinary progress, empty clocked activity versus no-rule
quiescence, failure without commit/events, successful and failed SAME-object
Reset/replay, signed/bool/literal observations, source and fresh-final report
rejection, native target refusal and unchanged outputs, dependency rebuilds,
unsafe sink/output rejection, input-management races, and corrupted transcript
rejection by the documented oracle command. Five static register declarations
instantiate six physical registers: four root plus one per each of two children.

## Independent acceptance and bindings

- [Sol code review APPROVE](code-review.md), 52 files.
- [Astra architecture and M4 exit APPROVE](architecture-review.md).
- candidate-sha256.json binds 35 code/fixture files; SHA-256
  `4b6a4899989cb1198f0d83a44001038c00adbd57f979a9b0946f702d81ec0613`.
- reviewed-documentation-sha256.json binds 17 reviewed documentation files;
  SHA-256 `1e8c1e4859bab635ce8f41993e05e53e5f81e628bc33c721e26454700d0cb43c`.
- accepted-documentation-sha256.json records the subsequent PM status-only
  promotion in the work item/ledger; product/test bytes did not change.

Implementation and independent tests used separate Luna instances. PM owned
compiler/RTL integration, workflow/build/publication wiring, evidence and final
acceptance. Code review was independent Sol high; scope and architectural
conformance were independent Astra xhigh.

## Deliberate boundary

M4 follows the user's narrowed revision-8 exit, not every full C3 deliverable.
The preview's generated.json records file management; it does not authenticate
code contents or certify a complete public ABI/source-map distribution. RTL is
aggregate private simulation input, not falsely source-owned packaging.
Full dut.h/C ABI, source-owned RTL/source maps, installed SDK, public new emit
cutover and old-route retirement remain later obligations. SYSTEM/EXPECT B were
not implemented. No new primitive, IR role attribute or Python DSL was added.
