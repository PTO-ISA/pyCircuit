# M5 migration guide

M5 replaces the prior multiple-frontend product route with one hard-break
workflow: compile each Python source independently, link the explicit unit
closure into a verified final design, then emit C++ or Verilog from that same
artifact. There is no fallback compiler or compatibility mode.

M5 is accepted for this bounded profile on 2026-10-01. The [final review](../reviews/20261001-m5-cutover-review.md) binds independent checks and PM acceptance to the product bytes.

## Supported profile

The current declared slice supports:

- Portless function `@module` definitions, with one public module per
  implementation source.
- Nested function `@rule` definitions with explicit registration calls in
  module scope.
- Finite scalar state with current/next behavior under one default clock.
- Nested modules using explicitly compiled and linked source units.
- Empty static arguments and no external input/output ports.

A rule definition is inert until registered. Rules have no arguments or data
return. State reads observe current values for the whole epoch; a `nonlocal`
assignment proposes a next value. Explicit candidate locals preserve computed
values for reuse.

## Compile, link, and emit

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
  -o .pycircuit_out/generated-cpp
pycircuit emit .pycircuit_out/design_top.ac --target verilog \
  -o .pycircuit_out/generated-rtl
```

This illustrates the phases; a build graph should compile each source once and
link the resulting exact unit closure. Pass parent imports through published
interface directories using `-I`. The compiler must not read child source
bodies as a fallback. `link` publishes a final artifact only after closure,
interface, specialization, and verifier checks pass. Each emitter revalidates
that final input.

The approved `emit` targets are `cpp` and `verilog`. Generated CMake preserves
source-owned implementation groups. The C++ target builds `pycircuit_system`
and `libpycircuit_dut`; the Verilog target builds the Verilator-backed
`pycircuit_system`. The runner configuration must set a
finite limit, for example `{"deadlock_window":null,"max_domain_cycles":{},"max_ticks":100,"schema":"agentic-model-config","version":"1"}`. Run it with
`pycircuit_system --config config.json`. Pass explicit `--events <path>` or
`--events -` to capture output. Omitted `--events` is silent.

## Installation profiles

Compiler development uses exact LLVM/MLIR 22.1.8. Runtime-only CMake users
configure with `PYC_BUILD_COMPILER_DEV=OFF`,
`PYC_BUILD_RUNTIME_LIB=ON`, and `PYC_BUILD_TESTING=OFF`. They consume:

```cmake
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
target_link_libraries(model PRIVATE pycircuit::pyc6_runtime)
```

Runtime exports `libpyc6_runtime` and does not discover LLVM. CompilerDev is
selected with `find_package(pycircuit CONFIG REQUIRED COMPONENTS CompilerDev)`
and intentionally carries compiler development dependencies.

## Retired routes

The hard break removes the old CycleAwareSignal and structural builder APIs,
the Agentic Circuit authoring namespace and QueueGraph semantic route, and the
PYC `pycc` route. Their old commands (`acc.py`, `acc`, `pycc`) are not aliases
for the new driver. Do not migrate code by changing only a command name: source
semantics changed to function modules, lexical nested rules, explicit
registration, and state proposals.

## Not in this profile

The following remain later capability work and must fail closed when used:

- complete `@system` plus EXPECT/driver-observation behavior;
- queues, FIFO/producer backpressure, and transactional resource protocols;
- memory, CDC, multiple clocks, and four-state source values;
- external typed input/output ports and external DUT ABI;
- nonempty static arguments and parameter-dependent interface shapes;
- dynamic collections or unbounded Python execution behavior.

Do not recover these through a retired implementation, and do not remove their
historical semantic oracles to make the new slice appear complete. Track each
needed capability in its owning backlog and obtain the required contract before
expanding this public profile.

## Caller migration inventory

The current example is `examples/pycircuit/counter/`, built through explicit
per-source producers. Legacy public examples, source namespaces, native
QueueGraph/PYC engines, old runtime, aliases, and their install registrations
are retired in the candidate. Current docs and gate callers use the single
route. Historical decisions, approval proposals and evidence remain available;
they do not activate old product paths.

The [retirement ledger](../work-items/m5-retirement-ledger.md) maps retired
oracles to current tests or explicit M3/M6 capability work. Candidate acceptance
still depends on the final gate results and independent review recorded in the
[M5 work item](../work-items/m5-cutover.md).
