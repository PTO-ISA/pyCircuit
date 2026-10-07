# Repository layout

| Directory | Responsibility |
| --- | --- |
| `python/pycircuit/` | Single Python source capture, public CLI and publication |
| `compiler/` | MLIR analysis, verified hardware IR and both code generators |
| `simulator/gfsim/` | Shared generated-model Runtime |
| `include/` | Runtime interfaces and standard C++/Verilog hardware leaves |
| `examples/hello_counter/` | Small introductory stateful design |
| `examples/counter/` | Enabled counter regression with wrap, hold and reset |
| `examples/` | Independent designs and the current catalog |
| `tests/` | Framework/API, backend, runtime and independent oracle tests |
| `flows/` | Build, validation and release entrypoints |
| `cmake/` | Installed build helpers for generated modules and examples |
| `packaging/` | SDK and wheel build/verification |
| `docs/` | Current language, architecture and contributor documentation |

Generated artifacts go to `.pycircuit_out/` or a chosen out-of-source build
directory. Raw logs and local work/review notes are ignored by Git. Historical
source revisions remain in Git history; retired frontends are not fallback
routes in the current tree.
