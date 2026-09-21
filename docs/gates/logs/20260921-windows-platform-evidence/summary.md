# windows-x86_64 platform verification (6.1.0 preflight)

## Scope

Decision 0265 records `windows-x86_64` as the third SDK platform profile. The
dispatch-only `platform-evidence.yml` lane builds and verifies one platform
candidate with exactly the commands the release lanes use, and never tags or
publishes. This log records the run that was accepted before the 6.1.0 release
candidate, after the platform bring-up fixes that followed the first
verification.

## Evidence

| Fact | Value |
| --- | --- |
| Run | `35618811587` on `main` |
| Source revision | `bebbd2b5dd665be5ad41288061c34abb0e84a968` |
| Candidate bytes | artifact `platform-evidence-bytes-windows-x86_64`, 132,093,470 bytes |
| Attestation | `platform-evidence-windows-x86_64.json`, `sha256:aad43efb7bb5d482b196ead51a90c90add77f75895cb05f2506ec70d58dd8bf9` |
| Universal wheels | artifact `platform-evidence-universal`, 355,754 bytes |
| Earlier accepted run | `35395492844`, revision `dccb53016cf01c5cccafcb87369fd226cc6e77e4`, recorded under `docs/gates/logs/20260918-windows-platform-evidence/` |

`platform-evidence-windows-x86_64.json` records every gate as `true`: exact
manifest closure, native dependency closure, relocated ACC DUT, deterministic
regeneration, transactional publication, C++ execution, Verilog emission, and
`verified`.

## Defects this run fixed

`35395492844` proved the platform, but the lane and the SDK regressed after it.
This run required the following fixes, each of which failed the lane before:

| Area | Defect | Fix |
| --- | --- | --- |
| Build | `pycc` included `psapi.h` before `windows.h`, so the Windows SDK headers failed with `unknown type name 'DWORD'` | include `windows.h` first |
| Build | `llvm::sys::fs::file_t` is a HANDLE on Windows, so `closeFile(file_t &)` could not take the descriptor returned by `createUniqueFile` | close the reserved file through `llvm::raw_fd_ostream` |
| Verification | `TEMP` is an 8.3 short path (`RUNNER~1`), and a venv created from that spelling could not start its own interpreter | canonicalize the workspace to its long spelling |
| Verification | pip does not materialize a launcher for the `acc.py` entry point on Windows | prefer the launcher, otherwise run the module through the venv interpreter |
| Verification | the universal `agentic_circuit` wheel does not carry the native compiler extension | put the SDK's bundled site-packages on `PYTHONPATH`, as the SDK launcher does |
| Verification | the driver refuses to clobber an existing artifact, so the determinism check could not reuse one output path | regenerate into a second path and compare bytes |
| Product | `acc -emit-cpp-bundle` published the bundle with `llvm::sys::fs::rename`, which cannot move a directory on Windows | both ACC tools share the `MoveFileExW`-based directory publication |

## Status

`windows-x86_64` is verified for the 6.1.0 candidate. The release workflow still
runs its own windows-2022 candidate and stable lanes at the release revision, and
the final release attestation requires all three platform attestations.
