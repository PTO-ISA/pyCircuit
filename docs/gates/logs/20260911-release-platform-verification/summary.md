# Release platform verification closure

## Scope

This gate evidence covers the candidate-verification failures exposed by
release run 34557596074 for Decisions 0232 and 0234. The run completed the full
source closure, both platform builds, exact four-wheel aggregation, and the
immutable 14-asset candidate before both platform verification jobs failed.
Acceptance, tag creation, and publication remained skipped.

## Failures and fixes

- Both SDKs lacked the six frozen release/model schemas at
  `share/pycircuit/schemas/`; the package generator now owns and inventories
  those exact files instead of relying on a test fixture to preinstall one.
- Linux `ldd` output `statically linked` was incorrectly recorded as a system
  dependency named `statically`; generator and verifier now ignore that status
  line.
- Relocated macOS Mach-O files retained signatures that passed a static
  `codesign --verify` check but were rejected by AMFI during Python extension
  loading. The package generator now explicitly ad-hoc signs every Mach-O after
  all install-name rewrites and before manifest hashing.

## Verification

- SDK release contract tests: 8 of 8 passed.
- Generated candidate positive acceptance and tamper rejection: passed.
- SDK schema/document checker: 6 schemas and 6 documents passed.
- Repository-management workflow DAG validator: passed.
- Strict decision status: 241 rows, no deferred or unverified decisions, all
  evidence paths present.
- Fresh current-checkout macOS arm64 toolchain build and Python 3.11 install:
  passed with LLVM 22.1.8.
- Fresh macOS candidate contains all six frozen schemas and passed exact file,
  hash, native dependency, and relocation checks.
- Full relocated macOS Python 3.11 model plan/emit, CMake build/run,
  incremental, same-root concurrency, topology, SDK mismatch, and unsupported
  boundary checks: passed.

The local uv-created Python 3.11 environment replaced only this host's broken
stdlib `ensurepip`, which aborts during nested venv creation. The release
workflow continues to use `actions/setup-python` 3.11 and remains the platform
and publication authority.
