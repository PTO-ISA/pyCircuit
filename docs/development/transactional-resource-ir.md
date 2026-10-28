# Transactional resource IR, contract 2

The replacement for issue #361 uses resources and normalized transactional
callables in the existing ACIR dialect. The production Python compile/link/emit
route publishes this contract and routes it through common analysis. `linked`
certifies structural closure, not executable transactions.

## Unlinked constraints

Source body/interface pairs carry `ac.source_contract = 3 : i64`; older source
artifacts require regeneration. This source publication version is separate from
the transactional resource contract and Routed contract versions.

`!ac.infer` denotes an unlinked payload variable whose identity is its SSA value.
`ac.payload.pending` preserves exact literals and operation/conversion relations.
Common dependency-driven analysis collects contexts separately from exact type
facts, then rewrites resolved expressions to ordinary payload operations. A
conversion target cannot change the natural width of an anchored Bits operation.
Unresolved provider-dependent expressions survive printing and rereading.

`ac.method.pending` preserves one invocation, receiver, arguments, transaction
placement and result demand (`ignored`, `scalar` or `tuple`). Ignored demand does
not specify zero returns. Published Method requirements aggregate all uses,
including conversion contexts, without inventing a return FunctionType. Link
binds actual providers and replaces each pending invocation with one normal call
with its complete signature. Unused actual results remain unused; call effects
remain conservative.

`ac.resource.connection` records resource identity edges and their connecting
Module. Link propagates instance bindings, exports and aliases, checks payload
compatibility and distinct allocation identities, then allocates one parent-owned
depth-2 Queue only for a connection without existing storage. Equal payloads do
not imply equal resources. Exports and aliases do not allocate storage.

Linked verification rejects pending expressions, pending calls, deferred
connections and unresolved payload/resource types. Transaction analysis and
backends receive only this closed representation. Bits Queue execution uses the
same resource identity and transaction plan as State.

## Resources

| Type | Meaning |
| --- | --- |
| `!ac.primitive<T>` | Unresolved resource constraint, only in `unlinked` |
| `!ac.state<T>` | State storage reference |
| `!ac.queue<T>` | Queue storage reference; depth belongs to allocation |
| `!ac.module_ref<@M>` | Reference to an existing concrete instance of `@M` |

Resources are distinct from payload data: they cannot participate in arithmetic,
be stored as payloads, or be ordinary Struct fields. Existing Bits, Enum,
Struct and Table payloads retain their logical and four-state representations.

`ac.resource.alloc` creates storage owned by its enclosing Module instance.
Allocation names are unique among that owner's resources and child instances.
Allocations have memory effects: equal types and initializers do not permit CSE
to merge storage. Separate instances of one definition own separate storage.

State accepts an optional type-matching closed pure initializer; omission remains
unspecified. Queue requires an explicit positive depth and has no initial
contents. Primitive constraints cannot be allocated. With `!T` a payload type:

```mlir
%state = ac.resource.alloc "state" initial(%initial : !T) : !ac.state<!T>
%queue = ac.resource.alloc "queue" depth 5 : !ac.queue<!T>
%child = ac.instance.ref ["child"] : !ac.module_ref<@Child>
```

An instance reference resolves an existing child path; it creates no instance.
An empty path, `ac.instance.ref []`, explicitly references the enclosing Module
instance (self). Its nominal type does not equate distinct instances or define
future generic Module language semantics.

## Interfaces

`output_kinds` has one entry per resource result, in signature order:

| Kind | Module body | Instance |
| --- | --- | --- |
| `bound` | Receives and yields the original external resource argument | Passes the parent's resource and returns its alias |
| `export` | Yields an existing resource | Returns the same resource, without allocating or adding an owner |

Body arguments and instance operands are signature inputs followed by bound
outputs in result order. Yield operands and instance results cover the complete
output signature. Repeated export and multi-level forwarding preserve identity;
resource aliases must ultimately refer to an allocation or a formal resource.
Normal instance printing exposes `inputs(...)` and `bound(...)`; generic
assembly remains supported, and Module `output_kinds` stays visible.

## Transactional callables

`ac.rule` defines a private transaction; `ac.rule.call` registers it explicitly
in a Module. Definitions alone do not register work. Global Rules are templates
without captures. A local Rule can only be registered in its owning Module;
each registration belongs to the registering instance.

Rule signatures contain resource inputs and already-bound resource outputs.
Registration produces no payload results. The body writes through Method calls
and ends with an empty `ac.yield`. The input/output split remains visible:

```mlir
ac.rule @copy ins(!P8) outs(!P8) captures() body {
^bb0(%src: !P8, %dst: !P8):
  %value = ac.method.call %src::@read() : (!P8) -> !B8
  ac.method.call %dst::@write(%value) : (!P8, !B8) -> ()
  "ac.yield"() : () -> ()
}
ac.rule.call @copy ins(%input) outs(%output) : (!P8, !P8)
```

Here `!P8` abbreviates `!ac.primitive<!B8>` and `!B8` is an eight-bit payload.
The complete parseable examples are the `transactional-v2-callables*.mlir`
fixtures in `tests/compiler/lit/IR/`.

`ac.method` defines a public same-cycle payload interface. Its parameters and
results cannot be resources or instance references; internal captures can be.
The body yields its declared payload results. Calls carry one SSA receiver,
a method name, and payload arguments/results, without a duplicate instance path:

```mlir
ac.method @observe : () -> !B8 captures(%state : !S8) body {
^bb0(%state: !S8):
  %old = ac.method.call %state::@read() : (!S8) -> !B8
  "ac.yield"(%old) : (!B8) -> ()
}
%result = ac.method.call %child::@observe() : (!ac.module_ref<@Child>) -> !B8
```

`!S8` abbreviates `!ac.state<!B8>`. A module receiver must reference an existing
instance; calling a Method never allocates one. A private Rule cannot be called
as a public Method.

Both definitions have independent guard and body regions. Their block arguments
are interface arguments followed by captures; for Rules the interface includes
inputs then bound outputs. Regions are isolated from implicit outer SSA captures.
Omitting `guard` normalizes to the empty guard region, which denotes constant
true. An explicit guard yields one four-state bit. Its runtime unknown-condition
check ordering is outside this representation contract.

Guards admit pure payload computation, old-State reads and read-only Queue
observations. Method calls in guards are checked through their full guard/body
call closure; no declared read-only label substitutes for that proof. Consuming,
writing, allocating and registering are forbidden in guards. Primitive `read`
may become Queue consumption, so it retains conservative effects. In unlinked
guards its read-only obligation is deferred until concrete binding: State read
can satisfy the obligation; a Queue pop cannot. Linked verification checks the
complete resolved guard before execution admission. Registration and Method calls retain conservative effects under CSE/DCE;
definition regions do not imply immediate execution at their definition sites.

The former pure `ac.rule` operation is now `ac.comb`. Former Method operations
are `ac.legacy.method` and `ac.legacy.method.call`; their C++ classes and legacy
behavior are retained. These are IR spelling changes, not Python decorator
changes. Saved callable bodies using the conflicting old spellings must be
regenerated; there is no alias or automatic conversion.

## Stages and follow-up

The builtin Module carries `ac.transactional = 2 : i64` and `ac.stage` set to
`"unlinked"` or `"linked"`. Invalid versions/stages never select legacy semantics.
Unlinked packages can contain primitive constraints and explicit imports with
`semantic_kind = "resources"`, the same signature and `output_kinds` as definitions.
Linked packages require concrete resources, matching interfaces and a resolved,
nonrecursive instance hierarchy and resolved callable signatures. Calls remain
Method calls rather than physical signals. Generic Method requirements are published as constraints and closed against the
actual provider. Structural closure alone does not establish execution support.

Production Python compile/link publishes and authenticates this contract.
Public saved-anchor manifest publication remains deferred. Structural
verification alone does not establish backend execution.

## Bounded State execution

The native route for a selected closed Module is also used by production link:

```bash
pycircuit-opt linked.mlir --ac-route-transactions='top=Counter' -o routed.mlir
pycircuit-opt routed.mlir --ac-lower-transactions -o final.mlir
```

The selected Module owns finite-width Bits State with explicit initial values
and Bits Queues that start empty.
Local Rules and global Rule templates execute only through explicit
registrations. Python autorule declarations normalize to one ordinary Rule and
one registration. Pure Bits computations and State `read`/`write` are supported.
Module conditional registration preserves result-free `scf.if` with `scf.yield`;
`ac.source.predicate` associates its i1 control with the original Bits value.
Definitions and allocations remain directly Module-owned. Branches admit pure
computation, read-only observations and registrations, with lexical scope and no
cross-branch local merging. Rule-body SCF is outside this slice.

User Method expansion and existing-instance hierarchy use common transactional
analysis, preserving callee ownership and the caller transaction’s final fire.
Queue pop reads old head during preparation and proposes consumption only on
final fire. Push requires old free capacity; head/peek requires old nonempty
occupancy without consumption. Unproven same-direction access conflicts are
rejected. The adapter uses owner-local `fifo_local` with actual depth and latency
one; Routed/Final replay verifies configuration, payload, status and proposal
ports against common analysis. Unsupported execution fails without
falling back to legacy analysis.

Every State read observes old Q, including a read after a write in the same
Rule. Writes propose updates. Independent State updates can commit together;
potentially overlapping writes to one State are rejected unless the analysis
proves mutually exclusive activation paths and guards. The proof includes
opposite branch polarities, nested conjunctions and one actual State compared
equal to distinct known constants. Each State receives exactly one logical update;
an unwritten State holds Q with its enable disabled.

The compiler supplies clock/reset ports and one rising-edge commit per complete
cycle. Synchronous reset restores initial values and suppresses normal effects
and checks for that cycle. Only the selected Module's declared State exports
become outputs, preserving order, names, types and aliases. Four-state payloads
remain data. The predicate bridge is not a knownness proof and cannot silently
convert X/Z to false. This adapter requires controls whose knownness can be proved
from initialization and all writes; unresolved X/Z control combinations are
diagnosed rather than assigned a new check-order meaning.

A direct Rule body may contain `ac.expect` with `kind = "assert"`, Bits1
condition, constant-true path and source location. It has no sampling operands
or legacy check identity. Assertions are checks, not guards: failure prevents
all proposed updates and committed sampling history from advancing. The
adapter generates the backend check identity and binding.

Routed and final artifacts carry `ac.routed_contract = 4 : i64`, separately from
legacy contracts 2/3. Versioned provenance preserves the selected top and the
serialized normalized linked source. Verification reparses that source,
reconstructs common analysis and compares actual resource, update, check and
export endpoints. Generated mappings locate objects but do not certify them.
Final verification also checks physical State endpoints; existing DFF/DFFE
refinement correspondence remains required. Native printing, rereading and
verification are supported; public saved-anchor manifest publication is deferred.

The Python route uses this same analysis and adapter. Direct transactional
gfsim and broader physical-edge export remain separate implementation work.

The implementation reuses prototype allocation, path and verifier concepts.
Structural Method/resource representation alone does not establish execution
support. State, Queue, Method and hierarchy execution
require their separate generated-backend evidence. Remaining migration work and actual
validation are tracked in the [replacement status](transactional-frontend-status.md).
