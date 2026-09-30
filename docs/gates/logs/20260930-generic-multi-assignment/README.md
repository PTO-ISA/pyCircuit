# Generic multi-assignment reconstruction repair — 2026-09-30

Base: `4fbf799669c5aa8807ce2f16f536d2c78c885671`. Candidate identity is the two
files in `candidate-files.json`: ProposalGraph.cpp and the independent source
regression. Checkout: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.

## Repair and preserved semantics

Generic final-use retention and dialect verification already support multiple
assignments. ProposalGraph had three stale singleton assumptions: one
RequiredUse, one data/enable pair during reconstruction, and the same pair during
verification. RequiredUse lookup now requires one exact UseID/source-ValueID
match. A shared helper resolves the target to exactly one output binding,
checks closed output/type/yield cardinality, and selects that target's
`2*i`/`2*i+1` operands in construction and verification. Missing or ambiguous
matches fail; accesses are checked before indexing. Null-target rejection is
also explicit in composition verification.

Only ProposalGraph.cpp changes product code. Numeric admission/closure,
FinalUses retention, generic final proof/inventory/anti-downgrade verification,
Python capture, both backends, runtime, ODS and public schemas are unchanged.
This repairs existing approved C1/C2 and C2-R1/M1 behavior; it adds no primitive
or new source form. In the nested-rule contract, a nonlocal captured reg write
proposes next, while rereading that name still reads Q. A local candidate is an
SSA value and can be reused immediately.

## Independent organization

- Root-cause analysis: multi_assignment_debug, debugger, Sol high, read-only.
- Implementation: unit_pair_fix, Luna high, exclusive ProposalGraph.cpp.
- Independent tests: multi_assignment_tests, separate Luna high, exclusive
  tests/system/test_generic_multi_assignment.py.
- PM: integration, changed-line formatting, native build, regression runs,
  evidence, docs and acceptance.
- Independent review: other_agent_code_review, Sol high, no authorship;
  [APPROVE](review.md), both hashes verified and focused suite independently rerun.

## Results

- Baseline raw `baseline-result.xml`, `baseline-pytest.log` and
  `baseline-command.txt`: four pytest failures consist of **two genuine positive
  source cases failing final link** plus **two negative-test setups blocked by
  that link failure**. The latter did not reach mutation and are not claimed as
  semantic negative-control failures.
- Final new regression: **5/5 passed**, no failures/errors/skips.
  `focused-final.xml/log/command.txt` were produced after test formatting.
- Existing Python/system lanes: **68/68 passed**, no failures/errors/skips.
  See `existing-python.xml/log/command.txt`.
- Native: **78/78 passed** (ProposalContracts 9, FinalProgram 51, executable
  backend closure 18), no failures/errors/skips/disabled. Logs/XML per binary;
  exact commands in `native.command.txt`.
- Native rebuild from this checkout passed; `build.log` and `build.command.txt`.
  The linker retained its pre-existing duplicate-library warnings.
- Applicable pre-commit checks passed: merge/whitespace, Ruff, Black, Markdown
  and API hygiene. YAML had no applicable files. No full release/SDK/platform
  matrix is claimed.

Each positive compiles types, child and root separately, links and saves final,
then emits in a fresh process. Monolithic C++, independently compiled source
C++ groups and RTL execute the same expected traces, including reset/rerun:

| Source case | Four observed values per run |
| --- | --- |
| Persistent count assignment then reread old Q | `[0, 0, 3, 10]` |
| One local candidate reused for two targets | `[3, 10, 3, 10]` |
| First output enabled, second output independently disabled | `[0, 200, 3, 200]` |

Repeated child definitions preserve separate storage; borrowed outputs add no
relay registers. Second-target redirection and duplicate-use-ID mutations are
rejected by both emit backends with rc=1, semantic diagnostics and no output.
The disabled case specifically distinguishes each output's enable from pair 0.

## Migration position

This closes the concrete M3 reconstruction issue discovered during M4 source-TU
work; bounded M2 stays accepted. M4's remaining declaration headers/generated
publication remain open. Parallel read-only architecture review found the
precise declaration-only final projection still needs a small contract packet:
[readiness note](../../../work-items/m4-declaration-final-readiness.md). No such
schema expansion or SYSTEM/EXPECT revision B change was implemented here.
