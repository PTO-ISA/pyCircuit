# Python source language reference

This page documents the active, bounded pyCircuit source contract. Python files
are captured as source; pyCircuit does not execute a design function to
simulate the model. The compiler resolves declarations, derives dependencies
and effects, verifies the common hardware representation, and feeds the same
final design to the C++ and Verilog emitters.

## Supported M5 profile

The currently documented profile accepts:

- One portless `@module` function per implementation source.
- Nested `@rule` functions and explicit calls in the module body to register
  those rules.
- Ordinary Python values and statically understood expressions within the
  approved scalar subset.
- A single default clock, with finite scalar state and current/next semantics.
- Nested modules and source imports when every source unit is compiled and
  linked explicitly.
- Empty static argument lists. The source has no external input/output port
  interface in this profile.

This profile does not provide a complete `@system` execution contract, queues,
producer backpressure, memory, CDC, multiple clocks, four-state source values,
external typed DUT ports, or dynamic collections. Such uses are rejected with
an error; they do not select an older frontend. These are later capability
contracts, not implied features of this page.

## Module and rule declarations

A module is an ordinary function decorated with `@module`. A rule is an
ordinary nested function decorated with `@rule`; its body runs once per
scheduled cycle in the generated model. Defining a rule does not register it.
Call it in the module's structural body to register that occurrence.

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

A public module source has no runtime parameters in the current M5 profile.
Ordinary scalar types and value expressions must have a statically provable
finite hardware representation. An annotation's declared range is enforced;
implicit width conversion and runtime-computed widths are not inferred.

Rules have no source parameters and no data return. Use ordinary pure helpers
for value computation. To propose state, bind the target with `nonlocal` and
assign a candidate. A read of the state name observes current state throughout
the cycle; assigning the name proposes next state. Reading it again still
observes current state, so use the candidate local if the computed value is
needed again.

```python
@rule
def advance():
    nonlocal count
    candidate = count + 1
    count = candidate
    observed_current = count
```

Here `observed_current` is the old value of `count`. A local assignment without
`nonlocal` is an SSA local, not persistent state. `nonlocal` may name only an
enclosing state binding admitted by the compiler; it cannot rebind a module
name, child handle, or host object.

A rule's registration call is structural syntax, not a call during ordinary
Python execution. Host I/O, arbitrary side effects, dynamic module graphs,
inheritance-based hardware construction, dynamic callables, state or instance
declarations inside a rule, and unsupported Python constructs are rejected.

## State and cycle behavior

Each owned state declaration has one current value and one proposed next value.
All reads during an epoch observe current state. Registered rules evaluate
against that current state; successful proposals are checked and then
committed at the cycle boundary. A child connected to parent-owned state reads
the same current value and proposes updates to the same state identity; a
connection does not create extra storage.

The current public profile has one default clock. It does not accept manually
created clocks/domains, cycle annotations, explicit signal delays, or user
selected pipeline balancing. Multi-clock and CDC behavior requires a later
contract.

Rules can use supported conditionals and expressions to choose whether to
propose a next value. A cycle with no state change is still a clock cycle. The
runner's finite cycle limit controls execution duration; an absent explicit
limit is not an unbounded-run promise.

## Source units and hierarchy

Compile every Python implementation or declaration source independently. A
source unit publishes its `.ac` body, `.interface.ac` declarations, dependency
file, and unit receipt. When compiling a parent, `-I` names published interface
unit directories. The compiler resolves child interfaces from those units and
does not read a child Python body as a fallback. Link receives the explicit
complete unit closure and the qualified top module.

One source owns one public module or declaration set. A module can instantiate
child modules within the bounded profile. Source identity and specialization
are established by compile and link metadata; filenames alone do not create
additional module definitions.

## Driver commands

```text
pycircuit compile -c <one-source.py> --source-root <root>
  [--package-prefix <dotted-prefix>] [-I <interface-unit-dir>]...
  -o <unit-dir> [--replace]

pycircuit link <unit-dir>... --top <qualified-module>
  [--parameters <bindings.json>] -o <design_top.ac> [--replace]

pycircuit emit <design_top.ac> --target cpp|verilog
  -o <generated-dir> [--replace]
```

`compile` accepts one source file. `link` explicitly receives all required
units and creates the verified final artifact. `emit` revalidates that artifact
and selects `cpp` or `verilog`; neither backend reparses Python. Publication
uses managed output directories and preserves prior outputs when a new
publication is rejected.

The current M5 profile has no static arguments, so omit `--parameters`. The
flag is reserved for approved static parameter bindings in profiles that expose
them; no source API or nonempty binding capability should be inferred here.

## Runner observations

Generated model CMake provides `pycircuit_system` and `libpycircuit_dut`. The
runner accepts a canonical finite configuration and optional `--events` sink.
Without `--events`, execution is silent and success or failure is conveyed by
the process status. `--events <path>` selects a new output file and `--events -`
selects standard output. Events are tooling output, not a model or runtime ABI.

## Explicitly unsupported

The hard break removed older public construction and compilation routes. The
following names and ideas are not active fallbacks: CycleAwareSignal and
CycleAwareDomain authoring, JIT `compile`, structural `Circuit` builders,
`acc.py`/`acc`, QueueGraph source semantics, and PYC `pycc` compilation.
Existing examples that still use those APIs are historical migration callers
and are not examples of the active language.

The current subset also does not claim: complete `@system` behavior, external
ports, queue/FIFO protocols, stateful memory primitives, CDC, multiple clock
domains, four-state values, arbitrary Python, dynamic loop bounds, runtime
module creation, or generated C++ class ABI stability. Rejected capability is
not silently lowered through a retired compiler.
