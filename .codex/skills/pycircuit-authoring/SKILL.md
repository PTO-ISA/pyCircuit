---
name: pycircuit-authoring
description: Write and migrate pyCircuit designs as readable sequential Python functions, module-owned variables and cohesive Rules. Use for examples, benchmark ports and consumer authoring; compiler and backend conformance tests have separate responsibilities.
---

# Pythonic pyCircuit authoring

Read the actual checkout's `AGENTS.md`, `docs/development/agent-frontend-guide.md`
and `docs/reference/language.md` before choosing syntax. Use verified examples
in `examples/GUIDE.md`. A skill summarizes authoring choices; the current
language reference and execution evidence determine supported features.

## Describe the program, not register pins

- Express calculation with ordinary typed `def`, local assignments, `if` and
  supported bounded `for`. Use `@module` calls for hierarchy and distinct state
  ownership. Do not execute design functions in Python.
- Declare persistent variables in their owning Module. Inside Rules, write the
  algorithm in statement order using meaningful names such as `total`, `saved`,
  `incoming` and `previous`. Rule locals are temporary computed values.
- Do not expose `clk`, reset, register `q`/`d`, DFF/DFFE binding, proposal wires or
  commit controls in ordinary author-facing sources, including benchmark ports.
  ACIR analysis/lowering chooses storage and generates its control connections.
- Rule definitions may be source-global like ordinary functions or nested to
  capture Module locals. Calling/registering a Rule establishes its owner and
  actual state bindings; its definition location does not. External Rules use
  explicit parameters rather than capturing a caller's locals.
- Use one cohesive Rule for operations that must commit together. Separate
  independent functions into separate Rules. Nested/cross-Module calls retain
  the outer transaction; they do not create another commit or cycle.
- Statements inside a Rule see earlier assignments. A saved local is a snapshot.
  Independent autonomous Rules observe committed values; explicit local `after`
  supplies ordered visibility. Source definition order alone is not scheduling.

## Keep interfaces small

Return one scalar directly: `-> ac.u8`, `-> bool` or an admitted Bits/Enum type.
Do not introduce a one-field Struct or a singleton tuple to wrap that value.
Use a Struct for multiple related fields passed together, and a typed tuple with
ordinary unpacking for independent outputs. Module and Rule calls use the same
principle. Tests specifically about nominal identity may need one-field Structs;
those are compiler conformance fixtures, not authoring examples to copy.

Use named Queues and `pop`/compute/`push` in one Rule. Resource readiness,
availability, atomic proposals and conflict checks belong to the compiler.
Retain actual algorithmic predicates and real external protocol fields.
Do not add full/empty/ready plumbing to ordinary internal connections or restore
Signal declaration/set/get APIs.

## Memory and sample timing

Use named Memory allocations and Rule operations, following the
[Memory contract](../../../docs/reference/language.md#memory-operations).
Synchronous `request` proposes a read; `result` is a nonconsuming committed
snapshot with the existing expiry, not an implicit wait. Separate producer and
consumer phases; save results in Module state when longer retention is needed.
For byte memory, read once into a local, compute, then write. Compiler-owned
availability and port checks must not become frontend ready/valid plumbing.

When authoring System scenarios, derive state changes from successful rising
edges and observations from Work samples. Use fixed `ac.uN` for wrapping
counters; checked bounded Integers require a proven in-range assignment. Keep
fixed-width comparison literals representable. Do not change stimuli or
independent expected values to hide a compiler or timing failure.

## External Verilog is an export boundary

Translate behavior into pyCircuit; do not transliterate `always` blocks or
primitive wiring into Python. A benchmark's original module/port names and
physical clock/reset belong to the generated RTL interface and host adapter.
Check the existing compiler identifier/export mapping; repair generic naming
there if necessary rather than polluting the source or inventing case-specific
wiring. Preserve exact widths, signedness, timing and reset semantics.

Do not tie off managed admission/check signals, add cycles, inject initialization
or change the oracle to make an unsupported interface pass. Record an unsupported
clock/reset/ABI requirement when the current compiler cannot express it.
Direct primitive pins are reserved for explicitly scoped compiler/backend
conformance fixtures. Their existence does not make them a fallback for designs.

## Four-state verification

Use `ac.four_state("10xz")` for exact-width simulation data. Use `ac.case_eq`
and `ac.is_known` only for diagnostic assertions/logs/Boolean reports, not
functional outputs, state, indexing or Rule guards. Ordinary equality still
propagates unknowns. Available X data and unavailable Rule results are different.

A System may declare finite `(clock, reset)` rows with `@ac.system(samples=...)`.
Each row is one sampling epoch; omit cycles options for this form. The compiler
runs the schedule and publishes only successful epochs. Select reset behavior
at link with `--reset-semantics strict|if_one`; default strict faults on rising
reset X/Z, while if_one treats only known 1 as reset. This never relaxes unknown
clock, enable or reached guard checks. Keep these controls out of DUT signatures.

## Verify and report

Use the existing capture → compile/link → verified ACIR → C++/Verilog route.
Compile from the same checkout. Preserve independent stimuli and oracles.
After porting, record real compile, simulation and failure results separately.
When an ordinary Python expression needs manual expansion, record the original
form, first failing stage and exact diagnostic; keep source-import, MLIR,
codegen, adapter and dataset defects distinct. Do not infer a root cause from
an error string or add compiler special cases for a benchmark name.
