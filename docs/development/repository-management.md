# Repository Management

This page defines repository authority, fork synchronization, and release
ownership for pyCircuit.

## Repository roles

| Repository | Role | Authority |
| --- | --- | --- |
| [`PTO-ISA/pyCircuit`](https://github.com/PTO-ISA/pyCircuit) | Canonical upstream | Product decisions, default branch, releases, packages, documentation, CI policy |
| `PTO-ISA/agentic-circuit` | Private archived migration source | Original commits, pull requests and audit history only; no source development or publishing |

The upstream repository is the only source of truth. Do not maintain a second
independent product history in a downstream fork or the standalone Agentic
Circuit repository. The latter is a private historical archive.

## Change flow

1. Open general compiler, runtime, documentation, and API changes against
   PTO-ISA/pyCircuit.
2. Develop processor-, accelerator-, model-comparison-, and board-specific
   designs and tooling in their owning consumer repositories against a pinned
   pyCircuit revision.
3. Submit only reusable language, IR, runtime, backend, and generic diagnostic
   fixes to framework source roots. Consumer designs and payload/trace adapters
   have no in-tree exception.
4. Update the downstream default branch from the upstream default branch after
   upstream changes merge.
5. Keep downstream-only commits focused and rebaseable; do not rewrite upstream
   release tags.

If an urgent downstream fix cannot wait for upstream review, record the
upstream issue or pull request and avoid creating a competing public API.

## Branch and review policy

- Protect the upstream default branch.
- Require pull request review and all gates applicable to the change class for
  ordinary contributors.
- Keep repository-administrator bypass permanently available. GitHub branch
  protection must set `enforce_admins=false`; an administrator may explicitly
  use an admin merge after reviewing the change and available gate evidence,
  without waiting for the ordinary approval count.
- Require semantic changes to cite decision IDs and evidence paths.
- Prevent direct release publication from downstream branches.
- Keep branch names descriptive and scoped to one change family.
- Delete merged topic branches when no active downstream dependency needs them.

The versioned desired state is
`.github/repository-governance.json`; GitHub settings must match it. Admin
bypass changes review enforcement for administrators only: required status
checks, code-owner review, stale-review dismissal, linear history, conversation
resolution, and the no-force-push/no-deletion rules remain the ordinary branch
policy. Admin merges remain visible in GitHub's pull-request and audit history.

## Release authority

Only PTO-ISA/pyCircuit may:

- create canonical version tags and GitHub releases;
- publish the `pycircuit-hisi` package;
- publish the `agentic-circuit` package;
- publish canonical compiler or runtime artifacts; and
- announce a language, framework-runtime, or toolchain compatibility level.

Downstream repositories may publish compatibility evidence, but must link to
the matching upstream revision and must not reuse canonical release tags for
divergent commits. Consumer design sources, testbenches, board files, payload
and trace adapters, and comparison scripts always remain out of tree.

## Fork synchronization

Before synchronizing the downstream fork:

1. Verify the target upstream commit and required gate results.
2. Fetch the upstream default branch.
3. Fast-forward or rebase downstream-only work onto that commit.
4. Run compatibility gates from the consumer repository, not from the
   pyCircuit worktree.
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

## Agentic Circuit retirement

Agentic Circuit source, ACIR/ACSim, gfsim, tests, schemas and frontend are owned
by the pyCircuit repository under the module roots defined by Decision 0157.
Do not land new source changes in the standalone repository after the migration
freeze.

Retirement completed on 2026-09-05 after pyCircuit PR #30 merged as
`cba1d938ddcfaadf021bbff5a91553869028e124`. The standalone repository:

1. ends at tombstone commit `9bc50aefbf1ac5a46a5aa94966f8bdc2811ff92a`;
2. has no open pull requests or issues;
3. has Actions disabled and no repository webhooks, deploy keys, invitations,
   direct collaborators, or team grants;
4. is private and archived; and
5. points its description and homepage to `PTO-ISA/pyCircuit`.

Access inherited from PTO-ISA organization ownership and the organization-wide
base repository permission remains governed at organization scope. Do not
change that global policy merely to specialize this archive.
