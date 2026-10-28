# Saved routed anchors

A saved anchor is a self-contained, configuration-bound routed graph. It can be
verified or refined after the Python sources and original source units have been
removed. Reload reads the saved definitions; it does not capture Python, resolve
implicit providers or replace the saved routing. Transactional routed verification
replays the retained Linked snapshot to authenticate the stored graph.

```bash
pycircuit link units/child units/top --top demo.top.Top --target anchor -o anchor
pycircuit link --from-anchor anchor --verify-only
pycircuit link --from-anchor anchor --target final -o design.ac
pycircuit emit design.ac --target cpp -o cpp
pycircuit emit design.ac --target verilog -o rtl
```

Ordinary source link still defaults to `--target final`. An anchor reload cannot
take source units, a new top, parameter overrides or reset-policy overrides.
Verification accepts no output or transformation options. Both emitters continue
to consume one verified final artifact. Direct high-level C++ emission and extern
implementation bindings have separate contracts and are not provided here.

## Stored authority

An anchor directory contains only its canonical `anchor.json` manifest and the
payload files listed there:

| Path | Contents |
| --- | --- |
| `graph.ac` | Package and root policy, nominal declarations, primitive and ordinary extern declarations; no executable Module bodies. |
| `definitions/<variant-id>/spec.ac` | Exact inherent properties and discardable declaration attributes. |
| `definitions/<variant-id>/body.ac` | One normalized routed Module definition, including typed SSA and source locations. |
| `sources/<owner-id>.interface.ac` | Original admitted source interface retained for provenance in schemas 1/2; absent in schema 3. |

Only the selected root's semantic definition closure is indexed. Multiple
occurrences share an immutable definition but retain independent storage and
allocation identity. Variants are scoped to the complete routed graph: new
caller demands can produce different signatures and variant identities without
changing the source publications. No universal ABI across arbitrary callers is
promised.

Each indexed definition identifies its source owner, exact spec/body digests and
direct semantic dependencies. Native loading follows the actual root closure,
parses each required body once and checks those dependencies against the index.
It compares both spec dictionaries directly with the body's properties and
attributes. Source-header normalization is not an exact saved-spec comparison.
Fragments are assembled into an ephemeral package for existing verification;
there is no merged body transport file or partially loaded emitter input.

Structural and routed verification precede configuration analysis. The saved
configuration pins the selected root, resolved integer defaults/overrides, root
domain, reset policy and graph/definition digests. Current public root admission
has no type-argument override. Native binders derive the loaded values again and
require exact agreement. A legal ordinary extern declaration can be verified in
an anchor while executable refinement still rejects a missing implementation.

Explicit refinement uses the existing independent routed-to-final proof and
hardware verifier. The final root records `ac.anchor_provenance`, containing
exactly `content_identity` and `configuration_identity` as lowercase SHA256
strings. This record is provenance; it does not authenticate source history or
replace refinement or hardware verification.

## Identity and format

Ordinary anchors use `kind="pycircuit-anchor"`, `schema=1` and
`routed_contract=2`. Anchors with a compiler-owned
[selected entry boundary](entry-boundary.md) use `schema=2` and
`routed_contract=3`. Transactional anchors use `schema=3` and
`routed_contract=4`. These three schema/contract pairs are admitted;
unsupported or mismatched versions require regeneration from source units.
The manifest has closed fields for owner, root, configuration and its identity,
complete content identity, file inventory, definition index, retained sources
and producer provenance. Duplicate fields, definitions, source owners, paths and
identities are rejected.

Schema 2 adds exactly `implementation_root` to the configuration object. Its
`{source, definition, variant_id}` row identifies the canonical entry adapter;
the existing manifest root, owner and configuration root identify the logical
Module selected by the user. Both rows must match indexed definitions and source
owners. Closure traversal starts at the implementation root, which reaches the
logical root through its single bound instance. Logical Integer arguments remain
in the existing configuration fields. Reload rederives both identities and
bindings after native admission; it cannot swap roots or substitute actuals.
An unrelated closed caller may be pruned from the saved closure without making
this entry boundary invalid.

Schema 3 uses the existing graph and definition inventory. Its `sources` list
is empty: `ac.source_units` and the embedded `ac.transactional_source` Linked
snapshot retain the source ownership and transaction provenance. The loader
verifies that provenance and routed correspondence before physical lowering.
It can be created directly from source units or with `link --from-linked
linked.ac --target anchor -o anchor`; neither reload requires original source
files or interfaces.

Canonical JSON uses literal UTF-8, sorted object keys, compact separators and
only required escapes. It has no floats or nonfinite numbers. Hardware integer
arguments use canonical decimal strings. Payload inventory entries are sorted by
path. Hashes cover canonical bytes without a
newline; the stored manifest has exactly one trailing LF. The manifest itself
is excluded from the payload inventory and its own content identity field is
excluded when computing the complete content hash. Producer version strings
record provenance and do not grant acceptance authority.

Variant identity hashes the source owner, canonical definition and exact spec
and body digests. Owner identity hashes the source package/path. Configuration
identity hashes the closed configuration object; content identity additionally
covers the manifest and its payload digests. These are content identities, not
semantic-equivalence hashes or authentication against a consistent rewrite of
the entire artifact. Locations can affect digests. Source arguments are sorted
by their verified owners before routing; declaration-local routes retain the
[first-discovery ordering contract](routed-anchor.md#routing-and-verification-authority).

Both Python transport and native direct loading enforce these format caps before
payload allocation or MLIR parsing:

| Resource | Maximum |
| --- | ---: |
| Manifest | 4 MiB |
| Payload files | 4,096 |
| Directories | 8,192 |
| Directory depth | 32 |
| Relative path | 1,024 UTF-8 bytes |
| One payload | 16 MiB |
| Aggregate payload | 64 MiB |

Existing graph, proof and clone budgets still apply. The format does not expand
compact Collections or claim lower peak memory without measurement.

## Publication and relocation

Creation snapshots explicit admitted source units under the existing ordered
publication locks. Native code writes only a fresh private stage; the existing
publisher owns public output replacement, journals and recovery.

Reload distinguishes valid managed controls from a portable artifact-only copy.
Managed inputs use ordered locks and recovery. Present malformed or incomplete
controls fail closed. A portable copy with no controls uses a bounded stable
snapshot without creating input metadata or requiring a writable input parent.
Both paths reject symlinks and input/output overlap. File type, identity and size
are checked around capture and every payload must match its pinned digest.
Native code consumes only the captured tree and independently checks its bounds
and structure.

Additional or missing files and directories reject the artifact. Replacing an
original source provider does not change an existing anchor. Replacing a bundled
body under an unchanged identity rejects it. Invalid input or failed refinement
cannot replace a previously published output; replacing an invalid managed
output follows the existing fail-closed publication policy.
