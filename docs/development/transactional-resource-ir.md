# Transactional resource IR, contract 2

PR-01 and PR-02 for issue #361 add resources and normalized transactional
callables to the existing ACIR dialect and `pycircuit-opt`. The Python compile/link/emit route is unchanged. `linked`
certifies structural closure, not executable transactions.

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
may become Queue consumption, so it is effectful and cannot satisfy a read-only
guard. Registration and Method calls retain conservative effects under CSE/DCE;
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
Method calls rather than physical signals. Generic Method requirements and
transaction execution remain outside this subset.

Existing source-unit/link, Routed/final, saved-anchor and backend paths reject
this contract. Structural verification does not establish backend execution.

The implementation reuses prototype allocation, path-resolution and verifier
patterns, replacing its resource/depth encoding, output binding and version.
PR-02 adds Rule/Method representation and verification only; PR-04 begins Python
integration; PR-07 supplies resource
specialization/binding and default Queue depth 2; PR-09 generalizes Module
reference requirements. Until binding is implemented, explicit instances require
matching resource kind and payload. No DUT example is migrated by this PR.
