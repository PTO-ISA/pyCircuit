# Implementing a design with the current source profile

This guide replaces the former CycleAwareSignal and structural-builder recipe.
For active source semantics, use function `@module` definitions, nested
`@rule` functions, and explicit registration. See the
[language reference](../reference/language.md).

## 1. Define the bounded model

Use one portless public module per Python source. The current profile has one
default clock, finite scalar state, and empty static arguments. A rule has no
arguments or data return. It reads current state and proposes updates with
`nonlocal` assignment. Use local candidate values for computations that must be
reused after proposing a next value.

Do not add external ports, full `@system` behavior, queues, memory/CDC,
multi-clock state, four-state source data, dynamic collections, or arbitrary
Python execution. Unsupported constructs must fail closed.

## 2. Compile each source unit

```bash
mkdir -p .pycircuit_out/units
pycircuit compile -c src/child.py --source-root src \
  --package-prefix demo -o .pycircuit_out/units/child
pycircuit compile -c src/top.py --source-root src \
  --package-prefix demo -I .pycircuit_out/units/child \
  -o .pycircuit_out/units/top
```

Parent compilation consumes the child unit's published interface. It must not
read the child Python body or compile the complete design in one invocation.

## 3. Link and emit

```bash
pycircuit link .pycircuit_out/units/child .pycircuit_out/units/top \
  --top demo.top.Top -o .pycircuit_out/design_top.ac
pycircuit emit .pycircuit_out/design_top.ac --target cpp \
  -o .pycircuit_out/cpp
pycircuit emit .pycircuit_out/design_top.ac --target verilog \
  -o .pycircuit_out/verilog
```

The final design is the shared backend input. Generated CMake compiles
source-owned C++ groups separately, then links `pycircuit_system` and
`libpycircuit_dut` against the installed Runtime. The example project at
`examples/pycircuit/counter/` contains a complete source graph and consumer
CMake setup using `CMAKE_PREFIX_PATH`.

Use a finite runner configuration. Event output is silent unless `--events`
selects a new file or stdout with `-`.

## 4. Validate and document the exact profile

Select gates from the changed contract and bind evidence to the exact candidate.
Check both backends against independent expectations, invalid-input/publication
preservation, Runtime-only install, CompilerDev toolchain requirements, and
retired-route scans as appropriate. A checked-in source or a command example is
not itself a passing gate. M5 is not complete until the candidate acceptance
requirements are met.

The [testing guide](testing-and-gates.md) describes evidence expectations; the
[M5 migration guide](m5-migration.md) lists current exclusions and caller work.
