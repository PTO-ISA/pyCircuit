# Get started

pyCircuit 6.1 uses ordinary Python functions decorated with `@module`, `@rule`
and `@system`. Typed variables describe state, rules compute updates, and MLIR
lowers the design to one verified hardware representation for C++ and Verilog.
A closed system generates its own native and Verilator simulation harnesses.

1. [Install](installation.md) the Python package, compiler and Runtime.
2. [Write a hello counter](quickstart.md) in a few lines.
3. [Build and run](tutorial.md) the checked-in example.

Use `import pycircuit as ac`, as the current examples do. The alias is ordinary
Python syntax; there is one frontend and no separate AC/PYC authoring mode or
versioned import.

The [language reference](../reference/language.md) documents supported types,
rules and operations. [Known limitations](../development/known-limitations.md)
records work reserved for later PRs.
