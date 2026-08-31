# Contributing to pyCircuit

pyCircuit is maintained by PTO-ISA as the canonical repository for the pyc6
hardware authoring and compilation stack. Contributions must preserve the
documented semantic contracts and include the gate evidence appropriate to the
change.

## Before you start

Read these sources of truth before changing behavior:

- [`docs/rfcs/pyc6-decisions.md`](docs/rfcs/pyc6-decisions.md)
- [`docs/pyc6-plan.md`](docs/pyc6-plan.md)
- [`docs/gates/decision_status_v6.md`](docs/gates/decision_status_v6.md)
- [`docs/development/contributing-workflow.md`](docs/development/contributing-workflow.md)
- [`docs/development/testing-and-gates.md`](docs/development/testing-and-gates.md)
- [`docs/development/review-and-merge.md`](docs/development/review-and-merge.md)

Semantic changes must name the affected pyc6 decision IDs. Enforce semantics
in the dialect, MLIR passes, or verifiers before relying on a backend behavior;
backend-only semantic fixes are not accepted. Cycle-aware signals remain part
of the pyc6 design and must follow the current decision corpus.

## Prerequisites

- Python 3.10 or newer
- LLVM/MLIR 22
- CMake 3.20 or newer
- Ninja
- Verilator 5.024 or newer for Verilator targets (CI pins 5.048)
- Git

Build and test only from the current checkout. Do not copy toolchains, shared
libraries, or generated outputs from another worktree.

## Set up a checkout

```bash
git clone https://github.com/PTO-ISA/pyCircuit.git
cd pyCircuit
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -e ".[dev,docs]"
bash flows/scripts/pyc build
```

## Validate a change

Start with the smallest lane that proves the change, then widen as required by
the validation matrix in `docs/development/testing-and-gates.md`.

```bash
pre-commit run --all-files
pytest tests/unit -m unit
python3 flows/tools/check_api_hygiene.py \
  compiler/frontend/pycircuit designs/examples docs README.md
mkdocs build
```

Compiler, runtime, example, or simulation changes normally also require:

```bash
bash flows/scripts/run_examples.sh
bash flows/scripts/run_sims.sh
```

Use `bash flows/scripts/run_sims_nightly.sh` and the Linx integration lanes when
the validation matrix calls for them. Archive semantic or flow-significant
evidence under `docs/gates/logs/<run-id>/`.

## Pull requests

Open a pull request against `PTO-ISA/pyCircuit` and complete the repository PR
template. A merge-ready change includes:

1. a focused summary and motivation;
2. affected pyc6 decision IDs or an explicit statement that none are affected;
3. exact gate commands and evidence paths;
4. documentation updates for changed behavior or workflow;
5. compatibility and residual-risk notes.

Use focused commits and do not add AI co-author trailers. CODEOWNERS review is
required for governed areas. CI runs pre-commit, unit tests, API hygiene, the
documentation build, LLVM 22 toolchain build, examples, simulations, and
wheel smoke; macOS and nightly lanes run according to their workflow triggers.

If GitHub Issues are unavailable, use a draft pull request with a minimal
reproducer to propose a non-security bug fix. Never disclose a vulnerability
in an issue or public pull request; follow [`SECURITY.md`](SECURITY.md).

## Conduct and licensing

Participation is governed by [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md). By
contributing, you agree that your contribution is licensed under the repository
[`LICENSE`](LICENSE).
