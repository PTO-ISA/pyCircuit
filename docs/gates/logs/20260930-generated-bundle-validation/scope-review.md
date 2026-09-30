# Independent architecture scope review

Date: 2026-09-30. Reviewer: decl_arch_conformance, gpt-6-astra / xhigh,
read-only. Base: `645b4ff19dbbeeb01eb7842e31e2a4832bdee9e9`.
Recommendation: implement the private receipt validator and managed snapshot
reader under existing C3-C approval; no new interface approval is required.
This is a scope review, not final implementation acceptance.

Current source parts lack real dut.h ABI and generated CMake/DUT linkage. Do not
call their publication complete C3 delivery. Reuse the existing directory
transaction engine; do not add a producer, placeholder ABI or public emit command.

The exact six-field generated receipt uses canonical quoted definition strings,
consistent with C2 and native publication owner reports. Validate JSON shape,
owner/target binding, safe sorted file paths and unique group membership; reject
undeclared files/directories and unsafe filesystem nodes. Header-only groups and
ungrouped support/core/CMake glue remain legal. Do not invent group-order or
role-extension rules. The current private profile explicitly rejects nonempty
static arguments; do not implement a Python StaticValue semantics engine.

Receipt/file consistency cannot prove code semantics, ABI completeness,
historical authorship or full original native owner inventory. Production
assembly must later compare with the native structured result. Full generated
bundle, model ABI, build/run and M5 public emit remain pending.

Independent tests must cover strict schema and unsafe inventory negatives,
immutable in-memory snapshots, existing publication replacement protection,
prepared/committed recovery, reentrant recovery, cleanup-pending read behavior,
and reader/writer exclusion with the real generated validator.
