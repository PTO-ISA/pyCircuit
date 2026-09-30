# SDK release contract status

This page contains the retired ACC-only SDK contract and is preserved as a
historical release record. It does not describe the active package graph or a
current release artifact.

The accepted M5 package contract is recorded by
[Decision 0283](../rfcs/pyc6-decisions.md#decision-0283-approved-source-unit-hardware-cutover-for-the-scalar-profile):
one public `pycircuit` driver; Runtime component
`pycircuit::pyc6_runtime` without LLVM; CompilerDev with exact LLVM/MLIR
22.1.8; model ABI v1; generator ABI 2. Candidate verification remains in
progress, including installed consumers and old-route retirement. This status
page does not claim publication or release readiness.

See [M5 migration](m5-migration.md) for current local build and consumer
commands. Historical manifest and platform details remain tied to the releases
that originally used them.
