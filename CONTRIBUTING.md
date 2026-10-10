# Contributing to pyCircuit

pyCircuit is a Python hardware language and compiler maintained by PTO-ISA.
All source authoring uses the `pycircuit` package and the same MLIR pipeline.

## Before you start

Read the [language reference](docs/reference/language.md),
[known limitations](docs/development/known-limitations.md),
[contributing workflow](docs/development/contributing-workflow.md) and
[testing guide](docs/development/testing-and-gates.md).

Implement hardware semantics in the existing MLIR analyses, verifiers and
passes. Keep capture thin and use the same verified final IR for both backends.
Do not add compatibility compilers, fixture-specific admission or backend-only
semantic repairs. Identify affected contracts and meaningful negative tests.

## Development setup

Use Python 3.10 or newer (3.11+ recommended), CMake 3.25+, Ninja, a C++20
compiler and exact LLVM/MLIR 22.1.8. RTL checks use Verilator; four-state tests
also use Icarus where supported.

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -e ".[dev,docs]"
bash tools/pyc build
```

Build from this checkout and keep generated files in `.pycircuit_out/`. Do not
copy compilers or libraries from another worktree.

## Validate changes

Start with the smallest test proving the change. PR CI runs bounded Python,
repository, formatting and documentation checks. API and example tests use:

```bash
bash tools/run_api_tests.sh --tier gate
bash tools/run_examples.sh --tier gate
```

Use `--tier nightly` for full registered coverage. Long oracle, mutation,
platform and package matrices run separately; preserve their scenarios and
report unexecuted checks explicitly. Raw logs under `docs/gates/logs/` are
ignored and uploaded as CI artifacts, not committed as source.

## Pull requests

Describe the problem, resulting behavior, affected contracts, commands/results
and remaining risks. Update current docs with behavior changes. Independent
review and exact candidate evidence matter more than migration counters.
Release publication has separate live validation and platform acceptance gates.
Do not add AI co-author trailers.

See [review and merge](docs/development/review-and-merge.md),
[security reporting](SECURITY.md), [conduct](CODE_OF_CONDUCT.md) and [license](LICENSE).
