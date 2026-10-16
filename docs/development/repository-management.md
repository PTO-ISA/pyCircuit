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

## Issue intake and pull request handoff

The GitHub issue form collects change type, owning area, expected/actual behavior,
a reproducer or use case, and candidate/environment details. Use `[Area] short
summary` titles, for example `[MLIR] Reject overlapping state writers`.
Choose the primary owner when a failure crosses stages; use `Other` if ownership
is uncertain or a new area is needed. Match the form selection to the title.

| Area | Routing responsibility |
| --- | --- |
| `Capture` | Python parsing and private source transport |
| `MLIR`, `IR` | Analysis/lowering passes and common hardware IR contracts |
| `Link` | Source publication, interfaces and explicit unit closure |
| `CPP`, `Verilog` | Target emission from verified final IR |
| `Runtime`, `System` | Runtime execution/storage and generated test-system closure |
| `CLI`, `Build`, `Package` | Public commands, build configuration and installed consumers |
| `CI`, `Docs`, `Other` | Automation, documentation and cross-cutting/unclassified work |

The contribution-metadata workflow checks issue titles on open, edit and reopen.
It adds `invalid-title` for an unsupported prefix or missing summary and removes
the label after correction. The title is the routing authority; the workflow
does not compare the form dropdown with it. Blank web issues are disabled;
CLI/API submissions still receive the title check. Private security reports use
the advisory link in the issue chooser. Existing issues are not bulk relabeled;
an edited or reopened issue must adopt the current format.

Maintainers confirm scope, assign an owner and apply existing type labels such
as `bug`, `enhancement` or `documentation` during triage. A request does not
approve new semantics. Follow [project governance](project-governance.md) for
contract decisions and bounded task ownership. No project-board or priority
state machine is implied by the templates.

PRs record the problem/result, change type, related issues, concrete changes,
validation, review checklist and reviewer notes. Use `Fixes #123` only for work
that fully closes an issue; use `Related to #123` for partial work and retain
follow-up scope explicitly. A PR title should use the same area convention;
only issue titles are automatically checked.

`PR description completeness` checks required sections, author-supplied details,
and selected change/validation checkboxes. Untouched template comments and
checkboxes alone do not satisfy it. Explicit failures, skipped/deferred checks
and N/A reasons are valid reporting; this check does not certify correctness,
test success or review completion. It uses a read-only pull-request token,
including for forks. Only the issue job can write labels, and it runs code from
the default branch, never a PR branch. Issue/PR text is read as JSON data rather
than interpolated into executable commands.

Validation follows [testing and gates](testing-and-gates.md): record the tested
commit and local content identity, exact commands/configuration, exit status,
selected tier and accessible evidence. Local ignored logs need an accompanying
summary or uploaded CI artifact for remote review. Template/documentation-only
work may omit hardware regressions with a reason and appropriate metadata/docs
checks; script changes are not automatically exempt. Deferred nightly coverage
stays NOT RUN. Independent review and ordinary branch protection still apply.

This is the required intake and review flow for subsequent pyCircuit issues and
PRs, including submissions made through agents or the CLI. `PR description
completeness` is a required main-branch status check alongside the two G0 checks
in `.github/repository-governance.json`. Issue titles are enforced by labeling;
GitHub does not block issue creation on this check. Existing PRs may need their
descriptions updated when edited or synchronized after rollout. Apply the
versioned required-check list to GitHub branch protection during rollout;
merging workflow files alone does not update that setting.

This structure adapts SuperScalarModel's
[issue form](https://github.com/LinxISA/SuperScalarModel/blob/51a29bea4267a277201609fbe0edc5c2d8349fc7/.github/ISSUE_TEMPLATE/issue.yml),
[PR template](https://github.com/LinxISA/SuperScalarModel/blob/51a29bea4267a277201609fbe0edc5c2d8349fc7/.github/pull_request_template.md)
and metadata checks. pyCircuit uses a single area prefix because it has one
product flow; compiler/runtime ownership and candidate-bound evidence replace
the reference repository's simulator modules and regression matrix.

## Issue-to-PR delivery

1. Resolve the issue repository and writable checkout separately. The canonical
   issue/base repository is `PTO-ISA/pyCircuit`; a contributor's head may live in
   a fork. Inspect remotes and PR metadata instead of assuming `origin` or a
   same-named branch identifies the write destination.
2. Search open and closed issues and related PRs before starting new work.
   Distinguish the same root cause from related symptoms. Read existing assignees,
   linked work and implementation status to avoid duplicating active work;
   assignments and project fields remain unchanged unless their update is part
   of the requested task.
3. Establish a bounded reproducer/use case and affected contract. The issue form's
   optional pass, performance and proposal fields capture specialized evidence.
   Use the applicable fields during triage; unavailable measurements or IR stay
   explicit gaps, not invented results. RFCs record alternatives, dependencies,
   hard-break/removal scope and unresolved decisions before semantic acceptance.
4. Track dependencies explicitly. Record prerequisite PR URLs and their state in
   the issue/PR; distinguish a dependency from a background reference. Break
   implementation into independently reviewable changes when possible, with
   a named owner, expected behavior and scoped verification for each.
5. Preserve task ownership through delivery. Inspect the working tree and index;
   stage only the task's exact files/hunks when committing is authorized. Do not
   include unrelated staged changes or switch a dirty shared checkout to another
   PR. Reuse the correct existing PR instead of opening a duplicate. Verify the
   tested commit and remote head after an authorized push.
6. Use [review and follow-up](review-and-merge.md#review-a-pull-request-and-follow-up)
   to reconcile findings and CI before delivery. Describe public source, CLI,
   artifact and Runtime changes from the actual diff; a `fix` or `refactor`
   commit label does not establish compatibility. Leave partial issues open and
   link remaining work rather than treating a merged slice as full completion.

Existing user authorization persists. A workflow does not introduce a repeated
permission gate for scoped local repairs, nor grant permission to post messages,
change assignments/projects, rewrite shared history or merge. If a remote create
request has an uncertain response, read back the target before retrying to avoid
duplicate issues or PRs.

The repo-local project-manager skill routes this flow; `pycircuit-pr-review`
provides independent implementation review, while `pycircuit-design-review`
retains proposal review. Shared policy stays in these development documents.

These practices adapt PyPTO's
[pass-bug intake](https://github.com/hw-native-sys/pypto/blob/8325a8de8422678d6699e17f31cef953ace66445/.github/ISSUE_TEMPLATE/pass_bug.yml),
[performance intake](https://github.com/hw-native-sys/pypto/blob/8325a8de8422678d6699e17f31cef953ace66445/.github/ISSUE_TEMPLATE/performance_issue.yml),
[RFC intake](https://github.com/hw-native-sys/pypto/blob/8325a8de8422678d6699e17f31cef953ace66445/.github/ISSUE_TEMPLATE/plan_rfc.yml)
and [shared developer skills](https://github.com/hw-native-sys/pypto-skills/tree/af1d7a016ce50ba109c4b4224580a6b758bde7da/plugins/pypto-developer/skills).
Its [repository skills](https://github.com/hw-native-sys/pypto/tree/8325a8de8422678d6699e17f31cef953ace66445/.claude/skills)
also inform scoped cross-layer review and artifact comparison. Its private
runner images, credential/session management, automatic approval infrastructure
and unconditional confirmation/full-suite policies are not pyCircuit defaults.

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
- publish the `pycircuit-hisi` package bundling the single pycircuit
  source-unit compiler and `pyc6_runtime`;
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
