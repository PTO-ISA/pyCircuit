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

## Rule authoring requirements and current admission

The [four-state/System supplement](../rfcs/four-state-system.md) defines explicit
X/Z literals, verification predicates, a shared root reset policy and generated
finite stimuli. The source and execution boundaries are described below.

The [Rule authoring contract](../development/agent-frontend-guide.md#rule-authoring-contract)
requires simple, cohesive, stateless Rules, Module-style typed inputs and
heterogeneous multi-output calls, Struct grouping by interface, and compiler-owned
resource/proposal guards. Independent functions belong in separate Rules;
effects that require atomic commit stay together. These requirements are tracked
in [issue #319](https://github.com/PTO-ISA/pyCircuit/issues/319).

Modules, Rules and Methods share admitted finite typed scalar, nominal payload
and fixed nonempty tuple signatures. A local Rule declaration automatically binds
one autonomous computation per Module/System instance. Its name denotes its
results in Module wiring; repeated references are fanout, not repeated activation.
Local formals bind through explicit default expressions, while captures remain
ordinary inputs. Source-global Rule templates use explicit actuals; template and
Method calls within a Rule stay in that outer transaction. Unannotated
executable Rules specialize from actual SSA types, ownership and closed facts.
The compiler carries output availability for guarded valued Rules; authors do
not add a valid field or a holding register to make their results usable.

Module and Rule connections use typed parameters and returned values. There is
no separate Signal resource or set/get interface in the language design. The
compiler owns value availability and dependency checks; persistent state and
Queue provide cross-cycle storage.

Scalar fixed-Bits Modules and their tuple results may declare keyword-only
static Integer parameters, for example
`def Identity(value: ac.bits[W], *, W: int = 8) -> ac.bits[W]`.
Direct same-source and imported calls separate static keywords from hardware
arguments and substitute each instance's actual widths. A caller may forward
its own declared parameter. Root overrides use the existing link command's
`--parameters` JSON file. Actual width, constant-fit and hardware-argument checks
still run at the bound instance; defaults are not proof for every instance.
This support does not add generic state initialization, typed-transaction static
parameters, type parameters, symbolic Integer domains or `range(W)` expansion.

The profiles below retain their shape, indexing, purity and control restrictions.
General queue/resource admission and complex SCF loop-control combinations remain
implementation and runtime acceptance work. Preserve explicit protocol checks
until equivalent inferred guards pass the original oracles; algorithmic
conditions remain part of authored behavior.

## Four-state values and verification

`ac.four_state("10xz")` constructs an exact-width `bits[4]` value, most significant
symbol first. Its argument must be a nonempty literal containing only ASCII
`0`, `1`, `x`, `X`, `z`, or `Z`. Uppercase is normalized. Prefixes, separators,
whitespace and runtime strings reject. Widening zero-extends; narrowing requires
an explicit slice. All-known patterns still have fixed-Bits source provenance,
not mathematical Integer/static-parameter authority. Struct and Table Bits fields
and closed state initializers preserve the same masks.

`ac.case_eq(a, b)` compares equal-width Bits including X/Z symbols exactly.
`ac.is_known(value)` tests whether all bits are 0/1. Both return a known Boolean
at runtime; neither supplies a compile-time fact. Ordinary `==` retains its
four-state behavior. These observer predicates are for assertions, logs and
known-Boolean reports. Diagnostic branches and helper/alias transport are
permitted; functional outputs, state writes, indices, Rule admission and hardware
call arguments cannot depend on them. The compiler checks data and control uses.
Availability is checked independently: an available X payload is legal, and an
unavailable result cannot be consumed even just to inspect its knownness.

These APIs describe deterministic simulation symbols. They do not establish
synthesis support for X detection or introduce multi-driver resolution.

## Behavioral variables and rules

Ordinary Module computations use the same assignment, `if`, `match` and admitted
literal-`for` value lowering as pure helpers. For example:

```python
@ac.module
def Adjust(value: ac.u8, enabled: ac.u1) -> ac.u8:
    result = value
    if enabled:
        result = result + 1
    return result
```

These unannotated locals are combinational values. Pure branches retain four-state
value merging; they do not implicitly allocate registers or require a known
selector. A name first defined inside a branch must be defined on every arm
before use. Module-top annotations retain their persistent declaration meaning;
updates to persistent owners belong in Rules. Hardware allocation, Rule
registration and annotated declarations remain outside runtime-controlled Module
branches. Protected ports, imports and instance bindings cannot be reassigned.
An existing child or Rule does not select a different ordinary statement language.
The current terminal-return and loop-admission restrictions still apply.

Top-level `NAME = literal` declares a source-owned immutable Integer or Boolean
constant. One unary `+` or `-` is permitted on an Integer literal. Definitions
are captured before dependent types/signatures; no design or host function is
executed. Direct, aliased, namespace and reexported imports consume the canonical
published declaration through explicit `-I` units. Constants retain Integer or
Boolean kind in ordinary expressions, widths and parameter defaults; Boolean
widths reject. Existing helper-local/formal shadowing and protected Module/Rule
binding rules apply. Reassignment, duplicate/protected bindings, annotated
globals, chained/destructuring targets, aliases, expression/call initializers and
other literal kinds reject. This is not general host Python evaluation. See the
[constant publication contract](../rfcs/literal-provider-constants.md) for source
authority and retirement before final hardware.

The behavioral slice uses `import pycircuit as ac`, unsigned `ac.bits[W]`
and `ac.u1` through `ac.u64`, and same-source `@ac.struct` declarations.
Struct fields are ordered, nominal and immutable values. Same-source constructors
may omit fields: explicit keywords override declared static defaults; omitted
fields without a default recursively become zero. An omitted nested struct uses
all-zero leaves; a declaration such as `inner: Inner = Inner()` explicitly opts
into Inner's declared defaults. Every declared default must be valid even if
unused or overridden. Defaults cannot depend on a caller's runtime bindings.
Unknown/duplicate fields and invalid types or ranges diagnose.

A Struct field annotated `bool` retains logical Boolean kind, independently of
its one-bit physical representation. Explicit construction, defaults and field
updates require Boolean values; Integer `0`/`1` and fixed `u1` actuals reject at
that boundary. Contextual aggregate zero and omitted same-source fields produce
logical `False`. Projection, nested records, helper calls and reconstruction of
unchanged fields retain this distinction, including independently published and
reexported declarations. Existing fixed-bit field conversions are unchanged.
The [Boolean field contract](../rfcs/boolean-record-fields.md) describes the
validated declaration constraint; it does not add implicit fixed-bit casts.

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

Module-scope annotated variables allocate persistent storage. Each admitted
scalar or structured state declaration lowers to a standard typed DFF/DFFE
storage child of its Module, using the same `ac.instance` input/output model as
other child modules. The frontend infers its storage connections; the author
does not supply a separate register instance. This classification depends on
the declaration, not on using a `uN` type name. Ordinary expression locals and
module-result bindings remain value/wire connections.

The [state-declaration authoring contract](../development/agent-frontend-guide.md#module-state-declarations-and-storage-children)
requires finite storage representations beyond Bits. The common transaction
path retains admitted Bits, Enum, Struct, Table, Boolean and bounded Integer
state, including Bits wider than a host integer. Existing shape, initialization
and nested-index restrictions remain; this does not admit every MLIR arithmetic
type or arbitrary Python value.

`entries = ac.table[N, Entry](init=0)` allocates a fixed one-dimensional table
of persistent entries. Contextual zero initializes each field recursively,
ignoring nonzero constructor defaults. `init=Entry()` instead constructs an
element using its defaults and broadcasts that value to independent owners.
Initializers must be compile-time values; inputs and existing state are not
initialization expressions in this slice.
An annotated Table may use a complete tuple/list image, such as
`entries: ac.table[2, ac.u8] = (1, 2)`. Its length and every element are checked
against the Table type. A scalar/Struct element initializer still broadcasts;
recursive zero keeps its existing behavior.

A same-source top-level `@ac.rule` takes these variables as ordinary parameters.
Assignments inside the rule follow sequential local value semantics; a read
after an assignment sees that rule's computed value. Rebinding a parameter
connected to a module-owned variable proposes an update to that owner. Rebinding
a pure input or local changes only the local value. The module can return the
rule's scalar, Struct or fixed-tuple result (legacy named result mappings remain admitted) directly. A local
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
Boolean and out-of-range bounds, nonunit steps and slice
assignment diagnose. Boolean and mathematical Integer values must not acquire
fixed-bit semantics through slicing. Table indexing accepts a proven nonnegative
half-open value interval within the declared depth, or the existing complete-width
proof. Closed Integer literals and fixed-bit remainder by a positive static
Integer carry bounded intervals; aliases and rule captures preserve those facts.
Admitted Integer-to-fixed conversions and fixed-bit zero extension preserve
nonnegative intervals without changing the original value's source kind. A slice
starting at bit zero preserves an interval only when every value in that interval
fits its result width; other slices use the result carrier's range. These private
facts do not turn a fixed-bit value into a logical Integer or Boolean constant.
Wrapping arithmetic does not inherit an earlier remainder bound. This is not
constant or known-bit authority: high X/Z index bits are retained, and runtime
divisors do not gain a remainder bound. Table slicing is unsupported. A narrower declaration still cannot
silently truncate: use an explicit slice at that boundary.

A conditional may refine a private value interval from one unsigned comparison
of the exact selected SSA value against a closed nonnegative constant, when
the other selected arm is also a closed nonnegative constant. For example, `x if x < 5 else 0` has known-value range `[0, 5)`, including when
`x` is an ordinary fixed-bit input. The supported comparison forms are
`<`, `<=`, `>`, `>=` and `==`, including reversed operands. Ordering
comparisons may refine either selected branch; equality refines only its true
branch. A missing input interval permits only a finite upper restriction. Unsupported guards,
nonclosed constants or an empty inferred branch leave existing facts unchanged.
This analysis does not change the comparison or selection hardware, declare
unknown bits known, or turn a singleton result range into a source constant.
It does not recursively analyze compound guards or imply an implicit narrowing
conversion.

`value[index]` reads one bit from an authoritative fixed-Bits value; index zero
is the least significant bit and the result is fixed `bits[1]`. Literal and
runtime selectors reuse the Table index rule: the complete nonnegative known
range must fit the actual resolved width. A wider selector is never truncated
before checking or shifting. A selector containing X/Z yields X, while a known
selector reading a Z bit preserves Z. Known out-of-range, negative and unproved
ranges reject. Boolean and bounded Integer selectors cross the existing fresh
unsigned boundary; this does not change their saved aliases' source kinds.
Boolean/Integer *data carriers* do not become indexable Bits implicitly.
Dynamic-width slices and bit/slice assignment are not admitted by this read API.

Closed power-of-two Integer port bounds may use equivalent static expressions,
for example `Annotated[int, range(8)]`, `range(4 + 4)` or `range(1 << 3)`.
The existing evaluator derives the exact width; symbolic `1 << WIDTH` retains
its original parameter binding. Bounds must yield a positive hardware width;
Boolean, non-power-of-two, zero, negative and singleton bounds remain rejected.

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

Unsigned fixed bits support `value << count` and `value >> count` with either a
proven static, nonnegative Integer count or an authoritative fixed Bits count.
Both results retain the input width.
Retained value/known/Z bits move unchanged and vacated positions become known
zero. A zero count is identity; a count at least the input width produces known
zero. Counts are compared at their full precision, including beyond 64 bits,
before lowering; they never wrap modulo the input width.

Destination conversion happens after the shift. For `a: ac.u8` equal to 128,
`wide: ac.u16 = a << 1` yields zero. Explicitly widening `a` to a u16 local
before shifting yields 256. A runtime Bits count retains all its bits, including
when wider than the data. An X/Z count produces a wholly unknown result.
Boolean inputs/counts, negative or runtime mathematical Integer counts,
unproved input widths and missing unsigned source authority diagnose. A local
alias, singleton runtime interval or unresolved parameter default is not static
count proof. The live induction binder of an expanded pure-helper loop is a
closed Integer for that iteration; aliases or rebound values do not acquire
this static-count authority. Mathematical Integer right shift keeps its separate exact-value
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
of a one-dimensional Table element even when its indices are dynamic. The
unified callable path currently rejects separate-Rule dynamic disjoint-address
writes to one Table; restoration of the address-grant behavior described below
remains unfinished. Independent Rules read committed state; ordinary call
order does not introduce state forwarding or write priority. Independent Rules
always read committed current state. Rule/Method `after` and native ordering
metadata are rejected through source, interface and saved-artifact validation.
Cycles and unproved conflicting writers reject. Ordinary local
assignments, if/else, struct defaults/construction/projection and proven table
indices retain their existing semantics. Overlapping writable parameter aliases
reject; read-only duplicate bindings are permitted. Unproved index ranges,
loop forms outside the bounded literal-range profile below, automatic arbitration between overlapping writers, cross-source
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

The address-grant contract permits overlapping indexed paths when ordinary
source expressions prove their addresses separate. This behavior is not yet
restored for separate Rules on the unified callable path:

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
unannotated value-return Rules specialize from actual SSA rather than a guessed
first-registration signature. An assigned or nested call infers its returned
scalar or tuple; a direct Module-return call retains the declared output context.
Early or branch-local returns remain rejected. A nested Rule call invokes the
callee in the current transaction rather than registering another transaction.

Within one rule, assignment order and its effective owner enable are unchanged.
Across rules, a known-enabled writer must not mask an unknown-enabled sibling
writer to another field of the same owner: the combined storage enable remains
unknown and standard storage checking rejects the epoch. Unknown data under a
known enable remains legal. In particular, one rule's unknown conditional field
write followed by an unconditional sibling-field write retains its previous
known-owner-enable/possibly-unknown-data behavior. Unknown enables
that reach standard storage use its existing failure semantics; complete checks
for masked unknown controls and pure no-write paths remain unfinished.

## Table transformations and reductions

Positive, closed one-dimensional Table values support pure mapping and the
existing fixed-bit reductions:

```python
incremented = values.map(lambda value: value + 1)
paired = values.map(lambda value, tag: Pair(value=value, tag=tag), tags)
total = values.fold(kind="add")
flags = values.map(lambda value: value != 0)
complete = flags.all()
present = flags.any()
count = flags.count()
```

Every additional map input has the same extent as its receiver. An expression
lambda has one required ordinary parameter per input, in receiver/input order.
Results are finite Bits, Boolean, Enum or nominal Struct values. Struct results
may contain admitted Table fields, including same-source constructors with
contextual tuple/list fields, static defaults and recursive zero initialization.
Imported nominal values retain their published identity; map does not broaden
imported constructor/default admission inside callbacks.

Map also accepts a named ordinary typed `def` through the pure value-helper
contract below. Each Table element type must exactly match its formal
annotation, and the result is fixed Bits. The helper is typed once with generic
row arguments and lowered through the existing map staging. Individual lane
values do not supply range or constant authority for typing its body. These
named callbacks may call other verified pure helpers within the callback operation limits.

Captures are immutable call-time Bits/Enum/Struct snapshots. A captured Struct
may contain Tables; explicitly indexed reads of captured Tables use the existing
index proofs and query staging. A bare Table cannot become an implicit scalar
capture or a callback result. Calls to helpers inside expression lambdas,
nested method/lambda callbacks, allocation, effects and general tuple/list
results remain unsupported, including in dead branches. Named callbacks have
no free-value captures. Capture validates ordinary lambda syntax; each method
checks its own parameter count during source lowering.

`fold` requires one literal `kind`: `add`, `mul`, `and`, `or`, `xor`, `min` or
`max`. Elements are unsigned fixed bits and the result keeps their width.
Reduction combines adjacent pairs in a balanced tree, carrying an odd final
element to the next level. Extent one is identity, including its value/known/Z
planes. Arithmetic wraps at the declared width and retains existing four-state
semantics. No callback or initial value is accepted by `fold`.

`all`, `any` and `count` require physical one-bit Bits/Boolean elements;
one-bit Enum carriers reject. The first two use
the existing `and`/`or` folds. Count widens elements before summing, returns
`bits[max(1, ceil(log2(N+1)))]`, and records the known-value interval `[0, N+1)`.
That interval does not establish known physical bits or permit indexing an
N-element Table with a possible count of N.

These methods lower to existing TableMap/Fold and scalar operations. The
source-wide budget includes replicated capture planes, Table-valued scalar
temporaries and fold-tree storage before publishing IR. Ordered prefix computations
can use the bounded source loops below; no Table `scan` method is introduced.
Named helper calls inside expression lambdas remain unsupported. A balanced
reduction does not implement a nonassociative prefix scan.

## Bounded source loops

Pure helpers and typed Rule/Method bodies have distinct admission boundaries.
Both compute within one Work evaluation; neither introduces a clock cycle.

### Pure helper loops

The pure expansion path, used by ordinary typed pure helpers, supports `for`
with a single local name and an unshadowed builtin `range` call whose sole
argument is an integer literal:

```python
accumulator: u8 = 0
for index in range(3):
    accumulator = accumulator + values[index]
    prefixes[index] = accumulator
```

MLIR expands the iterations into sequential SSA within the same Work evaluation;
the loop adds no clock, register or cycle. Python capture only records syntax.
Fixed-bit accumulators retain modular arithmetic. Integer values retain their
existing exact-value and source-kind rules.

This initial profile admits only `range(N)` with a non-Boolean captured Integer
Constant. Bound names, aliases, arithmetic expressions, negative spellings such
as `range(-1)`, keyword arguments, and start/stop/step forms are unsupported.
A local, formal, module binding or loop target that shadows `range` cannot gain
builtin authority. Nested loops, `while`, loop `else`, `break`, `continue` and
returns from loop bodies are unsupported.

The hidden induction ordinal is an Integer. Assignment to the visible binder
uses the existing binding boundary: a fresh name is an inferred Integer, while
an existing typed binding keeps its declared kind and width. Reassigning the
binder in the body does not change the next induction ordinal. Its final body
value remains visible after the loop. `range(0)` leaves incoming bindings intact
and does not bind names introduced only inside the body.

A zero-trip body still undergoes syntax, callee, effect and writer validation,
but its expressions are not evaluated or lowered. It emits no body operations,
checks or iteration origins. Forbidden effects cannot hide in a zero-trip loop.

The loop-containing invocation must have no persistent-owner writes; read-only
state snapshots and immutable local Struct/Table updates are permitted. This is
a current coverage boundary, not a permanent restriction on hardware language
loops. The body supports ordinary assignments, annotations, `pass`, existing
`if`/conditional expressions, projections, proven Table indexing, fixed slices,
`+`, `-`, `*`, `&`, `|`, `^`, fixed shifts, unsigned division/remainder,
supported unary/comparison/Boolean expressions,
nominal Struct construction and contextual Table literals. Calls are limited to
`concat`, `enum_to_bits` and verified pure value helpers. This does not broaden
the loop grammar or named Table callback restrictions; helper expansion is
charged to the existing loop and source-wide budgets.
Unary `not` and fixed-bit `~` retain their existing rules. Unary `+` and `-`
are not implemented in this profile, including zero-trip bodies; binary
subtraction such as `0 - 1` can produce a negative Integer.
Arithmetic reuses the ordinary expression type and range rules, including
full-width shift counts and positive fitting static divisors. Table methods,
other intrinsics, allocations, rule
registration, assertions and observations inside loops remain unsupported.

### Typed Rule and Method loops

Typed Rule/Method bodies use the SCF/transaction path and additionally admit
supported state writes, assertions, observations, `break`, `continue` and loop
`else`. Passing a Rule-loop test does not establish support in a pure helper.
`range(N)` takes one pure bound expression whose existing SSA facts prove a
nonnegative, closed mathematical Integer representable as signed 64-bit. Literal
defaults, explicitly bound constant arguments, aliases and closed arithmetic
retain those facts. A runtime value or interval alone, fixed Bits, Boolean and
negative bounds do not qualify. Pure-helper loops retain their literal-only
profile. Existing finite while comparison/progress proofs remain required.
General nesting, dynamic range bounds and `while True` with a
conditional break are not enabled by loop-else support.

A loop's `else` suite executes exactly once on natural completion, including
zero iterations. A reached `break` targeting that loop suppresses its else;
`continue`, including in the last iteration, does not. For example, this tested
scalar Rule adds fourteen with `u8` modular arithmetic:

```python
@ac.rule
def finish(value: ac.u8) -> ac.u8:
    for index in range(3):
        value = value + 1
    else:
        value = value + 11
    return value
```

For statically zero-trip typed `for` loops, else uses the incoming environment before any body
construction. Dead body references are not resolved, an existing induction-name
binding is preserved, and a name introduced only in the body remains unbound.
Continue-only completion is unconditional: an else-only local can remain visible
afterward. When break makes else conditional, ordinary definite-assignment rules
apply; define a later-used local on every retained path. Returns inside such a
conditional else remain unsupported branch-local returns.

Loop iterations and else retain sequential candidate visibility and saved local
snapshots in the surrounding transaction. Neither loop completion nor else
creates a commit boundary. Reached unknown control, resource rejection and failed
checks preserve the existing failure/discard behavior; they cannot publish else
state changes or observations. An otherwise admitted finite while loop uses the
same natural-completion/break distinction; a continue path that defeats its
progress proof remains rejected.

Expansion uses the existing source-wide resource limits. The compiler checks
trip counts, work, operations, storage and constant sizes before expanding paid
work; an unbounded or over-budget request fails before publishing a source unit.
Source occurrences created during expansion carry iteration frames while
retaining their source locations. Existing declared-type origins remain attached
to their declarations. A failed iteration leaves previous published artifacts
unchanged. Ordinary source assertions outside the loop retain whole-system
failure and discard semantics.

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
without defaults or variadic arguments. The same tuple binding is admitted at
module scope over an existing closed Table; both results become ordinary module
values that later Rules may capture. Module-scope queries cannot target a
persistent owner, input, Method or Rule registration. Existing scalar expressions and pure
intrinsics retain their ordinary type rules. Captures are immutable SSA snapshots;
an owner-bound state name is read once at the query statement, after earlier
proposals in that Rule. A saved local retains its earlier value. Lambda parameters
shadow outer bindings. These reads occur outside the pure per-row template;
callbacks do not acquire state or introduce another transaction.
Row-dependent reads of captured Tables retain normal index-admission checks.
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

Prefer a named Queue with typed calls inside Rules:

```python
tokens = ac.queue[Packet](depth=2, latency=1)

@ac.rule
def put(packet: Packet) -> None:
    tokens.push(packet)

@ac.rule
def get() -> Packet:
    return tokens.pop()
```

The named form defaults to `ready_policy="downstream_pop"`. `push(value)`
proposes one enqueue; `pop()` returns the committed head and proposes one dequeue;
`peek()` returns the committed head without consuming it. The compiler derives
resource admission and result availability. A blocked required access prevents
the entire outer Rule from committing, including writes in other child Modules.
Calls add no independent commit frame. Queue construction remains module-scoped;
aliases identify the same allocation, while separate constructions own separate
state. Declared logical payload types are preserved across calls.

Explicit protocol boundaries may use the three-output raw form:

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

The raw allocation must be one direct module-body assignment to three distinct,
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

Both source forms produce ordinary `ac.instance` nodes referring to canonical
native FIFO Modules. Typed accesses use `ac.method.call`; the existing transaction
lowerer supplies their pins and verifies the final instance wiring. The physical
FIFO also has a compiler-only committed-capacity output, independent of current
requests. It is not a fourth Python output. C++ and Verilog retain the existing
FIFO kernels; retired `ac.queue` and `ac.queue.push/pop/peek` IR operations reject.
Python `ac.queue[T]` remains the public constructor syntax.

The named form may bind raw `in_valid`/`in_data` together or raw `out_ready` at
construction. A raw-bound direction cannot also be driven transactionally, even
when the authored raw request is constant zero. Read-only `peek` does not claim
the dequeue port.

## Memory operations

Allocate a named Memory in its Module and call it from Rules:

```python
memory = ac.sync_mem[ac.u16](depth=16)

@ac.rule
def request_read(address: ac.u4) -> None:
    memory.request(address)

@ac.rule
def read_response() -> ac.u16:
    return memory.result()

@ac.rule
def store(address: ac.u4, value: ac.u16) -> None:
    memory.write(address, value)
```

`sync_mem[T]`, `sync_mem_dp[T]` and `byte_mem[T]` reuse the existing native
Memory Modules. `T` is fixed Bits or an admitted Struct/Enum; byte memory requires
a whole-byte payload. Depth is a positive closed static Integer. Allocation
handles are immutable; aliases retain the same physical memory. Clock, reset,
read/write enables, response validity and commit wiring are compiler-owned.

Synchronous `request(address, port=0)` proposes a read. `result(port=0)` returns
the committed response snapshot without consuming or refreshing it. The port
is a closed static Integer: zero for `sync_mem`, zero or one for `sync_mem_dp`.
A request becomes visible after a successful rising-edge Xfer. Concurrent reads
and writes to one address return the old data. The first idle rising edge retains
the response; the second invalidates it at Xfer. Work on that second edge still
observes the previous snapshot. Low, held, falling and discarded epochs do not
age responses. Reset invalidates responses and retains RAM.

A reached unavailable `result()` inhibits its entire outer Rule, including
previous requests, writes and child calls. An untaken access does not inhibit
it. Request and result in one Rule observe the previous response; they cannot
bootstrap an initially empty response. Use separate producer/consumer Rules or
an explicit controller phase. Save a returned value in Module state when the
algorithm must retain it beyond the response lifetime. Availability comes from
the primitive's live state, independently of payload knownness.

`byte_mem.read(address)` is an asynchronous read of committed storage. It may
feed later computation and a write in the same Rule. It has no request/result
pair. The compiler derives its reached read demand, drives a safe idle address
when inactive, and preserves reached selector diagnostics. A genuine
read-data-to-read-address combinational cycle remains invalid.

`write(address, value, mask=...)` uses the single write port shared by each
Memory's read ports. The default mask enables all byte lanes; low packed byte
lanes come first, including the partial last byte of a synchronous payload.
Enabled write data and masks retain the existing knownness requirements.
Known out-of-range synchronous reads return zero and writes do nothing; source
address widths still match the depth-derived signature. Memory addresses do
not acquire Table indexing's source in-range requirement.

Multiple potentially simultaneous requests or writes to one physical port
require an exclusivity proof or reject at link. Statement order does not create
ports; retired `after` metadata cannot grant priority or another port. Repeated byte reads may share a proven-equal full-width
address; a dominating read supplies a later equivalent access without adding
a feedback dependency. Multiple snapshot readers do not claim a request port.
The compiler checks these properties through actual owners and aliases.

The raw pin-oriented Memory calls remain available to primitive conformance
tests with their original payload arity. Ordinary examples use named operations.
Both targets consume the same verified final IR and existing storage kernels.

## Branches and binding boundaries

Ordinary `assert predicate` with an optional static string message is captured
inside behavioral and registered structural rules. Its condition observes the
statement's SSA value, and its source path includes preceding assertion
continuation. If/match joins select continuations along with values and enables;
they do not combine gated paths with an OR that would introduce spurious X.
Predicates use the Boolean or authoritative fixed-one-bit boundary below.
Check IDs, ordered obligations and condition/path result pairs are verified in
the same source-unit and common hardware flow, including under Python `-O`.
Both emitters consume verified source checks. C++ uses the shared whole-system
check lifecycle; RTL assertions require the managed lifecycle described in
[system execution](../architecture/system-execution.md). Compilation or linking
alone does not establish runtime assertion checking.

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
conversion bindings reuse their existing rules. The transaction profile below
adds statement-arm Rule/Method calls and body effects; that does not extend this
pure match profile. Returns within arms and allocation remain unsupported.
Capture/as, class, sequence, mapping, starred and guarded patterns reject.
Whole struct/table
selectors, masked patterns, unbound widths/type parameters and signed or
mathematical comparison selectors remain outside this profile; struct/table
payloads and updates are supported.

Native warnings and remarks are forwarded to the public compiler's stderr;
successful diagnostics do not change stdout or source-unit artifacts.

## Direct module composition

Modules with one admitted scalar, Enum, Struct or fixed Table result, or a fixed,
nonempty tuple return annotation, can be called directly in module scope. Use `-> tuple[T0, T1, ...]` and an explicit
`return value0, value1, ...` for independent outputs, then bind them with
`first, second, valid = Child(request)`. Each element uses the existing port type
and source-kind rules and becomes a separate SSA result of one `ac.instance`.
A Struct element remains one whole-Struct result. No tuple hardware operation,
packing wrapper, state or additional cycle is introduced. A single result uses `first = Child(request)` and can connect directly to
`second = Child(first)`. For example, `-> ac.u8` returns one Bits value;
`-> tuple[ac.u8]` remains a singleton tuple and requires unpacking. Scalar
results retain the same Boolean/Integer distinction, declared bounds and fixed
bit widening rules as input and tuple-result boundaries.
Each static call occurrence owns a separate instance; reusing its results only
fans out existing values.

For authoring, prefer one named Struct when several fields are normally consumed
together by one downstream module. Pass it as one typed result/input instead of
flattening its fields into parallel ports. Use tuple results for independent
interfaces or separately routed outputs; a tuple element may itself carry a
Struct. This keeps field evolution within the shared payload definition and its
users without changing the connection's argument/result count. It does not change
state, timing or scheduling semantics.

Tuple arity must be fixed and match both return and unpacking; `tuple[T]` uses
`return (value,)` and `value, = Child(...)`. Unpacking requires one flat Tuple of
distinct ordinary Names. Starred, list, nested, field and index targets, scalar
use of a tuple-return call as an ordinary payload, nested/variadic tuple
annotations, and writes into persistent or protected bindings reject. Ordinary
local rebinding is preserved.
Modules, Rules and Methods use the same fixed output-signature convention;
there is no general tuple-valued hardware expression. See the
[authoring guide](../development/agent-frontend-guide.md#module-hierarchy-and-independent-outputs)
for a complete example.
Nested calls in one expression are sibling instances in the enclosing module;
calls in a callee's body establish child hierarchy. Source order does not add
cycles, registers or commit priority.

A named Module result retains its immutable instance identity as well as its
output value. `child = Child(request)` can supply the single result and serve as
the receiver of `child.update(...)` in a Rule. A multiple-result binding holds an
output bundle that can later be unpacked with `first, second = child`; a
zero-result Module still supplies an instance handle. Immutable aliases retain
the same allocation and candidate state. Unpacked outputs are ordinary values
and do not acquire the instance handle.

A direct call may read a later module result or its fields. The later name must
have exactly one unannotated assignment whose right-hand side is a direct module
call: a simple name for a single result, or one of a tuple call's unpacked names.
It must have no other assignment, field/index mutation or shadow. This
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

A single typed Struct result uses one physical output, normally named `result`.
Tuple results use ordered physical outputs named `result_0`, `result_1`, and so
on. The compiler resolves name collisions deterministically; caller names do
not rename these ports. Source interfaces retain `ac.return_form = "tuple"` and
one ordered result constraint per source output; every constraint is verified
against the corresponding physical signature entry.
Both direct call forms also accept independently published module declarations
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

An imported Struct may annotate a returned value. Imported construction requires
every published field explicitly; omitted-default constructors still require
same-source declarations. Existing cross-source structural modules retain their
current flow when no ordinary source-call contract is declared.

## Structural modules, instances and rules

This retained structural profile supports existing named output mappings.
For new and migrated examples, use the scalar, Struct and tuple Module results
and direct calls described above. Explicit canonical `dff(T=...)` and
`dffe(T=...)` binding is also supported inside those typed Modules; preserve
physical pins when the design specifically exercises independent clock domains
or primitive reset/enable behavior. Ordinary design state uses annotated
Module-owned variables.

- `@module` declares a reusable definition, typed positional inputs and a named
  output-type mapping. The selected root may have explicit ports.
- A structural call such as `child = Child()` creates an instance. A call to
  that instance at Module scope binds its complete input signature.
- Directly nested `@rule` functions declare autonomous Work computations.
  Each declaration registers once per Module instance; its name denotes its
  computed results at Module wiring sites. Explicit activation calls reject.
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

## Pure value helpers

Use an ordinary typed `def` for reusable combinational computation. A helper
may be top-level or declared directly inside a Module/System. `@rule` identifies
an autonomous registration and is not a helper decorator.

```python
from pycircuit import u8

def bump(value: u8) -> u8:
    result: u8 = value + 1
    return result
```

Helpers require typed ordinary positional parameters and an explicit zero/one
result contract (`-> None` for no result). They reuse the existing hardware
binding boundaries and logical types; physical width alone does not permit an
Integer/Boolean/fixed-Bits conversion. Actual arguments are evaluated and bound
once, in source order. Defaults, keywords, variadic and positional-only
parameters are not admitted.

Bodies use existing typed expressions, local assignments and annotations,
immutable Struct field and Table element updates, `if/else`, bounded literal-range
loops, `pass` and one final return. Branches
retain definite-assignment and four-state selection rules. Existing arithmetic,
projection, constructor and Table restrictions still apply. Acyclic helper calls
are allowed. Loops follow the [bounded source loop contract](#bounded-source-loops):
iterations carry local SSA values within the same Work evaluation, and zero-trip
bodies still undergo capability validation. Updating a Table parameter or ordinary
captured aggregate constructs a new local value; it does not change the caller's
saved value or create persistent state. Directly nested loops, `while`, early
returns, nested declarations inside helpers,
allocation, Rule/Method calls, assertions and I/O are rejected even in dead
paths. A helper cannot read a persistent owner implicitly; pass its ordinary
payload explicitly.

A nested helper captures ordinary SSA values at its declaration, including
ordinary module-result wires. Later rebinding of the same name does not change
the capture. Calls between nested helpers use each callee's declaration snapshot;
local parameter/variable bindings retain normal lexical precedence. Helpers do
not capture owner or instance handles, clocks or reset. Top-level helpers have
no runtime captures; nested helpers remain private to their lexical module.

Source units publish typed helper bodies through the existing interface owner.
Consumers resolve that published closure without reading provider Python; link
checks the owning body against the interface. Native validation checks purity,
logical types, lexical ownership, recursion and bounded expansion before
inlining or elimination. Source helper functions/calls do not survive in final
hardware IR, and neither backend interprets Python helpers.

Ordinary expressions, registered Rule/Method bodies, named Table callbacks and
the existing bounded-loop caller share this helper implementation. Named Table
callbacks retain their fixed-Bits and zero-capture restrictions; calls inside
Table expression lambdas remain unsupported. Helper purity is separate from the
transaction execution and resource boundaries described below.

Defining a Rule does not execute it. Each Module/System-scope Rule call creates
a distinct registration, including repeated calls to one declaration. A call
inside a Rule or Method invokes the callable in the existing outer transaction;
a Method never registers autonomous work. General calls in Table callbacks,
selectors, guards and pure helpers remain rejected. A pure autonomous Rule
remains a Rule; its return type or absence of writes does not silently convert
it into a helper. Use a typed `def` for a pure expression helper.

## Immutable local wires

A structural module body may name an existing pure expression with a single
assignment, such as `next_value = (x + 1) & 255`, then reuse it in an output mapping and registered
rule. The name refers to the same SSA value. It adds no register, assignment-time
sample, implicit narrowing or range check. The computation runs during Work
from current inputs and old Q; existing output/input boundaries perform their
usual type and range checks. Alias chains retain source kind, interval and
mathematical provenance, including when captured by a rule.

Local names are immutable in this structural slice. They cannot shadow inputs,
static formals, instances, definitions, imports or active intrinsic grammar names.
A local Rule name is reserved for its autonomous declaration and computed results;
ordinary data cannot reuse that name before or after the declaration. Use distinct
names for saved values and callbacks. Output mapping keys do not declare variables,
so an ordinary local and its output key may share a name.

Ordinary wires captured by a Rule must be available in its lexical binding
snapshot. Rule results are connected at Module wiring sites, not captured by a
second local Rule. Previously saved ordinary aliases retain their SSA values.
General forward reads of ordinary locals and bindings after the Module's return
remain rejected; explicit child connections retain their documented forward-wiring
support. Fixed-result bindings retain their shape and freshness restrictions.

Only a Call resolved to an existing module or trusted leaf declares an instance;
a lexical shadow cannot redirect constructor lookup to an outer definition.
In this structural instance-declaration form, rebinding, tuple/attribute/subscript
targets, AnnAssign, rule-local instance allocation, general forward references
and arbitrary calls remain unsupported. The direct typed-result composition and
transaction profiles have their separate binding rules above and below.

Naming does not add static-alias evaluation for counts, widths or parameters.
A captured singleton interval is not proof of a literal mask. Compute a complete
masked candidate before capture when its boundary needs that proof. Ordinary
same-width wire/bitwise uses retain their existing behavior. Both capture paths
preserve numeric facts. Assertions and observations follow the supported System
and managed-RTL lifecycle described in the execution sections below.

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

## Method execution and remaining capabilities

The common callable route supports statement-arm Rule/Method calls, assertions,
observations, finite state and nominal local joins. Focused generated C++
workers 1/2 and four-state RTL tests exercise this bounded route. This evidence
does not establish general queue/resource admission, arbitrary SCF control or a
complete example/nightly gate. See the
[conditional-effect acceptance checklist](../rfcs/serial-rule-method/conditional-effects.md#acceptance-checklist)
for the broader obligations.

Native sampled-admission lowering and both backend lifecycles support same-domain
transactions over admitted finite Bits, Enum, Struct, Table, Boolean and bounded
Integer state, with unordered writers admitted only when their actual write
footprints are disjoint or their grants are proven mutually exclusive. Calls
within one outer transaction retain statement-order candidate visibility. Transaction bodies may be
straight-line or use bounded
statement `if`/`elif`/`else`/`pass` branches with finite nesting. The lowerer
emits sampled-check metadata and
complete transaction-wiring records. It authenticates the detached candidate
against the original source plan, then invokes the canonical final hardware
verifier before atomically replacing the input. Final-IR execution has focused
C++ workers 1/2 and four-state RTL evidence. Public straight-line dynamic source
tests cover independent transactions, cross-owner atomic calls and guard-entry
candidate snapshots through the same final IR. They check committed state and
clock history, including failure, Discard, Reset and frozen Work inputs.
Focused generated Method `@system` execution covers direct body assertions,
logs and reports, repeated calls, statement-time candidate snapshots and failure
before observation publication. C++ workers 1/2, generated RTL and real four-state
assertion probes exercise this bounded route. Verification belongs to the exact
tested candidate; prior checkpoints do not certify later changes.

The compiler independently publishes a bounded subset of `@method` and nested
transaction syntax as verified source units. Consumers use published interfaces
without provider Python or method bodies. Shared IR analysis resolves receiver
identities, checks the complete call/instance closure and records conservative
totality together with compiler-managed output availability. Logical Boolean,
Integer and fixed-bit contracts
remain distinct across interfaces; ordinary typed-result modules retain their
value-call behavior.

This source-compilation subset retains finite state, sequential local
assignments/calls, typed scalar/Struct/fixed-tuple results, lambda guards and
ordinary typed Module inputs and results alongside Method interfaces.
Same-source global Rule templates retain explicit actual bindings; importing a
global writable Rule template as a published callable is not admitted. A branch
may update admitted owners and join changed scalar locals, nominal Enums,
finite acyclic nominal Structs and admitted fixed Table payloads.
Changed nominal values on both arms must have the same nominal type; equal
packed layouts do not make different declarations interchangeable. Existing
locals inherit the branch-entry value on an unassigned arm; new locals used
after the join must be defined on both arms. An omitted owner assignment inherits
the candidate and write intent that entered the branch. Unconditional Method
calls may appear before or after a branch. Arms may also call Rules/Methods and
contain ordinary `assert`, `log` and `report`, including
nested `if`/`elif`/`else`. A conditional observation alone selects this route;
authors need no dummy Method, guard or state. Early or branch-local returns,
general transaction resources and complex loop-control combinations remain
outside this execution claim. Existing Table shape, initialization and nested
indexing restrictions still apply to partial updates and joins. Dynamic
disjoint-address writes from separate Rules to one Table currently reject on
the unified path; restoring that existing address-grant behavior is unfinished.
Calls and effects in selectors, guards, pure helpers, collection callbacks and
conditional expressions remain rejected. Static dead arms still undergo type,
closure, recursion, domain and potential-writer checks.

Ordinary single-target field assignments such as `local.inner.payload = value`
reconstruct an immutable local Struct, including replacement of a nested Struct
subtree. The root must be a currently bound ordinary payload; a captured or
returned payload remains a snapshot, and saved aliases keep their earlier value.
The declared field type checks the replacement while the root keeps its original
nominal binding. This does not write persistent state or mutate the producer.
Admitted indexed state and Table updates preserve their current root candidate
and untouched fields. This does not add arbitrary nested indexing, slice writes,
annotated field targets or new augmented-assignment forms. Scalar binding
conversions and four-state joins retain their existing rules.

Nominal joins lower through the same verified common IR: Struct fields are
projected, leaf values selected and the Struct reconstructed in declaration order.
Enum leaves, including standalone Enum locals, select their declared-width raw
carriers and reconstruct the nominal Enum. The generated membership result is
unused; invalid or X/Z carriers do not acquire a new membership fault. The
source plan authenticates this complete graph before final verification.

New nominal traversal, metadata, structural-container, materialization and proof
costs are prepaid against existing budgets. The inherited arbitrary-precision
arithmetic evaluator capacity policy remains OPEN; this slice does not establish
a fully bounded compiler or close `METHOD-ANALYSIS-BOUNDS`.

A guarded nested Rule uses the same callable path without requiring a dummy
Method or state. Registration arguments bind the callable's formal contract;
typed Rules publish their signatures, while executable unannotated Rules use
actual-SSA specialization. Source-global Rules do not implicitly capture state
or locals from an unrelated calling Module. Capture alone does not establish
execution support beyond the bounded generated tests described here.

Native final link lowers admitted finite state, repeated or nested Rule/Method
calls through static ordinary instances in the same physical clock/reset domain,
bounded branches and straight-line or statement-arm effects. Unordered writers
of one actual owner are admitted only through independent write-footprint
disjointness or grant-exclusivity proofs; an ancestor partial lane is composed
with its captured Q before crossing a hierarchy port.
Calls share their outer transaction's candidate versions; saved ordinary values
remain snapshots. Independent Rules read old Q regardless of declaration order. Separate structural instances
retain independent state. Scalar storage leaves use a plain DFF only for a
single local Rule candidate whose materialized commit enable folds to constant
one. Composed ordered/unordered or incoming candidates retain DFFE until their
pin-free enable proof is supported; Table collections retain element DFFE leaves. Both preserve clock/reset, hold and discard;
the same verified final artifact feeds C++ and Verilog.

Every lowered transaction has explicit clock/reset captures, ordered admission
controls, value-validity prefixes and a sampling-history binding. A control is a
callable guard or an effect-branch selection. Literal true/false,
state-free and read-only transactions follow the same route. Ordinary results
escaping an outer Rule carry compiler-managed availability. Conservative totality
still requires every reachable effect selector to be known and every reachable
guard to be true; statically unreachable paths do not prevent that proof. Known
selectors of either polarity pass selection. A may-block Rule can return a
value: MLIR retains its availability through downstream use and emission.
Authors do not supply a valid field, implicit zero or holding register. A valid
payload may itself contain X/Z; availability does not turn unknown data into an
admission failure.
Both branch arms start from the same incoming candidate data and raw write
intent. The join uses the original four-state selector for both components.
Missing writes inherit the branch-entry pair rather than returning to old Q;
identity writes retain intent. The physical storage enable is the four-state
AND of final transaction fire and joined raw intent. It is never converted by a
known-one or case-equality test, so unknown single-sided intent reaches the
existing active-edge failure and whole-system discard contract. Direct body
effects use final transaction fire and their selected source path, not an
individual state's enable. Pure branches keep their existing four-state joins.
Any arm containing a Method call or body effect adds a known-selector obligation
to that branch and its enclosing branches, including empty, read-only and
unguarded Method calls. An unselected arm bypasses its guards, including false
or X/Z guards. A selected false guard holds the entire outer transaction across
all owners and suppresses its body effects; it does not choose the other arm.
Saved results are available only after the call completes on the defining path,
even when the Method returns a literal or captured value. Only a verified join
can make arm-local results available after the branch; initializing one arm
does not supply a value on the other.
Each reached guard checks knownness at an active rising edge outside reset.
A known false guard holds that transaction; an earlier reached X/Z guard
causes whole-system zero commit even if a later guard is false. A false guard
prevents checks for later, unreached guards. C++ and managed RTL freeze checks
and physical pins during Work/PREPARE and commit history only after successful
whole-system checks. Successful false-fire, held and falling samples maintain
history; failure or Discard drops pending state and history.
Reached effect selectors likewise fault on X/Z at a rising edge outside reset,
causing whole-system zero commit even if a later guard is false. At low, held,
falling or reset samples they add no sampled fault; their unknown value still
makes final fire false and suppresses all effects in that transaction. With a
known selector, selected assertions retain their ordinary every-Work reset mask,
and selected observations may publish on successful held or falling epochs.

Cross-instance calls preserve original Module definitions and storage ownership.
Ordinary internal ports carry old Q and lexical SSA to the caller and candidate
D/E back to the owner; authors do not supply these connections. Original public
root ports remain unchanged, including when the selected root definition is
also used as a child elsewhere in the package. An occurrence without an
external writer receives disabled proposals. Module-local expressions can
save old Q as ordinary lexical SSA; later Method writes do not alter that
snapshot. Aliases of one instance share candidates, while different instances
of the same definition remain independent. Ancestor and descendant autonomous
registrations may perform whole-owner updates to one actual state when their
grants are proven mutually exclusive through the real instance wiring. Unordered
field/index writers are also admitted when their footprints are disjoint; an
ancestor partial lane is composed with its captured old Q before routing, and
the receiver composes its own domains over that complete lane. Incoming
proposals are closed against old Q and combined with local proposals before
storage; an inactive incoming value cannot replace old Q. Each registration
retains its own atomic boundary. Mixed ordered/unordered groups remain
unsupported. Unproved writer
conflicts reject, including identity writes and conflicts in unused ordinary
definitions. Uninstantiated reference-bearing templates follow the
[source-unit link closure](../development/source-unit-workflow.md).

Module-typed parameters are static references to already allocated ordinary
instances, distinct from typed hardware data inputs. For example,
`Writer(counter: Counter)` can capture `counter` in a nested callable and invoke
`counter.increment()`. References can pass through other Modules and aliases
without allocating storage. Binding requires the declared nominal Module type.
Runtime selection, state storage, rebinding and implicit allocation of a
reference reject. A link root cannot expose unbound reference formals. Reference
contracts and actual bindings survive independent source-interface publication;
MLIR generates ordinary hierarchy ports and atomic control connections.

Independent callbacks have no cross-transaction candidate forwarding. The retired
`after`/`ac.callable_after`/`ac.after` contracts and EHR entry metadata reject in
source IR, interfaces and loaded artifacts. Same-callback calls preserve their
sequential candidates and snapshots. Multiple writers require disjointness or
exclusivity; source order is not arbitration. Explicit combinational connections
use existing dependency analysis and must remain acyclic after state cuts.

A writable owner passed as an actual argument retains that owner through nested
formal calls. Rebinding the formal proposes a new candidate to that owner;
passing an ordinary saved SSA value only rebinds the callee's local value.
Saved aliases never gain ownership merely from equal types or provenance from a
state read. Ordinary actual expressions evaluate once in source order, including
keyword actuals. Reads of writable owner references occur at call entry after
those expressions complete. Reached proposal payload and address obligations
remain live even if a later proposal overwrites their candidate.

The current routing subset rejects a definition requiring extra physical ports
if that definition is also used by a Collection, and rejects transaction writer
composition across a Collection. Unaffected existing Collections retain their
original contracts; this does not add a source API for Method-owner collections.

Source compilation and independent interfaces retain Method/Rule body `assert`,
`log` and `report` as ordered typed effects, including statement-arm effects.
A log evaluates its
payload expressions once in order; an assert does not gate later Method calls.
State analysis retains their statement-time values and invocation identities;
false or unknown assertion conditions do not become guards or alter candidates.
Each authored effect path must be proven true; MLIR separately derives its
structural branch-selection path. Final link
materializes saved operands and checks them against the original source plan
before canonical final verification. All body-effect paths combine the
transaction's final fire with that effect's selected arm; a later false guard
suppresses earlier assertions and observations.
Body assertions retain ordinary reset masking and fail on false or unknown
conditions, including low, held and falling epochs. They do not acquire the
guard-known sampling binding. Observations publish only after successful
whole-system checking and precommit; failure preserves previous gauges/events
and the committed epoch. Repeated report names for the same logical owner still
conflict even in mutually exclusive arms; report once after the join instead.
General transaction resource admission remains unfinished. Valid uncalled
typed Rule and Method declarations remain fully published and checked before
link removes them. This subset does not complete cross-Rule dynamic Table address
grants, general resource/conflict/causality handling or effect-safe optimization. Cross-domain
transactions and unproved conflicts between autonomous writers remain rejected.
Existing ordinary hardware and its system route
retain their current contracts and limits.
See the
[branch-state contract](../rfcs/serial-rule-method/branch-state-ssa.md),
[conditional-call/effect contract](../rfcs/serial-rule-method/conditional-effects.md) and
[extension specification](../rfcs/serial-rule-method/README.md) for exact
semantics, rejection boundaries and acceptance obligations. These documents do
not by themselves establish that a particular candidate passed those checks.

IR support for [Table/collections](spec-collections.md), packed structs and
memory does not establish a Python authoring API for every operation. Beyond the
bounded behavioral slice above, complete `@system`, general collection authoring,
remaining source arithmetic,
additional queue flow/head-read policies, automatic clock-domain scheduling and CDC still need bounded implementation
and evidence. Closed systems execute reachable source assertions and publish
supported source observations after successful epochs. A registered nested rule
in a closed system may contain only supported `log` or `report` calls, including
statement-arm observations and systems with
persistent state. No dummy assertion or instance binding is required; empty
rules and unsupported observation forms
still diagnose. Ordinary modules with
reachable assertions require the managed RTL lifecycle described in
[system execution](../architecture/system-execution.md); ordinary-module RTL
observations still diagnose. Unsupported effects must not be silently dropped.

Dynamic hardware creation, host I/O inside designs, arbitrary Python execution,
legacy CycleAwareSignal/JIT/builders, QueueGraph, `acc.py`, `acc` and `pycc` are
not alternate compilation paths. Historical examples remain migration references
until their original behavior runs through this route.
