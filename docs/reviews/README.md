# Modernization Reviews

This directory stores independent design, code, architecture-conformance, and
integration reviews for the modernization program. Review independence and
acceptance rules are defined in
[Project Governance](../development/project-governance.md).

Name a review `<candidate>-<review-kind>.md` and record:

- proposal or candidate path and SHA-256;
- date, logical role, actual model, effort, and reviewer independence;
- scope and evidence examined;
- verdict: `approval-ready`, `revise`, `blocked`, or `accepted` as applicable;
- findings, disposition, residual risks, and invalidation conditions.

An `approval-ready` design review is permission to request the user's precise
approval. It is not approval of the interface or authorization to implement it.

## Completed reviews

- [C2-DECL scalar final projection](20260930-c2-decl-scalar-final-design-review.md): independent Astra xhigh approval-ready, exact revision-A hash archived; precise user approval pending, no product implementation.

- [Python numeric producer bridge](20260929-python-numeric-bridge.md): bounded real Python add/sub/comparison production through C1/D1 lowering accepted; 32 numeric and 68 combined system tests passed, with 19/19 ACIR targets. Numeric next-state and backend admission remain open.

- [M1 C and agent handoff](20260928-interface-handoff.md): independent Astra/high approval-ready for the Pythonic/interface design, W00–W12 checklist and V00–V49 matrix. Source semantics, output transport and QUIESCENT closure reviewed; product implementation remains unverified.
- [C2-M1 revision A history](20260928-c2-m1-module-system.md): initial review dispatch was unavailable; superseded for current design review by the bound M1 C handoff above.

- [C2-R1 unified register design](20260928-c2-r1-unified-register.md): independent Astra xhigh revision B is approval-ready after collection/initializer, hierarchical fatal projection and scoped retirement repairs. Exact user approval and implementation remain pending.

- [Migration governance activation](../gates/logs/20260927-migration-governance/independent-review.md): independent code review passed; no product-interface approval.
- [C1 source proposal revision A](../gates/logs/20260927-c1-source-review/revision-a-review.md): revise; superseded by the reviewed revision C below.

- [C1 source proposal revision C](../gates/logs/20260927-c1-source-review/revision-c-review.md): approval-ready; [user approval recorded](../rfcs/migration/approvals/c1-pythonic-source.md).

- [C1 private source capture](../gates/logs/20260927-c1-capture/review.md): independent code review PASS; integrated with 36 focused and 253 unit tests.
- [C2 MLIR proposal revision B](../gates/logs/20260927-c2-review/revision-b-review.md): revise; two remaining IR-contract findings, no interface approval.
- [C2 MLIR proposal revision C](../gates/logs/20260927-c2-review/revision-c-review.md): approval-ready; [user approval recorded](../rfcs/migration/approvals/c2-c3-foundation.md).
- [C3 driver/runtime proposal revision B](../gates/logs/20260927-c3-review/revision-b-review.md): revise; publication recovery and exact RTL parameter acceptance require repair.
- [C3 driver/runtime proposal revision C](../gates/logs/20260927-c3-review/revision-c-review.md): approval-ready; [user approval recorded](../rfcs/migration/approvals/c2-c3-foundation.md).
- [Capability and retirement inventory audit](../gates/logs/20260927-capability-inventory/revision-b-review.md): PASS for inventory completeness/status; no product validation.
- [C2-F01 foundational MLIR implementation](../gates/logs/20260927-c2-f01/review-b/summary.md): independent Sol high review PASS; [primary-checkout integration](../gates/logs/20260927-c2-f01/integration/results.md) passed 17 GTest and 4 lit cases.
- [C2-F02 private type/value contracts](../gates/logs/20260927-c2-f02/review-b/summary.md): independent Sol high review PASS; [primary integration](../gates/logs/20260927-c2-f02/integration/results.md) passed 34 GTest and 4 lit cases; real header authority remains pending.
- [C2-F03 identity contracts](../gates/logs/20260927-c2-f03/review-b/summary.md): independent Sol high PASS; [primary verification](../gates/logs/20260927-c2-f03/integration/results.md) passed 42 GTest and 4 lit cases; context/unit/link validation remains pending.
- [U01 private source transport](../gates/logs/20260927-u01-transport/review-b/summary.md): Sol high PASS after test portability repair; [primary verification](../gates/logs/20260927-u01-transport/integration/results.md) passed 266 unit and 4 configured parser tests. Native source/header compilation remains active.
- [U01 transport filesystem portability](../gates/logs/20260927-u01-transport/review-c/summary.md): Sol high PASS for synthetic quoted-path fixtures; the serializer is unchanged and 17 focused primary tests pass.
- [U01 native source/header review A](../gates/logs/20260927-u01-native/review-a/findings.md): Sol high REVISE, 12 findings; [independent test A](../gates/logs/20260927-u01-native/test-a/results.md) is RED. Superseded by review C below.
- [U01 native review C](../gates/logs/20260927-u01-native/review-c/summary.md): Sol high PASS; [acceptance](../gates/logs/20260927-u01-native/acceptance.md) binds 101 passing tests to isolated commit d6fb408e, without driver/final/backend claims.
- [C2-N1 namespace revision B](../gates/logs/20260928-c2-n1-review/revision-b-review.md): revise, validation packet only; no architecture blocker.
- [C2-N1 namespace revision C](../gates/logs/20260928-c2-n1-review/revision-c-review.md): independent Astra xhigh approval-ready; [user approval](../rfcs/migration/approvals/c2-n1-namespaces.md) recorded.
- [U02-A0 shared frontend extraction](../gates/logs/20260928-u02-a0/review-a/summary.md): independent Sol high PASS; [acceptance](../gates/logs/20260928-u02-a0/acceptance.md) binds 101 passing regressions and six unchanged artifacts to isolated commit b797eaf77. N1 is a separate candidate.
- [N1 source/header review A](../gates/logs/20260928-u02-n1/review-a/summary.md): Sol high REVISE; 116 selected tests pass, but two provenance repairs and four test-coverage gaps remain. No N1 acceptance.
- [Typed DUT I/O revision A](../gates/logs/20260928-dut-io-review/revision-a/independent-design-review.md): independent Astra xhigh REVISE, four exact contract/validation gaps; resource memo accepted only as advisory guidance. No interface approval.
- [N1 source/header review C](../gates/logs/20260928-u02-n1/review-c/summary.md): Sol high PASS after coverage/provenance/portability repairs; [acceptance](../gates/logs/20260928-u02-n1/acceptance.md) binds 126 selected passes to isolated commit 03625a3d. Three-category subset only.
- [Typed DUT I/O revision B](../gates/logs/20260928-dut-io-review/revision-b/independent-design-review.md): independent Astra xhigh approval-ready; precise user approval requested. No implementation authorization yet.
- [U02-A source modules](../gates/logs/20260928-u02-a/review-b/summary.md): Sol high PASS; [acceptance](../gates/logs/20260928-u02-a/acceptance.md) binds 159 passing selected tests and CTest 4/4 to isolated commit 946cd022. Original Accumulator/Core and backends remain open.
- [C2-A2 source-use revision B](../gates/logs/20260928-c2-a2-review/revision-b-review.md): independent Astra xhigh approval-ready; its lexical/reg rebasing was later approved through the R1/M1 joint implementation package.
- [N1 scalar constants review A](../gates/logs/20260928-u02-n1-constants/review-a.md): Sol high COMMENT; exact payload and same-value identity oracles need repair before local acceptance.
- [U02-B source math review A](../gates/logs/20260928-u02b-math/review-a.md): Sol high REQUEST CHANGES for linked-stage, `from_bits` provenance, and rule/helper ownership gaps; superseded by review B.
- [N1 scalar constants review B](../gates/logs/20260928-u02-n1-constants/review-b.md): Sol high PASS for repaired payload, same-value identity, and stale snapshot oracles; [acceptance](../gates/logs/20260928-u02-n1-constants/acceptance.md) is limited to scalar literal source/header behavior.
- [U02-B math repair design](../gates/logs/20260928-u02b-math/repair-plan.md): independent Astra xhigh architecture diagnosis keeps stage, calculation owner, and actual-SSA domain provenance within approved C2; revised implementation and review remain open.
- [U02-B rule analysis review A](../gates/logs/20260928-u02b-rule-analysis/review-a.md): Sol high REQUEST CHANGES for asymmetric alias deduplication and subscript Store-base read classification; superseded by review B.
- [U02-B rule analysis review B](../gates/logs/20260928-u02b-rule-analysis/review-b.md): Sol high PASS after both repairs; [acceptance](../gates/logs/20260928-u02b-rule-analysis/acceptance.md) is bounded to input identity and Store-target read classification.
- [U02-B source math review B](../gates/logs/20260928-u02b-math/review-b.md): Sol high PASS after all three authority repairs; [acceptance](../gates/logs/20260928-u02b-math/acceptance.md) covers only source-math ownership and local declaration provenance.
- [N1 module facade source/header review](../gates/logs/20260928-u02-n1-module/review.md): Sol high PASS; [acceptance](../gates/logs/20260928-u02-n1-module/acceptance.md) covers a real header-only consumer with two child instances and stale binding rejection.
- [C2-L1 current-read revision A](../gates/logs/20260928-c2-l1-review/revision-a-review.md): independent Astra xhigh REVISE; four contract/verification packet gaps.
- [C2-L1 current-read revision B](../gates/logs/20260928-c2-l1-review/revision-b-review.md): independent Astra xhigh approval-ready; its lexical/reg rebasing was later approved through the R1/M1 joint implementation package.
- [W02–W04 next-scalar candidate](20260928-w02-w04-next-scalar.md): independent Sol high PASS on the bounded unified-reg, lexical frontend, and source read/use slice; P3/header/link/runtime/backend closure remains open.
- [W04-P3 / W05 effects authority](20260928-w04-p3-w05-effects.md): independent acceptance PASS for IR-derived header publication and snapshot recomputation; helper/dynamic/link/runtime/backend closure remains open.
- [W06 ModuleGraph / StateID link](20260928-w06-module-graph.md): independent acceptance PASS for the bounded private instance graph, ownership, and actual-StateID alias closure; runtime and non-scalar closure remain open.
- [W07 ProposalGraph / conflict closure](20260928-w07-proposal-graph.md): independent acceptance PASS for private StateID proposal grouping, conflict recomputation, commit recipes, and link admission; check materialization, final lowering, runtime, and emitters remain open.
- [W08 GFSIM DFFE / flat system](20260928-w08-gfsim-runtime.md): independent acceptance PASS for pending D/E/reset publication, discard semantics, flat lifecycle, and failure-atomic Xfer barriers; generated adapter and publication remain open.
- [W09 observations](20260929-w09-observations.md): independent acceptance PASS for the bounded source observe graph, canonical Python print/log/report producer, and allocation-free runtime slots; checks, conditional paths, runner events, and synthesis projection remain open.
- [W09 checks and link permit](20260929-w09-check-link.md): independent acceptance PASS for source assert, conditional instrumentation, CheckGraph, global commit permit, and two-instance observation ordering; numeric proofs and runner publication remain open.
- [W10 FinalSystem / backend closure](20260929-w10-final-system-closure.md): frozen realization, reusable hierarchy, structured observations, Numeric N0-A source ops, B0/B1a/B1b exact witnesses, and the bounded C1 input-add lowering are accepted; remaining lowering, Python numeric control flow, graph/backend arithmetic, and V41-V44 remain open.
- [Numeric N0-C1 scalar lowering](20260929-n0-c1-scalar-lowering.md): independent Astra architecture CLEAR and Sol code-review PASS for transactional multi-rule SourceRead-plus-constant add lowering; 45/45 source-math and 19/19 configured ACIR targets pass.
- [Numeric N0-D1–D3 closure](20260929-n0-d1-d3-numeric-closure.md): exact subtraction/comparisons, explicit masking/low-bits, and checked range conversion are accepted after independent tests and Sol review; source-math is 75/75 and strict V41–V44 is honestly 1 green/7 red.
- [C3 private publication review A](../gates/logs/20260928-c3-publication/review-a.md): Sol high REQUEST CHANGES for file targets, sorted lock sets, HeaderView/recovery separation and Windows durability; private P1 is not accepted.
- [C3 publication filesystem review A](../gates/logs/20260928-c3-publication/fs-review-a.md): Sol high REQUEST CHANGES for cross-parent rename durability and Windows directory-handle identity; platform repair remains unaccepted.
- [C3 private publication transaction review B](../gates/logs/20260928-c3-publication/review-b.md): Sol high REQUEST CHANGES for committed-unfinished full validation before stable projection; seven earlier transaction findings closed.
- [C3 publication filesystem review B](../gates/logs/20260928-c3-publication/fs-review-b.md): Sol high PASS for cross-parent flush and Windows handle-identity repair; actual Windows execution remains an open platform gate.
- [C3 private publication transaction review C](../gates/logs/20260928-c3-publication/review-c.md): Sol high PASS after committed-read full validation; [P1 acceptance](../gates/logs/20260928-c3-publication/acceptance.md) remains private and macOS-tested, with Windows runtime gate open.
- [C3 stable source and source-unit files review A](../gates/logs/20260928-c3-source-files/review-a.md): Sol high REQUEST CHANGES for ancestor symlink race/special-file capture and missing command-wide input lock integration.
