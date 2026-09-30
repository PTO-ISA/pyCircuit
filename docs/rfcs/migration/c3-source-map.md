# C3-SM: source-owned provenance inventories

Revision: B. Status: DRAFT / UNAPPROVED. Implementation baseline: a21bb596.
Author: decl_arch_conformance (Astra xhigh); PM editorial consolidation.

## Scope

C3-C approves source ownership and the source-map file role, but does not
specify a replacement for the retired QueueGraph payload. This proposal fills
that gap for the M5 portless function-module, default-clock, empty-static-argument
profile. It requires independent review and precise user approval.

The map inventories provenance retained in verified final IR. It does not map
generated line ranges, provide a debugger protocol or authenticate historical
Python sources. Precise generated-line mapping is deferred. No Python, CLI, IR,
runtime, ABI or generated.json field changes. SYSTEM/EXPECT B remains excluded.

## Exact payload

```json
{
  "kind": "pycircuit-source-map",
  "source": {"package": "demo", "path": "types.py"},
  "generated_files": ["sources/demo/types.hpp"],
  "origins": [
    {
      "operation": "ac.type_alias",
      "origin": "{expansion = [], site = {ast_path = [], definition = @\"demo.types.Word\"}}",
      "location": "loc(\"types.py\":3:1)"
    }
  ]
}
```

All objects have exactly the illustrated fields. `kind` is the exact literal
shown; `source` is the existing SourceOwner; `generated_files` contains safe
POSIX relative paths. Origin row fields are nonempty strings: exact operation
name, standalone canonical printing of its existing `ac.origin` Occurrence
attribute, and standalone canonical printing of its Location attribute. Printing
must not use aliases requiring an external assembly. Preserve fused/compound
locations, expansion data and `loc(unknown)`; never manufacture coordinates.
An MLIR location is not automatically a validated Python SourceSpan.

No version, digest, executable schedule, operand graph or runtime state is added.

## Derivation and completeness

Use the same frozen verified final package as both emitters. Do not reopen
Python, source bodies or headers. Emit one map per source-owned final unit,
including implementation, declaration-only, empty and facade units. Recursively
collect every operation directly carrying `ac.origin` within that unit; do not
inherit origins onto operations lacking one. Sort tuples `(operation, origin,
location)` by UTF-8 bytes and coalesce identical tuples. This is an inventory
of retained provenance, not operation multiplicity or execution order.

Repeated instances do not duplicate definition maps. Placement origins remain
in the source unit owning the placement operation. For one final input, both
targets must have equal source/origins data.

`generated_files` is exactly the native emitter's non-map membership for that
owner, sorted by UTF-8 bytes without duplicates. Ownership must not be inferred
from filenames or generated text. Support/ABI/runner glue has no fabricated
Python owner. An RTL declaration owner without emitted hardware has an empty
generated_files list and a map-only source group.

## Paths and publication

Reuse C3 SourceOwner path legalization and replace the source suffix with
`.source-map.json`: e.g. `sources/demo/types.source-map.json`. `__init__.py`
uses the existing stem legalization. No disambiguating counter or digest.

Register each map exactly once in generated.json.files with role `source-map`
and exactly once in its matching source_groups row. Each generated_files entry
must exist in that same row and receipt file list. The map does not list itself.
Existing exact-directory, safe-path, collision, replacement and recovery rules
apply; publication-owner and receipt fields are unchanged.

## Validation and errors

Before publication compare the complete native owner set, actual source-group
membership and independently collected origin tuples against emitted maps.
Reject missing/extra maps, wrong owners, dropped/altered origins, false file
attribution and duplicate memberships.

Reject invalid JSON/UTF-8, duplicate keys, unknown/missing fields, wrong types,
invalid SourceOwner, unsafe/colliding paths, unsorted/duplicate arrays,
self-reference and missing/extra files. Origin/location strings round-trip as
the existing MLIR attribute categories with canonical printing; use native
validators, never a Python semantic compiler.

Saved-map rereads validate structure and directory consistency, not historical
authenticity or information absent from the supplied final IR. Maps never
authorize execution or replace final verification. Use existing nonzero error
handling; failure preserves previous output. No silent omission or fallback.

## Replacement and rollback

At M5 cutover retire the active `agentic-circuit-source-map` v0.1 schema,
QueueGraph mapping implementation and product references. Translate meaningful
provenance oracle assertions to the declared scope; historical evidence stays
historical. Do not fill legacy graph categories with empty placeholders.
Line directives remain diagnostic aids, not substitutes for this inventory.
Rollback reverts the complete candidate; no format-selection compatibility mode.

## Independent verification checklist

- Compile real sources separately; generate from saved final IR in a fresh
  process after Python, bodies and headers become unavailable.
- Compare an independent inventory of source owners and all directly retained
  origins for modules, registers, rules, placements, checks, observations and
  scalar declarations.
- Cover repeated children, unused/private declarations, empty/facade units and
  RTL map-only groups.
- Cover Unicode, __init__ stems and legalization collisions; no synthetic owners.
- Preserve compound and unknown locations exactly.
- Compare both targets on the same final artifact, excluding generated_files.
- Mutate every structural/completeness rule above, including dropped unused
  origins and false owner attribution; preserve old outputs on rejection.
- Run existing source-unit, scalar, M4 and ABI lanes on the candidate, plus
  installed compile/link/emit/build/run and static/dynamic retirement checks.
- Bind evidence to candidate hashes, approved proposal hash, commands, toolchain
  and raw outcomes. Planned tests are not passing evidence.

The design author cannot provide the independent approval-readiness review.

## Decision delta and executable gates

For this declared M5 profile, this proposal narrowly supersedes Decision 0250's
single `share/generated/source-map.json`, QueueGraph/PYC map categories,
manifest schema/path/hash binding, and promise that the output map reconstructs
all source alternate-origin/inline stacks. The replacement is one inventory per
final SourceOwner with exact retained Occurrence and Location data, registered
by C3 generated.json ownership. It does not reconstruct information absent from
final IR. Existing compiler obligations to preserve valid origins during its
supported lowering remain; this proposal does not authorize dropping metadata.
The inventory is authoritative only for the retained tuples it records.

Decision 0266's requirement for separate semantic/display-name data in source
maps is superseded here by exact operation names and the existing Occurrence
attribute's definition reference. No new semantic/display-name fields are
promised. Source semantic spelling in IR and existing generated-name
legalization obligations remain unchanged. Source metadata stays outside model
identity, and no hashing is reintroduced into IR or generated names.

Execute in the implementation checkout with the current candidate's helpers:

```sh
cmake --build .pycircuit_out/m5-root --target acir-source-unit-harness acir-design-harness acir-cpp-source-parts-harness -j 4
export PYCIRCUIT_NATIVE_BUILD="$PWD/.pycircuit_out/m5-root"
export ACIR_SOURCE_UNIT_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-source-unit-harness"
export ACIR_DESIGN_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-design-harness"
export ACIR_CPP_SOURCE_PARTS_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-cpp-source-parts-harness"
python3 -m pytest tests/system/test_m5_source_map.py tests/unit/test_m5_source_map_validation.py tests/system/test_m5_public_emit.py
python3 -m pytest tests/system/test_final_scalar_declarations.py tests/system/test_m4_preview_workflow.py tests/system/test_m5_model_abi.py tests/unit/test_generated_bundle.py
```

The three M5 test entrypoints in the first pytest command are planned additions,
not existing passing tests. They respectively own native final-only collection
and cross-target equality; structural/completeness mutation validation; and
public publication preservation plus installed routing. After hard break,
translate the private M4 workflow assertions to the public M5 lane and record
the exact replacement command in the candidate evidence; do not silently drop
its independent behavioral oracle.
