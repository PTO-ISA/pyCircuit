# Independent architecture and M4 exit review

Reviewer: `/root/decl_arch_conformance`, `gpt-6-astra` / xhigh; read-only.
Date: 2026-09-30. Verdict: **APPROVE — Architectural status: CLEAR**.
Base: `ce4fbbff6d3a3ae039b502905c7126d6ef06ee29`.
Code/fixture manifest: `4b6a4899989cb1198f0d83a44001038c00adbd57f979a9b0946f702d81ec0613`.
Documentation manifest: `1e8c1e4859bab635ce8f41993e05e53e5f81e628bc33c721e26454700d0cb43c`.
All 35 code/fixture and 17 documentation hashes independently matched.

The candidate satisfies revision 8's M4 architecture/workflow exit for the
macOS source-tree preview. The checked-in source DAG uses public compile/link;
actual C++ source groups compile independently and declarations remain headers.
Hardware and legitimate observations are separate from the external oracle.
Both backends use the same SystemRunner/SimExecutor lifecycle and serializer.
RTL Work captures old Q; Precommit uses captured checks; Xfer alone clocks;
Reset arms exactly one reset edge; HasWork follows registered rule activity.
Report-name uniqueness is enforced in shared observation validation before
publication, including final reconstruction. Snapshot and publication protection
cover managed/unmanaged transitions and existing destinations.

Reviewer inspected 17 workflow plus 10 oracle tests, 331 existing regressions,
and 116 native tests across seven binaries, with three declared Windows skips.
The clean native and documented empty-output builds succeeded. Both 13-line
runner transcripts matched byte-for-byte and the independent six-value oracle.
No architectural blocker remains. Full C3 ABI/source maps, source-owned RTL
packaging, installed SDK, public emit and old-route retirement remain outside
the revised M4 exit and are still obligations of their later delivery stages.
