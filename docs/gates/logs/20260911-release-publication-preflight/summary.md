# Release publication preflight

## Scope

This evidence covers the publication-chain failure exposed after release run
34562193478 accepted the exact 6.0.0 candidate for Decisions 0232 and 0234.

The run passed the full source closure, universal and platform wheel builds,
Linux x86_64 and macOS arm64 SDK construction, deterministic aggregation, both
relocated installed-consumer verification jobs, and the sole candidate
acceptance barrier. Tag creation then failed before any publication because the
runner had no Git committer identity.

## Closure

- The tag job configures the canonical GitHub Actions bot name and noreply
  address before creating the annotated tag.
- The job verifies the local object type, local peeled SHA, and remote peeled
  SHA against the explicit release input.
- Final attestation checks stable asset URLs against the GitHub Release
  `/releases/download/<tag>/` prefix rather than the unrelated release page
  `/releases/tag/<tag>` prefix.
- The repository `release` environment exists with no protection rule or
  deployment branch restriction; PyPI publication remains controlled by the
  existing `PYC_PUBLISH_PYPI` repository variable.

## Verification

- SDK/release workflow contract tests: 9 of 9 passed.
- Parsed accept-before-tag/publish workflow DAG: passed.
- YAML and changed-file pre-commit checks: passed.
- Repository tag rulesets: none.
- The accepted run created no tag, GitHub Release, GHCR package, or PyPI
  publication after the tag job failed.
