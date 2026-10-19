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

## Write a sequential program

Describe the function using meaningful variables, calculations, assignments and
calls. Put persistent declarations in the owning Module and cohesive state
transitions in Rules. Rule bodies read in statement order; ACIR supplies storage,
connections, admission and commit. Use names such as `saved`, `incoming`, `total`
and `previous`, rather than exposing register `q`/`d` terminals.

Ordinary designs and benchmark ports do not author clock/reset arguments,
DFF/DFFE bindings or proposal/commit wires. External Verilog names, clocks and
resets belong at the compiler export and verification boundary. An upstream
port list is not a template for the Python function signature. Preserve its
semantics through the verified compiler interface; report unsupported timing
instead of adding hand-written hardware plumbing to the frontend.

## Module hierarchy and independent outputs

Use ordinary typed `@module` functions to describe hardware hierarchy. Each
static module-call occurrence creates one child instance. Reusing its output
values fans out the existing wires; it does not create another instance or add
a clock cycle.

A single scalar is a direct argument or result: `-> ac.u8`, `-> bool` or an
admitted Bits/Enum type. Do not create a one-field Struct or a singleton tuple
just to wrap that scalar. Structs group multiple related fields.

Choose the interface grouping before choosing the return syntax. When several
fields normally travel together to one downstream module, prefer one named
Struct output and a matching Struct input. Connect that value directly instead
of passing every field as a separate port and rebuilding it downstream. Adding
or removing fields then changes the shared type and affected field users while
keeping the module's input/output count and wiring call shape stable.

Use a fixed, nonempty heterogeneous `tuple[T0, T1, ...]` for independent interfaces
or outputs routed separately. Return them in order and unpack the call directly;
each position is a separate ACIR result and may itself be a Struct. Group related
fields within each interface, rather than wrapping unrelated interfaces together
or flattening every field into an output. The following shows the tuple syntax:

```python
import pycircuit as ac

@ac.module
def Decode(word: ac.u8, enable: ac.u1) -> tuple[ac.u4, ac.u8, ac.u1]:
    return word[:4], word, enable

@ac.module
def Parent(word: ac.u8, enable: ac.u1) -> tuple[ac.u4, ac.u8, ac.u1]:
    tag, data, valid = Decode(word, enable)
    return tag, data, valid
```

If the fields instead all feed one execution interface, group them:

```python
@ac.struct
class Decoded:
    tag: ac.u4
    word: ac.u8
    valid: ac.u1

@ac.module
def DecodePacket(word: ac.u8, enable: ac.u1) -> Decoded:
    return Decoded(tag=word[:4], word=word, valid=enable)
```

The consumer declares `Execute(decoded: Decoded)`. Its parent passes the value
directly without repeating the field list at each module boundary. This grouping
adds no state, cycle or scheduling semantics. Field changes still change the
payload type/layout and may require consumer updates and recompilation; stable
port count does not mean binary interface compatibility.

Every element retains its declared type and source kind. Two different Struct
outputs remain distinct nominal types even if their packed widths match. An
existing single-Struct return still means one Struct output, not several ports.
Singleton tuple syntax remains admitted for compatibility and conformance tests;
ordinary single-value interfaces use the scalar directly.

One scalar or nominal result uses its admitted type directly: `-> ac.u8`,
`-> bool`, an Enum, a Struct or a fixed Table. Bind it with `value = Child(...)`.
This preserves its source kind and boundary constraints; Boolean and Integer
remain distinct even when both occupy one bit.

For a tuple signature, the return must be an explicit tuple of the declared arity. At a call site use
one flat tuple of distinct ordinary local names. Starred, nested, list, field
and index targets are rejected, as is treating the multi-output call as a scalar
hardware payload. Ordinary local rebinding retains its existing behavior; unpacking cannot
write persistent owners or protected hardware bindings. Modules, Rules and
Methods share this fixed output-signature convention; it does not create a
general hardware tuple value.

A named Module result also retains its immutable instance identity. For a single
result, `child = Child(request)` is both the result value and the handle used by
`child.update(...)` inside a Rule. A named multiple-result bundle can be unpacked
later with `header, payload = child`; a zero-result Module still supplies a handle.
An immutable alias refers to the same allocation. Unpacked output names alone
are values, not instance handles.

Output order is part of the published interface. Generated ports are named
`result_0`, `result_1`, and so on, with deterministic suffixes on name collisions;
caller variable names do not rename the callee's ports. Independent source-unit
interfaces publish the tuple return form and every result constraint, so imported
calls use the same rules without reading provider Python. Eligible forward
references to unpacked outputs keep the existing unambiguous-name and acyclic
connection requirements.

Apply this grouping rule to new or deliberately migrated module interfaces.
Existing examples are not a reason to flatten a shared interface or bundle
unrelated destinations. Preserve their timing and independent
oracles when migrating them. See the runnable example at
`examples/multi_outputs/README.md` and the
[language reference](../reference/language.md#direct-module-composition).

Examples should teach a current, distinct pattern. Start from `examples/GUIDE.md`;
the historical fixture archive is not a porting checklist. Keep syntax/type
stress in API tests and avoid repeating full host/RTL/System harnesses for a
small variation. Follow `examples/STANDARD.md` when retaining, combining or
retiring an old example.

## Rule authoring contract

Keep each Rule simple and cohesive. A Rule describes one function, including
related outputs and effects that must succeed together. Use separate Rules for
independent functions. Nesting a Rule or Method call keeps its state proposals,
guards and body effects within the caller's outer atomic transaction.

Defining a Rule does not execute it. Each static Module-scope Rule call registers
one outer transaction; repeated calls are distinct registrations. Inside a Rule,
a Rule or Method call invokes the existing callable body in that same transaction.
A Method is callable-only and cannot be registered in Module scope. These forms
share one Python capture, common verified IR and C++/Verilog route.

Use explicit finite input and result annotations for published callable
interfaces. A result can be one admitted scalar or nominal payload, `None`, or a
fixed, nonempty heterogeneous tuple. Use ordered tuple returns and direct flat
unpacking for multiple independent results; use a scalar for one value. Group fields that
travel together as one named Struct; use separate tuple positions for independent
interfaces. A callable invocation creates no Module instance, state or extra cycle.
For example, a reusable source-global Rule can propose an update to an actual
Module-owned variable passed as an argument:

```python
@ac.rule
def add_to(total: ac.u8, incoming: ac.u8) -> ac.u8:
    previous = total
    total = previous + incoming
    return total

@ac.module
def Accumulator(incoming: ac.u8) -> ac.u8:
    total: ac.u8 = 0
    return add_to(total, incoming)
```

Rule definitions can be source-global, as above, or nested in a Module to capture
locals. Definition location does not establish instance ownership: calls and
registration bind the actual Module state. An external Rule receives explicit
parameters; it does not implicitly capture the caller's local variables.

Existing unannotated executable Rules remain supported. Their finite argument
contracts come from actual SSA values and declared owners, preserving Boolean,
Integer, fixed-bit and nominal distinctions. Different actual contracts use
separate private specializations. A direct Module-return call keeps its enclosing
declared result context; unannotated assigned or nested calls infer their scalar
or tuple result from the returned SSA. Closed actual constants can supply an
admitted static loop bound. This does not treat defaults as actuals, invent a
public ABI from the first registration, or admit arbitrary Python execution.
Callable-only external interfaces without an unambiguous published contract need
explicit annotations. A source-global Rule receives its explicit arguments; it
does not capture unrelated locals or state from a calling Module.

Module variables own storage. Rule parameters, local annotations, saved values
and returned payloads are SSA values. Passing an actual writable owner name to a
Rule binds proposals to that owner; forwarding such a formal through another
call preserves the same actual owner. Direct assignments within the callee see
its current candidate. A saved ordinary value remains a snapshot and never gains
owner identity merely because it has the same type or came from a state read.
For example:

```python
@ac.struct
class Update:
    before: ac.u8
    after: ac.u8

@ac.module
def Counter() -> Update:
    count: ac.u8 = 5

    @ac.rule
    def bump(target: ac.u8) -> Update:
        before = target
        target = target + 1
        return Update(before=before, after=target)

    return bump(count)
```

Here `before` is the old candidate and `after` is the updated candidate. Passing a
saved value instead of `count` computes a local result without writing `count` or
rebinding the caller's saved value. Ordinary argument expressions execute once
in source order, including keyword actuals; writable owner references are read at
call entry after those expressions finish. Each reached owner proposal retains
its payload and address obligations even if a later proposal overwrites it.

A static child handle supports calls such as `child.update(amount)` inside a
Rule. Resolve the actual allocation, not its alias spelling or an equal type.
Aliases share state; separate calls constructing child Modules own separate
state. Calls route typed arguments, results and proposals through the existing
hierarchy, including independently compiled providers. Authors do not add
forwarding ports, writeback wires or handshake wrappers.

Ordinary outputs carry compiler-owned availability. A non-firing Rule produces
an unavailable value, with no implicit zero, default or holding register.
Availability follows calls, fields, joins and Module boundaries; a reached use of
an unavailable operand prevents admission. Valid payload X/Z bits remain data.
Authors do not add `valid` fields solely to represent this availability. Authored
protocol `valid`/`ready` signals retain their own interface meaning.

Within one transaction, reads observe sequential candidates and saved SSA values
keep their earlier snapshots. Independent outer transactions observe committed
state. Explicit `@ac.rule(after=(earlier,))` ordering can provide predecessor
candidate visibility in the same cycle; each named predecessor must identify one
unique local outer registration. A missing or ambiguous registration, a cycle,
or a conflicting writer is not resolved by source order or implicit priority.
The [EHR design reference](https://csg.csail.mit.edu/pubs/memos/Memo-479/memo479.pdf)
explains why forwarding visibility and the final storage choice must agree.

Whole-owner updates from ancestor and descendant Rule registrations may share
one state owner when the compiler proves their grants mutually exclusive from
the actual instance wiring and SSA values. Each Rule keeps its own atomic
boundary. The compiler combines incoming and local proposals before storage;
authors do not add forwarding or commit code. Repeated instances retain separate
state, including occurrences without an upstream writer. Cross-hierarchy
admission covers whole-owner updates and unordered field/index writers whose
write footprints are provably disjoint: a different Struct field at the common
selector prefix, a provably different one-dimensional Table address, or
mutually exclusive write paths. An ancestor partial lane is composed with its
captured old Q by its own domains before it crosses a hierarchy port; selectors
and masks are not new ports. The receiving definition composes its local
domains over that complete lane. Each proof layer re-derives the lane and
footprint independently. Independent cross-hierarchy `after=` ordering remains
unsupported; ordering between effects on one object is expressed by one parent
Rule calling child Methods in sequence inside a single outer transaction.
Mixed ordered/unordered groups on one owner also remain unsupported. Existing
local ordered and disjoint field/index behavior keeps its documented bounds.

Keep algorithmic conditions explicit. General queue/resource admission and its
full conflict, capacity and four-state matrix remain implementation and runtime
acceptance work in [issue #319](https://github.com/PTO-ISA/pyCircuit/issues/319).
Existing explicit handshakes must remain until equivalent inferred guards pass
the original oracles. Native Queue schemas or captured resource operations alone
do not establish that execution support. Finite SCF normalization likewise does
not admit arbitrary Python loops or every SCF operation; complex loop-control
combinations need their own generated execution evidence.

## Queue-facing interfaces

Ordinary hardware sources should expose typed Rule calls for queue traffic,
without repeating FIFO `valid`/`ready` or full/empty guards in each Module.
Declare a named Queue, call `push`/`pop` inside a Rule, and let the compiler
carry admission and output availability across calls. For example:

```python
@ac.module
def Increment():
    ingress = ac.queue[ac.u16](depth=2, latency=1)
    egress = ac.queue[ac.u16](depth=2, latency=1)

    @ac.rule
    def put(value: ac.u16) -> None:
        ingress.push(value)

    @ac.rule
    def get() -> ac.u16:
        return egress.pop()

    @ac.rule
    def transfer() -> None:
        value = ingress.pop()
        egress.push(value + 1)

    transfer()
```

`put` and `get` are callable interfaces; only `transfer()` is registered here
as autonomous work. A caller Rule can use `destination.put(source.get())` so
both queue operations share its atomic boundary. A denied required operation
stalls that whole Rule. Rule-local values remain combinational SSA.

Queue is a native child Module in the same instance and call graph. Its Python
constructor remains `ac.queue[T]`; authors do not import native FIFO symbols or
add a wrapper Module to obtain this behavior. `peek()` is a non-consuming read
of the committed head. Separate Queue constructions remain separate instances;
an alias keeps the original instance identity.

The runnable example at `examples/queue_rule_pipeline/README.md`
combines this pattern with a stateful child Module, a finite loop, local
`after` ordering and an ordinary Rule result consumed without an authored valid
field. Use this small complete System as the starting example for current Rule
and Queue authoring. Its output-backpressure checks require the Queue operations
and both parent/child state proposals to commit together or all hold.

Group fields that travel together in a named Struct payload. Do not add a
`Stream.valid` wrapper solely to transport compiler-owned Rule availability.
Explicit external RTL protocols may retain handshake fields in a boundary
adapter. Preserve existing arbitration, capacity, latency and four-state
oracles during migration; unsupported arbitration or routing remains an
implementation gap, not permission to invent author-facing readiness helpers.

Module/Rule connections use typed parameters and return values, with
compiler-propagated output availability. The standalone Signal resource design
has been cancelled: do not add Signal declarations, set/get operations or an
author-facing relation table to express these connections. Continue to use
module state and Queue for cross-cycle storage.

## Memory-facing interfaces

Use a named `ac.sync_mem[T](depth=N)`, `ac.sync_mem_dp[T](depth=N)` or
`ac.byte_mem[T](depth=N)` in the owning Module. Inside a Rule, write
`memory.request(address)`, `memory.result()` and `memory.write(address, value)`;
use `memory.read(address)` for asynchronous byte memory. Keep the value scalar
when it has one field. Physical controls, response availability, port conflicts
and atomic commit belong to the compiler.

A synchronous result is a committed snapshot with the existing two-idle-edge
lifetime. It does not wait for a request on the preceding source line, consume a
response or prolong its lifetime. A request followed by an unavailable result
in one Rule holds both operations. Separate request production from response
consumption, preserving the algorithm's latency/controller phases. Save the
snapshot in Module state if it must survive longer stalls. Do not use
`is_known(data)` as a readiness test or add authored valid/ready wrappers.

Prefer `previous = memory.read(address)` followed by ordinary computation and
`memory.write(address, updated)` for asynchronous read-dependent updates. Reuse
that local snapshot when possible. Memory aliases share physical port limits;
source order cannot authorize two simultaneous writes on one port. See
[Memory operations](../reference/language.md#memory-operations) for masks,
static ports, reset and address behavior. The DMA and Memory Banks examples use
operation calls while retaining their real controller phases and timing oracles.

For finite System schedules, state advances on successful rising edges; logs
observe each successful Work sample. A low/rising pair can therefore log the
same committed state twice. Use fixed `ac.uN` for wrapping counters; a checked
`Annotated[int, range(N)]` variable requires every assigned value to remain in
its declared range. Keep comparison literals representable in fixed-width peers.

## Module state declarations and storage children

A Module declares both child modules and its own persistent state variables.
Authors express a state variable with a module-scope declaration such as
`count: ac.u8 = 0` or `saved: Packet = Packet()`. The compiler supplies the
storage instance and its connections; authors do not write DFF/DFFE plumbing.

Treat these state variables as storage children of the enclosing Module after
lowering. They use the same `ac.instance` representation, instance identity and
typed input/output connections as other children. A DFFE leaf has clock, reset,
enable, data and initialization inputs, and a Q output. Its data, initialization
and Q share the variable's hardware payload type. DFF is the corresponding
unconditionally enabled storage behavior. Storage remains a stateful primitive
leaf; it is not a pure function and separate instances do not share state.

The storage requirement is based on the declared type, not a whitelist of the
`u1` through `u63` convenience spellings:

| Declared state type | Required storage treatment |
| --- | --- |
| `bits[W]` and fixed-width aliases, including wider widths | Preserve the declared width and four-state payload. |
| Enum | Preserve nominal identity and its declared encoding through storage. |
| Struct, including admitted nested records | Carry the structured value as a typed storage payload; retain its field layout and state ownership. |
| Boolean, bounded Integer and other arithmetic data types with a specified finite storage representation | Use the common type/lowering path, preserving the logical kind, range and arithmetic conversion rules. |

Arithmetic operations compute values; they do not determine whether a value is
stored. A module-scope state declaration establishes storage. An ordinary
combination such as `next_count = count + 1`, a helper local, or a child output
binding is still a value/wire, even if it has the same type as a state variable.
Do not add registers to these bindings. Initializers follow the existing static
initialization contract; a runtime arithmetic expression is not a new kind of
state initializer.

The arithmetic row is a required coverage principle, not an assertion that every
MLIR `arith` type or arbitrary Python value is already admitted. A type needs a
defined storage representation and a supported source contract. If an intended
Enum/Struct/arithmetic state type lacks its normal register-lowering path, record
and fix that implementation gap; do not hide it by requiring hand-written
register instances or unapproved casts to Bits.

### Required state-declaration checks

- Account for every persistent declaration and its owning Module/storage child;
  do not leave it as an ordinary temporary or merge independent instances.
- Check the declared logical type through initialization, updates and Q. Include
  Enum, structured and arithmetic state, not just `uN` examples. Preserve the
  applicable width, range, encoding, layout and four-state rules.
- Check that compiler-owned clock/reset/enable/data connections use the normal
  child-instance interface. An enable gates updates; absence of an update holds
  state. Grouping fields must preserve their update conditions and ownership.
- Preserve reset, hold, sampling and failed-commit behavior in the owning flow;
  the simplified frontend declaration does not add cycles or change visibility.
- Check each claimed type/path through source compilation, explicit linking and
  both generated targets. A successful type declaration or IR schema check alone
  does not establish C++/RTL execution support.

The common transaction path retains admitted finite Bits, Enum, Struct, Table,
Boolean and bounded Integer state. Wide Bits are not limited to host integer
width. Declared kinds, nominal identity, layout and four-state payloads survive
initialization, partial updates, joins and storage lowering. Fixed Tables use
collections of existing storage leaves. Existing restrictions on shapes,
initializers and nested indexing still apply; this adds no new floating-point or
arbitrary MLIR arithmetic type. Separate Rules with dynamic disjoint-address
writes to one Table currently reject on the unified callable path; restoring the
existing address-grant behavior remains unfinished. The
[language reference](../reference/language.md#method-execution-and-remaining-capabilities)
records the execution and remaining resource/control boundaries.

## Behavioral source profile

The example at `examples/rob/README.md` uses ordinary annotated
variables, nominal `@ac.struct` values, `ac.table[4, Entry](init=0)` allocation
and nested Rules in the owning storage Module. It returns a typed `RobResult`,
using default construction and field assignments, through the direct
`Rob` → `RobStorage` module call. Source capture remains syntax-only. The owning
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
The address-grant contract requires dynamic same-field indices to have explicit
address separation proved against assignment-time SSA, or proven mutually
exclusive whole-owner grants. Separate-Rule dynamic address separation currently
rejects on the unified callable path and remains restoration work. Range facts
alone do not separate addresses.
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
selection, including at module scope when the selected values feed later Rules,
or `entries.argmin(where=lambda entry: ..., key=lambda entry: ...)`
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
named ordinary typed `def` callbacks through the shared pure-helper lowering.
Named callbacks retain fixed-Bits parameters/results and no free captures.
Generic row types must exactly match their formal annotations; individual lane
values do not supply range or constant authority. Helper calls inside expression
lambdas remain unsupported. Use ordinary typed `def` for pure reuse and reserve
`@rule` for exactly one static autonomous registration per declaration/instance.
See [pure value helpers](../reference/language.md#pure-value-helpers) for lexical
snapshots, publication and the bounded body grammar. Ordered prefixes can use the
[bounded literal-range loop profile](../reference/language.md#bounded-source-loops)
in pure behavioral rules and ordinary typed helpers; it adds no scan API or
clock cycle. Helpers can update ordinary local Table elements and Struct fields
without modifying their caller's saved values. Preserve the loop's exact
range grammar, binder behavior, effect exclusions and resource bounds.

Prefer `tokens = ac.queue[T](depth=N, latency=L)` and call
`tokens.push(value)`, `tokens.pop()` or `tokens.peek()` inside a Rule. Allocation
is module-scoped and each construction creates a distinct Queue child; an alias
keeps the same instance identity. The compiler derives admission and returned
value availability, while Python exposes no clocks, reset or proposal wires. The
named form defaults to `ready_policy="downstream_pop"`; use
`ready_policy="local_occupancy"` when a full Queue must wait for capacity rather
than replace a popped head. Static `latency=L` delays availability while
keeping all waiting tokens within the declared depth. A birth at E0 becomes
available after E(L-1) commits; EL is the earliest consumption. An unavailable
head reads as packed zero after initialization. Existing Work/Xfer and
whole-system discard apply. Use the raw three-output constructor only at an
explicit external protocol boundary; do not build inter-Module handshakes from
it. See [queues](../reference/language.md#queues) for both forms and exact
admission.

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

Unsigned bits shifts use static nonnegative Integer or fixed Bits counts and retain the input
width, including before a later destination widening. They preserve retained
X/Z positions and fill with known zero; counts at least the width yield zero.
The importer reuses common width analysis and existing shl/lshr operations, with
full-precision count validation. Runtime Bits counts may be narrower or wider
than the data; high bits are never truncated, and X/Z counts yield wholly
unknown results. Runtime mathematical Integer counts remain unsupported.
Mathematical Integer right shift remains the
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

## Primitive conformance is a compiler test concern

Ordinary designs use Module variables and Rules. They do not instantiate or bind
DFF/DFFE pins. Explicit primitive binding remains a supported mechanism for
compiler/backend tests of physical domains, reset/enable and four-state behavior;
those fixtures are not source-authoring templates or a benchmark-porting fallback.
See the implementation and language references when working on those specific
compiler tests. Existing tests of primitive contracts and nominal type identity
retain their original independent oracles.

Use a single local assignment to share a supported pure expression, for example
`next_value = (x + 1) & 255`. Outputs and rules then reuse the original SSA value.
It recomputes during Work and retains old-Q semantics, source kind, interval and
mathematical provenance; assignment does not allocate storage or narrow it.
Structural local wires reject rebinding, protected-name shadowing, direct forward
references, new locals after Return and rule-local assignment. In ordinary
structural/behavioral Module/System bodies, a data local may precede a directly
nested Rule of the same name. Save an alias before the Rule declaration if the
data is needed afterwards: from that declaration onward, the original spelling
identifies the Rule and requires an admitted registration. Earlier helper captures keep their snapshots; later helpers
cannot capture the retired data spelling. Storage, instances, imports, formal
names and intrinsic grammar remain protected. This exception does not extend
typed transaction/Method-owner binding or change that structural profile's
forward-registration rules. Aliasing does not make shift
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

The verified IR supports more than these source surfaces. Remaining collection
forms, mathematical lowering, automatic clock-domain scheduling, CDC and some
physical/four-state System inputs are separate unfinished work.
Do not infer Python support from a hand-authored IR test. Unsupported source
and emission capabilities must diagnose; there is no old-route fallback.

## Source ownership and build

The public commands are `pycircuit compile`, `pycircuit link`, `pycircuit emit`
and `pycircuit run`.

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

## Method body checks and observations

The common callable path supports statement-arm calls and body effects, finite
state, nominal joins and compiler-managed results. Generated C++ workers 1/2 and
RTL tests exercise this bounded path. Those tests do not establish general
resource admission, arbitrary control flow or a full example/nightly gate.

Within the supported Method route, use ordinary `assert`, `log` and `report` in
the body or statement `if`/`elif`/`else` arms. A saved local value remains the
candidate value at that statement; later writes do not change it. Assertions check the completed
outer transaction and do not control later calls. All body effects use the final
transaction fire, so a later false guard suppresses earlier body effects too.
Put unrelated autonomous work in another registered Rule.

Rule and Method transaction bodies may use bounded statement `if`/`elif`/`else`
and `pass` over admitted finite state and scalar, Enum, Struct and Table locals.
The existing finite type, shape and indexed-update restrictions still apply.
Both arms start from the same incoming candidate. A missing write preserves that candidate
and its write intent, including a write made before the branch. A changed local
read after the join must have a value on both arms; an existing local inherits
its entry value on an unassigned arm. Nominal joins require the same declared
Struct or Enum type on both arms. Joins select data and raw write intent with the original four-state predicate; an identity assignment remains a
write. Calls and effects may appear before, inside or after a branch. A branch
containing a call or effect requires a known selector, as do its enclosing
branches; a pure branch keeps its existing four-state merge behavior.

For example, the existing conditional counter fixture uses this sequence inside
a registered Rule, with `owner` a static counter instance and `selector: u1`:

```python
@rule
def drive() -> None:
    first = owner.add(1)
    if selector:
        selected = owner.add(2)
        log("info", "selected true", selected)
    else:
        selected = owner.add(4)
        log("info", "selected false", selected)
    after = owner.add(3)
    log("info", "snapshots", first, selected, after)
```

Here `rule` and `log` are imported from `pycircuit`, and `add` returns the
updated candidate. Starting at Q=5 gives snapshots 6/8/11 for selector 1 and
6/10/13 for selector 0. Each arm starts from candidate 6; another autonomous
Rule still reads old Q=5. The values are statement-time snapshots.

Local Struct field assignments reconstruct values without storage or producer
mutation. For example, with `Packet.left` and `Packet.right` both `Cell` values
and `Cell` containing `data: ac.u8` and `tag: Tag`, a Method can use:

```python
@ac.method
def assemble(flag: ac.u1, payload: Packet) -> Packet:
    local = payload
    saved = local
    local.left.data = 7
    if flag:
        local.left = Cell(data=9, tag=Tag.FIVE)
    else:
        local.right = saved.left
    return local
```

Here `ac` is `pycircuit`, the records use `@ac.struct`, and `Tag.FIVE` is a
declared Enum member. `saved` and `payload` retain their original fields. Nested
field and subtree replacement checks the declared field type and preserves the
root nominal boundary. Direct Enum locals can also join. Pure joins preserve
four-state Bits and raw Enum carriers without introducing membership faults.
Admitted Struct/Table values retain their declared field and element types
through reconstruction and joins. Field and element updates preserve the current
root candidate; saved aliases remain snapshots. This does not admit arbitrary
nested indexing, slice updates, annotated field targets or new augmented-assignment
forms.

Unselected guards bypass even when false or X/Z. A selected false guard holds
the entire outer Rule across all owners and suppresses all its effects, including
earlier effects. It does not select `else`. A reached X/Z effect selector faults
at a rising edge outside reset; a later false guard cannot hide it. At low,
held, falling or reset samples it adds no sampled fault, but final fire remains
false and all transaction effects stay inactive. With known selectors, selected
assertions retain their every-Work reset mask, including held/falling samples;
selected observations can publish after a successful held/falling epoch.

A call result becomes available after that call completes on its source path,
even for a returned literal or capture. Bind a changed local on both arms to
use it after the join. A value-returning outer Rule may have dynamic admission;
the compiler propagates its result availability rather than requiring `-> None`.
Dead arms still receive type, recursion, closure, domain and writer checks. A conditional `log` or `report` alone needs
no dummy Method, state or guard. Reports with the same owner/name still conflict
across mutually exclusive arms; put one report after the join.

Keep selectors, guards, pure helpers, collection callbacks and conditional
expressions free of Rule/Method calls and effects. Early or branch-local returns,
cross-domain transactions and ambiguous competing writers remain rejected.
Bounded typed Rule/Method loops support Python-style `else`: natural completion
(including zero trips) runs it once, `break` suppresses it and `continue` does not.
Their effects remain in the outer transaction; a late failure commits neither
body nor else proposals. Keep the existing finite-range/progress and branch
binding/return restrictions. Pure helper loop-else remains unsupported. See the
[loop profiles](../reference/language.md#bounded-source-loops) for exact boundaries.
General SCF and resource admission remain outside this bounded claim. See the
[conditional-effect contract](../rfcs/serial-rule-method/conditional-effects.md)
and [branch-state contract](../rfcs/serial-rule-method/branch-state-ssa.md).

Source capture reuses expect/observe operations; MLIR owns the state versions,
invocation identities, six-field sampled inventory with tagged `controls`,
source paths and physical wiring. Authors do not supply final IDs or
transaction ports. A closed `@system` keeps its
compiler-generated clocks, resets and `pycircuit_sim` harness. Verification must
include actual generated C++/RTL execution and failed-epoch publication checks;
publication alone is insufficient. Admitted finite partial state updates use
this same path; transaction resources still require their separate acceptance. See the
[language boundary](../reference/language.md#method-execution-and-remaining-capabilities)
and [body-effect contract](../rfcs/serial-rule-method/sampled-body-effects.md).

## Four-state checks and finite System stimuli

Use `ac.four_state("10xz")` for explicit Bits data and `ac.case_eq` / `ac.is_known`
for verification. Do not build a Signal wrapper, add clock/reset parameters to a
DUT, or use X detection to implement functional state or admission decisions.
One scalar remains a scalar; related multiple fields use the usual Struct.

A closed System can specify its raw domain samples in its decorator:

```python
@ac.system(samples=((0, 0), (1, ac.four_state("x"))))
def Scenario():
    observed = Register(ac.four_state("10xz0011"))

    @ac.rule
    def check():
        assert ac.case_eq(observed, ac.four_state("00000000"))

    check()
```

Here `Register` has an eight-bit state initialized to zero and an unconditional
Rule storing its input. Each row is one Work/commit epoch. Checks see old state;
the second row commits the new data only under an explicitly linked `if_one`
reset policy. Under default `strict`, that row faults. The generated runner
performs host Reset before row zero and stops at the end or first failure.
Use no `--cycles` with explicit samples. Without samples, the existing low/high
cycle driver remains available. Keep logs and assertions in cohesive Rules;
the compiler owns clock driving, reset interpretation and atomic publication.

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

The 17 current examples live under `examples/<name>/` and are listed in
`examples/catalog.json`. Each directory owns its Python sources and explicit
module closure. Fifty-four superseded wrappers and their original oracles live
under `tests/fixtures/historical_examples/`; they are comparison assets, not
current examples or passing results. Retained independent native/RTL oracles
validate hardware behavior in the existing gate/nightly entrypoints; authoring
and ordinary simulation do not establish those oracle results.
