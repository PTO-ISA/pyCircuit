# windows-x86_64 platform verification

## Scope

Decision 0265 records `windows-x86_64` as the third SDK platform profile and
keeps it `implemented-unverified` until a real Windows runner proves it. The
release workflow cannot produce that proof: its platform lanes are
`needs: [full-validation]`, and full-validation runs
`check_decision_status.py --require-all-verified`, which fails while 0265 is
unverified. The dispatch-only `platform-evidence.yml` lane therefore builds and
verifies the platform with exactly the release commands.

## Evidence

| Fact | Value |
| --- | --- |
| Run | `35395492844` on `main` |
| Source revision | `dccb53016cf01c5cccafcb87369fd226cc6e77e4` |
| Candidate bytes | artifact `platform-evidence-bytes-windows-x86_64`, 153,723,127 bytes, `sha256:d06ba4af387bc5102675cc7a493b9244531bc0c84c69052e9533c8070dfba2a9` |
| Attestation | `platform-evidence-windows-x86_64.json`, `sha256:f3f7c0cafd6759b411d960f4f506ae269797aed10b173781aca595863417d721` |
| Universal wheels | artifact `platform-evidence-universal`, `sha256:39a3f2b16d8a2d43ce9e5d7e7ff8b7193a7c9c082362804ad6a2c6992f35690b` |

## Verified in this run

`platform-evidence-windows-x86_64.json` records every gate as `true`: exact
manifest closure, native dependency closure, relocated model consumer,
incremental determinism, topology change, parallel same root, mismatch
rejection, unsupported boundary rejection, and `verified`.

The lane provisioned the pinned clang-cl 22.1.8 driver
(`sha256:d96c2cc1736f4eb7fa43cb9bbdf56d93551a9ae0a9aadb9c99c3c3b2b712a234`) and
MLIR built from the pinned LLVM 22.1.8 source
(`sha256:922f1817a0df7b1489272d18134ee0087a8b068828f87ac63b9861b1a9965888`),
built the `windows-x86_64` candidate, relocated it twice, and ran the installed
smoke test: model plan, model emit, a repeated plan and emit compared byte for
byte, two concurrent emissions into one output root, a mismatched SDK identity
that must fail before publication, an unsupported boundary case, and the
consumer CMake build that compiles the generated bundle, exports only
`agentic_model_query_v1`, and runs the consumer executable.

## Defects this bring-up fixed

The platform had never been built, so bring-up fixed a series of real
portability defects. Each is a merged change with its own regression coverage.

| Area | Defect | Fix |
| --- | --- | --- |
| Compiler | MSVC front end aborts with C1001 on the ACIR codegen's recursive generic lambdas | build the profile with the pinned clang-cl 22.1.8 driver over the MSVC v143 ABI |
| Sources | `std::bit_width` used without `<bit>`; `clang-cl` ignores `-ffile-prefix-map` and needs the `/clang:` escape | include the header, map the prefix through the driver |
| Packaging | the extension linked `python3.lib`, which the hosted Python does not ship | link the versioned import library and stage the stable-ABI name |
| Packaging | `python311.dll` was not a declared system dependency | accept the Python runtime as the profile documents it |
| Product | `fcntl` imported unconditionally, and directory publication used the POSIX rename exchange | select `msvcrt` on Windows and replace the directory in portable steps |
| Packaging | the manifest's `files` list used pathlib order, which is not the POSIX string order the consumer requires | sort every recorded path by its POSIX relative form |
| Product | compiled tools were looked up without the `.exe` suffix | spell the suffix everywhere the packaged resolver already did |
| Native | `llvm::sys::fs::rename` cannot move a directory on Windows | publish with `MoveFileExW` and retry, keeping `fs::rename` elsewhere |
| Consumer | the consumer joined a native root with relative paths, producing a CMake escape error | normalize the root with `file(TO_CMAKE_PATH ...)` |
| SDK header | the entry point definition added `__declspec(dllexport)` after a declaration without it (C2375) | the header owns the export attribute behind `AGENTIC_MODEL_BUILD` |
| Harness | the consumer was built Debug against Release SDK binaries (LNK2038) | pin `-DCMAKE_BUILD_TYPE=Release` and `--config Release` |
| Harness | built artifacts were globbed in the build root, but a multi-config generator writes them to `Release/` | search by exact artifact name and require exactly one match |
| Workflows | `$ErrorActionPreference` does not fail a native command, so a failed packaging step was masked | check `$LASTEXITCODE` after every native packaging step |
