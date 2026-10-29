# Reading ACIR

Use `pycircuit print` to inspect either an Unlinked source-unit body or a saved
Linked ACIR file. It parses the actual MLIR without solving constraints or
running lowering passes. No original Python source is needed.

```bash
pycircuit print unit.ac
pycircuit print linked.ac --profile debug
pycircuit print linked.ac --profile full
pycircuit print linked.ac --config display.json -o readable.txt
```

The default `readable` profile produces a **reading view**, marked as such in
its first line. It displays signatures, SSA connections, guards, branches,
resource types, initial values, depths, calls and effects. Unresolved payloads,
Method result requirements and resource constraints remain visible. SSA names
are local to the view and are not stable identifiers.

`debug` additionally displays locations and recursive source metadata. `full`
uses the native MLIR printer, including debug locations, and can be parsed again
as MLIR. Display options do not filter `full`. Compilation, publication and
stage snapshots continue to use complete serialization. Reading views are not
compiler inputs. Printing leaves the input file unchanged, and an output path
that aliases the input is rejected.

## Display configuration

The installed Python package includes `acir_display.json`, the default option
table. A user JSON file overrides named global options and individual options
within each named operation rule. Profiles are applied first, then user options.
No YAML dependency is needed.

```json
{
  "globals": {
    "locations": false,
    "source_info": false,
    "empty_attributes": true,
    "compact_types": true
  },
  "ops": {
    "ac.instance": {
      "fields": ["instance_name", "callee", "parameters", "*"],
      "omit_empty": ["parameters", "type_arguments"]
    }
  }
}
```

`fields` selects operation attributes in display order. `*` includes all
remaining attributes; omitting it intentionally hides unlisted attributes.
`omit_empty` lists attributes to omit when empty, even if `empty_attributes` is
true. Operands, results, types, successors and regions always remain structural
parts of the view. Operations without a rule retain all attributes except those
explicitly excluded by global metadata filtering.

`source_info` controls recursive metadata filtering, using the global
`metadata_fields` array of exact attribute/dictionary field names. This applies
inside static expressions and type attributes as well as at the operation
boundary. Set `source_info` to true to retain provenance; it also keeps the
source-linked expression tree inside Bits types. With `compact_types` enabled
and `source_info` disabled, literal widths display as `!ac.bits<8>` and symbolic
widths retain their expression without evaluation. State, Queue, primitive,
Struct, Enum and Module reference types keep their category and nominal identity.
`compact_types: false` retains static-expression structure, while still applying
the selected metadata filter.

Installed commands locate the native `pycircuit-print` tool using the same
bundled or installed-prefix lookup as compile/link/emit. For development, set
`PYCIRCUIT_PRINTER` to the native executable built from this checkout.
