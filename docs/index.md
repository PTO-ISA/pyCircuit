# pyCircuit 6.1

Write hardware in Python. Compile it to C++ simulation models and Verilog
through one verified MLIR pipeline.

pyCircuit has one Python frontend, `pycircuit`, and one public command,
`pycircuit compile`, `link` and `emit`. Ordinary typed modules, state variables
and rules describe hardware; the compiler owns storage, dependencies and atomic
state updates. Python capture does not execute the design.

## Get started

- [Install the toolchain](getting-started/installation.md)
- [Write a hello counter](getting-started/quickstart.md)
- [Build and run it](getting-started/tutorial.md)
- [Browse examples](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/README.md)

## Reference

- [Python source language](reference/language.md)
- [Language and execution semantics](reference/language-specification.md)
- [Compiler pipeline](architecture/compiler-pipeline.md)
- [Known limitations and follow-up work](development/known-limitations.md)

Compiler development uses LLVM/MLIR 22.1.8. Generated C++ models link the
Runtime component independently of LLVM. Both backends consume the same saved
final IR. See [installation](getting-started/installation.md) for build profiles
and [testing](development/testing-and-gates.md) for gate and nightly coverage.
