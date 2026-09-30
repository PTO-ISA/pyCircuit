# M5-A generated model ABI

Status: M5-A accepted; overall M5 remains active.
Base: `26e096c0a8299a106f052d9c54dac4654d46b44d`.
Candidate: 12 code/test files in candidate-sha256.json, SHA-256
`20b7044e97002006a0e4ec8a24de0f7f2291dc6f2aa6b06803a69702c4cb4260`.
[Scope and retirement inventory](../../../work-items/m5-cutover.md).
[Reproduction](commands.md).

## Result

Verified native emission now supplies dut.h and a thin model_api.cpp. Each handle
owns one generated FinalSystem and one existing SimExecutor. The existing seven
callbacks, v1 enum values, 64/16/24-byte layouts and query symbol stay unchanged.
Generated CMake builds the source-owned TUs and same runtime into a shared DUT.
No separate scheduler, counter, parser, lifecycle or SystemRunner policy enters
the ABI; unrestricted {} configuration remains valid.

C++ admission reserves the exported global query name across declarations,
module families and namespaces, without banning nested names or changing IR/RTL
admission. The shared ABI header's only change documents reset from Failed.

## Evidence and independence

Luna implementation owns the new ABI emission helper; a separate Luna instance
owns C11/ctypes tests. PM integrates native transport/CMake and the existing
build-graph test's third consumer. Independent Sol reviews code and Astra checks
architecture. [Architecture APPROVE](architecture-review.md) and
[code review APPROVE](code-review.md).

Twelve new cases and 129 M4/scalar/receipt cases pass together: **141 passed,
zero failures/errors/skips**, after final formatting. The tests cover pure C
consumption, exact layouts/table, null/layout guards, pre-admission preservation,
configuration retry/unlimited/domain limits/ties, hold/quiescence, failed epochs,
latched diagnostics, reset/replay, separate handles, borrowed buffers, name
collisions and host constructor/Build failure. Fault probes use copied generated
artifacts in isolated subprocesses and do not add production fault switches.

The earlier M4 build-graph assertion expected two consumer objects per source;
M5-A adds the shared DUT. It now checks exactly one compile for each of the three
targets, preserving the per-source independent-TU requirement. Initial test setup
errors (C initializer warnings and missing helper environment) were corrected;
they are not claimed as product failures or semantic negative evidence.

M4's accepted revision was separately audited: all 35 code and 17 accepted doc
hashes still match the historical 26e096c0 blobs (m4-audit.json). That leaves zero
open required items in its revision-8 scope, not zero remaining productization.

## Remaining M5 work

Public emit/exports, source-owned RTL and source-map mapping, unified installation
and current-platform external consumer use, legacy source/build/install removal,
and synchronized decisions/docs/gates are still open. The old public routes were
not switched in this prerequisite. Full SDK/platform work remains separately
tracked. This packet is not M5 completion or a release claim.
