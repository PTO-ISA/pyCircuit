# Agent frontend guide

Use one active route: Python capture → independently compiled source units →
explicit link closure → verified common hardware IR → C++ or Verilog.
Python design functions are never executed by capture or used as a simulator.
MLIR owns hardware types, instances, connections and dependency verification.

Enum declarations and nominal type transport use this same route, including
independently compiled type-only and reexport providers. See the
[Enum reference](../reference/spec-enums.md) for members, equality/selection,
defaults and exact-width bit conversions. `decoded, valid =
ac.enum_from_bits[State](raw)` creates a nominal carrier and separate Boolean
membership. Decode into ordinary locals before assigning a persistent owner;
source variables still use the existing proposal/commit flow.

## Behavioral source profile

The [ROB example](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/rob/rob.py) uses ordinary annotated
variables, nominal `@ac.struct` values, `ac.table[4, Entry](init=0)` allocation and a
same-source top-level `@ac.rule`. It returns a typed `RobResult`, using default
construction and field assignments, through the direct `Rob` → `RobStorage`
module call. Source capture remains syntax-only. The owning
MLIR source pass validates static defaults, infers storage, sequential rule SSA,
guarded writes and existing instance/collection connections before interface
extraction. Static module-call analysis propagates hidden domain requirements
through stateless parents. It adds no IR
schema or backend execution path. Source authors do not supply DFFE or clock
and reset pins; generated physical `pyc_clk`/`pyc_rst` inputs use the existing
typed DUT binding.

The slice uses fixed-width unsigned bit payloads, exact local Integer values,
statically proven indices and multiple state-writing rules with disjoint owner
or nested Struct-field write sets, and one-dimensional Table writers on distinct
direct-literal elements or disjoint element fields. Table element payloads may be
Bits, Enum or recursively nested Struct. Struct fields may contain positive,
closed one-dimensional Tables. Contextual tuple/list literals construct their
elements under the declared type; static defaults, zero initialization and
whole-field replacement use the existing Struct/Table flow. Indexed partial
writes within these fields and a second source index level remain unsupported.
Dynamic same-field indices require
explicit address-separating grants proved against assignment-time SSA, or proven
mutually exclusive whole-owner grants. Range facts alone do not separate addresses.
Rules may register through ordinary statement
calls with no value return. The source write-analysis pass records potentially overlapping registrations;
lowering may discharge them using whole-owner grants or exact index separation,
and rejects unproved pairs before publishing the source unit; all rules retain old-state reads and existing
within-rule sequential semantics. Index safety can use a bounded nonnegative
interval (including static-divisor remainder) or full physical width; this does
not turn an interval into constant or known-bit authority.
Struct defaults require same-source declarations. Ordinary typed-result module
calls also consume independently published interfaces, including aliased
from-imports and facade reexports. Source argument kinds and hidden domains are
explicit interface facts checked against the provider body at link. Imported
Struct return annotations do not enable imported constructors or defaults.
See the [language reference](../reference/language.md)
for exact boundaries and the remaining dynamic-check limitations. Existing
structural sources below continue through the same importer and public flow.

Use `index, valid = entries.first(where=lambda entry: ...)` for first-match
selection, or `entries.argmin(where=lambda entry: ..., key=lambda entry: ...)`
for the minimum unsigned fixed-bit key. Both bind exactly two ordinary names;
valid is fixed one-bit, and a miss returns zero/zero. Pure one-argument lambdas
can capture immutable values and read captured Tables under the existing index
proof. Queries preserve call-time snapshots and do not reserve or consume entries.
See [Table queries](../reference/language.md#table-queries) for exact four-state
ordering, unknown-index caveats and unsupported callback forms. The compiler
stages existing scalar SSA into compact Table operations; cross-Table reads may
still cost N×M logical work/storage and are charged to source-wide budgets.

Typed Tables also support same-extent `values.map(lambda ..., *other_tables)`,
the seven closed unsigned `fold(kind=...)` reductions, and one-bit
`all()`/`any()`/`count()`. Map types one pure expression body and lowers it through
the existing Table operations; it does not execute Python per element. Count
widens before summing and retains its `[0, N+1)` range proof. Captures preserve
call-time values, including nominal Structs containing Tables. See
[Table transformations](../reference/language.md#table-transformations-and-reductions)
for exact result, callback, four-state and resource boundaries. Map also accepts
same-source named `@rule` callbacks from the fixed scalar expression subset.
Generic row types must exactly match their formal annotations; individual lane
values do not supply range or constant authority. Helper calls inside expression
lambdas remain unsupported. Ordered prefixes can use the
[bounded literal-range loop profile](../reference/language.md#bounded-source-loops)
in pure behavioral rules; it adds no scan API or clock cycle. Preserve its exact
range grammar, binder behavior, effect exclusions and resource bounds.

Use `ready, valid, data = ac.queue[T](valid, data, take, depth=N)`
for a complete-token FIFO. Allocation is module-scoped, with three fresh immutable
result names; Python does not expose clocks, reset or proposal wires. The default
ready policy uses local occupancy; full replacement requires explicit
`ready_policy="downstream_pop"`. Static `latency=L` delays availability while
keeping all waiting tokens within the declared depth. A birth at E0 becomes
available after E(L-1) commits; EL is the earliest consumption. An unavailable
head reads as packed zero after initialization. Existing Work/Xfer and
whole-system discard apply. See [queues](../reference/language.md#queues) for exact admission.

Behavioral statement `if` and pure conditional expressions share Boolean or
authoritative fixed-one-bit predicate checks. `and/or/not` use those same checks;
all-Boolean operands retain Boolean kind, while mixed Boolean/fixed-one-bit
`and/or` results are fixed one-bit in either operand order. Using a fixed value as
a predicate leaves its numeric meaning unchanged. Integer `0/1` and one-bit
Integer annotations do not become predicates from their physical width.

Behavioral branches inherit the same incoming SSA values and owner proposal
enables. An omitted assignment or `pass` preserves both, including any earlier
proposal. Joins select values and enables with the existing four-state semantics.
New locals must be bound on both paths before a later read; a subsequent
unconditional assignment can bind a partial local. Unknown effective enables
retain standard storage failure and whole-system discard behavior.

Inferred Integer locals can grow their representation while retaining Integer
kind. Explicit annotations and persistent owners preserve their original logical
kind and range/width contracts. An unannotated non-owner Integer rule formal
retains the call's representation and source facts, including negative values;
it does not acquire an unsigned range and cannot widen through reassignment.
An explicitly annotated formal checks its declared range, and owner-bound formals
retain the owner's contract. Reannotation cannot replace an existing declaration.

Statement and expression joins retain exact Integer representations. A fixed
branch peer can convert a fitting closed Integer, or a closed Boolean only for
a one-bit peer, symmetrically in either arm order. Supported closed operations,
aliases and rule captures preserve this proof; runtime inputs/state, singleton
intervals and unbound parameter defaults do not establish it. The converted copy
does not change the original producer's kind. This proof does not expand static
count, divisor, parameter or default admission. See
[branches and binding boundaries](../reference/language.md#branches-and-binding-boundaries)
for declaration joins, definite assignment and X/Z selection.

Behavioral `match` reuses that complete branch environment: the subject is read
once, first-match priority selects both values and owner enables, and omitted
writes retain incoming proposals. Use Integer literals for fixed bits, Boolean
patterns for Boolean selectors and qualified members for Enum selectors. A final
`_` supplies source fallback; logical coverage cannot remove physical X/Z or
invalid-code fallthrough. See [match statements](../reference/language.md#match-statements)
for supported patterns, diagnostics and the remaining selector restrictions.

Direct same-source module calls can connect to later module results and their
fields when those names have one unambiguous, unmutated call assignment. The
compiler creates each instance once and binds its arguments at that call's
lexical position, preserving prior local values. Existing field-sensitive MLIR
analysis validates the completed connections: opposing module connections may
be legal, while observed and unused combinational cycles reject. This source
binding capability does not establish a FIFO or backpressure policy. See
[direct module composition](../reference/language.md#direct-module-composition).

Unsigned bits shifts use static nonnegative Integer counts and retain the input
width, including before a later destination widening. They preserve retained
X/Z positions and fill with known zero; counts at least the width yield zero.
The importer reuses common width analysis and existing shl/lshr operations, with
full-precision count validation. Mathematical Integer right shift remains the
separate exact-value operation described below.

Use `ac.concat(high, low)` for fixed-bit transport, with one or more operands
ordered from most significant to least significant. Its natural width is the
sum of operand widths, before destination conversion. Unlike bitwise OR,
concatenation preserves Z and the complete value/known/Z planes. Static slices
plus concat express field insertion without adding another operation. Integer,
Boolean, raw Enum and aggregate operands do not silently become fixed bits;
use an explicit fixed binding or `enum_to_bits` where appropriate. See the
[language reference](../reference/language.md) for call restrictions.

`ac.popcount(value)` uses the same fixed-bit authority and ordinary expression
flow. Its result has `bit_width(input_width)` bits before any destination
widening. One-bit input is raw identity, while a wider input containing X/Z
produces an all-X count. The importer reuses existing bit operations in a
logarithmic graph, without a new IR operation or Runtime primitive. Logical
Integer/Boolean and raw aggregate/Enum operands require their existing explicit
conversion boundaries; physical width alone does not authorize the call.

Use `ac.count_leading_zeros(value)` or `ac.count_trailing_zeros(value)` for
endpoint zero runs. Both return fixed `bits[bit_width(input_width)]` before
destination conversion, return the input width for zero, and share popcount's
fixed-value admission. One-bit input is logically complemented, including
Z→X. Unknowns after the first known one are ignored; endpoint uncertainty can
retain known-zero high count bits, while other prefix uncertainty produces an
all-X natural count. See the [language reference](../reference/language.md)
for the exact rule. Lowering shares popcount's value algorithm and existing
bit operations, with an O(log W) graph and no new IR or Runtime primitive.

Use `index, valid = ac.priority_encode(value, order="low")` or
`index, valid, conflict = ac.onehot_encode(value, order="high")` for fixed-bit
selection. Index has natural width `max(1, bit_width(W-1))`; flags remain Boolean.
Unpack once into ordinary names, then use existing nominal struct construction
for named fields and transport. Explicit u1 fields convert flags to fresh fixed
wires without changing the original Boolean aliases. Calls do not return a
general tuple or support immediate `.index` projection. Both orders use pure
conditional merging for X/Z, and conflict shares popcount's unknown behavior.
Lowering and binding reuse existing common bits IR and the Enum result binder.
See the [language reference](../reference/language.md) for exact admission,
four-state behavior and existing emission limitations.

Unsigned fixed-bit division and remainder accept positive static Integer
divisors that fit the input width. Results retain that width before destination
conversion. Lowering uses existing widened multiplication, extraction and
subtraction; any numerator X/Z makes the whole result X, even for divisor one
or a power of two. Same-width authoritative fixed-bit runtime divisors use
combinational division with no added state or cycles. A runtime zero divisor
or any operand X/Z produces all X, matching native four-state RTL arithmetic.
Static Integer zero/negative and runtime Boolean/Integer divisors still reject.
A state initializer or ordinary Integer alias is not static proof; explicitly
fixed-bit state may instead use the runtime operation.
See the [language reference](../reference/language.md) for the exact admission
boundary; this does not add runtime mathematical Integer division.

## Structural module source profile

A `@module` definition declares ordinary typed input arguments and a named return
mapping for output types. Its body declares child instances, immutable local
wires and nested stateless
`@rule` functions. Explicit calls register those rules. State belongs to standard
leaf instances, including `dffe`; `nonlocal` persistent-state writes are rejected.

```python
from pycircuit import dffe, module, rule

@module
def Cell(clk: bool, rst: bool, en: bool, data: bool, init: bool) -> {"q": bool}:
    state = dffe(T=bool)

    @rule
    def bind_state():
        state(clk=clk, rst=rst, en=en, d=state.q ^ data, init=init)

    bind_state()
    return {"q": state.q}
```

A parent imports `Cell`, declares `cell = Cell()`, binds all its inputs in a
registered rule, and reads `cell.q`. Definition and instance are distinct.
Instance declarations are structural; rule bodies do not allocate hardware.

Use a single local assignment to share a supported pure expression, for example
`next_value = (x + 1) & 255`. Outputs and rules then reuse the original SSA value.
It recomputes during Work and retains old-Q semantics, source kind, interval and
mathematical provenance; assignment does not allocate storage or narrow it.
Structural local wires reject rebinding, shadowing, direct forward references,
new locals after Return and rule-local assignment. Aliasing does not make shift
counts, divisors or parameter/default positions static. See
[immutable local wires](../reference/language.md#immutable-local-wires).

The return mapping also accepts supported pure bitwise, comparison, selection
and literal expressions. These become combinational SSA through the same
expression path used by rules. Return expressions do not add an implicit
register. Conditional expressions use the shared predicate and value-join rules
above; statement branches and mutable rule locals belong to the behavioral
profile. Exact finite Integer `+`, `-`, `*` and proven constant masks share
SSA range/type lowering. Integer selections use full branch representations
before the final boundary conversion; see the [language reference](../reference/language.md#exact-integer-expressions)
for boundary conversion and remaining restrictions.
Nonnegative Integer right shift by an actually static nonnegative Integer count
reuses existing bit extraction, retaining the complete SSA representation and
X/Z in retained positions. Runtime signed inputs and dynamic/unbound counts
remain unsupported; existing fully static signed evaluation is unchanged.

The source closure gate exercises bool and inline
`Annotated[int, range(1 << WIDTH)]` ports, keyword-only integer defaults,
multiple definitions per source, bool literals and cross-file bindings.
Equivalent hardware types may have different source locations; provenance is
preserved for diagnostics and declaration authority. Different widths and
nominal types remain different types.

The verified IR supports more than these source surfaces. General collection
authoring, complete `@system`, remaining mathematical lowering,
FIFO protocols, automatic clock-domain scheduling and CDC are separate unfinished work.
Do not infer Python support from a hand-authored IR test. Unsupported source
and emission capabilities must diagnose; there is no old-route fallback.

## Source ownership and build

The public commands are `pycircuit compile`, `pycircuit link` and
`pycircuit emit`.

Compile each source independently. Parent compilation consumes published child
interfaces through `-I`, not the child's Python or body as a fallback. The native
compiler first runs `ac-analyze-rule-writes` over capture and consumes its
cached declaration/path plan in the owning lowering pass. It reuses `ac.value.merge`
for disjoint Struct writers and preserves unknown rule-owner enables. It simplifies constructed-record
projections with `ac-simplify-record-wires`, then extracts declarations and
actual SSA dependencies with `ac-extract-source-interface` on a clone.
The same pass is inspectable in pycircuit-opt. Structural validity is separate
from supplied provider and canonical-builtin authority; public compilation keeps
the registry-backed body/interface checks before publication. Link takes
all required units explicitly. Keep the Python basename in source-owned `.ac`,
`.hpp`/`.cpp` and RTL groups; source directories disambiguate equal basenames.
Both emitters consume the same final artifact. Never compile a whole source
system and split generated text afterward.

Record simplification checks every intact module before mutation and compares
fresh canonical dependency summaries afterward. It follows nominal declarations
and SSA values, preserving saved record versions and exact result types. The
upstream operation-seeded rewrite driver processes only record creates/gets and
identity extracts created by this pass; original scalar operations, rules,
instances and state remain outside cleanup. This reduces retained record
transport without changing capture or eliminating initial importer construction.

The development tool `pycircuit-opt` also registers upstream `canonicalize` and
`cse` for explicit experiments. These are not automatically run by source
compilation: both perform dead-operation cleanup that is broader than the
record pass. ACIR experiments require closed-package hardware verification
before and after transformation; ordinary MLIR verification does not establish
source-unit authority or preserve checks on operations that were erased.

The shared installed `PycircuitExamples.cmake` helper runs the public commands
for each registered design. A design lists its module-owning `SOURCES` in
dependency order, `PACKAGE`, `TOP` and `ENTRY_SOURCE`. Use one helper call per
design binary directory. Each design supplies its own driver, finite config,
RTL testbench and independent golden assertions. The verifier compares C++
workers 1/2 and RTL Work traces.

Build the registered designs together:

```sh
cmake -S examples -B /absolute/build/examples -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/examples --parallel 4
ctest --test-dir /absolute/build/examples --output-on-failure --no-tests=error
```

Use canonical output paths without symlink components. Build artifacts stay
outside the source tree. Publication preserves existing managed outputs when
compilation, validation or emission fails.

## Typed DUT and runner

C++ emission provides source modules and `pycircuit_system.hpp` with `pyc_root`
and typed `pyc_dut::Inputs/Outputs`, `drive`, `sample`, `system` and `observations`.
The DUT owns its root and execution pool. CMake's generated `pycircuit_modules`
target is reusable; the example separately builds its testbench/runner executable
`pycircuit_system` against the installed Runtime. No DUT shared-library port C ABI
is delivered by this packet.

Use `SystemRunner` callbacks for host initialization, complete input drive and
successful output sampling. `--workers N` selects a positive worker count; the
DUT is constructed using `runner.workers()`. The compiler delegates only proven
independent instance subtrees, and collections remain whole tasks. All worker
Work calls finish before system checking and serial Xfer. Failure discards state
and clock proposals for the whole tree. No Eval lifecycle is added.

One Step is one Work/Xfer sampling epoch, not an implicit full clock period.
The host explicitly drives clock levels. Outputs are the successful Work's
old-Q snapshot; Xfer does not reevaluate parent combination. `sample()` rejects
before the first successful epoch and after failure. `drive/sample` belong to
the control thread between epochs and must not race execution.

Driven runners use finite `max_ticks` in sampling epochs; nonempty
`max_domain_cycles` is rejected until a clock-domain driver contract exists.
Hardware reset pins, host Reset and cold restart are distinct. Memory Reset does
not clear its contents; use a fresh DUT for an independent cold run.

## Examples

The current examples live under `examples/<name>/` and are listed in
`examples/catalog.json`. Each directory owns its Python sources and explicit
module closure. Retained independent native/RTL oracles validate hardware
behavior in the existing gate/nightly entrypoints; authoring and ordinary
simulation do not establish those oracle results.
