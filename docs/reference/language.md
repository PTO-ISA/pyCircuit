# Python source language reference

The [complete language and execution specification](language-specification.md)
connects these source details to MLIR, codegen, Runtime and independent acceptance.

The active source route captures Python syntax without executing design code,
compiles each source to a published unit, links an explicit closure, and emits
C++ or Verilog from one verified hardware artifact. The current module source
contract and complete example are documented in the
[agent frontend guide](../development/agent-frontend-guide.md).

Python Enum declarations with explicit encoding widths, nominal annotations,
imports/reexports, members, equality, selection, defaults and explicit bit
conversions are supported. `decoded, valid = ac.enum_from_bits[State](raw)` binds
the raw nominal carrier and separate Boolean membership to two ordinary local
names. Declaration-only, empty and facade files use the ordinary independent
compile/link flow. See [nominal enums](spec-enums.md) for exact boundaries.

## Behavioral variables and rules

The behavioral slice uses `import pycircuit as ac`, unsigned `ac.bits[W]`
and `ac.u1` through `ac.u64`, and same-source `@ac.struct` declarations.
Struct fields are ordered, nominal and immutable values. Same-source constructors
may omit fields: explicit keywords override declared static defaults; omitted
fields without a default recursively become zero. An omitted nested struct uses
all-zero leaves; a declaration such as `inner: Inner = Inner()` explicitly opts
into Inner's declared defaults. Every declared default must be valid even if
unused or overridden. Defaults cannot depend on a caller's runtime bindings.
Unknown/duplicate fields and invalid types or ranges diagnose.

Struct fields may contain positive, closed one-dimensional `Table[N, T]`
values, where `T` is Bits, Enum or a finite, acyclic nominal Struct. A tuple or
list literal constructs such a field only when its expected Table type is
known; its length and every element must match that type. Static defaults,
recursive zero initialization, whole-value copies, projection, selection and
whole-field replacement preserve the declared element order and all four-state
bits. This does not introduce general tuple values, dynamic Table shapes or
Table-of-Table source indexing. Indexed partial writes within a Table-valued
Struct field remain unsupported.

Canonical direct imports such as `from pycircuit import u1, u8` retain the same
fixed-bit authority as `ac.u1` and `ac.bits[N]`; aliases and namespace imports
also resolve through their actual provider. Unrelated or shadowed names reject.
Imported Struct constructors in ordinary expressions and annotated rule results
use their published nominal fields and require every field explicitly, including
nested constructors. Provider defaults are not published and cannot supply
omitted fields. Imported static initializers and Table-query constructor
callbacks retain their existing narrower admission.

Module-scope annotated variables allocate persistent storage.
`entries = ac.table[N, Entry](init=0)` allocates a fixed one-dimensional table
of persistent entries. Contextual zero initializes each field recursively,
ignoring nonzero constructor defaults. `init=Entry()` instead constructs an
element using its defaults and broadcasts that value to independent owners.
Initializers must be compile-time values; inputs and existing state are not
initialization expressions in this slice.

A same-source top-level `@ac.rule` takes these variables as ordinary parameters.
Assignments inside the rule follow sequential local value semantics; a read
after an assignment sees that rule's computed value. Rebinding a parameter
connected to a module-owned variable proposes an update to that owner. Rebinding
a pure input or local changes only the local value. The module can return the
rule's typed struct result or existing named result mapping directly. A local
`result = Result()` allocates no storage; `result.field = value` reconstructs
its SSA value, including nested field paths. Previously saved values remain
snapshots. Persistent field updates participate in the root owner's write set
and alias checks, and untouched fields retain their previous value.
An indexed field assignment such as `entries[index].payload.tag = value`
updates only that field or Struct subtree. Even with an X/Z index, other fields
retain the current rule-local candidate, including earlier updates in that rule.
The existing full-width index equality selects the target data; it does not
become a new storage enable. An explicit RHS read such as `entries[index].a`
still follows unknown-index read semantics, and whole-element assignment still
selects the entire replacement value.
The [ROB example](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/rob/rob.py)
shows table entries, pointers and occupancy updated together.

The source-lowering MLIR pass infers storage and write enables using existing
rule, instance, collection, struct, table and bits operations. A table remains
one collection of storage leaves; indexed updates are compact table maps.
All state proposals prepare during Work and commit together through the existing
whole-system Xfer boundary. Failure discards pending data and clock history.
Missing branch assignments hold the existing value. Outputs describe the rule's
local computation during Work; they do not imply that Xfer reevaluates outputs.

Bits arithmetic is fixed-width unsigned: addition, subtraction and multiplication
wrap modulo 2^W. This differs from the exact finite Integer expressions of the
existing `Annotated` source spelling below. Struct and table values retain
their aggregate types; scalar leaves are `!ac.bits`. This slice does not add a
new arith-dialect normalization pipeline.

Unsigned fixed bits support explicit static slices, with bit zero denoting the
least significant bit. `value[low:high]` returns `bits[high-low]`; omitted bounds
mean zero and the source width. The bounds must be static Integers satisfying
`0 <= low < high <= width`. A step may be omitted or a static Integer equal to
one. Selected value/known/Z bits are preserved, including slices across native
word boundaries. For example, `addr: ac.u40` permits `addr[12:40]` and
`packet.data[:8]` explicitly selects a low byte.

There is no clamping or negative-index normalization. Empty, reversed, dynamic,
Boolean and out-of-range bounds, nonunit steps, scalar bit indexing and slice
assignment diagnose. Boolean and mathematical Integer values must not acquire
fixed-bit semantics through slicing. Table indexing accepts a proven nonnegative
half-open value interval within the declared depth, or the existing complete-width
proof. Closed Integer literals and fixed-bit remainder by a positive static
Integer carry bounded intervals; aliases and rule captures preserve those facts.
Wrapping arithmetic does not inherit an earlier remainder bound. This is not
constant or known-bit authority: high X/Z index bits are retained, and runtime
divisors do not gain a remainder bound. Table slicing is unsupported. A narrower declaration still cannot
silently truncate: use an explicit slice at that boundary.

`ac.concat(first, second, ...)` joins one or more unsigned fixed-bit values,
with the first operand in the most significant positions. A singleton preserves
its operand. Each operand retains its own width; the result width is their sum.
Existing destination conversion happens afterward, so a wider destination
zero-extends the completed result and a narrower destination requires a slice.
Concatenation preserves every value, known and Z bit, including payload bits
under unknown masks. It adds no storage or sampling boundary.

Use existing slices to replace static bit ranges. For a 32-bit word,
`ac.concat(word[12:32], replacement, word[:4])` replaces bits 4 through 11 when
replacement has type `ac.u8`. Omit empty end segments; a full replacement is simply
the replacement value. Slice assignment and dynamic insertion remain unsupported.

Operands must already be fixed-bit values: Integer/Boolean literals and values,
raw Enum, Struct and Table values do not convert implicitly. Use
`ac.enum_to_bits` explicitly for an enum carrier. Qualified and aliased imports
resolve by their bindings, and ordinary fixed aliases and rule captures retain
their type authority. Empty calls, keyword/starred arguments, subscription and
unresolved or overflowing widths diagnose. Concatenation is an ordinary source
expression; it does not add a static initializer evaluator or expand observation
emission support.

`ac.popcount(value)` counts set bits in one unsigned fixed-bit operand. For
input width W, its natural result width is `bit_width(W)` (equivalently
`floor(log2(W)) + 1`), before destination conversion. Zero returns zero and
all ones returns W. The result remains fixed bits, including at width one.

The singleton reduction is identity: a one-bit operand preserves its complete
value/known/Z planes. For wider operands, any X/Z makes the whole natural count
X, with no Z; the value plane underneath that computed X is unspecified.
Destination widening zero-extends the natural count afterward. This distinction
preserves the original bit-count reduction semantics.

The source call accepts exactly one positional fixed-bit value, including
ordinary aliases, captures and explicit `enum_to_bits` results. Logical
Integer/Boolean values and raw Enum or aggregate values do not convert
implicitly, even when their physical width fits. Keywords, star expansion,
subscription, unresolved/nonpositive widths and implicit narrowing diagnose.
This expression adds no state or static-initializer evaluator. It lowers through
existing bit operations with a logarithmic number of IR operations and compact
scalar literals; runtime work still scales with W and existing emission capacity
limits apply. Maximum representable width is not a practical allocation promise.

`ac.count_leading_zeros(value)` and `ac.count_trailing_zeros(value)` count
consecutive zero bits from the most- or least-significant end. They share
popcount's one-positional-operand fixed-bit authority and positive resolved-width
requirements. Their natural result is unsigned `bits[bit_width(W)]`, with range
0 through W; an all-zero input returns W. Destination widening happens afterward,
and narrowing requires an explicit slice. The helpers add no storage or sampling
boundary and reuse existing common bits operations in an O(log W) graph.

Their four-state behavior preserves the historical zero-count trees. Starting
at the selected endpoint, let d be the position of the first known one, or W
if there is none. A known-zero prefix returns known d. If only the endpoint is
X/Z and positions 1 through d−1 are known zero, the lowest `bit_width(d)` result
bits are X and higher natural bits are known zero. Any other X/Z before that
first known one makes the natural result all-X. Unknown suffix bits after the
known one do not affect the result. Results contain no Z, and latent computed
value bits under X are unspecified. For example, four-bit leading count maps
`1xxx→000`, `x111→00x`, `x011→0xx` and `00x1→xxx`. At W=1 both helpers are
logical complement: `0→1`, `1→0`, and `X/Z→X`. Widening adds known-zero high bits.
These expressions retain existing observation-emission limitations and emission
capacity checks; representable enormous widths do not imply executable payloads.

Priority encoding uses fixed-result unpacking:

```python
index, valid = ac.priority_encode(value, order="low")
index, valid, conflict = ac.onehot_encode(value, order="high")
```

Both accept one authoritative unsigned fixed-bit operand, as for popcount.
The optional keyword-only `order` is the literal `"low"` (default) or `"high"`.
Low order chooses the least-significant asserted bit; high chooses the most
significant. The natural index is fixed `bits[max(1, bit_width(W-1))]`.
Zero input returns index zero and false valid. Valid and conflict are logical
Boolean values; conflict means more than one asserted bit. Destination widening
happens after natural-width computation; narrowing requires an explicit slice.

Four-state index behavior is an ordered pure conditional fold: start with zero,
visit positions from lowest to highest priority, and select that position's
index when its input bit is true. An X/Z condition merges equal result bits and
makes differing bits X. Valid is the OR reduction. Conflict follows popcount
greater than one, becoming X if any input bit is X/Z, including at W=1.
For example, high-order `x100` returns index `1x`, valid true and conflict X;
low-order `x100` returns index `10` and valid true. At W=1 the index is always
fixed zero, while valid and conflict preserve their distinct Boolean semantics.
Computed outputs contain no Z; latent values under computed X are unspecified.
Historical procedural encoder variants disagreed on unknown inputs; their
procedural skipping of X/Z does not define these helpers.

The operand is evaluated once. Unpack into exactly two or three distinct ordinary
names under existing binding and branch rules; all destinations are checked
before any result binding is published. Whole-result assignment, immediate
`.index`/`.valid`/`.conflict` projection, indexing, direct producer return or
observation, chained/aggregate targets and direct owner writes diagnose. Store
named results in an ordinary declared struct for later field access. Existing
explicit u1 field/storage conversion creates a fresh fixed wire from a Boolean;
it does not change the original flag or its aliases into fixed bits. Raw flags
therefore cannot be operands to popcount or another fixed-bit helper.

The helpers share Enum's fixed-result binding and existing bit operations. Their
IR graph is O(log² W), with compact scalar literals and no new IR or Runtime
primitive. Existing backend capacity and observation-emission limits still apply;
huge-width compile/link support is not a runtime allocation guarantee.

Unsigned fixed bits also support `value << count` and `value >> count` with a
proven static, nonnegative Integer count. Both results retain the input width.
Retained value/known/Z bits move unchanged and vacated positions become known
zero. A zero count is identity; a count at least the input width produces known
zero. Counts are compared at their full precision, including beyond 64 bits,
before lowering; they never wrap modulo the input width.

Destination conversion happens after the shift. For `a: ac.u8` equal to 128,
`wide: ac.u16 = a << 1` yields zero. Explicitly widening `a` to a u16 local
before shifting yields 256. Boolean inputs/counts, negative or runtime counts,
unproved input widths and missing unsigned source authority diagnose. A local
alias, singleton runtime interval or unresolved parameter default is not static
count proof. Mathematical Integer right shift keeps its separate exact-value
contract below; runtime mathematical left shift remains unsupported.

For an unsigned fixed `bits[W]` numerator, `value // divisor` and
`value % divisor` accept a proven static Integer divisor satisfying
`1 <= divisor < 2**W`. Each result retains W; an explicit destination conversion
occurs afterward. The compiler derives an exact widened reciprocal product and
extracts the quotient, then uses multiplication/subtraction for a requested
remainder. It reuses existing common bits operations without adding a division
primitive. Any X/Z bit in the numerator makes the entire arithmetic result X,
including division or remainder by one and powers of two.

A runtime divisor is also accepted when both operands already have authoritative
unsigned fixed-bit types of the same positive, resolved width. `//` and `%` are
combinational operations: they insert no state, handshake or extra sampling
cycle. Generated gfsim uses the existing CPU arithmetic for machine widths and
multiword arithmetic above them; Verilog uses unsigned division/remainder.
For fully known operands with a nonzero divisor, results are unsigned quotient
and remainder. A runtime zero divisor or any X/Z bit in either operand makes the
whole result X, following native four-state RTL arithmetic. The simulator checks
these conditions before using CPU division, so it never invokes host division
by zero. Consumer-specific total arithmetic policies belong in their wrappers:
for example, PR151's PTO quotient0/remainder=dividend convention is not the
semantics of the generic bits operator.

Proven static Integer zero, negative or too-wide divisors still diagnose.
Runtime Boolean/Integer divisors and differing fixed widths are not implicitly
converted. Register initializers, ordinary Integer aliases, singleton intervals
and unresolved parameter defaults do not establish a static divisor; an
explicitly fixed-bit register or value may instead use the runtime operation.
Runtime mathematical Integer division/remainder and wholly static source `//`/`%`
expressions remain unsupported; the common mathematical evaluator's separate
floor/modulo and zero-error semantics are unchanged. The PR's iterative
ready/valid divider is a separate module integration, not an alternative timing
interpretation of these ordinary expressions.

Persistent behavioral modules and modules containing queues receive generated physical `pyc_clk` and
`pyc_rst` inputs through the existing model signature. These names are reserved
and absent from Python authoring. The host runner drives them through the typed
DUT interface. One Step remains one sampling epoch, not a complete clock period.

A behavioral module may register multiple state-writing rules when their write
sets are proven disjoint: different persistent declarations, different nested
Struct fields, different direct-literal elements of one Table, or disjoint fields
of a one-dimensional Table element even when its indices are dynamic. All rules read the same old state; call
order does not introduce state forwarding or write priority. Ordinary local
assignments, if/else, struct defaults/construction/projection and proven table
indices retain their existing semantics. Overlapping writable parameter aliases
reject; read-only duplicate bindings are permitted. Unproved index ranges,
general loops, automatic arbitration between overlapping writers, cross-source
behavioral rules and cross-domain transactions remain unsupported.

The registered MLIR pass `ac-analyze-rule-writes` analyzes source capture
before lowering. It retains declaration/registration identity and assignment
paths, including identity writes. Lowering consumes that cached analysis and uses
existing `ac.value.merge` to combine disjoint proposals. For a one-dimensional
Table of Bits, Enum or recursively nested Struct values, existing `ac.table.map`
combines complete candidates against old Q. Parent fields expand into unique
scalar leaves; distinct static elements may therefore update a parent field and
a child field without presenting overlapping paths to the common verifier.
The storage owner is bound once with its original rule-owner enables and poison.

Capture-time index disjointness recognizes direct Integer literals. Lowering can
also prove different closed index constants from actual SSA, including folded
arithmetic and aliases; a spelling or range annotation alone is not a proof.
Thus `entries[0].a` and `entries[2].a` may compose, while writes to
`entries[i].a` and `entries[j].a` still reject when their separation is unproved.
Dynamic `entries[i].a` and `entries[j].b` can compose because their fields differ.
Whole-element/field, identity and alias conflicts remain checked. Table-of-Table
source indexing, indexed partial writes within Table-valued Struct fields and a
second index level remain unsupported. Overlapping writers
may use mutually exclusive whole-owner grants or the explicit address-separation
proof described below. Neither supplies implicit arbitration.

When source expressions supply mutually exclusive complete owner enables, rules
may write the same scalar, overlapping Struct paths or overlapping Table domains:

```python
grant_a = request_a
grant_b = request_b & ~grant_a
write_a(state, data_a, grant_a)
write_b(state, data_b, grant_b)
```

The compiler must prove that both final owner enables cannot be known one at
once. It traces real rule captures/yields and uses bounded necessary facts for
one-bit constants, NOT/AND/OR, four-state selects, and exact typed
constant comparisons. Unsupported producers remain opaque; unproved overlaps
reject. Identical names, default values and expression spelling do not establish
identity. The proof does not rewrite executable enables: `p & ~p` still produces
X for unknown input and retains the existing unknown-enable failure behavior.
A rule that conditionally writes one field but unconditionally writes another
has an enabled owner; its inner field condition is not a whole-owner grant.

For overlapping indexed paths, ordinary source expressions can permit both
writers when their addresses separate:

```python
grant_a = request_a
grant_b = request_b & (~grant_a | (index_a != index_b))
write_a(entries, index_a, data_a, grant_a)
write_b(entries, index_b, data_b, grant_b)
```

The equivalent conditional expression is also supported. The proof uses exact
assignment-time index SSA, not the index variable's final value. It checks every
overlapping path pair. Under both whole-owner enables being known one, the sets
of ordinals whose full-width equality is not known zero must be disjoint. Thus
partial X/Z indices with a known differing bit can separate; two masks merely
being unable to both equal known one is insufficient.

The compiler transports private equality-mask Tables through existing Rule and
TableMap results and merges only each leaf's retained write domain. Original
owner enables and poison remain outside those masks. Explicit guards determine
policy; the proof does not rewrite the executable expressions. General select
proof includes the case where an X/Z selector has two arms with the same known
result. Unknown enables still cause existing whole-system failure/discard.

Different dynamic widths needing new widening-equivalence reasoning,
arithmetic/range-only separation, implicit selection provenance and per-assignment
branch exclusion remain unproved unless whole-owner exclusion already suffices.
Whole-table replacement needs whole-owner exclusion; indexed whole-element
updates may use address proof. Analysis errors and exhausted budgets reject.

Capture analysis now distinguishes a valid plan from completed overlap proof.
`ac-analyze-rule-writes` reports pending overlapping pairs; its successful plan
construction alone is not source admission. Lowering proves every pending pair
before binding proposals for a module and checks the whole-source discharge
ledger before publication. Cached plans remain immutable. Analysis work and
proof depth/queries/facts/pairs are bounded; exhaustion rejects with a diagnostic.
The CompilerDev analysis exposes `isPlanValid()` and pending queries; the former
`isValid()` name has been removed.

Selected candidates use both their write domain and original owner enable, so a
disabled overlapping candidate cannot restore old Q over an active update.
Registration order supplies no priority. The source must apply its intended
grant to all transaction effects, including input acceptance and output valid;
this support does not infer a winner, cancel effects or supply a scheduler.

Module-scope statement calls can register rules without dummy result values.
Fallthrough, final bare return and `-> None` rules have no source outputs; a
`-> None` rule cannot return a non-None value. A typed return may be ignored without
dropping its updates/checks. Void rules reject in value contexts. Existing
unannotated value-return rules retain their contract; early or conditional
returns and nested rule registration are not added by this support.

Within one rule, assignment order and its effective owner enable are unchanged.
Across rules, a known-enabled writer must not mask an unknown-enabled sibling
writer to another field of the same owner: the combined storage enable remains
unknown and standard storage checking rejects the epoch. Unknown data under a
known enable remains legal. In particular, one rule's unknown conditional field
write followed by an unconditional sibling-field write retains its previous
known-owner-enable/possibly-unknown-data behavior. Unknown enables
that reach standard storage use its existing failure semantics; complete checks
for masked unknown controls and pure no-write paths remain unfinished.

## Table queries

Use pure callbacks to select from an existing positive, closed, one-dimensional Table:

```python
free_index, free_valid = entries.first(where=lambda entry: not entry.valid)
issue_index, issue_valid = entries.argmin(
    where=lambda entry: entry.valid and ready_tags[entry.tag],
    key=lambda entry: entry.age,
)
```

Both calls require exactly two distinct ordinary result names. `where` and `key`
are required keyword arguments; callbacks are one-argument expression lambdas,
without defaults or variadic arguments. Existing scalar expressions and pure
intrinsics retain their ordinary type rules. Captures are immutable SSA snapshots;
row-dependent reads of captured Tables retain normal index-admission checks.
Named-def callbacks, nested queries, allocation, module/rule calls, instrumentation
and mutation are unsupported, including inside a callback's dead branch. General
lambda values or tuple results are not introduced.

For extent N, index has fixed type `bits[max(1, ceil(log2(N)))]`, and valid has
fixed type `bits[1]`. No match returns index zero and valid zero. Index's known-value
range is `[0, N)`. Selection observes the call-time snapshot and does not reserve
or consume an entry. `first` chooses the lowest matching index; `argmin` chooses
the lowest unsigned key, with the lowest index breaking ties. Four-state
predicates and keys retain the existing conditional-merge semantics; a logical
range proof does not establish that a physical value is known.

## Queues

```python
ready, available, value = ac.queue[ac.u8](
    valid, data, take, depth=2, ready_policy="downstream_pop"
)
```

The signature is `ac.queue[T](valid, data, take, *, depth, latency=1, ready_policy="local_occupancy")`. The three inputs may be positional
or named. Configuration is static and keyword-only. Controls accept Boolean
or authoritative fixed one-bit values through the existing predicate rules;
mathematical Integer `0/1` does not become a predicate. Payloads use existing
fixed-bit, nominal and admitted one-dimensional table types.

The allocation must be one direct module-body assignment to three distinct,
fresh names. Result bindings cannot be reassigned or mutated through fields or
indices. A future queue result can supply a connection: each result keeps its
own ordinal, while input expressions bind once at the lexical call position.
Existing common analysis checks the completed graph, including unused inputs
and cycles. Dynamic, nested-expression or rule-local queue allocation rejects.

Default ready depends only on available capacity. Explicit
`ready_policy="downstream_pop"` permits a full queue to pop and replace its head
on one edge when its old head is available. There is no empty flow-through.
`latency=L` is a positive static integer fitting u64: a token captured on edge
E0 becomes available after edge E(L-1) commits and can first be consumed on EL.
Waiting tokens occupy the declared depth; latency never adds token slots. An
initialized queue with no available head returns known-zero valid/data even when
occupied. With latency one, depth one has half-rate throughput under the local
policy; larger latency also constrains throughput. See the
[queue contract](spec-queues.md) for reset, four-state, capacity and Work/Xfer
semantics. Queue owners reuse existing whole-system commit/discard and do not
introduce writes to unrelated source variable owners.

Clock/reset and transaction proposals remain compiler-owned. Queue presence
propagates a hidden physical domain through ordinary module calls, including
imported stateless parents. This does not add behavioral static parameters or
general source collection allocation.

## Branches and binding boundaries

Ordinary `assert predicate` with an optional static string message is captured
inside behavioral and registered structural rules. Its condition observes the
statement's SSA value, and its source path includes preceding assertion
continuation. If/match joins select continuations along with values and enables;
they do not combine gated paths with an OR that would introduce spurious X.
Predicates use the Boolean or authoritative fixed-one-bit boundary below.
Check IDs, ordered obligations and condition/path result pairs are verified in
the same source-unit and common hardware flow, including under Python `-O`.
This is source/IR support: both emitters still reject `ac.expect` until the
execution packet is complete. Compilation or linking does not establish runtime
assertion checking. See source-check progress (historical local record).

Statement `if` in behavioral rules and conditional expressions (`a if test else
b`) share predicate and value-join rules. A condition must be Boolean or an
authoritative fixed `bits[1]/u1` value. Integer values, including `1` and a
one-bit `Annotated[int, range(1 << 1)]` input, are not conditions. A one-bit
representation without source authority is insufficient. Using a fixed one-bit value as a predicate leaves its original fixed-width numeric meaning
available to later expressions. Comparisons produce Boolean values.

`and`, `or` and logical `not` use the same predicate checks. All-Boolean `and/or`
operands produce Boolean; mixing Boolean and fixed one-bit operands produces
fixed `u1` in either order, including longer operand lists. `not` preserves the
operand's Boolean or fixed-one-bit kind. These operations use the existing
four-state bit logic; they do not add Python's operand-returning or short-circuit
runtime-check behavior. Bitwise inversion retains its separate contract.

Each statement branch starts from the same incoming local values and owner
proposal enables. Assignments then execute sequentially within that branch.
At the join, a branch that omits an assignment inherits the incoming value and
enable, including proposals made before the `if`. `pass` changes neither.
For example, within a behavioral rule:

```python
state = state + 1
if update:
    state = state + 2
else:
    pass
```

The else path retains the preceding `state + 1` proposal and its write enable.
The join selects both values and enables. A known condition preserves the chosen
value's X/Z bits. An unknown condition preserves common known bits and common Z
bits, producing X elsewhere. An effective unknown enable retains standard
storage failure and whole-system discard semantics.

A new local is usable after a branch only when both paths bind it. A one-sided
local can be used inside its branch, and a later unconditional assignment can
bind it for subsequent use. An explicit annotation on a partial binding still
constrains that later assignment; it does not create an initial value. Returns
inside branches remain unsupported.

An ordinary inferred Integer local keeps its Integer kind but may grow or change
its representation: `n = 1; n = n + 1` and branches assigning `n = 0` or `n = 2`
retain exact values. Inferred Boolean and fixed-width locals retain their kind
and type. Persistent owners and explicitly annotated locals keep their original
logical type and range/width boundary through initialization, assignment and
joins. Boolean bindings require Boolean values; Integer `0/1` and fixed `u1` do
not become Boolean merely because they fit one bit.

An unannotated non-owner Integer rule formal keeps the call's original physical
representation, source interval and constant facts. Its representation is not an
unsigned range: capturing `0 - 1` and computing `n + 2` can produce `1`.
Reassigning that formal cannot enlarge its bound representation, although a new
expression or local may have a wider one. An explicitly annotated Integer formal
must match the actual's logical kind and type and satisfy its declared range.
A formal bound to persistent state retains the owner's original contract.

Reannotation cannot replace an owner, formal or earlier explicit local contract.
If a new local has an explicit annotation in one branch and an inferred binding
in the other, that declaration constrains both values before joining. Explicit
declarations in both branches must agree. Adding an annotation to an inferred
local constrains subsequent assignments without changing earlier SSA aliases.

Integer/Integer joins retain exact source kind and the interval hull, using a
complete common representation before selection, including negative and wide
values. Boolean/Boolean remains Boolean; Boolean/Integer rejects. Two fixed
branches require equal hardware widths. With exactly one fixed `bits[W]` branch,
the other can convert only when it is a proven closed source Integer in
`0 <= value < 2**W`, or a proven closed Boolean with `W == 1`. This rule is
symmetric in arm order and applies to both statement and expression joins.
The conversion creates a fixed-width SSA value; it does not relabel the original
Integer or Boolean producer. Aggregate joins retain existing nominal/shape rules.

Closed source proof comes from supported static Integer/Boolean values and pure
source operations whose required operands are all proven closed. Aliases and
rule captures preserve that proof. Thus a literal, `1 + 1`, or an alias to that
closed expression can meet a fixed branch peer. Runtime inputs, current state,
singleton runtime intervals, unbound parameters and expressions such as
`runtime * 0` do not supply the proof. A known initializer is not proof about a
later state read. This branch-join proof does not expand static admission for
shift counts, divisors, widths, parameters or declared defaults; ordinary aliases
in those positions retain their existing restrictions.

## Match statements

Behavioral rules support Python `match` statements using the same values, binding
boundaries and owner write enables as `if`. For example, inside a rule:

```python
value: ac.u4 = 7
match selector:
    case 0:
        value = 1
    case 1:
        pass
    case _:
        value = 9
```

For known selector 1, `value` remains 7. The subject is evaluated once. Every
arm starts from the same incoming environment, then follows normal sequential
statement semantics. First-match priority selects an entire arm, including all
owner write enables. A selected arm that omits a write retains any proposal made
before the match; it does not replace it with a fresh read or a new write.

Supported selectors are Boolean, authoritative fixed bits with a closed width,
nominal Enum, and the existing nonnegative bit-backed Integer comparison profile.
Boolean selectors use `True`/`False` patterns, fixed/Integer selectors use exact
Integer literals, and Enum selectors use canonical members such as `State.IDLE`.
Boolean and one-bit fixed values do not interchange pattern kinds. Integer keys
must fit without truncation; `-0` is zero, while a negative value cannot match an
unsigned selector. OR-patterns combine supported atoms. A catch-all `_` must be
the final arm and cannot appear inside an OR-pattern.

Duplicate alternatives inside one arm reject after canonical normalization, so
`0 | -0` is a duplicate. Repeated or fully shadowed keys in later arms produce
nonfatal diagnostics but remain in the four-state selection. X/Z follows existing
equality, OR and select semantics, including preservation of common known bits
and common Z. Neither known-code coverage nor Enum member coverage eliminates
physical fallthrough or proves that a selector is valid or known.

Without `_`, unmatched values retain the incoming environment. A new local still
needs a value on every retained path before use, even when cases cover every known
code. Existing locals can be initialized before the match; a later unconditional
assignment can also initialize an unused partial local. Joins retain existing
ordered binary typing rather than inferring a new type from all arms together.

Nested `if`/`match`, ordinary field/table updates and admitted two-name Enum
conversion bindings reuse their existing rules. Returns within arms, allocation,
module/rule calls and external effects remain unsupported. Capture/as, class,
sequence, mapping, starred and guarded patterns reject. Whole struct/table
selectors, masked patterns, unbound widths/type parameters and signed or
mathematical comparison selectors remain outside this profile; struct/table
payloads and updates are supported.

Native warnings and remarks are forwarded to the public compiler's stderr;
successful diagnostics do not change stdout or source-unit artifacts.

## Direct module composition

Same-source modules with typed struct returns can be called directly in module
scope: `first = Child(request)` followed by `second = Child(first)`. The returned
values connect through existing SSA and whole-struct ports. Each static call
occurrence owns a separate instance; reusing `first` only fans out its value.
Nested calls in one expression are sibling instances in the enclosing module;
calls in a callee's body establish child hierarchy. Source order does not add
cycles, registers or commit priority.

A direct call may read a later module result or its fields. The later name must
have exactly one unannotated simple assignment whose right-hand side is a direct
module call, with no other assignment, field/index mutation or shadow. This
restriction applies when looking up a name before its declaration; ordinary
sequential local rebinding and field updates remain available without forward
lookup. Inputs, state and already-bound locals take precedence over future names.

Call arguments bind where that call appears in source order. For example:

```python
x = first
a = A(b.field, x)
x = second
b = B(x)
```

Here A receives `first` as its second argument and B receives `second`. Reading
`b.field` early does not evaluate B's arguments early. Each nested or identical
call occurrence, including a zero-argument call, still creates its own instance.
Reusing one result creates no additional instance.

Opposing connections between module results are permitted when their individual
field dependencies are acyclic. Existing common hardware analysis checks the
completed graph, including unused connections and storage inputs. Storage Q
provides its existing temporal boundary; true combinational cycles diagnose.
General forward reads of ordinary local expressions remain unsupported.

The compiler resolves the call graph before finalizing signatures and propagates
hidden sampling/reset requirements through stateless parents. Recursive module
graphs, dynamic or rule-local instance creation, and annotated module-call result
bindings diagnose. Use an unannotated result local; its type comes from the
callee, avoiding ambiguity with persistent variable declarations.

Typed struct results use one existing physical output, normally named `result`;
the compiler resolves a collision with an input name deterministically. Python
does not name that physical output or repeat a dictionary of field names.
Direct typed-struct calls also accept independently published module declarations
through from-imports, aliases and facade reexports. Their interfaces explicitly
carry source argument kinds, result form and hidden-domain mapping. Consumer
compilation uses those validated interfaces; link compares the complete contract
against the real provider body. It does not read provider source or infer a
hidden domain from port names. Source formals remain required and no static/type
arguments are admitted for this call form.

Boolean and Integer formals retain their declared source-kind requirements;
equal physical widths do not make fixed bits logical values. Fixed-bit formals
use the existing unsigned destination conversion, including fitting Integer
values and Boolean-to-`u1`, while preserving the original producer's kind.
Same-source and imported calls use this same binding boundary.

An imported Struct may annotate a returned value. Imported construction and
omitted-default constructors remain unsupported; defaults still require
same-source declarations. Existing cross-source structural modules retain their
current flow when no ordinary source-call contract is declared.

## Structural modules, instances and rules

- `@module` declares a reusable definition, typed positional inputs and a named
  output-type mapping. The selected root may have explicit ports.
- A structural call such as `child = Child()` creates an instance. A call to
  that instance inside a registered `@rule` binds its complete input signature.
- Nested `@rule` functions are stateless Work computations. Defining a rule is
  inert until its explicit registration in the module body.
- Storage is explicit: `dff(T=...)`, `dffe(T=...)` and the standard memory leaves
  own state. Ordinary modules and wires add no register delay.
- Reads observe current Q throughout Work; Xfer commits prepared data and clock
  history. `nonlocal` state proposal syntax belongs to the retired source model.
- Outputs may use input/child values, literals and supported pure bitwise,
  comparison or selection expressions in the structural return mapping.
  Proven integer arithmetic is also supported as described below. Expressions
  reuse ordinary combinational SSA; they add no rule or register.
  Missing/repeated bindings and incompatible types diagnose.

The current source tests cover `bool`, inline
`Annotated[int, range(1 << WIDTH)]`, keyword-only integer defaults and type
arguments for standard leaves. Source location differences do not change
hardware type meaning. Bool literals become known width-appropriate bits;
width/range mismatches remain errors.

## Immutable local wires

A structural module body may name an existing pure expression with a single
assignment, such as `next_value = (x + 1) & 255`, then reuse it in an output mapping and registered
rule. The name refers to the same SSA value. It adds no register, assignment-time
sample, implicit narrowing or range check. The computation runs during Work
from current inputs and old Q; existing output/input boundaries perform their
usual type and range checks. Alias chains retain source kind, interval and
mathematical provenance, including when captured by a rule.

Local names are immutable in this structural slice. They cannot shadow inputs, static
formals, instances, rules, definitions, imports (including local import aliases)
or active intrinsic grammar names. Direct right-hand references must already
exist. Output mapping string keys do not declare variables, so a local and its
output key may share a name. Locals after registration but before Return are
available to deferred rule lowering; new pure locals after Return diagnose.

Only a Call resolved to an existing module or trusted leaf declares an instance;
a lexical shadow cannot redirect constructor lookup to an outer definition.
Rebinding, tuple/attribute/subscript targets, AnnAssign, rule-local assignments,
general forward references and arbitrary calls remain unsupported.

Naming does not add static-alias evaluation for counts, widths or parameters.
A captured singleton interval is not proof of a literal mask. Compute a complete
masked candidate before capture when its boundary needs that proof. Ordinary
same-width wire/bitwise uses retain their existing behavior. Both capture paths
preserve numeric facts, but source instrumentation emission remains unsupported.

## Exact integer expressions

Local finite Integer values support `+`, `-` and `*`. Lowering derives finite
intervals, widens operands before arithmetic and converts only at a completed
module-output or instance-input boundary proven to fit. Use an explicit known
nonnegative mask, for example `(a + b) & 255`, for modular arithmetic. An
unmasked sum that might overflow its declared output is rejected. Intermediates
may be wider than 64 bits; high X/Z bits remain significant until after the
arithmetic operation. A zero mask produces known zero.

Runtime `a >> k` accepts an Integer input with a proven nonnegative interval
and an actually known nonnegative static Integer count. It extracts from the
complete SSA representation: zero count preserves all bits, a count at or above
the actual width produces known zero, and other counts retain the upper bits
including their X/Z state. Discarded low X/Z do not contaminate retained bits.
Arithmetic still precedes extraction; `(a + 1) >> 8` with eight-bit a=255 yields
one from the nine-bit sum. A narrow or singleton mathematical interval does not
justify discarding high physical X/Z bits before the shift.

Runtime signed inputs, dynamic counts and unresolved formal counts diagnose.
A parameter default or singleton runtime interval is not proof of a static
count. Fully static expressions keep the existing exact evaluator, including
signed floor right shift where admitted: `((0 - 7) >> 1) & 255` yields 252.
Boolean input/count and negative counts remain errors.

Statement and expression choices use the shared
[branch and binding rules](#branches-and-binding-boundaries). All-Integer choices
combine branch intervals and retain complete representations before destination
conversion, including negative and mathematical intermediates.
Bitwise `&`, `|` and `^` on two Boolean values preserve Boolean meaning. Mixing
a Boolean and fixed `bits[1]/u1` produces a fixed `u1` result in either operand
order, preserving the common four-state semantics. That result can be used as
a predicate or in fixed-width arithmetic without changing either input's kind.
An explicitly unsigned destination may widen an authoritative unsigned fixed
value: for example, `wide: ac.u32 = small` in a rule zero-extends a `u4` value.
The same boundary applies to declared unsigned struct fields, scalar table
elements and behavioral module arguments/results. Low value/known/Z bits are
preserved and added bits are known zero. Narrowing is rejected; arithmetic
still requires equal-width operands, so convert before combining widths.
Equivalent widths preserve the value even when constant width expressions
differ. Boolean-to-wider-bits and fixed-to-wider-mathematical-Integer conversions
are not admitted by this rule. Rule formal/owner bindings and structural
module bindings retain their existing exact-type requirements.
For Integer `a`, `((a + 1) if flag else 1023) & 255` keeps both Integer
representations intact until after selection. Fixed-peer coercion must satisfy
the closed-source proof and fit checks above; it cannot silently truncate a
branch to the destination width. `True` and `False` remain Boolean values with
intrinsic one-bit representation.

Boolean and Integer remain distinct even at width one. Known mixed kinds and
Integer conditions diagnose. Local annotations and standard leaf type arguments
provide kind information. Validated ordinary source-call contracts also carry
Boolean, Integer and fixed-bit authority across source units. Declarations
without that contract do not establish new mathematical conversion facts;
their existing pure wire connections remain available.

Pure same-width bitwise expressions retain parameterized constants without
treating a default as the only possible binding. Arithmetic requires actual
range/static proofs. Comparisons containing mathematical intermediates,
unknown imported mathematical kinds or conditions, unproved bounds and required
dynamic range checks currently diagnose. Some numerically equal widths with
different static expression trees also require future type normalization; no
invalid equal-width resize is emitted. This is not arbitrary Python execution.

## Commands and files

```text
pycircuit compile -c <source.py> --source-root <root>
  [--package-prefix <prefix>] [-I <published-interface-unit>]...
  -o <unit-directory> [--replace]
pycircuit link <complete-unit-closure> --top <qualified-module>
  -o <root-source-basename.ac> [--replace]
pycircuit emit <final.ac> --target cpp|verilog
  -o <generated-directory> [--replace]
```

A source may contain several definitions. Parent compilation consumes published
interfaces, and link verifies their authority against the supplied bodies.
The compiler lowers the capture through its owning source MLIR pass and runs the registered
`ac-extract-source-interface` MLIR pass on a clone. The pass derives
module declarations and dependency summaries from the intact body SSA; it does
not read provider source or execute Python. Dialect-owned source structure checks
validate its input/output. Registry-backed body/interface checks independently
recompute summaries and verify supplied provider/builtin authority before
publication. A structurally valid interface alone is not trusted authority.
Definition and source ownership are independent of instance placement. Generated
files retain source basenames. Invalid inputs leave previously published outputs
intact.

## Execution boundary

Generated modules use `wire<T>` and Work/Xfer. C++ provides a typed `pyc_dut`;
a host testbench supplies input values and explicit clock levels through the
shared SystemRunner. One Step counts a sampling epoch. `sample()` observes the
successful Work output, not a second evaluation after Xfer. Independent subtrees
may run concurrently; whole-system checking precedes all state commits.

The first loop builds a native runner and executes the same IR as RTL against
independent values. It does not add a shared-library port C ABI. Host Reset is
not a clocked reset pulse or a promise to clear memory.

## Remaining source capabilities

IR support for [Table/collections](spec-collections.md), packed structs and
memory does not establish a Python authoring API for every operation. Beyond the
bounded behavioral slice above, complete `@system`, general collection authoring,
remaining source arithmetic,
additional queue flow/head-read policies, automatic clock-domain scheduling and CDC still need bounded implementation
and evidence. Closed systems execute reachable source assertions and publish
supported source observations after successful epochs. A registered nested rule
in a closed system may contain only supported unconditional `log` or `report`
calls, including when the system declares persistent state. No dummy assertion or
instance binding is required; empty rules and unsupported observation forms
still diagnose. Ordinary modules with
reachable assertions require the managed RTL lifecycle described in
[system execution](../architecture/system-execution.md); ordinary-module RTL
observations still diagnose. Unsupported effects must not be silently dropped.

Dynamic hardware creation, host I/O inside designs, arbitrary Python execution,
legacy CycleAwareSignal/JIT/builders, QueueGraph, `acc.py`, `acc` and `pycc` are
not alternate compilation paths. Historical examples remain migration references
until their original behavior runs through this route.
