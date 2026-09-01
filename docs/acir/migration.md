# Agentic Circuit Repository Migration

Agentic Circuit source and development authority are consolidated from
`PTO-ISA/agentic-circuit` into `PTO-ISA/pyCircuit`. The result is one active
source repository without collapsing the ACIR and PYC semantic layers. The old
repository remains public only until the operational retirement gate closes.

## Recorded baselines

| Item | Revision |
| --- | --- |
| pyCircuit import parent | `1f1651f9bff4293deb1613324ab575b3322ab38b` |
| Agentic Circuit import parent | `756002e2998b11dfe1fed14dc3d63cdad8be694c` |
| Import tag | `agentic-circuit/import-0.4` |
| Consolidation merge | `824a8434f72ba4c5da28402002c7c91897f120db` |
| Migrated PR #23 merge | `a08618d48b595d67ca7d7e0a1b238ea45e4b80df` |

The import is a non-squash merge. Original Agentic Circuit commits remain
reachable, while the imported working tree is namespaced under
`components/agentic-circuit/`.

The implementation is reviewed in
[pyCircuit PR #4](https://github.com/PTO-ISA/pyCircuit/pull/4). The migrated
ACIR-to-C++ work from source PR #23 is reviewed in
[pyCircuit PR #5](https://github.com/PTO-ISA/pyCircuit/pull/5).

## Compatibility commitments

- Preserve `agentic-circuit` as a distribution and CLI.
- Preserve `agentic_circuit` as the import namespace.
- Preserve ACPy epoch `0.4` and canonical ACIR unless a later accepted decision
  records an intentional break.
- Preserve ACIR and ACSim dialect/tool names.
- Replace obsolete external pyCircuit and `libpyc4_runtime` assumptions with
  repository-local pyCircuit 6 and `libpyc6_runtime` contracts.
- Keep Cycle-Aware Signal as the canonical pyCircuit 6 hardware frontend.

## Collaboration migration

Every open issue, pull request, and source branch receives an explicit target
record. Pull requests cannot be transferred across GitHub repositories, so
their commits, authorship, descriptions, review conclusions, and source links
must be preserved in replacement pyCircuit pull requests or a documented
superseded record.

The collaboration inventory is maintained in
[`agentic-circuit-collaboration.md`](agentic-circuit-collaboration.md).

## Cutover rule

Code consolidation is complete. Repository retirement is not complete: the
current QEMU/PYC comparison still requires a current independent PYC producer
or reviewed current fixture. Until that gate passes, keep the source repository
public and unarchived, keep source PR #18 and #23 open as provenance records,
and do not disable its remaining review surface.

The standalone repository remains active until:

1. the AC frontend, ACIR/ACSim, gfsim, and ACIR-to-PYC closure passes;
2. the full pyCircuit 6 and required Linx integration closure passes on the
   same integrated revision;
3. evidence is archived under one `docs/gates/logs/<run-id>/` bundle; and
4. issues, pull requests, branches, packages, secrets and webhooks have a
   recorded disposition.

After those conditions pass, publishing authority is removed from the old
repository. It is converted to private, direct/team/outside access is removed,
`zhoubot` remains the only explicitly granted repository user (subject to
GitHub organization-owner access), and the repository is archived read-only.
