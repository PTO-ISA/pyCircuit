# Agent frontend guide

Use the single active pyCircuit source route documented here. A source file is
captured by `pycircuit compile`, linked with an explicit source-unit closure,
and emitted as C++ or Verilog from one verified final design. The Python layer
captures declarations; MLIR owns name resolution, type and effect analysis,
checks, and hardware lowering. Do not execute design functions as a simulator
or add a second semantic compiler in Python.

## Current authoring profile

The supported M5 slice consists of:

- one portless `@module` function per implementation source;
- nested `@rule` functions, explicitly registered by calls in module scope;
- finite scalar state with current/next cycle semantics;
- one default clock;
- nested source units compiled and linked explicitly; and
- an empty static argument list.

The profile has no external ports. Complete `@system`, queues, memory, CDC,
multi-clock, four-state, dynamic collection, and external typed DUT contracts
are outside this slice. Unsupported constructs must fail closed with a useful
diagnostic. Do not compensate by routing through retired APIs.

## Source form

```python
from typing import Annotated
from pycircuit import module, rule

Byte = Annotated[int, range(256)]

@module
def Counter():
    count: Byte = 0

    @rule
    def tick():
        nonlocal count
        candidate = (count + 1) & 255
        count = candidate

    tick()
```

Module and rule definitions are captured, not invoked as Python model code. A
rule definition is inert until its explicit module-scope registration call.
Rules have no arguments or data return. Reads observe current state throughout
the cycle; `nonlocal` assignment proposes next state. Pure helpers can return
ordinary values. State, instances, and side effects do not belong inside rule
bodies.

Keep source-level hardware intent in ordinary Python values and annotations
that the active profile supports. Do not introduce Queue/FIFO objects, manual
interface or direction declarations, `Reg`/`Signal` objects, user scheduling
controls, or effect annotations. Those are not public source contracts here.

## Source ownership and imports

Each implementation source owns its public definition and produces one source
unit. Compile a child first and pass its published interface directory with
`-I` when compiling the parent. Parent compilation consumes compiler-published
interface declarations, not child bodies. Link explicitly lists the complete
unit closure and qualified top. Do not compile an entire system and split it
into source outputs later.

C++ and Verilog consume the same final artifact. Preserve one source-owned
C++ `.hpp`/`.cpp` group per implementation source, plus the generated core and
runtime glue. Generated CMake compiles the source groups independently and
links the model executable and DUT library.

## Build and run flow

```bash
mkdir -p .pycircuit_out/units
pycircuit compile -c src/top.py --source-root src \
  --package-prefix demo -o .pycircuit_out/units/top
pycircuit link .pycircuit_out/units/top --top demo.top.Top \
  -o .pycircuit_out/design_top.ac
pycircuit emit .pycircuit_out/design_top.ac --target cpp \
  -o .pycircuit_out/cpp
cmake -S .pycircuit_out/cpp -B .pycircuit_out/cpp-build -G Ninja
cmake --build .pycircuit_out/cpp-build
```

Use `--target verilog` for the RTL bundle. The generated runner is
`pycircuit_system`; use a finite configuration. It emits no event stream unless
`--events <path>` or `--events -` is explicitly supplied.

## Do not copy old examples

Current repository examples include legacy CycleAwareSignal, builder,
Agentic Circuit, QueueGraph and PYC compiler callers awaiting migration. Do not
use their code or instructions as support evidence for the new contract. The
active language guide and the [M5 migration guide](m5-migration.md) define the
current source subset and the remaining caller inventory.
