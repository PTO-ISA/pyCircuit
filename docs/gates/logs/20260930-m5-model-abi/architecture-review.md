# Independent architecture conformance

Reviewer: decl_arch_conformance, gpt-6-astra / xhigh, read-only.
Verdict: **APPROVE — Architectural status: CLEAR**. Date: 2026-09-30.
Base: `26e096c0a8299a106f052d9c54dac4654d46b44d`.
Candidate manifest SHA-256:
`20b7044e97002006a0e4ec8a24de0f7f2291dc6f2aa6b06803a69702c4cb4260`.
All 12 listed source/test hashes independently matched.

Opaque handles own a FinalSystem, report metadata and existing SimExecutor;
callbacks add no execution policy. C3 ABI shapes, enums, signatures and query
symbol are preserved. Create nulls output before allocation and publishes only
after Build; C boundaries contain exceptions. Guarded native emission produces
the ABI files and CMake links actual source-owned TUs with the same runtime.
Global query-symbol collisions reject through shared C++ admission; nested names
remain valid. Pure C consumers require no compiler headers.

Reviewer inspected the 141 passing cases with no skips, including unlimited
configuration, limits/ties, failure/reset, handles, output preservation, names and
host fault injection. This approves the bounded M5-A prerequisite only; public
cutover, installation/runtime packaging, RTL delivery and deletion remain open.
