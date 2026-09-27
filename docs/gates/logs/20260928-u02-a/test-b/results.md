# U02-A independent review-b verification

Candidate baseline HEAD: `03625a3dc3b368be9dbac4bfff7ffdb1d3b54466`

Fresh test-b uses the current-checkout compiler and harness built under `.pycircuit_out/u02-modules/test-b/build`.

## Results

- Native U02-A module/reset/snapshot contracts: 19/19 passed.
- Preserved source contracts: 36/36 passed.
- Preserved SourceUnit/header tests: 14/14 passed.
- Preserved N1 namespace/fixed-identifier tests: 14/14 passed.
- System tests: 76/76 passed, zero skips: 51 packet/source-admission, 5 N1 namespace, 20 U02-A module tests.
- Combined selected tests: 159/159 passed.
- CTest discovery: exactly 4 isolated targets.
- CTest execution: 4/4 passed.
- clang-format, Ruff format/check, and `GIT_WORK_TREE="$PWD" git diff --check` passed.
- Test sources are 580, 507, and 464 lines; every file is below 600 lines.

## Closed review-a coverage

- Structurally valid child headers with required, defaulted, or explicitly supplied static parameters all fail closed in U02-A and never lower to empty `ac.static_args`.
- Connection Default metadata remains accepted. Parent omission and literal actuals reject; an explicit parent DFFE handle succeeds.
- Valid inactive `@rule` code may read `self.result`, emits no active rule, and does not change the registered result W-only effect summary.
- Inactive rules reject missing returns, unsupported calls, unknown self members, unbound locals, and wrong return types.
- Missing/bogus source stage and unit kind fail closed.
- Positive `Request(3, True)` reset remains accepted.
- Source and mutated-IR negatives reject Word/bool mismatch, wrong nominal record, missing/wrong constructor, duplicate and unknown constructor arguments.
- `verifyBodySnapshots` accepts the canonical body and rejects isolated constructor-body, parameter-default, record-field order/type, snapshot-role, and snapshot-owner tampering while the owning header remains byte-for-byte unchanged.
- A body retaining snapshots fails when its provider header is omitted.

An initial test-b oracle incorrectly treated inactive `return self.result` as forbidden by the registered W-only effect summary. Independent review corrected this: inactive methods contribute no active effects, so ordinary typed member reads remain legal. The final oracle makes this a positive and uses genuinely malformed inactive bodies as negatives.

## Scope

This evidence remains bounded to U02-A source module signatures, DFFE/reset, rule/instance shape, registered effects, static-argument fail-closed behavior, snapshots, and header-only parent compilation. It does not claim original full Accumulator/Core U02-B math/SCF, linking, final commit closure, backend execution, five-category completion, or installed SDK behavior.
