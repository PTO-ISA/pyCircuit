# Get started

pyCircuit 6.1 uses ordinary Python functions decorated with `@module` and
`@rule`. Typed variables describe state, rules compute updates, and MLIR lowers
the design to one verified hardware representation for C++ and Verilog.

1. [Install](installation.md) the Python package, compiler and Runtime.
2. [Write a hello counter](quickstart.md) in a few lines.
3. [Build and run](tutorial.md) the checked-in example.

Use `import pycircuit as pyc`; `ac` in existing examples is simply another
Python import alias for the same frontend. There is no separate AC or PYC
authoring mode and no versioned frontend import.

The [language reference](../reference/language.md) documents supported types,
rules and operations. [Known limitations](../development/known-limitations.md)
records work reserved for later PRs.
