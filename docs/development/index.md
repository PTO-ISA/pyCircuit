# Development Guide

pyCircuit 6 development is decision-driven and gate-first. CycleAwareSignal and
automatic cycle balancing are current product contracts, not compatibility
surfaces.

The same repository also owns Agentic Circuit, ACIR/ACSim, and gfsim. Changes
under the AC module roots defined by Decision 0157 follow the AC G0/G1/G2
matrix and keep the separate frontend and IR boundaries defined by Decision
0150.

## Core references

- [V6 language specification](../reference/language.md)
- [pyCircuit 6 decisions](../rfcs/pyc6-decisions.md)
- [pyCircuit 6 evolution plan](../pyc6-plan.md)
- [Decision status](../gates/decision_status_v6.md)
- [Evidence contract](../gates/README.md)

## Contributor workflow

- [Contributing workflow](contributing-workflow.md)
- [Testing and gates](testing-and-gates.md)
- [Stable SDK release contract](sdk-release-contract.md)
- [Review and merge](review-and-merge.md)
- [Repository management](repository-management.md)
- [Repository layout](repository-layout.md)

## Build and gate commands

```bash
bash flows/scripts/pyc build
bash flows/scripts/run_examples.sh
bash flows/scripts/run_sims.sh
bash flows/scripts/run_sims_nightly.sh
bash flows/scripts/run_agentic_circuit.sh
python3 flows/tools/summarize_gate_run.py --run-id <run-id>
```

GitHub Actions runs the configured pull request lanes. The nightly workflow
exercises only heavy-tier simulations; normal and semantic simulations have
separate owners. Use the same `PYC_GATE_RUN_ID` across related local lanes so
evidence lands in one reviewable directory.

## Local environment

After building the toolchain:

```bash
export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/toolchain/install"
export PATH="$PYC_TOOLCHAIN_ROOT/bin:$PATH"
export PYC_GATE_RUN_ID="local-$(date +%Y%m%d-%H%M%S)"
bash flows/scripts/run_examples.sh
bash flows/scripts/run_sims.sh
```

## Repository layout

The repository is organized by responsibility rather than by one build flow.
See [Repository Layout](repository-layout.md) for the complete ownership map,
placement checklist, and automated layout contracts.

Use the repository's GitHub issue and pull request surfaces only when enabled by
the PTO-ISA organization. Do not document unofficial support channels as
maintained project infrastructure.
