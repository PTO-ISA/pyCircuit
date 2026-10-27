# Transactional resource IR, contract 2

This is the structural foundation for the replacement tracked by issue #361.
The Python compile/link/emit route has not switched to this contract. The
existing `pycircuit-opt` parser and verifier accept the resource subset described
here; `linked` certifies structural closure only, not executable transactions.

## Types and allocation

| Type | Meaning |
| --- | --- |
| `!ac.primitive<T>` | An unresolved resource constraint, permitted only in `unlinked` |
| `!ac.state<T>` | A reference to State storage |
| `!ac.queue<T>` | A reference to Queue storage |
| `!ac.module_ref<@M>` | A reference to an existing concrete instance of `@M` |

Resource references are not data. They cannot participate in payload arithmetic,
be stored as a resource payload, or appear as ordinary Struct fields. Existing
Bits, Enum, Struct and Table payload representations retain their logical and
four-state identity. Concrete Module reference matching here does not specify
future generic Module language semantics.

`ac.resource.alloc` establishes storage identity at Module structure level. Its
name is unique among allocations and child instances in that owner. Allocation
has memory effects: identical types and initializers do not authorize CSE to
merge two allocations. Each occurrence of a Module instance owns separate
internal allocations.

State may have a type-matching closed pure initializer. Omission remains
unspecified; the verifier does not insert zero. Queue must have an explicit
positive integer depth and cannot have initial contents. Depth belongs to the
allocation, so two Queues of different depths can have the same reference type.
Primitive constraints cannot be allocated. Frontend insertion of default Queue
depth 2 is future binding work.

Normal allocation/reference assembly is compact (with `!T` denoting an existing
payload type and `%initial` a closed pure expression):

```mlir
%state = ac.resource.alloc "state" initial(%initial : !T) : !ac.state<!T>
%queue = ac.resource.alloc "queue" depth 5 : !ac.queue<!T>
%child = ac.instance.ref ["child"] : !ac.module_ref<@Child>
```

## Module interfaces and identity

`output_kinds` has one `bound` or `export` entry per result, in signature order.
All these outputs are resource references.

| Kind | Body | Instance |
| --- | --- | --- |
| `bound` | Receives an externally supplied block argument and yields that argument | Receives the parent's resource as an operand; its corresponding result aliases it |
| `export` | Yields an existing resource | Returns a reference to that same resource |

Body arguments are ordinary signature inputs followed by bound output arguments
in output order. `ac.yield` supplies the full output signature. An instance takes
signature inputs followed by bound output operands and returns all outputs in
signature order. Exporting or forwarding a reference creates neither storage
nor another owner. Repeated export preserves identity. A bound result cannot be
replaced with a newly allocated internal resource.

Normal instance printing separates `inputs(...)` from `bound(...)`; the parser
checks that the grouping matches the callee's interface. The generic operation
form remains readable and uses the same verified operand order. Module
`output_kinds` remains visible beside the signature in normal printing.

`ac.instance.ref` resolves a path through existing child instances. It does not
instantiate its target. Two instances of the same definition remain distinct;
the nominal Module reference type alone does not identify an instance.

## Stage admission

The enclosing builtin Module carries `ac.transactional = 2 : i64` and
`ac.stage = "unlinked"` or `"linked"`. Marker presence selects this contract;
unknown versions, malformed versions, missing markers and wrong stages reject
without fallback to legacy semantics. Contract 1 from the prototype is not
accepted as contract 2.

Unlinked structure can contain primitive constraints and explicit Module import
declarations. These imports use `semantic_kind = "resources"` with the same
signature and `output_kinds` as definitions; legacy dependency and transaction
summaries are not part of this subset. Linked structure requires concrete resources, resolved instance
definitions, matching interfaces and a closed nonrecursive instance hierarchy.
Explicit instance bindings currently require matching resource kind and payload;
primitive-to-concrete specialization is PR-07 work, not an implicit conversion
performed by this structural verifier.
Resource alias chains must reach an existing allocation or a formal resource;
feeding an instance's bound result back into itself cannot fabricate storage.
Neither stage admits Method requirements, Rule execution, transaction operations
or arbitrary unrecognized contract metadata as verified content.

The existing source-unit, Routed and final hardware validators do not interpret
these resources as data ports. Current link, saved-anchor and backend paths
reject this contract. No backend execution, simulation or hardware refinement is
established by structural verification.

## Reuse and follow-up

The `feat/transactional-acir-gfsim` prototype supplies allocation effects,
instance-path resolution and structural-verifier patterns. This implementation
rewrites its combined resource/depth encoding, output binding and version
selection. It does not import the prototype Python importer, Method analysis,
transaction execution or gfsim generator, and leaves the legacy payload and
physical-operation semantics intact.

PR-02 can build Rule/Method definitions on these resource identities. PR-07's
binding work supplies actual connection resolution and frontend defaults;
PR-09 generalizes Module reference requirements and cross-file linking beyond
the concrete references in this subset. PR-04 starts formal Python compile/link
integration and example migration. Public source-unit publication, common transaction
analysis and executable backend adaptation remain follow-up work. No existing
example or test is retired by PR-01.
