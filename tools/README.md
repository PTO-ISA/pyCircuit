# Repository maintenance tools

[Compiler maintenance tools](pycircuit/README.md) regenerate source-owned data
and example navigation. These tools support development; the public commands
remain `pycircuit compile`, `link`, `emit` and `run`.

Build and verification entrypoints live in [`flows/`](../flows/), installed
CMake helpers in [`cmake/`](../cmake/), and SDK/wheel assembly in
[`packaging/`](../packaging/). See the [repository map](../docs/development/repository-layout.md)
for the ownership boundaries. Generated outputs belong under `.pycircuit_out/`.
