## Summary

<!-- Describe the problem, trigger, impact, and resulting behavior. -->

## Change type

- [ ] Bug fix
- [ ] Feature / behavior change
- [ ] Performance
- [ ] Refactoring / cleanup
- [ ] Tests / CI
- [ ] Documentation / configuration

## Related issues

<!-- Use Fixes #123 only when this PR closes the issue; otherwise Related to #123.
List prerequisite PRs as Depends on <PR URL>; distinguish them from background
references and state whether they have merged. Write None if no issue applies. -->

## Changes

<!-- Identify the key files/owners and affected source, IR, runtime or package
contracts. Link decision records for semantic changes. Include hard breaks,
removed callers and documentation updates; use N/A with a reason when needed.
For cross-layer changes, identify affected capture, MLIR/IR, publication/link,
C++/Verilog, Runtime/System, CLI/package and test/doc owners. Explain unaffected
boundaries rather than requiring unrelated changes in every layer. -->

## Validation

<!-- Record actual commands, configuration, exit status and outcomes, tested
commit SHA (plus local diff/content identity), and CI artifact/log links.
Use PASS / FAIL / NOT RUN / N/A with reasons. A local docs/gates/logs/<run-id>/
path must be accompanied by an accessible summary or uploaded CI artifact.

Choose relevant checks from docs/development/testing-and-gates.md:
  bash tools/run_api_tests.sh --tier gate
  bash tools/run_examples.sh --tier gate
Record exact selected tier/tests. C++/Verilog evidence must use the same verified
final IR; include generated @system execution when affected. Runtime/package
changes need the relevant installed-consumer checks.

Long oracle/coverage/mutation matrices stay in existing nightly entrypoints;
do not run deferred coverage or count it as passed. Documentation/template-only
changes may omit hardware regressions with a reason and appropriate syntax,
metadata, link and documentation checks. Scripts are not automatically exempt.
Do not leave illustrative commands here as if they had been executed. -->

<!-- For performance claims, compare baseline/candidate SHAs using identical
inputs, build settings and workers. Give units, repetitions and variation plus
correctness results. Output text differences alone do not prove equivalence. -->

- [ ] Executed checks and outcomes are recorded above
- [ ] Skipped, deferred or unavailable checks and reasons are recorded above

## Review checklist

<!-- Check applicable items; explain exceptions in Reviewer notes. These boxes
do not replace required CI, independent review or candidate acceptance. -->

- [ ] Scope and affected contracts are identified; new proposals are distinguished from approved behavior
- [ ] Tests cover relevant supported behavior and rejection/rollback boundaries
- [ ] Both backends preserve common-IR semantics where affected; no fixture-specific admission or fallback shim
- [ ] Active documentation and remaining callers are updated where needed
- [ ] No generated build output, temporary evidence logs or secrets are committed
- [ ] Required CI and independent reviews are complete for this candidate

## Reviewer notes

<!-- Identify risky paths, test gaps, remaining limitations and follow-up issues.
Record independent implementation/test/review roles and actual model/effort for
agent-managed work when required by project governance. Write None if no notes
apply. Do not claim a review or full nightly pass that has not occurred. -->
