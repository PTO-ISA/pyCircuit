# pyCircuit 6.1

Write hardware in Python. Compile it to C++ simulation models and Verilog
through one verified MLIR pipeline.

pyCircuit has one Python frontend and four public commands: `pycircuit compile`,
`pycircuit link`, `pycircuit emit` and `pycircuit run`. Typed modules, persistent
state and stateless rules describe hardware. Closed `@system` compositions
generate their own C++ and Verilator simulation harnesses. Python capture parses
the design without executing it; MLIR owns hardware analysis and lowering.

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
