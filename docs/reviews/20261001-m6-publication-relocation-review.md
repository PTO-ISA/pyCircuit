# M6-01 independent validation review

Date: 2026-10-01. Reviewer: `/root/m5_complete_candidate_review`,
gpt-5.6-sol / high. Test authors: independent Luna high instances
`m5_semantic_oracle_migration` and `m6_relocated_prefix_tests`.
Product baseline unchanged: `d351079b4e4f2ba254cc57950f27d59c51dd7c38`.

Verdict: **APPROVE; acceptance-ready for the bounded macOS-arm64 V47/V48 packet**.
Test/fixture aggregate SHA-256:
`2d65e3c76ad557a3de030a0cdbbc53b4518fa5007c4705a789de4dce44f9dc45`.
Formula: sorted path + NUL + file SHA-256 + newline over nine files.

Key reviewed hashes:

- Process recovery test: `85138d31452b1135888dd6d9ae05f6b7440bf9753ba8a06e916823c05ee86c24`.
- Relocation test: `7599d813ef55129d701ffe77ed9f55eb41277c43e0090ae1ba805b817ece7bb5`.
- Crash runner: `74bea858c17bdd09953325394c88d3892c70dc99c56854d93fec6d588303ec80`.

The reviewer independently ran the combined gate: 5 passed in 25.76 seconds.
V48 proves real SIGKILL, all six replacement checkpoints, bounded first-publish
coverage, precise journal/stage/previous layouts, reentrant interrupted recovery,
lock inode preservation, deterministic pipe/flock blocking and OS release.
V47 proves absent original prefix, clean isolated Python, relocated CLI and
native dependency closure, final-only dual emission, Runtime-only CMake selection
and both backends' independent observation oracle.

Earlier test-shape findings were corrected before this verdict; no product bug
was established. PM verified the same file hashes after formatting and ran the
combined gate on Python 3.14.6 (5 passed) plus regression (131 passed, 3 actual
Windows-only skips). The independent alternate run used Python 3.12.12.

PM acceptance: this packet is done. It does not close all M6 or authorize a new
interface, parallel scheduler, platform guarantee or M7 release publication.
See the [gate evidence](../gates/logs/20261001-m6-process-relocation/README.md)
and [work packet](../work-items/m6-publication-relocation.md).
