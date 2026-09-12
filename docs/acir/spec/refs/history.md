# Historical Repository Record

Agentic Circuit development moved from `PTO-ISA/agentic-circuit` into this
repository. The source repository is now private and archived; its issues are
disabled and its remaining pull requests are closed. It is provenance, not a
supported source, package, issue tracker, or compatibility path.

## Consolidation record

| Item | Revision or disposition |
| --- | --- |
| pyCircuit import parent | `1f1651f9bff4293deb1613324ab575b3322ab38b` |
| Agentic Circuit import parent | `756002e2998b11dfe1fed14dc3d63cdad8be694c` |
| Historical import tag | `agentic-circuit/import-0.4` |
| Consolidation merge | `824a8434f72ba4c5da28402002c7c91897f120db` |
| Responsibility-layout hard break | Decision 0157 |
| Consumer-design separation | Decisions 0158 and 0235 |

The import was a non-squash merge, so original commits and authorship remain
reachable. Source pull request #23 was migrated and reviewed through pyCircuit
PR #5. Source pull request #18 was closed after its remaining work was tracked
and implemented in pyCircuit. Git history preserves the detailed collaboration
inventory that was used during cutover.

The supported distribution and import names remain `agentic-circuit` and
`agentic_circuit`; those are current product identities, not compatibility
aliases. ACIR and ACSim also remain independent compiler layers. Only the old
repository-development workflow and source-tree paths were retired.

## Pre-migration plans and audits {#REF-HISTORY-001}
<!-- ndf: kind=info level=may layer=L0 status=stable -->
<!-- ndf: origin-kind=git repository=https://github.com/PTO-ISA/agentic-circuit.git revision=5514f886f9967d3f06551f030b74d1fcaccd383e origin-status=verbatim -->

Commit `5514f886f9967d3f06551f030b74d1fcaccd383e` is the final tree before
the release-layout hard break. It contains the complete dated implementation
plans, superseded design proposals, phase audits, and their original paths.
This includes the epoch 0.4 Table-abstraction prototype formerly published as
an active design page; current Table semantics are defined by the specification
and Decisions 0237 through 0241.

NDF clauses in the current tree retain stable decisions and verification
relationships. Git history retains the full historical prose.
