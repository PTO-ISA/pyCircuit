# Issue 61 release workflow repository closure

## Scope

This evidence covers the repository-addressable portions of Decisions 0232 and
0234 Part A. The implementation provides a SHA-pinned manual workflow, exact
four-wheel candidate aggregation, deterministic platform archives, one
candidate-acceptance barrier, retained-byte-only publication, stable-URL
redownload, and source/tag/release-bound final attestations.

The repository validator parses the workflow `jobs.needs` graph. Contract tests
mutate that graph and inject a publish-time rebuild to prove that acceptance
bypass and rebuilt publication bytes fail. Linux candidate construction now
installs and checks `patchelf`. Platform validation covers incremental repeat,
same-root concurrency, topology change, SDK mismatch rollback, unsupported
runtime-bound input, exact native dependency closure, relocated consumer build,
and the sole exported model symbol. Final attestations validate every source
revision, candidate tag, stable release URL, verification flag, and platform
identity before linking the Actions run from both release notes and issue 61.

## Local results

- SDK release contract tests: PASS, 7 of 7.
- SDK schema/document checker: PASS, 6 schemas and 6 documents.
- Repository-management validator, including parsed DAG checks: PASS.
- Ruff, Python compilation, YAML parsing, strict MkDocs, and diff check: PASS.
- Fresh current-checkout LLVM 22 / Python 3.11 toolchain build and install:
  PASS; installed `agentic-circuit model {plan,emit-cpp}` is present.
- Fresh macOS arm64 candidate archive exact manifest/native relocation check:
  PASS with `--skip-install`.

The local Python 3.11 distribution aborts inside its own `venv/ensurepip`, so a
local relocated installed-consumer run cannot be claimed. The workflow uses
`actions/setup-python` 3.11 and executes that full gate on both Linux x86_64 and
macOS arm64. No tag, release, GHCR publication, or stable-URL verification was
performed locally.

## Status

Decisions 0232 and 0234 are `implemented-unverified`. Promotion requires a real
two-platform workflow run; Decision 0234 Part B remains a mandatory per-release
post-publication stop condition.
