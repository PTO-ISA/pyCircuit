# Quickstart

This quickstart uses the bounded M5 source profile: portless `@module`
functions, nested `@rule`s, default clock, and empty static arguments. The
example produces a closed model without external ports.

## Prepare a compiler installation

Use Python 3.11+, CMake, Ninja, a C++20 compiler, and LLVM/MLIR 22.1.8. Build
and install CompilerDev plus Runtime as described in
[installation](installation.md), then make the installed `pycircuit` visible on
`PATH`.

## Write a source

Create `src/counter.py`:

```python
from typing import Annotated
from pycircuit import module, rule

Word = Annotated[int, range(256)]

@module
def Counter():
    count: Word = 0

    @rule
    def tick():
        nonlocal count
        count = (count + 1) & 255

    tick()
```

The annotation is a supported scalar state declaration in this profile. The
rule definition does not register itself; `tick()` in the module body is the
explicit registration. A rule reads current state and assigns its next value
through `nonlocal`.

## Compile, link, and emit

```bash
mkdir -p .pycircuit_out/units
pycircuit compile -c src/counter.py --source-root src \
  --package-prefix demo -o .pycircuit_out/units/counter
pycircuit link .pycircuit_out/units/counter --top demo.counter.Counter \
  -o .pycircuit_out/design_top.ac
pycircuit emit .pycircuit_out/design_top.ac --target cpp \
  -o .pycircuit_out/cpp
pycircuit emit .pycircuit_out/design_top.ac --target verilog \
  -o .pycircuit_out/verilog
```

The final artifact is shared input to both emitters. Compile imported source
files separately, publish their unit directories, pass those directories as
`-I` inputs when compiling parents, and provide the complete unit closure to
`link`.

Generated CMake provides the `pycircuit_system` executable and
`libpycircuit_dut`. Set a finite limit, for example `{"deadlock_window":null,"max_domain_cycles":{},"max_ticks":100,"schema":"agentic-model-config","version":"1"}`, in
the runner configuration and invoke `pycircuit_system --config config.json`.
Without an explicit event sink it runs silently. Add `--events <new-file>` or
`--events -` to capture events to a file or standard output.

## Continue

- [Language reference](../reference/language.md)
- [Agent frontend guide](../development/agent-frontend-guide.md)
- [Testing and gates](../development/testing-and-gates.md)
- [M5 migration guide](../development/m5-migration.md)
