# Repository Layout

pyCircuit is organized by responsibility. A file belongs where its product
role is owned, not where a single workflow happens to consume it. This keeps
the two Python frontends, compiler layers, runtimes, examples, tests, and
release tooling independently understandable.

## Source and runtime roots

| Root | Ownership |
| --- | --- |
| `python/` | The `pycircuit`, `agentic_circuit`, and shared semantic-core Python distributions |
| `compiler/` | PYC and ACIR/ACSim dialects, passes, native tools, and code generators |
| `library/` | Stable pyCircuit C++ runtime headers and qualified Verilog implementations |
| `simulator/` | The gfsim architecture-modeling runtime |

These roots define product behavior. Semantic changes start in the appropriate
frontend, dialect, pass, or verifier and then propagate to every applicable
backend.

## User and contributor roots

| Root | Ownership |
| --- | --- |
| `docs/` | User guides, reference material, architecture, decisions, and contributor documentation |
| `examples/` | Small public demonstrations of supported framework behavior |
| `benchmarks/` | Performance workloads and measurement harnesses; never correctness authority |
| `tests/` | Unit, system, integration, MLIR, C++, Verilog, and golden regression evidence |
| `tools/` | User-facing and product-maintenance utilities, separated by frontend |
| `flows/` | Build, CI, gate, and release orchestration plus private flow helpers |

Public examples are documentation-quality product surfaces. Larger reusable
correctness cases live under `tests/integration`; performance-only drivers live
under `benchmarks`. Complete CPU, NPU, accelerator, SoC, board, and
product-specific testbench sources belong in consumer repositories.

## Contract and delivery roots

| Root | Ownership |
| --- | --- |
| `schemas/` | Versioned machine-readable contracts, inventories, and registries |
| `toolchains/` | Pinned external toolchain identities and lock metadata |
| `packaging/` | SDK, archive, and wheel assembly and verification |
| `cmake/` | Shared CMake package and installation templates |
| `.github/` | Required CI, nightly diagnostics, release workflows, and repository templates |
| `third_party/` | Pinned external reference material with provenance and license records |

Generated binaries, profiles, traces, manifests, and build trees go under
`.pycircuit_out/` or another ignored temporary directory. They are not source
inputs and must not be committed.

## Tool ownership

`tools/pycircuit/` contains utilities that users and maintainers run directly,
such as trace inspection, IR inventory checks, module graphs, and schematic
views. `tools/agentic-circuit/` owns the corresponding Agentic contract,
catalog, release-layout, and SDK checks.

`flows/tools/` is intentionally narrower. Its scripts implement repository
gates and build orchestration and are normally called by `flows/scripts/` or
CI. A utility does not belong there merely because a gate also uses it.

## Placement checklist

Before adding a file, answer these questions:

- Does it define language or runtime behavior? Put it in the owning product
  source root.
- Does it teach supported public usage? Put it in `examples/` and add it to the
  example discovery gate.
- Does it prove correctness? Put it in the matching `tests/` layer.
- Does it measure performance? Put it in `benchmarks/`.
- Does it orchestrate builds or gates? Put it in `flows/`.
- Is it a direct user or maintainer utility? Put it in the matching `tools/`
  subtree.
- Is it a complete consumer design or product adapter? Keep it in the consumer
  repository.

The automated layout contracts live in
`tests/unit/test_repository_layout.py`, `tests/unit/test_example_layout.py`,
and `tools/agentic-circuit/check-release-layout.py`.
