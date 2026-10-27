# Review and merge

Review the change against its approved contract and its exact candidate. The
active product route is `pycircuit compile` → `pycircuit link` →
`pycircuit emit`; the current source profile is intentionally bounded.

## Review checklist

- Does the change stay within the approved Python, IR, CLI, runtime, and
  packaging contracts?
- Are typed module definitions, separate instances, stateless rules, explicit
  global-template bindings, explicit ordinary Rule registration, once-only
  autorule normalization and standard
  storage leaves described consistently? Do Work
  outputs observe old Q and whole-system checks precede Xfer commits?
- Are unsupported capabilities rejected without a fallback route?
- Does each source compile independently, with parent compilation consuming
  published interfaces and link receiving the complete closure?
- Do C++ and Verilog use the same verified final artifact and preserve source
  ownership?
- Does Runtime-only CMake discovery avoid LLVM/MLIR, while CompilerDev pins
  exact 22.1.8?
- Are generated output protections, diagnostics, and package inventories
  covered by candidate-bound evidence?
- Do active docs distinguish current support from remaining capability
  backlog and historical callers?

## Evidence and status

A reviewer should be able to reproduce the relevant gate from the stated
checkout and inspect raw output. A plan, fixture, test name, or previous
revision's evidence is not a current pass. Record skipped checks and unresolved
callers explicitly. Documentation-only work must not claim implementation or
execution results.

## Review a pull request and follow up

Establish the repository, PR number, base branch/SHA, head repository/branch/SHA
and merge base before reviewing. Review the full merge-base-to-head diff and
relevant surrounding code; a new push does not limit the review to the last
commit. Match any local checkout to that head and record local edits separately.
A changed base or head invalidates the prior review's claim about the candidate.

Read the PR description, inline review threads, submitted review bodies and
conversation comments. Paginate each connection, including comments inside
threads; the first page or `gh pr view --comments` alone is not a complete
feedback inventory. Treat feedback as evidence to check against current code.
A resolved thread does not prove a fix, and an out-of-diff review finding must
not disappear because it lacks an inline anchor.

Classify each actionable finding as addressed, still open, disputed with concrete
evidence, or deferred with an explicit reason/follow-up. Deduplicate the same
root cause while retaining its original thread/comment IDs. Assess technical
content equally for bot and human feedback. Use existing authorization to repair
bounded defects; genuinely new semantic decisions retain the proposal/review
boundary in [project governance](project-governance.md).

For each finding, report severity, the triggering case, impact, precise location
and supporting evidence. Keep optional style suggestions separate from blocking
correctness findings; do not raise their severity to fit a reporting threshold.
An independent reviewer reports `clear`, `findings`, or `incomplete` with the
reviewed base/head (and local content identity if applicable), coverage and tests
actually run. Missing diff, discussion or required context means `incomplete`,
not a clean review. `Clear` is a review result, not merge approval or proof that
tests passed.

During authorized PR follow-up, inspect checks for the current head. For a
GitHub Actions check, inspect its run/job status before fetching logs; whole-run
logs may be unavailable until the run completes. External checks use their
provider's details. Pending, canceled, unavailable and failed checks are distinct
from success. Attribute failures to this change, the baseline or infrastructure
only when the evidence supports that classification.

After a repair, rerun affected checks and obtain independent review where
required. When pushing is authorized, verify the remote PR head equals the
tested local commit. Posting replies and resolving threads require authorization
for those actions. An authorized reply cites the pushed fix and evidence; resolve
only an addressed inline thread after the reply succeeds and verify its resolved
state. Disputed/deferred findings remain open; review bodies and conversation
comments are not resolvable threads.

Before declaring the PR ready, recheck its base/head, all feedback surfaces and
required checks. Reinspect changed code if either revision moved. While CI runs,
poll with backoff and work on independent known fixes; perform the complete
feedback recheck when checks finish rather than repeatedly downloading unchanged
threads. Stop a repair loop when the same head, findings and failure recur without
new evidence; report the concrete blocker. Never infer readiness solely from
green CI or automatically merge because a review is clear.

See [testing and gates](testing-and-gates.md), the
[source-unit workflow](source-unit-workflow.md), and the active
[language reference](../reference/language.md).
