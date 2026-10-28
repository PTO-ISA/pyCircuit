# Transactional resource IR, contract 2

PR-01 for issue #361 adds resource structure to the existing ACIR dialect and
`pycircuit-opt`. The Python compile/link/emit route is unchanged. `linked`
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
Its nominal type does not equate distinct instances or define future generic
Module language semantics.

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

## Stages and follow-up

The builtin Module carries `ac.transactional = 2 : i64` and `ac.stage` set to
`"unlinked"` or `"linked"`. Invalid versions/stages never select legacy semantics.
Unlinked packages can contain primitive constraints and explicit imports with
`semantic_kind = "resources"`, the same signature and `output_kinds` as definitions.
Linked packages require concrete resources, matching interfaces and a resolved,
nonrecursive instance hierarchy. Method requirements and transaction execution
are outside this resource-only subset.

Existing source-unit/link, Routed/final, saved-anchor and backend paths reject
this contract. Structural verification does not establish backend execution.

The implementation reuses prototype allocation, path-resolution and verifier
patterns, replacing its resource/depth encoding, output binding and version.
PR-02 adds Rule/Method; PR-04 begins Python integration; PR-07 supplies resource
specialization/binding and default Queue depth 2; PR-09 generalizes Module
reference requirements. Until binding is implemented, explicit instances require
matching resource kind and payload. No DUT example is migrated by this PR.
