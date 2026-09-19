# Source, MLIR, and generated C++ naming

This document defines the pyCircuit 6 naming contract for Agentic Circuit
artifacts. Names have two distinct roles: source-readable semantic names and
target-language spellings. A backend name
must never replace or become the source of semantic identity.

## Accepted Decision 0274 target and current implementation gap

Decision 0274 is the target naming authority. At the F4/F5 hard cutover, one
implementation Python/AC source stem owns one `.hpp`/`.cpp` pair, source-owned
interface shards own nominal declarations, and one parameterized source
definition emits one readable C++ and RTL module family. Typed static
parameters and admitted generate branches carry specialization; generated
identifiers contain no specialization suffix, `__`, repeated enclosing-module
prefix, or opaque suffix.

The sections below document the currently implemented baseline until that
cutover: it still uses `.h`, concrete-symbol/specialization spellings, and some
double-underscore compiler names. Those spellings are implementation gaps, not
permission to weaken Decision 0274. The emitter, package consumers, tests, and
this baseline section must update atomically in one hard break. No dual naming
mode, alias, fallback, or compatibility flag is admitted.

## Design principles

- Preserve a meaningful author name for debugging, hierarchy review, interface
  documentation, and generated-source inspection.
- Keep direction, endpoint, lane, and stage information when the author encodes
  those facts in a name. For example, `input_select_valid_0` remains that exact
  semantic spelling through ACIR display metadata; the compiler does not reorder
  it or infer a different endpoint convention.
- Use the MLIR symbol plus the typed, ordered static-argument dictionary for
  specialization equality. Content summaries are not language identities.
- A name must not change merely because an unrelated definition or another
  specialization is added to the system.
- Code reuse and state ownership are separate. Equal specialization identities
  reuse one generated implementation; each instance still owns independent
  Queues, Tables, reservations, runtime IDs, and persistent state.

These principles are compatible with Linx RTL conventions such as lower
snake-case module names and `{driver}_{receiver}_{signal}_{lane}` interface
names. pyCircuit remains consumer-neutral and does not impose a product prefix.

## Python to ACIR

The frontend preserves case-sensitive Python spelling as follows:

| Python entity | ACIR representation |
| --- | --- |
| `@ac.system def name` | `ac.system @name` |
| `@ac.module def name` without static specialization | `ac.module @name` |
| statically specialized module | `ac.module @name__<parameter-name>_<value>...` |
| assignment receiving a module result | `ac.instance @<assignment>` |
| multiple result assignments | names joined with `__` |
| typed input/output names | `ac.input_display_names` / `ac.output_display_names` |
| rule and local names | rule identity plus `ac.display_name` and source provenance |

`@Top`, `%source_<N>`, `%result_<N>`, fanout names, and other documented
reserved spellings are compiler-owned. They do not claim to be Python source
names. The selected `ac.system`, interface display arrays, instance identity,
and source map provide the source-facing mapping for that wrapper.

The complete `__ac_` prefix is compiler-owned for generated definition, scope,
and temporary names. Python systems and modules must not declare names with
that prefix. Record-field adapters use readable definitions of the form
`__ac_project_<StructSpecialization>__<field-path>`; equal input type and field
path reuse one adapter class, while each callsite retains its own Python source
location and instance identity.

Static-argument fragments are emitted from parameter names and canonical typed
values in declaration order. The MLIR symbol and explicit static-argument
dictionary together are the specialization identity; no hidden key
participates in equality.

## ACIR to C++

### Readable base

Generated C++ module classes use the existing Pascal-style conversion:

1. treat every non-alphanumeric character as a word boundary;
2. uppercase the first alphanumeric character and the first character after a
   boundary;
3. preserve the remaining alphanumeric characters;
4. prefix `_` when the result is empty or starts with a digit.

The selected system `demo_system` therefore becomes `DemoSystem`.

### Module class and source-named file

Every structured QueueGraph module uses the readable definition spelling:

```text
<ReadableDefinition>
```

Generated file names never contain a hash or a compiler-owned `Module_`
prefix. Every specialization of the same Python definition is grouped into a
pair whose stem preserves the Python/ACIR definition spelling after the
portable C++ identifier legalizer:

```text
include/generated/modules/<SourceDefinition>.h
src/generated/modules/<SourceDefinition>.cpp
```

The class and file expose only structural names. They remain directly checkable
against verified ACIR through the `definition`, NDF, and Python source comments.
Two different Python definitions that legalize to the same class or file stem
fail closed and must be renamed; the compiler does not hide ambiguity behind a
hash.

Distinct specializations use readable parameter suffixes on the C++ class, for
example `Queue_depth_16`. Repeated instances with the same definition and typed
arguments reuse that class. A collision after legalization fails closed; the
compiler never resolves it with a hash.

### Other C++ identifiers

For ports, members, and internal helpers, the generator:

1. keeps ASCII letters, digits, and `_`;
2. replaces other bytes with `_`;
3. prefixes `_` when empty or digit-leading;
4. appends `_` for a C++ keyword;
5. appends `_2`, `_3`, and so on for a collision in deterministic plan order.

These local spellings are generated-source implementation details. Source
display names and complete semantic identities remain available in QueueGraph
and source-map artifacts.

## Examples

```text
Python system:        demo_system
ACIR system:          @demo_system
C++ model class:      DemoSystem

Python module:        alu_pipeline
ACIR definition:      @alu_pipeline
specialization:       lanes=4, width=32
C++ class:            AluPipeline_lanes_4_width_32
C++ file stem:        alu_pipeline

Python interface:     input_select_valid_0
ACIR display name:    input_select_valid_0
C++ local identifier: input_select_valid_0
```

## NDF traceability

Adjacent Python comments using `# ndf:` and `# ndf:requires` are captured from
the original source closure before Python AST normalization removes comments.
They become non-semantic `ac.ndf_ids` and `ac.ndf_requires` metadata. The
metadata does not participate in specialization identity.

Generated C++ prints the NDF identifiers beside the existing rule/module and
`source: path:line:column` comments. A reviewer can therefore move from a C++
policy or module class to its NDF contract and original Python location without
using a generated symbol as semantic authority.

## `.ac` package boundary

For structured systems, CMake invokes `acc.py` independently on every executable
Python source. Each invocation publishes one source-named `.ac`; a separate core
invocation compiles selected-system composition and explicit unit links.
All definitions authored in one Python file share that source unit. Multiple
typed specializations of a definition remain separate MLIR symbols inside it.
The compiler never creates source units by splitting one previously compiled
whole-system IR.

Native `acc -c <package>.ac` links the units in memory, rejects missing or
duplicate definitions and interface mismatches, then emits C++, a multi-TU C++
bundle, or Verilog. It never accepts a hidden whole-core definition file as a
substitute for module linking. The package does not embed producer release
identity; consumers pin the package release and exact Git `source_revision`
out of band.

Generated source groups preserve AC ownership one-to-one and use the same
readable Python stem. `include/generated/dut.h` exposes the typed selected root
for consumer-owned runners, while `model.h` keeps the generic lifecycle ABI.
Both are hash-free and carry no embedded release identity. Core/interface glue is separate. Verilog follows the canonical
linked ACIR -> PYC -> `pycc` path and remains fail-closed when hierarchy support
is incomplete; flattening is not a compatibility workaround.
