# Source-unit workflow

pyCircuit captures ordinary Python hardware descriptions, resolves and lowers
hardware semantics in MLIR, and emits C++ and Verilog from the same verified
final artifact. There is one public compile/link/emit route. Capture never
executes design functions.

## Compile and link

Compile each source independently. Consumers use published interfaces through
`-I`; link receives the complete explicit unit closure. For a two-source design:

```bash
mkdir -p .pycircuit_out/units
pycircuit compile -c src/child.py --source-root src \
  --package-prefix demo -o .pycircuit_out/units/child
pycircuit compile -c src/top.py --source-root src \
  --package-prefix demo -I .pycircuit_out/units/child \
  -o .pycircuit_out/units/top
pycircuit link .pycircuit_out/units/child .pycircuit_out/units/top \
  --top demo.top.Top -o .pycircuit_out/design_top.ac
pycircuit emit .pycircuit_out/design_top.ac --target cpp \
  -o .pycircuit_out/cpp
pycircuit emit .pycircuit_out/design_top.ac --target verilog \
  -o .pycircuit_out/verilog
```

Generated CMake builds source-owned `pycircuit_modules` against the Runtime
component. Host drivers use the typed `pyc_dut` and shared SystemRunner, supply
inputs and explicit clock/reset levels, and set a finite run limit. Work reads
old state; successful whole-system checking precedes Xfer commit.

## Scope and verification

The [language reference](../reference/language.md) defines current admission and
unsupported constructs. The [frontend guide](agent-frontend-guide.md) describes
source authoring. CompilerDev requires exact LLVM/MLIR 22.1.8; Runtime-only
consumers use `find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)` and
`pycircuit::pyc6_runtime` without LLVM discovery.

The current example catalog includes retained supported designs. Incomplete
historical examples and API drafts were removed at the user's delivery cutoff
on 2026-10-07; removal does not establish their implementation or verification.
Historical planning and logs remain local ignored records and in Git history.
They are not dependencies of examples, API tests or documentation builds.

Use [testing and gates](testing-and-gates.md) for short validation and nightly
coverage. Preserve source-import IR, transformed IR and both backend outputs
through existing manifests when running compiler validation. Long oracle,
coverage and platform matrices are scheduled separately; omitted runs must be
reported as not run.

## Retired routes

CycleAwareSignal/JIT, structural builders, Agentic Circuit/QueueGraph, `acc.py`,
`acc`, and `pycc` are retired. They are not aliases or fallback implementations.
