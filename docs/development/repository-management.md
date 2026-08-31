# Repository Management

This page defines repository authority, fork synchronization, and release
ownership for pyCircuit.

## Repository roles

| Repository | Role | Authority |
| --- | --- | --- |
| [`PTO-ISA/pyCircuit`](https://github.com/PTO-ISA/pyCircuit) | Canonical upstream | Product decisions, default branch, releases, packages, documentation, CI policy |
| [`LinxISA/pyCircuit`](https://github.com/LinxISA/pyCircuit) | Downstream fork | Linx integration staging and downstream validation |

The upstream repository is the only source of truth. Do not maintain a second
independent product history in the LinxISA fork.

## Change flow

1. Open general compiler, runtime, documentation, and API changes against
   PTO-ISA/pyCircuit.
2. Develop Linx-specific integration in the downstream fork when it depends on
   Linx repositories or infrastructure.
3. Extract reusable fixes from Linx integration and submit them upstream.
4. Update the downstream default branch from the upstream default branch after
   upstream changes merge.
5. Keep downstream-only commits focused and rebaseable; do not rewrite upstream
   release tags.

If an urgent downstream fix cannot wait for upstream review, record the
upstream issue or pull request and avoid creating a competing public API.

## Branch and review policy

- Protect the upstream default branch.
- Require pull request review and all gates applicable to the change class.
- Require semantic changes to cite decision IDs and evidence paths.
- Prevent direct release publication from downstream branches.
- Keep branch names descriptive and scoped to one change family.
- Delete merged topic branches when no active downstream dependency needs them.

The exact repository ruleset lives in GitHub settings. This document states the
policy that settings must enforce.

## Release authority

Only PTO-ISA/pyCircuit may:

- create canonical version tags and GitHub releases;
- publish the `pycircuit-hisi` package;
- publish canonical compiler or runtime artifacts; and
- announce a language, ABI, trace-schema, or toolchain compatibility level.

The LinxISA fork may publish integration evidence and downstream test results,
but must link to the matching upstream revision and must not reuse canonical
release tags for divergent commits.

## Fork synchronization

Before synchronizing the downstream fork:

1. Verify the target upstream commit and required gate results.
2. Fetch the upstream default branch.
3. Fast-forward or rebase downstream-only work onto that commit.
4. Run the Linx integration gates from the downstream worktree.
5. Record the upstream commit in the integration report.

Never resolve fork drift by force-pushing an unreviewed divergent history over
the canonical upstream branch.

## Ownership transfer checklist

When repository or organization ownership changes, verify all of the following:

- the canonical repository URL and default branch;
- GitHub organization teams and least-privilege access;
- branch protection and required status checks;
- repository secrets, environments, and release credentials;
- package ownership and trusted publishing configuration;
- CI, release, documentation, and security links;
- issue and pull request templates;
- webhook, bot, and app installations;
- dependency and security alert ownership; and
- the downstream fork relationship and remotes.

Do not treat a GitHub transfer alone as completion. Repository metadata,
credentials, automation, and package authority must point to PTO-ISA.

## Historical names

Historical gate logs and compatibility identifiers may retain earlier version
labels. Keep them stable unless an accepted migration decision defines the
replacement and compatibility window. Current product documentation must still
identify the language as pyCircuit 6.
