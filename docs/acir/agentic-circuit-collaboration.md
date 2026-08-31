# Agentic Circuit Collaboration Migration

Source repository: `PTO-ISA/agentic-circuit`
Inventory baseline: `756002e2998b11dfe1fed14dc3d63cdad8be694c`

## Open pull requests

| Source | Title | Source head | Migration disposition |
| --- | --- | --- | --- |
| [#18](https://github.com/PTO-ISA/agentic-circuit/pull/18) | Popcount and round-robin ACIR-to-PYC lowering | `feature/issue11-pyc-phase1-wsl` | Source head preserved as `agentic-circuit/pr-18-head`; [pyCircuit issue #6](https://github.com/PTO-ISA/pyCircuit/issues/6) records the pyc6 replacement and superseded disposition |
| [#23](https://github.com/PTO-ISA/agentic-circuit/pull/23) | ACIR-to-C++ generation pipeline | `xiekunpeng:feature/acir-emit-cxx` | Four unique commits migrated with authorship and merged through [pyCircuit PR #5](https://github.com/PTO-ISA/pyCircuit/pull/5) at `a08618d4`; source head preserved as `agentic-circuit/pr-23-head` |

Source PRs remain open until the target-side closure conditions are satisfied.
They are provenance records, not active merge targets for new source changes.

## Open issues

| Source | Title | Target |
| --- | --- | --- |
| [source #7](https://github.com/PTO-ISA/agentic-circuit/issues/7) | Generated C++ embeds full SHA-256 in names | Transferred to [pyCircuit #7](https://github.com/PTO-ISA/pyCircuit/issues/7) with `area:acir` |
| [source #8](https://github.com/PTO-ISA/agentic-circuit/issues/8) | Add atomic `ac.try_transfer` primitive | Transferred to [pyCircuit #8](https://github.com/PTO-ISA/pyCircuit/issues/8) with `area:acir` |
| [source #14](https://github.com/PTO-ISA/agentic-circuit/issues/14) | Parameterized blocks, SimQueue ABI and provider specialization | Transferred to [pyCircuit #9](https://github.com/PTO-ISA/pyCircuit/issues/9) with `area:acir` |
| [source #26](https://github.com/PTO-ISA/agentic-circuit/issues/26) | Suggested ACPy/ACIR table design | Transferred to [pyCircuit #10](https://github.com/PTO-ISA/pyCircuit/issues/10) with `area:acir` |
| [source #27](https://github.com/PTO-ISA/agentic-circuit/issues/27) | Correlated messages across independent queues | Transferred to [pyCircuit #11](https://github.com/PTO-ISA/pyCircuit/issues/11) with `area:acir` |

## Branch and access audit

- Preserve `agentic-circuit/import-0.3` at the source main baseline.
- Record every source branch as imported, migrated, superseded, or intentionally
  retained before repository closure.
- Revoke old Actions, environments, deploy keys, webhooks and publishing
  credentials after the final gate bundle passes.
- Remove teams and outside collaborators before changing visibility.
- Verify effective private-repository access after cutover and attach the audit
  result to the final migration evidence.
