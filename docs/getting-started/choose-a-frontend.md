# Choose the active frontend

There is one active pyCircuit frontend and one compile route. Use Python
function modules and nested rules, compile each source independently, link the
complete source-unit closure, and emit C++ or Verilog from the same verified
final design.

## Current profile

| Surface | Active contract |
| --- | --- |
| Python | Portless function `@module`, nested `@rule`, explicit registration |
| Clock/state | One default clock; finite scalar state; current reads and proposed next values |
| Static arguments | Empty in the current profile |
| Build | `pycircuit compile` → `pycircuit link` → `pycircuit emit` |
| Backends | C++ and Verilog from the same saved final artifact |
| Runtime | One `pycircuit::pyc6_runtime`; no LLVM dependency for Runtime consumers |

The source contract does not include external ports, a complete `@system`
runner interface, queues, memory/CDC, multiple clocks, four-state source
values, or dynamic collection behavior. Unsupported uses produce errors.

## Build the example

Install Runtime and CompilerDev from this checkout as described in
[installation](installation.md), then configure the source-owned counter with
the installed prefix:

```bash
cmake -S examples/pycircuit/counter \
  -B .pycircuit_out/counter -G Ninja \
  -DCMAKE_PREFIX_PATH="$PWD/.pycircuit_out/install"
cmake --build .pycircuit_out/counter
cmake -S .pycircuit_out/counter/cpp \
  -B .pycircuit_out/counter/run -G Ninja \
  -DCMAKE_PREFIX_PATH="$PWD/.pycircuit_out/install"
cmake --build .pycircuit_out/counter/run
.pycircuit_out/counter/run/pycircuit_system \
  --config examples/pycircuit/counter/config.json --events -
```

The example's `design_top.py` is portless and has no static arguments. Child
module connections are register references, not external DUT ports. The runner
configuration limits execution to three cycles; event output is selected
explicitly.

## Historical APIs

CycleAwareSignal and CycleAwareDomain, structural builders, Agentic Circuit and
QueueGraph source semantics, `acc.py`, `acc`, and `pycc` are retired. They are
not alternative installation profiles or fallback routes. Existing callers
are migration inputs, not evidence of current support.

Continue with the [quickstart](quickstart.md), [language reference](../reference/language.md),
and [M5 migration guide](../development/m5-migration.md).
